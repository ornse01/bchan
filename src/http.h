/*
 * http.h
 *
 * Copyright (c) 2009-2015 project bchan
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

#ifndef __HTTP_H__
#define __HTTP_H__

typedef struct http_t_ http_t;
typedef struct http_responsecontext_t_ http_responsecontext_t;

IMPORT http_t* http_new();
IMPORT VOID http_delete(http_t *http);
IMPORT W http_connect(http_t *http, UB *host, W host_len, UH port);
IMPORT W http_send(http_t *http, UB *bin, W len);
IMPORT W http_waitresponseheader(http_t *http);
IMPORT W http_getstatus(http_t *http);
IMPORT UB* http_getheader(http_t *http);
IMPORT W http_getheaderlength(http_t *http);
IMPORT http_responsecontext_t* http_startresponseread(http_t *http);
IMPORT VOID http_endresponseread(http_t *http, http_responsecontext_t *ctx);
IMPORT VOID http_close(http_t *http);

IMPORT W http_responsecontext_nextdata(http_responsecontext_t *context, UB **bin, W *len);

#endif
