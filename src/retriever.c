/*
 * retriever.c
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
#include	<errcode.h>
#include	<btron/btron.h>
#include	<btron/bsocket.h>
#include	<util/zlib.h>

#include    "retriever.h"
#include    "http.h"

#ifdef BCHAN_CONFIG_DEBUG
# define DP(arg) printf arg
# define DP_ER(msg, err) printf("%s (%d/%x)\n", msg, err>>16, err)
#else
# define DP(arg) /**/
# define DP_ER(msg, err) /**/
#endif

struct datretriever_t_ {
	datcache_t *cache;
	B *server;
	B *board;
	B *thread;
	W server_len;
	W board_len;
	W thread_len;
	http_t *http;
};

LOCAL VOID itoa(UB *str, UW i)
{
	UB conv[] = "0123456789";
	W j=0;

	if (i > 1000000000) {
		str[j++] = conv[(i/1000000000)%10];
	}
	if (i > 100000000) {
		str[j++] = conv[(i/100000000)%10];
	}
	if (i > 10000000) {
		str[j++] = conv[(i/10000000)%10];
	}
	if (i > 1000000) {
		str[j++] = conv[(i/1000000)%10];
	}
	if (i > 100000) {
		str[j++] = conv[(i/100000)%10];
	}
	if (i > 10000) {
		str[j++] = conv[(i/10000)%10];
	}
	if (i > 1000) {
		str[j++] = conv[(i/1000)%10];
	}
	if (i > 100) {
		str[j++] = conv[(i/100)%10];
	}
	if (i > 10) {
		str[j++] = conv[(i/10)%10];
	}
	str[j++] = conv[i%10];
	str[j++] = '\0';
}

LOCAL W datretriever_checkcacheheader_LastModified(datretriever_t *retriever, UB **str, W *len)
{
	UB *header,*p,*start,*end;
	W header_len,i;

	datcache_getlatestheader(retriever->cache, &header, &header_len);
	if (header == NULL) {
		return -1;
	}
	p = strstr(header, "Last-Modified:");
	if (p == NULL) {
		return -1;
	}
	p = strchr(p, ':');
	for (i=1; p[i] != '\r'; i++) {
		if (p[i] != ' ') {
			break;
		}
	}
	start = p+i;
	end = strchr(start, '\r');
	if (end == NULL) {
		return -1;
	}

	*str = start;
	*len = end - start;

	return 0;
}

LOCAL W datretriever_checkcacheheader_Etag(datretriever_t *retriever, UB **str, W *len)
{
	UB *header,*p,*start,*end;
	W header_len,i;

	datcache_getlatestheader(retriever->cache, &header, &header_len);
	if (header == NULL) {
		return -1;
	}
	p = strstr(header, "ETag:");
	if (p == NULL) {
		return -1;
	}
	p = strchr(p, ':');
	for (i=1; p[i] != '\r'; i++) {
		if (p[i] != ' ') {
			break;
		}
	}
	start = p+i;
	end = strchr(start+1, '\"');
	if (end == NULL) {
		return -1;
	}

	*str = start;
	*len = end - start + 1;

	return 0;
}

EXPORT W datretriever_check_rangerequest(datretriever_t *retriever, UB **last_modified, W *last_modified_len, UB **etag, W *etag_len, W *size)
{
	W err;
	W lm_len,et_len;
	UB *lm, *et;

	err = datretriever_checkcacheheader_LastModified(retriever, &lm, &lm_len);
	if (err < 0) {
		return err;
	}
	err = datretriever_checkcacheheader_Etag(retriever, &et, &et_len);
	if (err < 0) {
		return err;
	}

	*last_modified = lm;
	*last_modified_len = lm_len;
	*etag = et;
	*etag_len = et_len;
	*size = datcache_datasize(retriever->cache);

	return 0;
}

#define HTTP_ERR_SEND_LEN(http, str, len) \
   err = http_send((http), (str), (len)); \
   if(err < 0){ \
     return err; \
   }

