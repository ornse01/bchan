/*
 * submitutil.h
 *
 * Copyright (c) 2010-2011 project bchan
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

#include    "cookiedb.h"

#ifndef __SUBMITUTIL_H__
#define __SUBMITUTIL_H__

typedef enum {
	submitutil_poststatus_notfound,
	submitutil_poststatus_true,
	submitutil_poststatus_false,
	submitutil_poststatus_error,
	submitutil_poststatus_check,
	submitutil_poststatus_cookie
} submitutil_poststatus_t;

IMPORT W submitutil_makeheaderstring(UB *host, W host_len, UB *board, W board_len, UB *thread, W thread_len, W content_length, UB **header, W *header_len);
IMPORT submitutil_poststatus_t submitutil_checkresponse(UB *body, W len);
IMPORT W submitutil_makenextrequestbody(UB *prev_body, W prev_body_len, UB **next_body, W *next_len);
IMPORT W submitutil_makenextheader(UB *host, W host_len, UB *board, W board_len, UB *thread, W thread_len, W content_length, UB *prev_header, W prev_header_len, UB **header, W *header_len);
IMPORT W submitutil_updatecookiedb(cookiedb_t *db, UB *prev_header, W prev_header_len, UB *host, W host_len, STIME time);
IMPORT W submitutil_setnamemailcookie(cookiedb_t *db, UB *host, W host_len, STIME time, UB *name, W name_len, UB *mail, W mail_len);
IMPORT W submitutil_makeheaderstring2(UB *host, W host_len, UB *board, W board_len, UB *thread, W thread_len, W content_length, STIME time, cookiedb_t *cdb, UB **header, W *header_len);
IMPORT W submitutil_makeerrormessage(UB *body, W body_len, TC **msg, W *msg_len);

#ifdef BCHAN_CONFIG_DEBUG
IMPORT VOID SUBMITUTIL_POSTSTATUS_DP(submitutil_poststatus_t status);
#else
#define SUBMITUTIL_POSTSTATUS_DP(status) /**/
#endif

#endif
