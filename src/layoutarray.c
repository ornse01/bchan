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

LOCAL VOID datlayout_res_initialize(datlayout_res_t *layout_res, datparser_res_t *res)
{
	layout_res->parser_res = res;
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
