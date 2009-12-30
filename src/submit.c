/*
 * submit.c
 *
 * Copyright (c) 2009 project bchan
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

struct ressubmit_t_ {
	http_t *http;
	datcache_t *cache;
};

#define MAKEHEADER_ERR_LEN(dest, dest_len, src, len) \
   err = sjstring_appendasciistring(dest, dest_len, src, len); \
   if(err < 0){ \
     return err; \
   }

#define MAKEHEADER_ERR(dest, dest_len, src) MAKEHEADER_ERR_LEN(dest, dest_len, (src), strlen((src)))

#define MAKEHEADER_NUM_ERR(dest, dest_len, num) \
   err = sjstring_appendUWstring(dest, dest_len, num); \
   if(err < 0){ \
     return err; \
   }

LOCAL W ressubmit_makeheader(ressubmit_t *submit, W body_len, UB **header, W *header_len)
{
	UB *str = NULL;
	W len = 0;
	UB *host, *board, *thread;
	W host_len, board_len, thread_len;
	W err;

	datcache_gethost(submit->cache, &host, &host_len);
	datcache_getborad(submit->cache, &board, &board_len);
	datcache_getthread(submit->cache, &thread, &thread_len);

	MAKEHEADER_ERR(&str, &len, "POST /test/bbs.cgi HTTP/1.1\r\n");
	MAKEHEADER_ERR(&str, &len, "Host: ");
	MAKEHEADER_ERR_LEN(&str, &len, host, host_len);
	MAKEHEADER_ERR(&str, &len, "\r\n");
	MAKEHEADER_ERR(&str, &len, "Accept: */*\r\n");
	MAKEHEADER_ERR(&str, &len, "Referer: http://");
	MAKEHEADER_ERR_LEN(&str, &len, host, host_len);
	MAKEHEADER_ERR(&str, &len, "/test/read.cgi/");
	MAKEHEADER_ERR_LEN(&str, &len, board, board_len);
	MAKEHEADER_ERR(&str, &len, "/");
	MAKEHEADER_ERR_LEN(&str, &len, thread, thread_len);
	MAKEHEADER_ERR(&str, &len, "/\r\n");
	MAKEHEADER_ERR(&str, &len, "Content-Length: ");
	MAKEHEADER_NUM_ERR(&str, &len, body_len);
	MAKEHEADER_ERR(&str, &len, "\r\n");
	MAKEHEADER_ERR(&str, &len, "Content-Type: application/x-www-form-urlencoded\r\n");
	MAKEHEADER_ERR(&str, &len, "Accept-Language: ja\r\nUser-Agent: Monazilla/1.00 (bchan/0.01)\r\nConnection: close\r\n\r\n");

	*header = str;
	*header_len = len;

	return 0;
}

LOCAL W ressubmit_firstpost(ressubmit_t *submit, UB *header, W header_len, UB *body, W body_len, UB **responsebody, W *responsebody_len)
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

typedef enum {
	ressubmit_poststatus_notfound,
	ressubmit_poststatus_true,
	ressubmit_poststatus_false,
	ressubmit_poststatus_error,
	ressubmit_poststatus_check,
	ressubmit_poststatus_cookie
} ressubmit_poststatus_t;

