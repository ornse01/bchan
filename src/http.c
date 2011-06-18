/*
 * http.c
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
#include	<bctype.h>
#include	<bstring.h>
#include	<errcode.h>
#include	<btron/btron.h>
#include	<btron/bsocket.h>
#include	<util/zlib.h>

#include    "http.h"

#ifdef BCHAN_CONFIG_DEBUG
# define DP(arg) printf arg
# define DP_ER(msg, err) printf("%s (%d/%x)\n", msg, err>>16, err)
#else
# define DP(arg) /**/
# define DP_ER(msg, err) /**/
#endif

struct http_t_ {
	SOCKADDR addr;
	W sockid;
	UB *header_buffer;
	W header_buffer_len;
	W header_buffer_rcv_len;
	W header_len;
	http_responsecontext_t *context;
};

EXPORT W http_connect(http_t *http, UB *host, W host_len)
{
	W sock, err;
	B buf[HBUFLEN], *str;
	HOSTENT ent;
	struct sockaddr_in *addr_in;

	if (http->sockid > 0) {
		return -1;
	}

	str = malloc(host_len+1);
	if (str == NULL) {
		return -1;
	}
	strncpy(str, host, host_len);
	str[host_len] = '\0';

	err = so_gethostbyname(str, &ent, buf);
	free(str);
	if (err < 0) {
		return err;
	}

	addr_in = (struct sockaddr_in *)&(http->addr);
	addr_in->sin_family = AF_INET;
	addr_in->sin_port = htons( 80 );
	addr_in->sin_addr.s_addr = *(unsigned int *)(ent.h_addr_list[0]);

	sock = so_socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		return sock;
	}
	http->sockid = sock;

	err = so_connect(http->sockid, &(http->addr), sizeof(SOCKADDR));
	if (err < 0) {
		so_close(http->sockid);
		return err;
	}

	return 0;
}

EXPORT W http_send(http_t *http, UB *bin, W len)
{
	if (http->sockid < 0) {
		return -1;
	}
	return so_send(http->sockid, bin, len, 0);
}

LOCAL W http_parsehttpresponse(http_t *http)
{
	W i,len;
	B *str;

	len = http->header_buffer_len;
	str = http->header_buffer;

	for (i = 0; i < len; i++) {
		if ((str[i] == '\r')
			&&(str[i+1] == '\n')
			&&(str[i+2] == '\r')
			&&(str[i+3] == '\n')) {
			str[i] = '\0';
			str[i+1] = '\0';
			str[i+2] = '\0';
			str[i+3] = '\0';
			return i;
		}
	}

	return -1;
}

EXPORT W http_waitresponseheader(http_t *http)
{
	W off, rcvlen, header_len;

	for (off = 0; ;) {
		rcvlen = so_recv(http->sockid, http->header_buffer + off, http->header_buffer_len - off, 0);
		if (rcvlen < 0) {
			return rcvlen;
		}
		off += rcvlen;

		header_len = http_parsehttpresponse(http);
		if (header_len >= 0) {
			http->header_len = header_len;
			http->header_buffer_rcv_len = off;
			break;
		}

		if (off >= http->header_buffer_len) {
			http->header_buffer_len += 1024;
			http->header_buffer = realloc(http->header_buffer, http->header_buffer_len);
			if (http->header_buffer == NULL) {
				return -1;
			}
		}
	}

	return 0;
}

EXPORT W http_getstatus(http_t *http)
{
	B *str;

	if (http->header_buffer == NULL) {
		return -1;
	}
	str = strchr(http->header_buffer, ' ');
	if (str == NULL) {
		return -1;
	}
	return atoi(str+1);
}

EXPORT UB* http_getheader(http_t *http)
{
	return http->header_buffer;
}

EXPORT W http_getheaderlength(http_t *http)
{
	return http->header_len;
}

LOCAL W http_checkheader_content_encoding(http_t *http)
{
	B *str;

	str = strstr(http->header_buffer, "Content-Encoding: gzip");
	if (str != NULL) {
		return 1;
	}
	str = strstr(http->header_buffer, "Content-Encoding");
	if (str != NULL) {
		return -1;
	}
	return 0;
}

LOCAL W http_checkheader_transfer_encoding(http_t *http)
{
	B *str;

	str = strstr(http->header_buffer, "Transfer-Encoding: chunked");
	if (str != NULL) {
		return 1;
	}
	return 0;
}

LOCAL W http_checkheader_content_length(http_t *http)
{
	B *str;

	str = strstr(http->header_buffer, "Content-Length:");
	if (str == NULL) {
		return -1;
	}
	str += 15;
	return atoi(str);
}

