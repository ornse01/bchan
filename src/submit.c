/*
 * submit.c
 *
 * Copyright (c) 2009-2010 project bchan
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source
 *    distribution.
 *
 */

#include	<basic.h>
#include	<bstdlib.h>
#include	<bstdio.h>
#include	<bstring.h>
#include	<tstring.h>
#include	<errcode.h>
#include	<btron/btron.h>

#include    "submit.h"
#include    "http.h"
#include    "cache.h"
#include    "postres.h"
#include    "sjisstring.h"
#include    "parselib.h"
#include    "submitutil.h"

struct ressubmit_t_ {
	http_t *http;
	datcache_t *cache;
};

LOCAL W ressubmit_simplerequest(ressubmit_t *submit, UB *header, W header_len, UB *body, W body_len, UB **responsebody, W *responsebody_len)
{
	UB *host, *r_body = NULL, *bin;
	W err, host_len, r_len = 0, len;
	http_responsecontext_t *context;

	datcache_gethost(submit->cache, &host, &host_len);

	err = http_connect(submit->http, host, host_len);
	if (err < 0) {
		printf("error http_connect\n");
		return err;
	}
	err = http_send(submit->http, header, header_len);
	if (err < 0) {
		printf("error http_send 1\n");
		return err;
	}
	err = http_send(submit->http, body, body_len);
	if (err < 0) {
		printf("error http_send 2\n");
		return err;
	}
	err = http_waitresponseheader(submit->http);
	if (err < 0) {
		printf("error http_waitresponseheader\n");
		return err;
	}

	context = http_startresponseread(submit->http);
	if (context == NULL) {
		printf("error http_startresponseread\n");
		return -1;
	}
	for (;;) {
		err = http_responsecontext_nextdata(context, &bin, &len);
		if (err < 0) {
			printf("error http_responsecontext_nextdata\n");
			return -1;
		}
		if (bin == NULL) {
			break;
		}
		printf("http_responsecontext_nextdata len = %d\n", len);
		sjstring_appendasciistring(&r_body, &r_len, bin, len);
	}
	http_endresponseread(submit->http, context);

	*responsebody = r_body;
	*responsebody_len = r_len;

	return 0;
}

LOCAL W ressubmit_makeheader(ressubmit_t *submit, W body_len, UB **header, W *header_len)
{
	UB *host, *board, *thread;
	W host_len, board_len, thread_len;

	datcache_gethost(submit->cache, &host, &host_len);
	datcache_getborad(submit->cache, &board, &board_len);
	datcache_getthread(submit->cache, &thread, &thread_len);

	return submitutil_makeheaderstring(host, host_len, board, board_len, thread, thread_len, body_len, header, header_len);
}

LOCAL W ressubmit_makenextheader(ressubmit_t *submit, W body_len, UB *prev_header, W prev_header_len, UB **header, W *header_len)
{
	UB *host, *board, *thread;
	W host_len, board_len, thread_len;

	datcache_gethost(submit->cache, &host, &host_len);
	datcache_getborad(submit->cache, &board, &board_len);
	datcache_getthread(submit->cache, &thread, &thread_len);

	return submitutil_makenextheader(host, host_len, board, board_len, thread, thread_len, body_len, prev_header, prev_header_len, header, header_len);
}

