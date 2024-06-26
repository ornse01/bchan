/*
 * layoutarray.h
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

#include    "parser.h"
#include    "layoutstyle.h"
#include    "tadlib.h"

#ifndef __LAYOUTARRAY_H__
#define __LAYOUTARRAY_H__

/* tmp */
typedef struct datlayout_box_t_ datlayout_box_t;
struct datlayout_box_t_ {
	W l,t,r,b;
};

IMPORT VOID datlayout_box_getoffsetrect(datlayout_box_t *box, datlayout_style_t *style, W *l, W *t, W *r, W *b);
IMPORT VOID datlayout_box_getcontentrect(datlayout_box_t *box, datlayout_style_t *style, W *l, W *t, W *r, W *b);

#define DATLAYOUT_RES_FLAG_IDNG       0x00000001
#define DATLAYOUT_RES_FLAG_IDCOLOR    0x00000002
#define DATLAYOUT_RES_FLAG_INDEXNG    0x00000004
#define DATLAYOUT_RES_FLAG_INDEXCOLOR 0x00000008
#define DATLAYOUT_RES_FLAG_WORDNG     0x00000010

typedef struct datlayout_res_t_ datlayout_res_t;
struct datlayout_res_t_ {
	datparser_res_t *parser_res;
	W index;
	W flag;
	COLOR indexcolor;
	COLOR idcolor;
	struct {
		datlayout_box_t res;
		datlayout_box_t resheader;
		datlayout_box_t resmessage;
	} box;
	struct {
		RECT rel_number_pos;
		RECT rel_id_pos;
	} headerinfo;
	struct {
		actionlist_t *name;
		actionlist_t *mail;
		actionlist_t *body;
	} action;
};

IMPORT VOID datlayout_res_setindexcolor(datlayout_res_t *layout_res, COLOR col);
IMPORT VOID datlayout_res_setidcolor(datlayout_res_t *layout_res, COLOR col);
IMPORT VOID datlayout_res_enableindexcolor(datlayout_res_t *layout_res);
IMPORT VOID datlayout_res_disableindexcolor(datlayout_res_t *layout_res);
IMPORT Bool datlayout_res_isenableindexcolor(datlayout_res_t *layout_res);
IMPORT VOID datlayout_res_enableidcolor(datlayout_res_t *layout_res);
IMPORT VOID datlayout_res_disableidcolor(datlayout_res_t *layout_res);
IMPORT Bool datlayout_res_isenableidcolor(datlayout_res_t *layout_res);
IMPORT VOID datlayout_res_enableindexNG(datlayout_res_t *layout_res);
IMPORT VOID datlayout_res_disableindexNG(datlayout_res_t *layout_res);
IMPORT Bool datlayout_res_isenableindexNG(datlayout_res_t *layout_res);
IMPORT VOID datlayout_res_enableidNG(datlayout_res_t *layout_res);
IMPORT VOID datlayout_res_disableidNG(datlayout_res_t *layout_res);
IMPORT Bool datlayout_res_isenableidNG(datlayout_res_t *layout_res);
IMPORT VOID datlayout_res_enablewordNG(datlayout_res_t *layout_res);
IMPORT VOID datlayout_res_disablewordNG(datlayout_res_t *layout_res);
IMPORT Bool datlayout_res_isenablewordNG(datlayout_res_t *layout_res);
IMPORT VOID datlayout_res_getid(datlayout_res_t *layout_res, TC **id, W *id_len);
IMPORT Bool datlayout_res_issameid(datlayout_res_t *layout_res, TC *id, W id_len);
IMPORT VOID datlayout_res_getviewrect(datlayout_res_t *res, datlayout_style_t *resstyle, W *l, W *t, W *r, W *b);
IMPORT VOID datlayout_res_getcontentrect(datlayout_res_t *res, datlayout_style_t *resstyle, W *l, W *t, W *r, W *b);
IMPORT VOID datlayout_resheader_getviewrect(datlayout_res_t *res, datlayout_style_t *resstyle, W *l, W *t, W *r, W *b);
IMPORT VOID datlayout_resheader_getcontentrect(datlayout_res_t *res, datlayout_style_t *resstyle, W *l, W *t, W *r, W *b);
IMPORT VOID datlayout_resmessage_getcontentrect(datlayout_res_t *res, datlayout_style_t *resstyle, W *l, W *t, W *r, W *b);

typedef struct datlayoutarray_t_ datlayoutarray_t;

IMPORT datlayoutarray_t* datlayoutarray_new();
IMPORT VOID datlayoutarray_delete(datlayoutarray_t *layoutarray);
IMPORT W datlayoutarray_appendres(datlayoutarray_t *layoutarray, datparser_res_t *parser_res);
IMPORT Bool datlayoutarray_getresbyindex(datlayoutarray_t *layoutarray, W index, datlayout_res_t **res);
IMPORT Bool datlayoutarray_getresfirst(datlayoutarray_t *layoutarray, datlayout_res_t **res);
IMPORT Bool datlayoutarray_getreslast(datlayoutarray_t *layoutarray, datlayout_res_t **res);
IMPORT VOID datlayoutarray_clear(datlayoutarray_t *layoutarray);
IMPORT W datlayoutarray_length(datlayoutarray_t *layoutarray);
IMPORT VOID datlayoutarray_getbodybox(datlayoutarray_t *layoutarray, datlayout_box_t *box);
IMPORT VOID datlayoutarray_setbodybox(datlayoutarray_t *layoutarray, datlayout_box_t *box);
IMPORT VOID datlayoutarray_orrectbodybox(datlayoutarray_t *layoutarray, W l, W t, W r, W b);

#endif