typedef struct {
	http_t *http;
	UB *buffer; /* should refactor buffer handling. common with identity */
	W buffer_len;
	W buffer_rcv_len;
	W buffer_push_len;
	W current_chunked_len;
	W current_chunked_push_len;
} http_chunkediterator_t;

LOCAL W http_chunkediterator_start(http_chunkediterator_t *iterator, http_t *http)
{
	iterator->http = http;
	iterator->buffer_rcv_len = http->header_buffer_rcv_len - (http->header_len + 4);

	if (iterator->buffer_rcv_len < 1024) {
		iterator->buffer_len = 1024;
	} else {
		iterator->buffer_len = iterator->buffer_rcv_len;
	}
	iterator->buffer = malloc(iterator->buffer_len);
	if (iterator->buffer == NULL) {
		return -1;
	}

	if (iterator->buffer_rcv_len > 0) {
		memcpy(iterator->buffer, http->header_buffer + http->header_len + 4, iterator->buffer_rcv_len);
	}
	iterator->buffer_push_len = 0;
	iterator->current_chunked_len = 0;
	iterator->current_chunked_push_len = 0;

	return 0;
}

LOCAL W http_chunkediterator_getnext_readchunksize(http_chunkediterator_t *iterator)
{
	UB ch;
	W i, len = 0, rcvlen;
	enum {
		READING_SIZE,
		READING_EXT,
		ESC_r,
		FIN
	} state = READING_SIZE;

	if (iterator->buffer[iterator->buffer_push_len] == '\r') { /* tmp */
		for (i = 0; i < 2; i++) {
			iterator->buffer_push_len++;
			if (iterator->buffer_rcv_len == iterator->buffer_push_len) {
				DP(("[http] buffer update in chunked size reading\n"));
				rcvlen = so_recv(iterator->http->sockid, iterator->buffer, iterator->buffer_len, 0);
				if (rcvlen <= 0) {
					DP_ER("so_recv error:", rcvlen);
					return rcvlen;
				}
				iterator->buffer_rcv_len = rcvlen;
				iterator->buffer_push_len = 0;
			}
		}
	}

	for (state = READING_SIZE; state != FIN;) {
		ch = iterator->buffer[iterator->buffer_push_len++];
		switch(state){
		case READING_SIZE:
			if (isdigit(ch)) {
				len = len*16 + 0 + ch - '0';
				break;
			}
			if(('a' <= ch)&&(ch <= 'h')){
				len = len*16 + 0xa + ch - 'a';
				break;
			}
			if(('A' <= ch)&&(ch <= 'H')){
				len = len*16 + 0xa + ch - 'A';
				break;
			}
			if(ch == ';'){
				state = READING_EXT;
				break;
			}
			if(ch == '\r'){
				state = ESC_r;
				break;
			}
			return -1;
		case READING_EXT:
			if (ch != '\r') {
				break;
			}
			state = ESC_r;
			break;
		case ESC_r:
			if(ch == '\n'){
				state = FIN;
			}
			break;
		case FIN:
			DP(("error state\n"));
			break;
		}

		if (iterator->buffer_rcv_len == iterator->buffer_push_len) {
			DP(("[http] buffer update in chunked size reading\n"));
			rcvlen = so_recv(iterator->http->sockid, iterator->buffer, iterator->buffer_len, 0);
			if (rcvlen <= 0) {
				DP_ER("so_recv: error", rcvlen);
				return rcvlen;
			}
			iterator->buffer_rcv_len = rcvlen;
			iterator->buffer_push_len = 0;
		}
	}

	iterator->current_chunked_len = len;
	iterator->current_chunked_push_len = 0;

	return 0;
}