EXPORT W ressubmit_respost(ressubmit_t *submit, postresdata_t *post)
{
	UB *body, *header, *response_header, *responsebody;
	W body_len, header_len, err, response_header_len, responsebody_len;
	UB *host, *board, *thread;
	W host_len, board_len, thread_len;
	UB *next_body, *next_header, *next_response;
	W next_body_len, next_header_len, next_response_len;
	W ret;
	STIME time;
	submitutil_poststatus_t bodystatus;

	datcache_gethost(submit->cache, &host, &host_len);
	datcache_getborad(submit->cache, &board, &board_len);
	datcache_getthread(submit->cache, &thread, &thread_len);

	get_tim(&time, NULL);
	err = postresdata_genrequestbody(post, board, board_len, thread, thread_len, time, &body, &body_len);
	if (err < 0) {
		return RESSUBMIT_RESPOST_ERROR_CLIENT;
	}
	err = ressubmit_makeheader(submit, body_len, &header, &header_len);
	if (err < 0) {
		free(body);
		return RESSUBMIT_RESPOST_ERROR_CLIENT;
	}

	printf("%s", header);
	printf("%s\n\n", body);

	err = ressubmit_simplerequest(submit, header, header_len, body, body_len, &responsebody, &responsebody_len);
	if (err < 0) {
		free(body);
		free(header);
		return RESSUBMIT_RESPOST_ERROR_CLIENT;
	}
	free(body);
	free(header);

	response_header = http_getheader(submit->http);
	response_header_len = http_getheaderlength(submit->http);
	printf("%s\n\n", response_header);
	sjstring_debugprint(responsebody, responsebody_len);
	printf("\n");

	if (http_getstatus(submit->http) != 200) {
		return RESSUBMIT_RESPOST_ERROR_STATUS;
	}

	bodystatus = submitutil_checkresponse(responsebody, responsebody_len);
	submitutil_poststatus_debugprint(bodystatus);

	switch (bodystatus) {
	case submitutil_poststatus_notfound:
		free(responsebody);
		return RESSUBMIT_RESPOST_ERROR_CONTENT;
	case submitutil_poststatus_true:
		free(responsebody);
		return RESSUBMIT_RESPOST_SUCCEED;
	case submitutil_poststatus_false:
		free(responsebody);
		return RESSUBMIT_RESPOST_DENIED;
	case submitutil_poststatus_error:
		free(responsebody);
		return RESSUBMIT_RESPOST_DENIED;
	case submitutil_poststatus_check:
		free(responsebody);
		return RESSUBMIT_RESPOST_DENIED;
	default:
		free(responsebody);
		return RESSUBMIT_RESPOST_ERROR_CLIENT;
	case submitutil_poststatus_cookie:
		break;
	}

	dly_tsk(1000);

	err = submitutil_makenextrequestbody(responsebody, responsebody_len, &next_body, &next_body_len);
	free(responsebody);
	if (err < 0) {
		return RESSUBMIT_RESPOST_ERROR_CLIENT;
	}
	err = ressubmit_makenextheader(submit, next_body_len, response_header, response_header_len, &next_header, &next_header_len);
	if (err < 0) {
		free(next_body);
		return RESSUBMIT_RESPOST_ERROR_CLIENT;
	}
	printf("%s", next_header);
	printf("%s\n", next_body);

	err = ressubmit_simplerequest(submit, next_header, next_header_len, next_body, next_body_len, &next_response, &next_response_len);
	if (err < 0) {
		free(next_header);
		free(next_body);
		return RESSUBMIT_RESPOST_ERROR_CLIENT;
	}

	printf("%s\n\n", http_getheader(submit->http));

	sjstring_debugprint(next_response, next_response_len);
	printf("\n");

	bodystatus = submitutil_checkresponse(next_response, next_response_len);
	submitutil_poststatus_debugprint(bodystatus);

	free(next_header);
	free(next_body);
	free(next_response);

	printf("complete\n");

	switch (bodystatus) {
	case submitutil_poststatus_notfound:
		ret = RESSUBMIT_RESPOST_ERROR_CONTENT;
		break;
	case submitutil_poststatus_true:
		ret = RESSUBMIT_RESPOST_SUCCEED;
		break;
	case submitutil_poststatus_false:
		ret = RESSUBMIT_RESPOST_DENIED;
		break;
	case submitutil_poststatus_error:
		ret = RESSUBMIT_RESPOST_DENIED;
		break;
	case submitutil_poststatus_check:
		ret = RESSUBMIT_RESPOST_DENIED;
		break;
	case submitutil_poststatus_cookie:
		ret = RESSUBMIT_RESPOST_DENIED;
		break;
	default:
		ret = RESSUBMIT_RESPOST_ERROR_CLIENT;
		break;
	}

	return ret;
}

EXPORT ressubmit_t* ressubmit_new(datcache_t *cache)
{
	ressubmit_t *submit;

	submit = (ressubmit_t*)malloc(sizeof(ressubmit_t));
	if (submit == NULL) {
		return NULL;
	}
	submit->http = http_new();
	if (submit->http == NULL) {
		free(submit);
		return NULL;
	}
	submit->cache = cache;

	return submit;
}

EXPORT VOID ressubmit_delete(ressubmit_t *submit)
{
	http_delete(submit->http);
	free(submit);
}
