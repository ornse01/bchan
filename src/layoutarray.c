/*
 * layoutarray.c
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

#include    "layoutarray.h"
#include    "array.h"

#include	<basic.h>
#include	<bstdlib.h>
#include	<bstdio.h>

#ifdef BCHAN_CONFIG_DEBUG
# define DP(arg) printf arg
# define DP_ER(msg, err) printf("%s (%d/%x)\n", msg, err>>16, err)
#else
# define DP(arg) /**/
# define DP_ER(msg, err) /**/
#endif

EXPORT VOID datlayout_res_setindexcolor(datlayout_res_t *layout_res, COLOR col)
{
	layout_res->indexcolor = col;
}

EXPORT VOID datlayout_res_setidcolor(datlayout_res_t *layout_res, COLOR col)
{
	layout_res->idcolor = col;
}

EXPORT VOID datlayout_res_enableindexcolor(datlayout_res_t *layout_res)
{
	layout_res->flag = layout_res->flag | DATLAYOUT_RES_FLAG_INDEXCOLOR;
}

EXPORT VOID datlayout_res_disableindexcolor(datlayout_res_t *layout_res)
{
	layout_res->flag = layout_res->flag & ~DATLAYOUT_RES_FLAG_INDEXCOLOR;
}

EXPORT Bool datlayout_res_isenableindexcolor(datlayout_res_t *layout_res)
{
	if ((layout_res->flag & DATLAYOUT_RES_FLAG_INDEXCOLOR) != 0) {
		return True;
	}
	return False;
}

EXPORT VOID datlayout_res_enableidcolor(datlayout_res_t *layout_res)
{
	layout_res->flag = layout_res->flag | DATLAYOUT_RES_FLAG_IDCOLOR;
}

EXPORT VOID datlayout_res_disableidcolor(datlayout_res_t *layout_res)
{
	layout_res->flag = layout_res->flag & ~DATLAYOUT_RES_FLAG_IDCOLOR;
}

EXPORT Bool datlayout_res_isenableidcolor(datlayout_res_t *layout_res)
{
	if ((layout_res->flag & DATLAYOUT_RES_FLAG_IDCOLOR) != 0) {
		return True;
	}
	return False;
}

EXPORT VOID datlayout_res_enableindexNG(datlayout_res_t *layout_res)
{
	layout_res->flag = layout_res->flag | DATLAYOUT_RES_FLAG_INDEXNG;
}

EXPORT VOID datlayout_res_disableindexNG(datlayout_res_t *layout_res)
{
	layout_res->flag = layout_res->flag & ~DATLAYOUT_RES_FLAG_INDEXNG;
}

EXPORT Bool datlayout_res_isenableindexNG(datlayout_res_t *layout_res)
{
	if ((layout_res->flag & DATLAYOUT_RES_FLAG_INDEXNG) != 0) {
		return True;
	}
	return False;
}

EXPORT VOID datlayout_res_enableidNG(datlayout_res_t *layout_res)
{
	layout_res->flag = layout_res->flag | DATLAYOUT_RES_FLAG_IDNG;
}

EXPORT VOID datlayout_res_disableidNG(datlayout_res_t *layout_res)
{
	layout_res->flag = layout_res->flag & ~DATLAYOUT_RES_FLAG_IDNG;
}

EXPORT Bool datlayout_res_isenableidNG(datlayout_res_t *layout_res)
{
	if ((layout_res->flag & DATLAYOUT_RES_FLAG_IDNG) != 0) {
		return True;
	}
	return False;
}

EXPORT VOID datlayout_res_getid(datlayout_res_t *layout_res, TC **id, W *id_len)
{
	if (layout_res->parser_res->dateinfo.id == NULL) {
		*id = NULL;
		*id_len = 0;
		return;
	}

	/* except "ID:" */
	*id = layout_res->parser_res->dateinfo.id + 3;
	*id_len = layout_res->parser_res->dateinfo.id_len - 3;
}

LOCAL VOID datlayout_res_initialize(datlayout_res_t *layout_res, datparser_res_t *res)
{
	layout_res->parser_res = res;
	layout_res->flag = 0;
	layout_res->indexcolor = 0x10000000;
	layout_res->idcolor = 0x10000000;
	layout_res->box.res.l = 0;
	layout_res->box.res.t = 0;
	layout_res->box.res.r = 0;
	layout_res->box.res.b = 0;
	layout_res->action.name = NULL;
	layout_res->action.mail = NULL;
	layout_res->action.body = NULL;
}

