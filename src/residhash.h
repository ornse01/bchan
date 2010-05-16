/*
 * residhash.h
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
#include	<bsys/queue.h>
#include	<btron/dp.h>

#ifndef __RESIDHASH_H__
#define __RESIDHASH_H__

#ifndef RESIDHASH_BASE
#define RESIDHASH_BASE 10 
#endif

typedef struct residhash_node_t_ {
	QUEUE queue;
	TC *id;
	W id_len;
	UW attr;
	COLOR color;
} residhash_node_t;

struct residhash_t_ {
	W datanum;
	residhash_node_t tbl[RESIDHASH_BASE];
};
typedef struct residhash_t_ residhash_t;

IMPORT W residhash_initialize(residhash_t *residhash);
IMPORT VOID residhash_finalize(residhash_t *residhash);
IMPORT W residhash_adddata(residhash_t *residhash, TC *idstr, W idstr_len, UW attr, COLOR color);
#define RESIDHASH_SEARCHDATA_NOTFOUND 0
#define RESIDHASH_SEARCHDATA_FOUND    1
IMPORT W residhash_searchdata(residhash_t *residhash, TC *idstr, W idstr_len, UW *attr, COLOR *color);
IMPORT VOID residhash_removedata(residhash_t *residhash, TC *idstr, W idstr_len);

#endif