LOCAL ressubmit_poststatus_t ressubmit_checkresponse(UB *body, W len)
{
	W cmp;
	UB *ptr, *title_last, *found;
	UB title_true[] = {0x82, 0xdc, 0x82, 0xb5, 0x82, 0xbd, '\0'};
	UB title_error[] = {0x82, 0x64, 0x82, 0x71, 0x82, 0x71, '\0'};
	UB title_cookie[] = {0x8a, 0x6d, 0x94, 0x46, '\0'};
	UB title_ocha[] = {0x82, 0xa8, 0x92, 0x83, '\0'};

	ptr = body;
	for (;;) {
		ptr = strstr(ptr, "<!--");
		if (ptr == NULL) {
			break;
		}
		ptr++;
		for (;ptr < body + len;ptr++) {
			if (*ptr != ' ') {
				break;
			}
		}
		cmp = strcmp(ptr, "2ch_X:true");
		if (cmp == 0) {
			return ressubmit_poststatus_true;
		}
		cmp = strcmp(ptr, "2ch_X:false");
		if (cmp == 0) {
			return ressubmit_poststatus_false;
		}
		cmp = strcmp(ptr, "2ch_X:error");
		if (cmp == 0) {
			return ressubmit_poststatus_error;
		}
		cmp = strcmp(ptr, "2ch_X:check");
		if (cmp == 0) {
			return ressubmit_poststatus_check;
		}
		cmp = strcmp(ptr, "2ch_X:cookie");
		if (cmp == 0) {
			return ressubmit_poststatus_cookie;
		}
	}

	for (ptr = body;ptr < body + len; ptr++) {
		if (*ptr == '<') {
			cmp = strncasecmp(ptr, "<title>", 7);
			if (cmp != 0) {
				continue;
			}
			ptr += 7;
			title_last = strstr(ptr, "</");
			if (title_last == NULL) {
				return ressubmit_poststatus_notfound;
			}
			found = strstr(ptr, title_true);
			if ((found != NULL)&&(found < title_last)) {
				return ressubmit_poststatus_true;
			}
			found = strstr(ptr, title_error);
			if ((found != NULL)&&(found < title_last)) {
				return ressubmit_poststatus_error;
			}
			found = strstr(ptr, title_cookie);
			if ((found != NULL)&&(found < title_last)) {
				return ressubmit_poststatus_cookie;
			}
			found = strstr(ptr, title_ocha);
			if ((found != NULL)&&(found < title_last)) {
				return ressubmit_poststatus_error;
			}
		}
	}

	return ressubmit_poststatus_notfound;
}

LOCAL W ressubmit_appendstring_chref_before_urlencode(UB **dest, W *dlen, UB *src, W slen)
{
	charreferparser_t parser;
	charreferparser_result_t result;
	UB *amp, *rem, ch_b;
	W i, rem_len, ch_w, err;

	rem = src;
	rem_len = slen;

	for (;;) {
		amp = sjstring_searchchar(rem, rem_len, '&');
		if (amp == NULL) {
			return sjstring_appendurlencodestring(dest, dlen, rem, rem_len);
		}
		err = sjstring_appendurlencodestring(dest, dlen, rem, amp - rem);
		if (err < 0) {
			return err;
		}

		rem_len -= amp - rem;

		err = charreferparser_initialize(&parser);
		if (err < 0) {
			return err;
		}
		for (i = 0; i < rem_len; i++) {
			result = charreferparser_parsechar(&parser, amp[i]);
			if (result == CHARREFERPARSER_RESULT_DETERMINE) {
				ch_w = charreferparser_getcharnumber(&parser);
				if ((ch_w >= 0)&&(ch_w < 256)) {
					ch_b = ch_w & 0xff;
#if 1
					if (ch_b == 0x0a) {
						UB ch_b2 = 0x0d;
						err = sjstring_appendurlencodestring(dest, dlen, &ch_b2, 1);
						if (err < 0) {
							return err;
						}
					}
#endif
					err = sjstring_appendurlencodestring(dest, dlen, &ch_b, 1);
					if (err < 0) {
						return err;
					}
				} else {
					err = sjstring_appendurlencodestring(dest, dlen, amp, i+1);
					if (err < 0) {
						return err;
					}
				}
				i++;
				break;
			}
			if (result == CHARREFERPARSER_RESULT_INVALID) {
				break;
			}
		}
		charreferparser_finalize(&parser);

		rem_len -= i;
		rem = amp + i;

		if (rem_len <= 0) {
			break;
		}
	}

	return 0;
}

