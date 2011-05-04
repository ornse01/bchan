/*
 * cookiedb.h
 *
 * Copyright (c) 2011 project bchan
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

#include    <basic.h>

#ifndef __COOKIEDB_H__
#define __COOKIEDB_H__

typedef struct cookiedb_t_ cookiedb_t;
typedef struct cookiedb_readheadercontext_t_ cookiedb_readheadercontext_t;
typedef struct cookiedb_writeheadercontext_t_ cookiedb_writeheadercontext_t;

IMPORT cookiedb_t* cookiedb_new(LINK *db_lnk, W rectype, UH subtype);
IMPORT VOID cookiedb_delete(cookiedb_t *db);
IMPORT VOID cookiedb_clearallcookie(cookiedb_t *db);
IMPORT W cookiedb_writefile(cookiedb_t *db);
IMPORT W cookiedb_readfile(cookiedb_t *db);

IMPORT cookiedb_readheadercontext_t* cookiedb_startheaderread(cookiedb_t *db, UB *host, W host_len, UB *path, W path_len, STIME time);
IMPORT VOID cookiedb_endheaderread(cookiedb_t *db, cookiedb_readheadercontext_t *context);
IMPORT W cookiedb_readheadercontext_appendchar_attr(cookiedb_readheadercontext_t *context, UB ch);
IMPORT W cookiedb_readheadercontext_appendchar_name(cookiedb_readheadercontext_t *context, UB ch);
IMPORT W cookiedb_readheadercontext_appendchar_comment(cookiedb_readheadercontext_t *context, UB ch);
IMPORT W cookiedb_readheadercontext_appendchar_domain(cookiedb_readheadercontext_t *context, UB ch);
IMPORT W cookiedb_readheadercontext_appendchar_path(cookiedb_readheadercontext_t *context, UB ch);
IMPORT W cookiedb_readheadercontext_appendchar_version(cookiedb_readheadercontext_t *context, UB ch);
IMPORT W cookiedb_readheadercontext_setsecure(cookiedb_readheadercontext_t *context);
IMPORT W cookiedb_readheadercontext_setexpires(cookiedb_readheadercontext_t *context, STIME expires);
IMPORT W cookiedb_readheadercontext_setmaxage(cookiedb_readheadercontext_t *context, W seconds);

IMPORT cookiedb_writeheadercontext_t* cookiedb_startheaderwrite(cookiedb_t *db, UB *host, W host_len, UB *path, W path_len, Bool secure, STIME time);
IMPORT VOID cookiedb_endheaderwrite(cookiedb_t *db, cookiedb_writeheadercontext_t *context);
IMPORT Bool cookiedb_writeheadercontext_makeheader(cookiedb_writeheadercontext_t *context, UB **str, W *len);

#endif
