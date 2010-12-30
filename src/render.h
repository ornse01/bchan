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
#include    "layoutstyle.h"

#ifndef __RENDER_H__
#define __RENDER_H__

typedef struct datrender_t_ datrender_t;

IMPORT datrender_t* datrender_new(GID target, datlayoutstyle_t *style, datlayoutarray_t *layoutarray);
IMPORT VOID datrender_delete(datrender_t *render);
IMPORT W datrender_draw(datrender_t *render, RECT *r);
IMPORT VOID datrender_setviewrect(datrender_t *render, W l, W t, W r, W b);
IMPORT VOID datrender_getviewrect(datrender_t *render, W *l, W *t, W *r, W *b);
IMPORT VOID datrender_scrollviewrect(datrender_t *render, W dh, W dv);
/* these value should be same in tadlib.h */
#define DATRENDER_FINDACTION_TYPE_ANCHOR 0
#define DATRENDER_FINDACTION_TYPE_URL    1
#define DATRENDER_FINDACTION_TYPE_NUMBER 2
#define DATRENDER_FINDACTION_TYPE_RESID  3
IMPORT W datrender_findaction(datrender_t *render, PNT rel_pos, RECT *r, W *type, UB **start, W *len, W *resindex);

#endif