#define HTTP_ERR_SEND(http, str) HTTP_ERR_SEND_LEN((http), (str), strlen((str)))

LOCAL W datretriever_http_sendheder(datretriever_t *retriever)
{
	W err;

	HTTP_ERR_SEND(retriever->http, "GET /");
	HTTP_ERR_SEND(retriever->http, retriever->board);
	HTTP_ERR_SEND(retriever->http, "/dat/");
	HTTP_ERR_SEND(retriever->http, retriever->thread);
	HTTP_ERR_SEND(retriever->http, ".dat HTTP/1.1\r\n");
	HTTP_ERR_SEND(retriever->http, "Accept-Encoding: gzip\r\n");
	HTTP_ERR_SEND(retriever->http, "HOST: ");
	HTTP_ERR_SEND(retriever->http, retriever->server);
	HTTP_ERR_SEND(retriever->http, "\r\n");
	HTTP_ERR_SEND(retriever->http, "Accept: */*\r\n");
	HTTP_ERR_SEND(retriever->http, "Referer: http://");
	HTTP_ERR_SEND(retriever->http, retriever->server);
	HTTP_ERR_SEND(retriever->http, "/test/read.cgi/");
	HTTP_ERR_SEND(retriever->http, retriever->board);
	HTTP_ERR_SEND(retriever->http, "/");
	HTTP_ERR_SEND(retriever->http, retriever->thread);
	HTTP_ERR_SEND(retriever->http, "/\r\n");
	HTTP_ERR_SEND(retriever->http, "Accept-Language: ja\r\nUser-Agent: Monazilla/1.00 (bchan/0.01)\r\nConnection: close\r\n\r\n");

	return 0;
}

LOCAL W datretriever_http_sendheader_partial(datretriever_t *retriever, UB *etag, W etag_len, UB *lastmodified, W lastmodified_len, W rangebytes)
{
	W err;
	UB numstr[11];

	HTTP_ERR_SEND(retriever->http, "GET /");
	HTTP_ERR_SEND(retriever->http, retriever->board);
	HTTP_ERR_SEND(retriever->http, "/dat/");
	HTTP_ERR_SEND(retriever->http, retriever->thread);
	HTTP_ERR_SEND(retriever->http, ".dat HTTP/1.1\r\n");
	HTTP_ERR_SEND(retriever->http, "HOST: ");
	HTTP_ERR_SEND(retriever->http, retriever->server);
	HTTP_ERR_SEND(retriever->http, "\r\n");
	HTTP_ERR_SEND(retriever->http, "Accept: */*\r\n");
	HTTP_ERR_SEND(retriever->http, "Referer: http://");
	HTTP_ERR_SEND(retriever->http, retriever->server);
	HTTP_ERR_SEND(retriever->http, "/test/read.cgi/");
	HTTP_ERR_SEND(retriever->http, retriever->board);
	HTTP_ERR_SEND(retriever->http, "/");
	HTTP_ERR_SEND(retriever->http, retriever->thread);
	HTTP_ERR_SEND(retriever->http, "/\r\n");

	HTTP_ERR_SEND(retriever->http, "If-Modified-Since: ");
	HTTP_ERR_SEND_LEN(retriever->http, lastmodified, lastmodified_len);
	HTTP_ERR_SEND(retriever->http, "\r\n");
	HTTP_ERR_SEND(retriever->http, "If-None-Match: ");
	HTTP_ERR_SEND_LEN(retriever->http, etag, etag_len);
	HTTP_ERR_SEND(retriever->http, "\r\n");
	HTTP_ERR_SEND(retriever->http, "Range: bytes=");
	itoa(numstr, rangebytes);
	HTTP_ERR_SEND(retriever->http, numstr);
	HTTP_ERR_SEND(retriever->http, "-\r\n");
	HTTP_ERR_SEND(retriever->http, "\r\n");

	HTTP_ERR_SEND(retriever->http, "Accept-Language: ja\r\nUser-Agent: Monazilla/1.00 (bchan/0.01)\r\nConnection: close\r\n\r\n");

	return 0;
}

