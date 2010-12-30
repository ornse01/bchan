/*
 * render.h
 *
 * Copyright (c) 2010 project bchan
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

#include    "layoutarray.h"

#ifndef __RENDER_H__
#define __RENDER_H__

typedef struct datdraw_t_ datdraw_t;

IMPORT datdraw_t* datdraw_new(GID target, datlayoutstyle_t *style, datlayoutarray_t *layoutarray);
IMPORT VOID datdraw_delete(datdraw_t *draw);
IMPORT W datdraw_draw(datdraw_t *draw, RECT *r);
IMPORT VOID datdraw_setviewrect(datdraw_t *draw, W l, W t, W r, W b);
IMPORT VOID datdraw_getviewrect(datdraw_t *draw, W *l, W *t, W *r, W *b);
IMPORT VOID datdraw_scrollviewrect(datdraw_t *draw, W dh, W dv);
/* these value should be same in tadlib.h */
#define DATDRAW_FINDACTION_TYPE_ANCHOR 0
#define DATDRAW_FINDACTION_TYPE_URL    1
#define DATDRAW_FINDACTION_TYPE_NUMBER 2
#define DATDRAW_FINDACTION_TYPE_RESID  3
IMPORT W datdraw_findaction(datdraw_t *draw, PNT rel_pos, RECT *r, W *type, UB **start, W *len, W *resindex);

#endif
