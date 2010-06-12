/*
 * resindexhash.h
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

#ifndef __RESINDEXHASH_H__
#define __RESINDEXHASH_H__

#ifndef RESINDEXHASH_BASE
#define RESINDEXHASH_BASE 10 
#endif

typedef struct resindexhash_node_t_ {
	QUEUE queue;
	TC index;
	UW attr;
	COLOR color;
} resindexhash_node_t;

struct resindexhash_t_ {
	W datanum;
	resindexhash_node_t tbl[RESINDEXHASH_BASE];
};
typedef struct resindexhash_t_ resindexhash_t;

IMPORT W resindexhash_initialize(resindexhash_t *resindexhash);
IMPORT VOID resindexhash_finalize(resindexhash_t *resindexhash);
IMPORT W resindexhash_adddata(resindexhash_t *resindexhash, W index, UW attr, COLOR color);
#define RESINDEXHASH_SEARCHDATA_NOTFOUND 0
#define RESINDEXHASH_SEARCHDATA_FOUND    1
IMPORT W resindexhash_searchdata(resindexhash_t *resindexhash, W index, UW *attr, COLOR *color);
IMPORT VOID resindexhash_removedata(resindexhash_t *resindexhash, W index);
IMPORT W resindexhash_datanum(resindexhash_t *resindexhash);

struct resindexhash_iterator_t_ {
	resindexhash_t *resindexhash;
	W tbl_index;
	resindexhash_node_t *node;
};
typedef struct resindexhash_iterator_t_ resindexhash_iterator_t;

IMPORT VOID resindexhash_iterator_initialize(resindexhash_iterator_t *iter, resindexhash_t *target);
IMPORT VOID resindexhash_iterator_finalize(resindexhash_iterator_t *iter);
IMPORT Bool resindexhash_iterator_next(resindexhash_iterator_t *iter, W *index, W *attr, W *color);

#endif
