/*
 * cache.h
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

#include    <basic.h>
#include    <btron/vobj.h>

#ifndef __CACHE_H__
#define __CACHE_H__

#define DATCACHE_RECORDTYPE_MAIN 31
#define DATCACHE_RECORDTYPE_INFO 30
#define DATCACHE_RECORDSUBTYPE_RETRIEVE 0x0001
#define DATCACHE_RECORDSUBTYPE_HEADER 0x0002
#define DATCACHE_RECORDSUBTYPE_RESIDINFO 0x0003
#define DATCACHE_RECORDSUBTYPE_RESINDEXINFO 0x0004

#define DATCACHE_RESIDDATA_FLAG_NG    0x00000001
#define DATCACHE_RESIDDATA_FLAG_COLOR 0x00000002
#define DATCACHE_RESINDEXDATA_FLAG_NG    0x00000001
#define DATCACHE_RESINDEXDATA_FLAG_COLOR 0x00000002

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
IMPORT W datcache_addresiddata(datcache_t *cache, TC *idstr, W idstr_len, UW attr, COLOR color);
#define DATCACHE_SEARCHRESIDDATA_NOTFOUND 0 /* RESIDHASH_SEARCHDATA_NOTFOUND */
#define DATCACHE_SEARCHRESIDDATA_FOUND    1 /* RESIDHASH_SEARCHDATA_FOUND */
IMPORT W datcache_searchresiddata(datcache_t *cache, TC *idstr, W idstr_len, UW *attr, COLOR *color);
IMPORT VOID datcache_removeresiddata(datcache_t *cache, TC *idstr, W idstr_len);
IMPORT W datcache_addresindexdata(datcache_t *cache, W index, UW attr, COLOR color);
#define DATCACHE_SEARCHRESINDEXDATA_NOTFOUND 0 /* RESINDEXHASH_SEARCHDATA_NOTFOUND */
#define DATCACHE_SEARCHRESINDEXDATA_FOUND    1 /* RESINDEXHASH_SEARCHDATA_FOUND */
IMPORT W datcache_searchresindexdata(datcache_t *cache, W index, UW *attr, COLOR *color);
IMPORT VOID datcache_removeresindexdata(datcache_t *cache, W index);

IMPORT datcache_datareadcontext_t* datcache_startdataread(datcache_t *cache, W start);
IMPORT VOID datcache_enddataread(datcache_t *cache, datcache_datareadcontext_t *context);

IMPORT Bool datcache_datareadcontext_nextdata(datcache_datareadcontext_t *context, UB **bin, W *len);

#endif
