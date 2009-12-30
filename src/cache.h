/*
 * cache.h
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

#include    <basic.h>
#include    <btron/vobj.h>

#ifndef __CACHE_H__
#define __CACHE_H__

#define DATCACHE_RECORDTYPE_MAIN 31
#define DATCACHE_RECORDTYPE_INFO 30
#define DATCACHE_RECORDSUBTYPE_RETRIEVE 0x0001
#define DATCACHE_RECORDSUBTYPE_HEADER 0x0002

typedef struct datcache_t_ datcache_t;
typedef struct datcache_datareadcontext_t_ datcache_datareadcontext_t;

IMPORT datcache_t* datcache_new(VID vid);
IMPORT VOID datcache_delete(datcache_t *cache);
IMPORT W datcache_appenddata(datcache_t *cache, UB *data, W len);
IMPORT VOID datcache_cleardata(datcache_t *cache);
IMPORT VOID datcache_getlatestheader(datcache_t *cache, UB **header, W *len);
IMPORT W datcache_updatelatestheader(datcache_t *cache, UB *header, W len);
IMPORT VOID datcache_gethost(datcache_t *cache, UB **header, W *len);
IMPORT VOID datcache_getborad(datcache_t *cache, UB **borad, W *len);
IMPORT VOID datcache_getthread(datcache_t *cache, UB **thread, W *len);
IMPORT W datcache_datasize(datcache_t *cache);
IMPORT W datcache_writefile(datcache_t *cache);
IMPORT datcache_datareadcontext_t* datcache_startdataread(datcache_t *cache, W start);
IMPORT VOID datcache_enddataread(datcache_t *cache, datcache_datareadcontext_t *context);

IMPORT Bool datcache_datareadcontext_nextdata(datcache_datareadcontext_t *context, UB **bin, W *len);

#endif
