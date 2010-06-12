/*
 * resindexhash.c
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

#include    "resindexhash.h"

#include	<bstdio.h>
#include	<bstdlib.h>
#include	<bsys/queue.h>

#ifdef BCHAN_CONFIG_DEBUG
# define DP(arg) printf arg
# define DP_ER(msg, err) printf("%s (%d/%x)\n", msg, err>>16, err)
#else
# define DP(arg) /**/
# define DP_ER(msg, err) /**/
#endif

LOCAL resindexhash_node_t* resindexhash_node_new(W index, UW attr, COLOR color)
{
	resindexhash_node_t *node;

	node = (resindexhash_node_t*)malloc(sizeof(resindexhash_node_t));
	if (node == NULL) {
		return NULL;
	}

	QueInit(&(node->queue));
	node->index = index;
	node->attr = attr;
	node->color = color;

	return node;
}

LOCAL VOID resindexhash_node_delete(resindexhash_node_t *hashnode)
{
	QueRemove(&(hashnode->queue));
	free(hashnode);
}

LOCAL W resindexhash_calchashvalue(W index)
{
	return index % RESINDEXHASH_BASE;
}

LOCAL resindexhash_node_t* resindexhash_searchnode(resindexhash_t *resindexhash, W index)
{
	W hashval;
	resindexhash_node_t *node, *buf;

	if (resindexhash->datanum == 0) {
		return NULL;
	}

	hashval = resindexhash_calchashvalue(index);
	buf = resindexhash->tbl + hashval;

	for (node = (resindexhash_node_t *)buf->queue.next; node != buf; node = (resindexhash_node_t *)node->queue.next) {
		if (node->index == index) {
			return node;
		}
	}

	return NULL;
}

EXPORT W resindexhash_adddata(resindexhash_t *resindexhash, W index, UW attr, COLOR color)
{
	resindexhash_node_t *hashnode, *buf;
	W hashval;

	hashnode = resindexhash_searchnode(resindexhash, index);
	if (hashnode != NULL) {
		hashnode->attr = attr;
		hashnode->color = color;
		return 0;
	}

	hashval = resindexhash_calchashvalue(index);
	buf = resindexhash->tbl + hashval;

	hashnode = resindexhash_node_new(index, attr, color);
	if (hashnode == NULL) {
		return -1; /* TODO */
	}

	QueInsert(&(hashnode->queue), &(buf->queue));
	resindexhash->datanum++;

	return 0;
}

EXPORT W resindexhash_searchdata(resindexhash_t *resindexhash, W index, UW *attr, COLOR *color)
{
	resindexhash_node_t *hashnode;

	hashnode = resindexhash_searchnode(resindexhash, index);
	if (hashnode == NULL) {
		return RESINDEXHASH_SEARCHDATA_NOTFOUND;
	}

	*attr = hashnode->attr;
	*color = hashnode->color;
	return RESINDEXHASH_SEARCHDATA_FOUND;
}

EXPORT VOID resindexhash_removedata(resindexhash_t *resindexhash, W index)
{
	resindexhash_node_t *hashnode;

	hashnode = resindexhash_searchnode(resindexhash, index);
	if (hashnode == NULL) {
		return;
	}
	resindexhash_node_delete(hashnode);
	resindexhash->datanum--;
}

EXPORT W resindexhash_datanum(resindexhash_t *resindexhash)
{
	return resindexhash->datanum;
}

EXPORT W resindexhash_initialize(resindexhash_t *resindexhash)
{
	W i;
	for (i=0; i < RESINDEXHASH_BASE;i++) {
		QueInit(&(resindexhash->tbl[i].queue));
	}
	return 0;
}

EXPORT VOID resindexhash_finalize(resindexhash_t *resindexhash)
{
	W i;
	resindexhash_node_t *buf,*buf_next;
	Bool empty;

	for(i = 0; i < RESINDEXHASH_BASE; i++){
		buf = resindexhash->tbl + i;
		for(;;){
			empty = isQueEmpty(&(buf->queue));
			if (empty == True) {
				break;
			}
			buf_next = (resindexhash_node_t*)buf->queue.next;
			resindexhash_node_delete(buf_next);
		}
	}
}

EXPORT Bool resindexhash_iterator_next(resindexhash_iterator_t *iter, W *index, W *attr, W *color)
{
	resindexhash_t *hash;
	resindexhash_node_t *node;

	hash = iter->resindexhash;

	if (iter->tbl_index == RESINDEXHASH_BASE) {
		return False;
	}

	*index = iter->node->index;
	*attr = iter->node->attr;
	*color = iter->node->color;

	for (;;) {
		node = (resindexhash_node_t*)iter->node->queue.next;
		if (hash->tbl + iter->tbl_index != node) {
			iter->node = node;
			break;
		}
		iter->tbl_index++;
		if (iter->tbl_index == RESINDEXHASH_BASE) {
			break;
		}
		iter->node = hash->tbl + iter->tbl_index;
	}

	return True;
}

EXPORT VOID resindexhash_iterator_initialize(resindexhash_iterator_t *iter, resindexhash_t *target)
{
	resindexhash_node_t *node;

	iter->resindexhash = target;
	iter->tbl_index = 0;
	iter->node = (resindexhash_node_t*)iter->resindexhash->tbl;
	for (;;) {
		node = (resindexhash_node_t*)iter->node->queue.next;
		if (iter->resindexhash->tbl + iter->tbl_index != node) {
			iter->node = node;
			break;
		}
		iter->tbl_index++;
		if (iter->tbl_index == RESINDEXHASH_BASE) {
			break;
		}
		iter->node = iter->resindexhash->tbl + iter->tbl_index;
	}
}

EXPORT VOID resindexhash_iterator_finalize(resindexhash_iterator_t *iter)
{
}