LOCAL W http_chunkediterator_getnext(http_chunkediterator_t *iterator, UB **ptr, W *len)
{
	W err, rcvlen;

	if (iterator->buffer_rcv_len == iterator->buffer_push_len) {
		rcvlen = so_recv(iterator->http->sockid, iterator->buffer, iterator->buffer_len, 0);
		if (rcvlen == EX_CONNABORTED) {
			DP(("http_chunkediterator_getnext: finish by EX_CONNABORTED\n"));
			*ptr = NULL;
			*len = 0;
			return 0;
		}
		if (rcvlen == 0) {
			DP(("http_chunkediterator_getnext: finish by rcvlen == 0\n"));
			*ptr = NULL;
			*len = 0;
			return 0;
		}
		if (rcvlen < 0) {
			DP_ER("so_recv error:", rcvlen);
			return rcvlen;
		}
		iterator->buffer_rcv_len = rcvlen;
		iterator->buffer_push_len = 0;
	}

	if (iterator->current_chunked_len == iterator->current_chunked_push_len) {
		err = http_chunkediterator_getnext_readchunksize(iterator);
		if (err < 0) {
			DP_ER("http_chunkediterator_getnext_readchunksize:", err);
			return err;
		}
	}
	if (iterator->current_chunked_len == 0) {
		*ptr = NULL;
		*len = 0;
		return 0;
	}

	*ptr = iterator->buffer + iterator->buffer_push_len;
	if (iterator->current_chunked_len - iterator->current_chunked_push_len > iterator->buffer_rcv_len - iterator->buffer_push_len) {
		*len = iterator->buffer_rcv_len - iterator->buffer_push_len;
	} else {
		*len = iterator->current_chunked_len - iterator->current_chunked_push_len;
	}
	iterator->current_chunked_push_len += *len;
	iterator->buffer_push_len += *len;

	return 0;
}

LOCAL VOID http_chunkediterator_finalize(http_chunkediterator_t *iterator)
{
	free(iterator->buffer);
}

struct http_identityiterator_t_ {
	http_t *http;
	W content_length;
	UB *buffer; /* should refactor buffer handling. common with chunked */
	W buffer_len;
	W buffer_rcv_len;
	W buffer_push_len;
	W total_push_len;
};
typedef struct http_identityiterator_t_ http_identityiterator_t;

LOCAL W http_identityiterator_start(http_identityiterator_t *iterator, http_t *http)
{
	iterator->http = http;
	iterator->content_length = http_checkheader_content_length(http);
	iterator->buffer_rcv_len = http->header_buffer_rcv_len - (http->header_len + 4);

	if (iterator->buffer_rcv_len < 1024) {
		iterator->buffer_len = 1024;
	} else {
		iterator->buffer_len = iterator->buffer_rcv_len;
	}
	iterator->buffer = malloc(iterator->buffer_len);
	if (iterator->buffer == NULL) {
		return -1;
	}

	if (iterator->buffer_rcv_len > 0) {
		memcpy(iterator->buffer, http->header_buffer + http->header_len + 4, iterator->buffer_rcv_len);
	}
	iterator->buffer_push_len = 0;
	iterator->total_push_len = 0;

	return 0;
}

LOCAL W http_identityiterator_getnext(http_identityiterator_t *iterator, UB **ptr, W *len)
{
	W rcvlen;

	if ((iterator->content_length >= 0)
		&&(iterator->total_push_len > iterator->content_length)) {
		DP(("http_identityiterator_getnext: finish by Content-Length\n"));
		*ptr = NULL;
		*len = 0;
		return 0;
	}

	if (iterator->buffer_rcv_len == iterator->buffer_push_len) {
		rcvlen = so_recv(iterator->http->sockid, iterator->buffer, iterator->buffer_len, 0);
		if (rcvlen == EX_CONNABORTED) {
			DP(("http_identityiterator_getnext: finish by EX_CONNABORTED\n"));
			*ptr = NULL;
			*len = 0;
			return 0;
		}
		if (rcvlen == 0) {
			DP(("http_identityiterator_getnext: finish by rcvlen == 0\n"));
			*ptr = NULL;
			*len = 0;
			return 0;
		}
		if (rcvlen < 0) {
			DP_ER("so_recv error:", rcvlen);
			return rcvlen;
		}
		iterator->buffer_rcv_len = rcvlen;
		iterator->buffer_push_len = 0;
	}

	*ptr = iterator->buffer;
	*len = iterator->buffer_rcv_len;
	iterator->buffer_push_len += *len;
	iterator->total_push_len += *len;
	if ((iterator->content_length >= 0)
		&&(iterator->total_push_len > iterator->content_length)) {
		*len -= iterator->total_push_len - iterator->content_length;
	}

	return 0;
}

LOCAL VOID http_identityiterator_finalize(http_identityiterator_t *iterator)
{
	free(iterator->buffer);
}

enum HTTP_TRANSFERENCODEITERATOR_TYPE_T_ {
	HTTP_TRANSFERENCODEITERATOR_TYPE_IDENTITY,
	HTTP_TRANSFERENCODEITERATOR_TYPE_CHUNKED,
};
typedef enum HTTP_TRANSFERENCODEITERATOR_TYPE_T_ HTTP_TRANSFERENCODEITERATOR_TYPE_T;