LOCAL W ressubmit_makenextrequestbody(UB *body, W body_len, UB **next_body, W *next_len)
{
	UB *ptr;
	UB *newbody = NULL;
	UB *name = NULL, *value = NULL;
	W name_len = 0, value_len = 0;
	W newbody_len = 0;
	W err;
	Bool first = True;

	for (ptr = body; ptr < body + body_len;) {
		ptr = strstr(ptr, "<input ");
		if (ptr == NULL) {
			break;
		}
		name = NULL;
		value = NULL;
		name_len = 0;
		value_len = 0;
		for (;ptr < body + body_len;) {
			for (;ptr < body + body_len;ptr++) {
				if (*ptr != ' ') {
					break;
				}
			}
			if (*ptr == '>') {
				if ((name == NULL)||(name_len == 0)) {
					break;
				}
				if (first != True) {
					err = sjstring_appendasciistring(&newbody, &newbody_len, "&", 1);
					if (err < 0) {
						return err;
					}
				}
				first = False;
				err = sjstring_appendasciistring(&newbody, &newbody_len, name, name_len);
				if (err < 0) {
					return err;
				}
				sjstring_appendasciistring(&newbody, &newbody_len, "=", 1);
				if (err < 0) {
					return err;
				}
				err = ressubmit_appendstring_chref_before_urlencode(&newbody, &newbody_len, value, value_len);
				if (err < 0) {
					return err;
				}
				break;
			}
			/* check attribute */
			if (strncasecmp(ptr, "name=\"", 6) == 0) {
				ptr += 6;
				name = ptr;
				for (;ptr < body + body_len;ptr++) {
					if (*ptr == '"') {
						break;
					}
				}
				name_len = ptr - name;
				ptr++;
			} else if (strncasecmp(ptr, "name=", 5) == 0) {
				ptr += 5;
				name = ptr;
				for (;ptr < body + body_len;ptr++) {
					if ((*ptr == ' ')||(*ptr == '>')) {
						break;
					}
				}
				name_len = ptr - name;
			} else if (strncasecmp(ptr, "value=\"", 7) == 0) {
				ptr += 7;
				value = ptr;
				for (;ptr < body + body_len;ptr++) {
					if (*ptr == '"') {
						break;
					}
				}
				value_len = ptr - value;
				ptr++;
			} else if (strncasecmp(ptr, "value=", 6) == 0) {
				ptr += 6;
				value = ptr;
				for (;ptr < body + body_len;ptr++) {
					if ((*ptr == ' ')||(*ptr == '>')) {
						break;
					}
				}
				value_len = ptr - value;
			} else {
				/* skip attribute */
				for (;ptr < body + body_len;ptr++) {
					if (*ptr == ' ') {
						break;
					}
				}
			}
		}
	}

	*next_body = newbody;
	*next_len = newbody_len;

	return 0;
}

LOCAL W ressubmit_makecookieheader(UB **str, W *len, UB *prev_header, W prev_header_len)
{
	UB *ptr, *start;
	W cookie_len, err;
	Bool first = True;

	for (ptr = prev_header; ptr < prev_header + prev_header_len;) {
		ptr = strstr(ptr, "Set-Cookie:");
		if (ptr == NULL) {
			break;
		}
		ptr += 11;
		for (;ptr < prev_header + prev_header_len; ptr++) {
			if (*ptr != ' ') {
				break;
			}
		}
		start = ptr;
		for (;ptr < prev_header + prev_header_len; ptr++) {
			if (*ptr == ';') {
				break;
			}
		}
		cookie_len = ptr + 1 - start;
		if (first == True) {
			MAKEHEADER_ERR(str, len, "Cookie: ");
			first = False;
		}
		MAKEHEADER_ERR_LEN(str, len, start, cookie_len);
		MAKEHEADER_ERR(str, len, " ");
	}

	if (first == False) {
		MAKEHEADER_ERR(str, len, "\r\n");
	}

	return 0;
}

LOCAL W ressubmit_makenextheader(ressubmit_t *submit, W body_len, UB *prev_header, W prev_header_len, UB **header, W *header_len)
{
	UB *str = NULL;
	W len = 0;
	UB *host, *board, *thread;
	W host_len, board_len, thread_len;
	W err;

	datcache_gethost(submit->cache, &host, &host_len);
	datcache_getborad(submit->cache, &board, &board_len);
	datcache_getthread(submit->cache, &thread, &thread_len);

	MAKEHEADER_ERR(&str, &len, "POST /test/bbs.cgi HTTP/1.1\r\n");
	MAKEHEADER_ERR(&str, &len, "Host: ");
	MAKEHEADER_ERR_LEN(&str, &len, host, host_len);
	MAKEHEADER_ERR(&str, &len, "\r\n");
	MAKEHEADER_ERR(&str, &len, "Accept: */*\r\n");
	MAKEHEADER_ERR(&str, &len, "Referer: http://");
	MAKEHEADER_ERR_LEN(&str, &len, host, host_len);
	MAKEHEADER_ERR(&str, &len, "/test/read.cgi/");
	MAKEHEADER_ERR_LEN(&str, &len, board, board_len);
	MAKEHEADER_ERR(&str, &len, "/");
	MAKEHEADER_ERR_LEN(&str, &len, thread, thread_len);
	MAKEHEADER_ERR(&str, &len, "/\r\n");
	MAKEHEADER_ERR(&str, &len, "Content-Length: ");
	MAKEHEADER_NUM_ERR(&str, &len, body_len);
	MAKEHEADER_ERR(&str, &len, "\r\n");
	err = ressubmit_makecookieheader(&str, &len, prev_header, prev_header_len);
	if (err < 0) {
		return err;
	}
	MAKEHEADER_ERR(&str, &len, "Content-Type: application/x-www-form-urlencoded\r\n");
	MAKEHEADER_ERR(&str, &len, "Accept-Language: ja\r\nUser-Agent: Monazilla/1.00\r\nConnection: close\r\n\r\n");

	*header = str;
	*header_len = len;

	return 0;
}