/* from http://www.monazilla.org/index.php?e=198 */
#if 0
"
GET /[板名]/dat/[スレッド番号].dat HTTP/1.1
Accept-Encoding: gzip
Host: [サーバー]
Accept: */*
Referer: http://[サーバー]/test/read.cgi/[板名]/[スレッド番号]/
Accept-Language: ja
User-Agent: Monazilla/1.00 (ブラウザ名/バージョン)
Connection: close
"
#endif

EXPORT W datretriever_request(datretriever_t *retriever)
{
	W err, ret = -1, len, status, lm_len, et_len, data_len;
	UB *bin, *lm, *et;
	http_responsecontext_t *ctx;

	err = http_connect(retriever->http, retriever->server, retriever->server_len);
	if (err < 0) {
		return err;
	}

	err = datretriever_check_rangerequest(retriever, &lm, &lm_len, &et, &et_len, &data_len);
	if (err >= 0) {
		err = datretriever_http_sendheader_partial(retriever, et, et_len, lm, lm_len, data_len - 1);
		if (err < 0) {
			DP_ER("datretriever_http_sendheader_partial:", err);
			http_close(retriever->http);
			return err;
		}
		err = http_waitresponseheader(retriever->http);
		if (err < 0) {
			DP_ER("http_waitresponseheader:", err);
			http_close(retriever->http);
			return err;
		}

		status = http_getstatus(retriever->http);
		if (status < 0) {
			DP_ER("http_getstatus error:", status);
			http_close(retriever->http);
			return status;
		}
		DP(("HTTP/1.1 %d\n", status));
		if (status == 206) {
			ctx = http_startresponseread(retriever->http);
			if (ctx == NULL) {
				DP(("http_startresponseread error\n"));
				http_close(retriever->http);
				return -1; /* TODO */
			}

			err = http_responsecontext_nextdata(ctx, &bin, &len);
			if (err < 0) {
				DP_ER("http_responsecontext_nextdata error", err);
				http_endresponseread(retriever->http, ctx);
				return -1;
			}
			if (bin != NULL) {
				if (bin[0] != '\n') {
					/* todo all reloading. */
					DP(("todo all reloading\n"));
					http_endresponseread(retriever->http, ctx);
					return -1;
				}
				datcache_appenddata(retriever->cache, bin+1, len-1);
			}

			for (;;) {
				err = http_responsecontext_nextdata(ctx, &bin, &len);
				if (err < 0) {
					DP_ER("http_responsecontext_nextdata error", err);
					http_endresponseread(retriever->http, ctx);
					return -1;
				}
				if (bin == NULL) {
					break;
				}

				datcache_appenddata(retriever->cache, bin, len);
			}

			http_endresponseread(retriever->http, ctx);

			bin = http_getheader(retriever->http);
			len = http_getheaderlength(retriever->http);
			err = datcache_updatelatestheader(retriever->cache, bin, len);
			if (err < 0) {
				DP_ER("datcache_updatelatestheader error", err);
				return err;
			}

			DP(("dat append\n"));
			ret = DATRETRIEVER_REQUEST_PARTIAL_CONTENT;
		} else if (status == 304) {
			DP(("not modified\n"));
			http_close(retriever->http);
			ret = DATRETRIEVER_REQUEST_NOT_MODIFIED;
		} else if (status == 416) {
			/* todo all reloading. */
			http_close(retriever->http);
			ret = DATRETRIEVER_REQUEST_ALLRELOAD;
		} else {
			http_close(retriever->http);
		}
	} else {
		err = datretriever_http_sendheder(retriever);
		if (err < 0) {
			http_close(retriever->http);
			return err;
		}

		err = http_waitresponseheader(retriever->http);
		if (err < 0) {
			http_close(retriever->http);
			return err;
		}

		status = http_getstatus(retriever->http);
		if (status < 0) {
			http_close(retriever->http);
			return status;
		}
		DP(("HTTP/1.1 %d\n", status));
		if (status != 200) {
			http_close(retriever->http);
			return 0;
		}

		ctx = http_startresponseread(retriever->http);
		if (ctx == NULL) {
			return -1; /* TODO */
		}

		datcache_cleardata(retriever->cache);

		for (;;) {
			err = http_responsecontext_nextdata(ctx, &bin, &len);
			if (err < 0) {
				DP_ER("error http_responsecontext_nextdata", err);
				http_endresponseread(retriever->http, ctx);
				return -1;
			}
			if (bin == NULL) {
				break;
			}

			datcache_appenddata(retriever->cache, bin, len);
		}

		http_endresponseread(retriever->http, ctx);

		bin = http_getheader(retriever->http);
		len = http_getheaderlength(retriever->http);
		err = datcache_updatelatestheader(retriever->cache, bin, len);
		if (err < 0) {
			return err;
		}

		ret = DATRETRIEVER_REQUEST_ALLRELOAD;
	}

	return ret;
}

