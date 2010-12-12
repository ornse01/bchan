/*
 * layoutarray.h
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

#include	<basic.h>

#include    "parser.h"
#include    "tadlib.h"

#ifndef __LAYOUTARRAY_H__
#define __LAYOUTARRAY_H__

typedef struct datlayout_box_t_ datlayout_box_t;
struct datlayout_box_t_ {
	W l,t,r,b;
};

typedef struct datlayout_res_t_ datlayout_res_t;
struct datlayout_res_t_ {
	datparser_res_t *parser_res;
	W index;
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

typedef struct datlayoutarray_t_ datlayoutarray_t;

IMPORT datlayoutarray_t* datlayoutarray_new();
IMPORT VOID datlayoutarray_delete(datlayoutarray_t *layoutarray);
IMPORT W datlayoutarray_appendres(datlayoutarray_t *layoutarray, datparser_res_t *parser_res);
IMPORT Bool datlayoutarray_getresbyindex(datlayoutarray_t *layoutarray, W index, datlayout_res_t **res);
IMPORT Bool datlayoutarray_getresfirst(datlayoutarray_t *layoutarray, datlayout_res_t **res);
IMPORT Bool datlayoutarray_getreslast(datlayoutarray_t *layoutarray, datlayout_res_t **res);
IMPORT VOID datlayoutarray_clear(datlayoutarray_t *layoutarray);
IMPORT W datlayoutarray_length(datlayoutarray_t *layoutarray);

#endif