EXPORT W ressubmit_respost(ressubmit_t *submit, postresdata_t *post)
{
	UB *body, *header, *response_header, *responsebody;
	W body_len, header_len, err, response_header_len, responsebody_len;
	UB *host, *board, *thread;
	W host_len, board_len, thread_len, print_body_len;
	STIME time;
	TC *print_body;
	ressubmit_poststatus_t bodystatus;

	datcache_gethost(submit->cache, &host, &host_len);
	datcache_getborad(submit->cache, &board, &board_len);
	datcache_getthread(submit->cache, &thread, &thread_len);

	get_tim(&time, NULL);
	err = postresdata_genrequestbody(post, board, board_len, thread, thread_len, time, &body, &body_len);
	if (err < 0) {
		return err;
	}
	err = ressubmit_makeheader(submit, body_len, &header, &header_len);
	if (err < 0) {
		free(body);
		return err;
	}

	printf("%s", header);
	printf("%s\n\n", body);

	err = ressubmit_firstpost(submit, header, header_len, body, body_len, &responsebody, &responsebody_len);
	if (err < 0) {
		free(body);
		free(header);
		return err;
	}

	response_header = http_getheader(submit->http);
	response_header_len = http_getheaderlength(submit->http);
	printf("%s\n\n", response_header);

	bodystatus = ressubmit_checkresponse(responsebody, responsebody_len);
	switch (bodystatus) {
	case ressubmit_poststatus_notfound:
		printf("info notfound\n");
		break;
	case ressubmit_poststatus_true:
		printf("info true\n");
		break;
	case ressubmit_poststatus_false:
		printf("info false\n");
		break;
	case ressubmit_poststatus_error:
		printf("info error\n");
		break;
	case ressubmit_poststatus_check:
		printf("info check\n");
		break;
	case ressubmit_poststatus_cookie:
		printf("info cookie\n");
		break;
	}

	print_body_len = sjstotcs(NULL, responsebody);
	print_body = malloc(sizeof(TC)*(print_body_len + 1));
	sjstotcs(print_body, responsebody);
	print_body[print_body_len] = TNULL;
	{
		W i;
		for (i=0;i<print_body_len;i++) {
			printf("%C", print_body[i]);
		}
		printf("\n");
	}
	free(print_body);

	if (bodystatus != ressubmit_poststatus_cookie) {
		return 0; /* TODO */
	}

	dly_tsk(1000);

	{
		UB *next_body, *next_header, *next_response;
		W next_body_len, next_header_len, next_response_len;
		printf("AA\n");
		ressubmit_makenextrequestbody(responsebody, responsebody_len, &next_body, &next_body_len);
		printf("BB\n");
		ressubmit_makenextheader(submit, next_body_len, response_header, response_header_len, &next_header, &next_header_len);
		printf("%s", next_header);
		printf("%s\n", next_body);

#if 1
		err = ressubmit_firstpost(submit, next_header, next_header_len, next_body, next_body_len, &next_response, &next_response_len);

		printf("%s\n\n", http_getheader(submit->http));
		
		print_body_len = sjstotcs(NULL, next_response);
		print_body = malloc(sizeof(TC)*(print_body_len + 1));
		sjstotcs(print_body, next_response);
		print_body[print_body_len] = TNULL;
		{
			W i;
			for (i=0;i<print_body_len;i++) {
				printf("%C", print_body[i]);
			}
			printf("\n");
		}
		free(print_body);

		bodystatus = ressubmit_checkresponse(next_response, next_response_len);
		switch (bodystatus) {
		case ressubmit_poststatus_notfound:
			printf("info notfound\n");
			break;
		case ressubmit_poststatus_true:
			printf("info true\n");
			break;
		case ressubmit_poststatus_false:
			printf("info false\n");
			break;
		case ressubmit_poststatus_error:
			printf("info error\n");
			break;
		case ressubmit_poststatus_check:
			printf("info check\n");
			break;
		case ressubmit_poststatus_cookie:
			printf("info cookie\n");
			break;
		}

		free(next_response);
#endif
	}

	free(responsebody);
	free(body);
	free(header);

	printf("complete\n");

	return 0;
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