typedef struct {
	http_t *http;
	HTTP_TRANSFERENCODEITERATOR_TYPE_T type;
	http_identityiterator_t identity;
	http_chunkediterator_t chunked;
} http_transferencodeiterator_t;

LOCAL W http_starttransferencodeiterator(http_t *http, http_transferencodeiterator_t *iterator)
{
	W is_chunked;

	is_chunked = http_checkheader_transfer_encoding(http);
	if (is_chunked) {
		DP(("Transfer-Encoding: chunked\n"));
		iterator->type = HTTP_TRANSFERENCODEITERATOR_TYPE_CHUNKED;
		return http_chunkediterator_start(&iterator->chunked, http);
	} else {
		DP(("Transfer-Encoding: identity\n"));
		iterator->type = HTTP_TRANSFERENCODEITERATOR_TYPE_IDENTITY;
		return http_identityiterator_start(&iterator->identity, http);
	}
}

LOCAL W http_transferencodeiterator_getnext(http_transferencodeiterator_t *iterator, UB **ptr, W *len)
{
	switch (iterator->type) {
	case HTTP_TRANSFERENCODEITERATOR_TYPE_IDENTITY:
		return http_identityiterator_getnext(&iterator->identity, ptr, len);
	case HTTP_TRANSFERENCODEITERATOR_TYPE_CHUNKED:
		return http_chunkediterator_getnext(&iterator->chunked, ptr, len);
	}
	return -1; /* TODO */
}

LOCAL VOID http_transferencodeiterator_finalize(http_transferencodeiterator_t *iterator)
{
	switch (iterator->type) {
	case HTTP_TRANSFERENCODEITERATOR_TYPE_IDENTITY:
		http_identityiterator_finalize(&iterator->identity);
		break;
	case HTTP_TRANSFERENCODEITERATOR_TYPE_CHUNKED:
		http_chunkediterator_finalize(&iterator->chunked);
		break;
	}
}

/* http://ghanyan.monazilla.org/gzip.html */
/* http://ghanyan.monazilla.org/gzip.txt */
LOCAL W gzip_headercheck(UB *buf)
{
	W i, method, flags, len;

	if ((buf[0] != 0x1f)||(buf[1] != 0x8b)) {
		DP(("error gzip format 1\n"));
		return -1;
	}

	i = 2;
	method = buf[i++];
	flags  = buf[i++];
	if (method != Z_DEFLATED || (flags & 0xE0) != 0) {
		DP(("error gzip format 2\n"));
		return -1;
	}
	i += 6;

	if (flags & 0x04) { /* skip the extra field */
		len = 0;
		len = (W)buf[i++];
		len += ((W)buf[i++])<<8;
		i += len;
	}
	if (flags & 0x08) { /* skip the original file name */
		while (buf[i++] != 0) ;
	}
	if (flags & 0x10) {   /* skip the .gz file comment */
		while (buf[i++] != 0) ;
	}
	if (flags & 0x02) {  /* skip the header crc */
		i += 2;
	}

	return i;
} 

typedef struct {
	http_t *http;
	http_transferencodeiterator_t iter;
	z_stream z;
	Bool is_gziped;
	Bool is_finished;
	UB *buffer;
	W buffer_len;
} http_gzipiterator_t;

LOCAL W http_startgzipiterator(http_t *http, http_gzipiterator_t *iterator)
{
	UB *bin;
	W gzipheader_len, bin_len, err;

	err = http_checkheader_content_encoding(http);
	if (err < 0) {
		DP(("unsupported Content-Encoding\n"));
		return -1; /* unsupported */
	}

	iterator->http = http;
	http_starttransferencodeiterator(http, &(iterator->iter));
	if (err == 0) {
		iterator->is_gziped = False;
		return 0; /* not compressed */
	}
	iterator->is_gziped = True;
	iterator->is_finished = False;

	iterator->buffer_len = 1024;
	iterator->buffer = malloc(sizeof(B)*1024);
	if (iterator->buffer == NULL) {
		DP_ER("inflate buffer alloc", 0);
		return -1;
	}

	err = http_transferencodeiterator_getnext(&(iterator->iter), &bin, &bin_len);
	if (err < 0) {
		DP_ER("http_transferencodeiterator_getnext error", err);
		free(iterator->buffer);
		return -1;
	}

	gzipheader_len = gzip_headercheck(bin);
	if (gzipheader_len < 0) {
		DP_ER("gzip_headercheck error\n", gzipheader_len);
		free(iterator->buffer);
		return -1;
	}

	iterator->z.zalloc = Z_NULL;
	iterator->z.zfree = Z_NULL;
	iterator->z.opaque = Z_NULL;

	iterator->z.next_in = Z_NULL;
	iterator->z.avail_in = 0;

	err = inflateInit2(&(iterator->z), -MAX_WBITS);
	if (err != Z_OK) {
		DP_ER("inflateInit2 error:", err);
		free(iterator->buffer);
		return -1;
	}

	iterator->z.next_in = bin + gzipheader_len;
	iterator->z.avail_in = bin_len - gzipheader_len;
	iterator->z.next_out = iterator->buffer;
	iterator->z.avail_out = iterator->buffer_len;

	return 0;
}

