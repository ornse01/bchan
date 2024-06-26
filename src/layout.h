/*
 * layout.h
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

#include    "parser.h"
#include    "layoutarray.h"
#include    "layoutstyle.h"

#ifndef __LAYOUT_H__
#define __LAYOUT_H__

typedef struct datlayout_t_ datlayout_t;

IMPORT datlayout_t* datlayout_new(GID gid, datlayoutstyle_t *style, datlayoutarray_t *layoutarray);
IMPORT VOID datlayout_delete(datlayout_t *layout);
IMPORT W datlayout_appendres(datlayout_t *layout, datparser_res_t *parser_res);
IMPORT VOID datlayout_getdrawrect(datlayout_t *layout, W *l, W *t, W *r, W *b);
IMPORT TC* datlayout_gettitle(datlayout_t *layout);
IMPORT W datlayout_gettitlelen(datlayout_t *layout);
IMPORT VOID datlayout_clear(datlayout_t *layout);
IMPORT W datlayout_getthreadviewrectbyindex(datlayout_t *layout, W n, W *l, W *t, W *r, W *b);

#endif
