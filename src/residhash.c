/*
 * residhash.c
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

#include    "residhash.h"

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

LOCAL residhash_node_t* residhash_node_new(TC *idstr, W idstr_len, UW attr, COLOR color)
{
	residhash_node_t *node;

	node = (residhash_node_t*)malloc(sizeof(residhash_node_t));
	if (node == NULL) {
		return NULL;
	}

	node->id = malloc(sizeof(TC)*idstr_len);
	if (node->id == NULL) {
		free(node);
		return NULL;
	}
	memcpy(node->id, idstr, sizeof(TC)*idstr_len);
	node->id_len = idstr_len;
	QueInit(&(node->queue));
	node->attr = attr;
	node->color = color;

	return node;
}

LOCAL VOID residhash_node_delete(residhash_node_t *hashnode)
{
	QueRemove(&(hashnode->queue));
	free(hashnode->id);
	free(hashnode);
}

LOCAL W residhash_calchashvalue(TC *idstr, W idstr_len)
{
	W i,num = 0;

	for (i = 0; i < idstr_len; i++) {
		num += idstr[i];
	}

	return num % RESIDHASH_BASE;
}

LOCAL residhash_node_t* residhash_searchnode(residhash_t *residhash, TC *idstr, W idstr_len)
{
	W hashval;
	residhash_node_t *node, *buf;

	if (residhash->datanum == 0) {
		return NULL;
	}

	hashval = residhash_calchashvalue(idstr, idstr_len);
	buf = residhash->tbl + hashval;

	for (node = (residhash_node_t *)buf->queue.next; node != buf; node = (residhash_node_t *)node->queue.next) {
		if ((node->id_len == idstr_len)&&(memcmp(node->id, idstr, idstr_len) == 0)) {
			return node;
		}
	}

	return NULL;
}

EXPORT W residhash_adddata(residhash_t *residhash, TC *idstr, W idstr_len, UW attr, COLOR color)
{
	residhash_node_t *hashnode, *buf;
	W hashval;

	hashnode = residhash_searchnode(residhash, idstr, idstr_len);
	if (hashnode != NULL) {
		hashnode->attr = attr;
		hashnode->color = color;
		return 0;
	}

	hashval = residhash_calchashvalue(idstr, idstr_len);
	buf = residhash->tbl + hashval;

	hashnode = residhash_node_new(idstr, idstr_len, attr, color);
	if (hashnode == NULL) {
		return -1; /* TODO */
	}

	QueInsert(&(hashnode->queue), &(buf->queue));
	residhash->datanum++;

	return 0;
}

EXPORT W residhash_searchdata(residhash_t *residhash, TC *idstr, W idstr_len, UW *attr, COLOR *color)
{
	residhash_node_t *hashnode;

	hashnode = residhash_searchnode(residhash, idstr, idstr_len);
	if (hashnode == NULL) {
		return RESIDHASH_SEARCHDATA_NOTFOUND;
	}

	*attr = hashnode->attr;
	*color = hashnode->color;
	return RESIDHASH_SEARCHDATA_FOUND;
}

EXPORT VOID residhash_removedata(residhash_t *residhash, TC *idstr, W idstr_len)
{
	residhash_node_t *hashnode;

	hashnode = residhash_searchnode(residhash, idstr, idstr_len);
	if (hashnode == NULL) {
		return;
	}
	residhash_node_delete(hashnode);
	residhash->datanum--;
}

EXPORT W residhash_initialize(residhash_t *residhash)
{
	W i;
	for (i=0; i < RESIDHASH_BASE;i++) {
		QueInit(&(residhash->tbl[i].queue));
	}
	return 0;
}

EXPORT VOID residhash_finalize(residhash_t *residhash)
{
	W i;
	residhash_node_t *buf,*buf_next;
	Bool empty;

	for(i = 0; i < RESIDHASH_BASE; i++){
		buf = residhash->tbl + i;
		for(;;){
			empty = isQueEmpty(&(buf->queue));
			if (empty == True) {
				break;
			}
			buf_next = (residhash_node_t*)buf->queue.next;
			residhash_node_delete(buf_next);
		}
	}
}

EXPORT Bool residhash_iterator_next(residhash_iterator_t *iter, TC **idstr, W *idstr_len, W *attr, W *color)
{
	residhash_t *hash;
	residhash_node_t *node;

	hash = iter->residhash;

	if (iter->tbl_index == RESIDHASH_BASE) {
		return False;
	}

	*idstr = iter->node->id;
	*idstr_len = iter->node->id_len;
	*attr = iter->node->attr;
	*color = iter->node->color;

	for (;;) {
		node = (residhash_node_t*)iter->node->queue.next;
		if (hash->tbl + iter->tbl_index != node) {
			iter->node = node;
			break;
		}
		iter->tbl_index++;
		if (iter->tbl_index == RESIDHASH_BASE) {
			break;
		}
		iter->node = hash->tbl + iter->tbl_index;
	}

	return True;
}

EXPORT VOID residhash_iterator_initialize(residhash_iterator_t *iter, residhash_t *target)
{
	residhash_node_t *node;

	iter->residhash = target;
	iter->tbl_index = 0;
	iter->node = (residhash_node_t*)iter->residhash->tbl;
	for (;;) {
		node = (residhash_node_t*)iter->node->queue.next;
		if (iter->residhash->tbl + iter->tbl_index != node) {
			iter->node = node;
			break;
		}
		iter->tbl_index++;
		if (iter->tbl_index == RESIDHASH_BASE) {
			break;
		}
		iter->node = iter->residhash->tbl + iter->tbl_index;
	}
}

EXPORT VOID residhash_iterator_finalize(residhash_iterator_t *iter)
{
}