LOCAL W http_gzipiterator_getnext(http_gzipiterator_t *iterator, UB **ptr, W *len)
{
	UB *c_bin;
	W c_len, err;

	if (iterator->is_gziped == False) {
		return http_transferencodeiterator_getnext(&(iterator->iter), ptr, len);
	}
	if (iterator->is_finished == True) {
		*ptr = NULL;
		*len = 0;
		return 0;
	}

	for (;;) {
		if (iterator->z.avail_in == 0) {
			err = http_transferencodeiterator_getnext(&(iterator->iter), &c_bin, &c_len);
			if (err < 0) {
				DP_ER("http_transferencodeiterator_getnext error:", err);
				return err;
			}
			if (c_bin == NULL) {
				DP(("full consumed\n"));
				break;
			}
			iterator->z.next_in = c_bin;
			iterator->z.avail_in = c_len;
		}
		err = inflate(&(iterator->z), Z_NO_FLUSH);
		if (err == Z_STREAM_END) {
			*ptr = iterator->buffer;
			*len = iterator->buffer_len - iterator->z.avail_out;
			iterator->is_finished = True;
			break;
		}
		if (err != Z_OK) {
			DP_ER("inflate error:", err);
			return -1;
		}
		if (iterator->z.avail_out == 0) {
			*ptr = iterator->buffer;
			*len = iterator->buffer_len;
			iterator->z.next_out = iterator->buffer;
			iterator->z.avail_out = iterator->buffer_len;
			return 0;
		}
	}

	return 0;
}

LOCAL VOID http_gzipiterator_finalize(http_gzipiterator_t *iterator)
{
	http_transferencodeiterator_finalize(&(iterator->iter));
	if (iterator->is_gziped == False) {
		return;
	}
	inflateEnd(&(iterator->z));
	free(iterator->buffer);
}

struct http_responsecontext_t_ {
	http_gzipiterator_t gzip_iterator;
};

EXPORT W http_responsecontext_nextdata(http_responsecontext_t *context, UB **bin, W *len)
{
	return http_gzipiterator_getnext(&(context->gzip_iterator), bin, len);
}

LOCAL http_responsecontext_t* http_responsecontext_new(http_t *http)
{
	http_responsecontext_t *context;
	W err;

	context = (http_responsecontext_t*)malloc(sizeof(http_responsecontext_t));
	if (context == NULL) {
		return NULL;
	}
	err = http_startgzipiterator(http, &(context->gzip_iterator));
	if (err < 0) {
		free(context);
		return NULL;
	}
	return context;
}

LOCAL VOID http_responsecontext_delete(http_responsecontext_t *context)
{
	http_gzipiterator_finalize(&(context->gzip_iterator));
	free(context);
}

EXPORT http_responsecontext_t* http_startresponseread(http_t *http)
{
	http_responsecontext_t* context;

	if (http->context != NULL) {
		return NULL;
	}

	context = http_responsecontext_new(http);
	if (context == NULL) {
		return NULL;
	}

	http->context = context;

	return context;
}

EXPORT VOID http_endresponseread(http_t *http, http_responsecontext_t *ctx)
{
	http_responsecontext_delete(ctx);
	http->context = NULL;
	so_close(http->sockid);
	http->sockid = -1;
}

EXPORT VOID http_close(http_t *http)
{
	so_close(http->sockid);
	http->sockid = -1;
}

EXPORT http_t* http_new()
{
	http_t *http;

	http = (http_t*)malloc(sizeof(http_t));
	if (http == NULL) {
		return NULL;
	}
	http->sockid = -1;
	http->header_buffer = malloc(1024);
	if (http->header_buffer == NULL) {
		free(http);
		return NULL;
	}
	http->header_buffer_len = 0;
	http->header_buffer_rcv_len = 0;
	http->header_len = 0;
	http->context = NULL;

	return http;
}

EXPORT VOID http_delete(http_t *http)
{
	if (http->header_buffer != NULL) {
		free(http->header_buffer);
	}
	free(http);
}
