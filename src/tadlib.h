/*
 * tadlib.h
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
#include	<btron/dp.h>
#include	<tad.h>

#ifndef __TADLIB_H__
#define __TADLIB_H__

typedef struct actionlist_t_ actionlist_t;

#define ACTIONLIST_ACTIONTYPE_ANCHOR 0
#define ACTIONLIST_ACTIONTYPE_URL    1
#define ACTIONLIST_ACTIONTYPE_NUMBER 2
#define ACTIONLIST_ACTIONTYPE_RESID  3
IMPORT actionlist_t* actionlist_new();
IMPORT VOID actionlist_delete(actionlist_t *alist);
IMPORT W actionlist_findboard(actionlist_t *alist, PNT pos, RECT *r, W *type, UB **start, W *len);

IMPORT W tadlib_calcdrawsize(TC *str, W len, GID gid, SIZE *sz, actionlist_t **alist);
IMPORT W tadlib_drawtext(TC *str, W len, GID gid, W dh, W dv);
IMPORT W tadlib_remove_TA_APPL_calcsize(TC *str, W len);
IMPORT W tadlib_remove_TA_APPL(TC *str, W len, TC *dest, W dest_len);
IMPORT W tadlib_UW_to_str(UW val, TC *dest, W dest_len);
IMPORT VOID tadlib_separete_datepart(TC *str, W len, TC **date, W *date_len, TC **id, W *id_len, TC **beid, W *beid_len);

struct taditerator_t_ {
	TC *bin;
	W len;
	W index;
};
typedef struct taditerator_t_ taditerator_t;

enum TADITERATOR_RESULT_T_ {
	TADITERATOR_RESULT_CHARCTOR,
	TADITERATOR_RESULT_SEGMENT,
	TADITERATOR_RESULT_END,
};
typedef enum TADITERATOR_RESULT_T_ TADITERATOR_RESULT_T;

IMPORT VOID taditerator_initialize(taditerator_t *iterator, TC *bin, W strlen);
IMPORT VOID taditerator_finalize(taditerator_t *iterator);
IMPORT W  taditerator_next(taditerator_t *iterator, TC **pos, TC *segment, LTADSEG **seg, W *segsize, UB **data);

typedef struct tctokenchecker_valuetuple_t_ tctokenchecker_valuetuple_t;
struct tctokenchecker_valuetuple_t_ {
	TC *name;
	W val;
};

typedef struct tctokenchecker_t_ tctokenchecker_t;
struct tctokenchecker_t_ {
	tctokenchecker_valuetuple_t *namelist;
	W namelistnum;
	TC *endtokens;
	W stringindex;
	W listindex_start;
	W listindex_end;
	W flag;
};

enum {
	TCTOKENCHECKER_CONTINUE,
	TCTOKENCHECKER_CONTINUE_NOMATCH,
	TCTOKENCHECKER_DETERMINE,
	TCTOKENCHECKER_NOMATCH,
	TCTOKENCHECKER_AFTER_END
};

IMPORT VOID tctokenchecker_initialize(tctokenchecker_t *checker, tctokenchecker_valuetuple_t *namelist, W namelistnum, TC *endchars);
IMPORT VOID tctokenchecker_finalize(tctokenchecker_t *checker);
IMPORT VOID tctokenchecker_clear(tctokenchecker_t *checker);
IMPORT W tctokenchecker_inputchar(tctokenchecker_t *checker, TC c, W *val);
IMPORT VOID tctokenchecker_getlastmatchedstring(tctokenchecker_t *checker, TC **str, W *len);

typedef struct {
#if BIGENDIAN
	UB subid;
	UB attr;
#else
	UB attr;
	UB subid;
#endif
	UH appl[3];
} TT_BCHAN;

#define TT_BCHAN_SUBID_ANCHOR_START 1
#define TT_BCHAN_SUBID_ANCHOR_END   2
#define TT_BCHAN_SUBID_URL_START    3
#define TT_BCHAN_SUBID_URL_END      4

#endif