#ifdef BCHAN_CONFIG_DEBUG
EXPORT VOID DATRETRIEVER_DP(datretriever_t *retriever)
{
	UB *header;

	header = http_getheader(retriever->http);
	if (header == NULL) {
		return;
	}

	printf("%s\n\n", header);
	{
		UB *str;
		W i,len,err;
		err = datretriever_checkcacheheader_LastModified(retriever, &str, &len);
		if (err >= 0) {
			printf("Last-Modified: ");
			for (i=0;i<len;i++) {
				putchar(str[i]);
			}
			printf("\n");
		}
	}
	{
		UB *str;
		W i,len,err;
		err = datretriever_checkcacheheader_Etag(retriever, &str, &len);
		if (err >= 0) {
			printf("Etag: ");
			for (i=0;i<len;i++) {
				putchar(str[i]);
			}
			printf("\n");
		}
	}

	printf("\n");
	printf("\n");
}
#endif

EXPORT datretriever_t* datretriever_new(datcache_t *cache)
{
	datretriever_t *retriever;
	UB *str;
	W len;

	retriever = malloc(sizeof(datretriever_t));
	if (retriever == NULL) {
		goto error_fetch;
	}
	retriever->cache = cache;

	datcache_gethost(cache, &str, &len);
	if (str == NULL) {
		goto error_server;
	}
	retriever->server = malloc(sizeof(UB)*(len+1));
	if (retriever->server == NULL) {
		goto error_server;
	}
	memcpy(retriever->server, str, len);
	retriever->server[len] = '\0';
	retriever->server_len = len;

	datcache_getborad(cache, &str, &len);
	if (str == NULL) {
		goto error_board;
	}
	retriever->board = malloc(sizeof(UB)*(len+1));
	if (retriever->board == NULL) {
		goto error_board;
	}
	memcpy(retriever->board, str, len);
	retriever->board[len] = '\0';
	retriever->board_len = len;

	datcache_getthread(cache, &str, &len);
	if (str == NULL) {
		goto error_thread;
	}
	retriever->thread = malloc(sizeof(UB)*(len+1));
	if (retriever->thread == NULL) {
		goto error_thread;
	}
	memcpy(retriever->thread, str, len);
	retriever->thread[len] = '\0';
	retriever->thread_len = len;

	retriever->http = http_new();
	if (retriever->http == NULL) {
		goto error_http;
	}

	return retriever;

error_http:
	free(retriever->thread);
error_thread:
	free(retriever->board);
error_board:
	free(retriever->server);
error_server:
	free(retriever);
error_fetch:

	return NULL;
}

EXPORT VOID datretriever_delete(datretriever_t *retriever)
{
	if (retriever->http != NULL) {
		http_delete(retriever->http);
	}

	if (retriever->thread != NULL) {
		free(retriever->thread);
	}
	if (retriever->board != NULL) {
		free(retriever->board);
	}
	if (retriever->server != NULL) {
		free(retriever->server);
	}
	free(retriever);
}
