/*
 * postres.h
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
#include	<btron/hmi.h>

#ifndef __POSTRES_H__
#define __POSTRES_H__

typedef struct postresdata_t_ postresdata_t;

IMPORT postresdata_t* postresdata_new();
IMPORT VOID postresdata_delete(postresdata_t *post);
IMPORT W postresdata_readfile(postresdata_t *post, VLINK *vlnk);
IMPORT W postresdata_genrequestbody(postresdata_t *post, UB *board, W board_len, UB *thread, W thread_len, STIME time, UB **body, W *body_len);
IMPORT TC* postresdata_getfromstring(postresdata_t *post);
IMPORT W postresdata_getfromstringlen(postresdata_t *post);
IMPORT TC* postresdata_getmailstring(postresdata_t *post);
IMPORT W postresdata_getmailstringlen(postresdata_t *post);
IMPORT TC* postresdata_getmessagestring(postresdata_t *post);
IMPORT W postresdata_getmessagestringlen(postresdata_t *post);

#endif