LOCAL VOID datlayout_res_finalize(datlayout_res_t *layout_res)
{
	if (layout_res->action.body != NULL) {
		actionlist_delete(layout_res->action.body);
	}
	if (layout_res->action.mail != NULL) {
		actionlist_delete(layout_res->action.mail);
	}
	if (layout_res->action.name != NULL) {
		actionlist_delete(layout_res->action.name);
	}
	datparser_res_delete(layout_res->parser_res);
}

struct datlayoutarray_t_ {
	arraybase_t array;
	datlayout_box_t bodybox;
};

EXPORT W datlayoutarray_appendres(datlayoutarray_t *layoutarray, datparser_res_t *parser_res)
{
	datlayout_res_t res;

	datlayout_res_initialize(&res, parser_res); /* TODO: modify allocation and copy timing. */

	return arraybase_appendunit(&layoutarray->array, &res);
}

EXPORT Bool datlayoutarray_getresbyindex(datlayoutarray_t *layoutarray, W index, datlayout_res_t **res)
{
	VP p;
	Bool found;

	found = arraybase_getunitbyindex(&layoutarray->array, index, &p);
	if (found == False) {
		return False;
	}

	*res = (datlayout_res_t *)p;

	return True;
}

EXPORT Bool datlayoutarray_getresfirst(datlayoutarray_t *layoutarray, datlayout_res_t **res)
{
	VP p;
	Bool found;

	found = arraybase_getunitfirst(&layoutarray->array, &p);
	if (found == False) {
		return False;
	}

	*res = (datlayout_res_t *)p;

	return True;
}

EXPORT Bool datlayoutarray_getreslast(datlayoutarray_t *layoutarray, datlayout_res_t **res)
{
	VP p;
	Bool found;

	found = arraybase_getunitlast(&layoutarray->array, &p);
	if (found == False) {
		return False;
	}

	*res = (datlayout_res_t *)p;

	return True;
}

EXPORT VOID datlayoutarray_clear(datlayoutarray_t *layoutarray)
{
	arraybase_truncate(&layoutarray->array, 0);
}

EXPORT W datlayoutarray_length(datlayoutarray_t *layoutarray)
{
	return arraybase_length(&layoutarray->array);
}

EXPORT VOID datlayoutarray_getbodybox(datlayoutarray_t *layoutarray, datlayout_box_t *box)
{
	box->l = layoutarray->bodybox.l;
	box->t = layoutarray->bodybox.t;
	box->r = layoutarray->bodybox.r;
	box->b = layoutarray->bodybox.b;
}

EXPORT VOID datlayoutarray_setbodybox(datlayoutarray_t *layoutarray, datlayout_box_t *box)
{
	layoutarray->bodybox.l = box->l;
	layoutarray->bodybox.t = box->t;
	layoutarray->bodybox.r = box->r;
	layoutarray->bodybox.b = box->b;
}

EXPORT VOID datlayoutarray_orrectbodybox(datlayoutarray_t *layoutarray, W l, W t, W r, W b)
{
	if (layoutarray->bodybox.l > l) {
		layoutarray->bodybox.l = l;
	}
	if (layoutarray->bodybox.t > t) {
		layoutarray->bodybox.t = t;
	}
	if (layoutarray->bodybox.r < r) {
		layoutarray->bodybox.r = r;
	}
	if (layoutarray->bodybox.b < b) {
		layoutarray->bodybox.b = b;
	}
}

EXPORT datlayoutarray_t* datlayoutarray_new()
{
	datlayoutarray_t *layoutarray;
	W err;

	layoutarray = (datlayoutarray_t *)malloc(sizeof(datlayoutarray_t));
	if (layoutarray == NULL) {
		return NULL;
	}
	err = arraybase_initialize(&layoutarray->array, sizeof(datlayout_res_t), 500);
	if (err < 0) {
		free(layoutarray);
		return NULL;
	}
	layoutarray->bodybox.l = 0;
	layoutarray->bodybox.t = 0;
	layoutarray->bodybox.r = 0;
	layoutarray->bodybox.b = 0;
	return layoutarray;
}

EXPORT VOID datlayoutarray_delete(datlayoutarray_t *layoutarray)
{
	W i, max;
	datlayout_res_t *res;

	max = datlayoutarray_length(layoutarray);
	for (i = 0; i < max; i++) {
		datlayoutarray_getresbyindex(layoutarray, i, &res);
		datlayout_res_finalize(res);
	}

	arraybase_finalize(&layoutarray->array);
	free(layoutarray);
}
