/*
 * array.c
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
#include	<bstdlib.h>
#include	<bstdio.h>
#include	<bsys/queue.h>

#include    "array.h"

#ifdef BCHAN_CONFIG_DEBUG
# define DP(arg) printf arg
# define DP_ER(msg, err) printf("%s (%d/%x)\n", msg, err>>16, err)
#else
# define DP(arg) /**/
# define DP_ER(msg, err) /**/
#endif

LOCAL arraybase_datanode_t* arraybase_datanode_next(arraybase_datanode_t *node)
{
	return (arraybase_datanode_t *)node->queue.next;
}

LOCAL arraybase_datanode_t* arraybase_datanode_prev(arraybase_datanode_t *node)
{
	return (arraybase_datanode_t *)node->queue.prev;
}

LOCAL VOID arraybase_datanode_insert(arraybase_datanode_t *node, arraybase_datanode_t *que)
{
	QueInsert(&node->queue, &que->queue);
}

LOCAL VOID arraybase_datanode_getunitbyindex(arraybase_datanode_t *node, W unitsize, W index, VP *p)
{
	/* comparing index and data lentgh is this user. */
	*p = (VP)(node->data + unitsize * index);
}

LOCAL VOID arraybase_datanode_writebyindex(arraybase_datanode_t *node, W unitsize, W index, VP p)
{
	VP dest;

	/* comparing index and data lentgh is this user. */
	arraybase_datanode_getunitbyindex(node, unitsize, index, &dest);
	memcpy(dest, p, unitsize);
}

LOCAL W arraybase_datanode_initialize(arraybase_datanode_t *node, W unitsize, W denom)
{
	QueInit(&node->queue);
	node->data = (UB*)malloc(sizeof(UB)*unitsize*denom);
	if (node->data == NULL) {
		return -1; /* TODO */
	}
	return 0;
}

LOCAL VOID arraybase_datanode_finalize(arraybase_datanode_t *node)
{
	free(node->data);
	QueRemove(&node->queue);
}

LOCAL arraybase_datanode_t* arraybase_datanode_new(W unitsize, W denom)
{
	arraybase_datanode_t *node;
	W err;

	node = (arraybase_datanode_t *)malloc(sizeof(arraybase_datanode_t));
	if (node == NULL) {
		return NULL;
	}
	err = arraybase_datanode_initialize(node, unitsize, denom);
	if (err < 0) {
		free(node);
	}

	return node;
}

LOCAL VOID arraybase_datanode_delete(arraybase_datanode_t *node)
{
	arraybase_datanode_finalize(node);
	free(node);
}

EXPORT Bool arraybase_getunitbyindex(arraybase_t *arraybase, W index, VP *p)
{
	W i, i_num, i_data;
	arraybase_datanode_t *node;

	if (index < 0) {
		return False;
	}
	if (index >= arraybase->datanum) {
		return False;
	}

	i_num = index / arraybase->denom;
	i_data = index % arraybase->denom;

	node = &arraybase->datalist;
	for (i = 0; i < i_num; i++) {
		node = arraybase_datanode_next(node);
	}

	arraybase_datanode_getunitbyindex(node, arraybase->unitsize, i_data, p);

	return True;
}

EXPORT Bool arraybase_getunitfirst(arraybase_t *arraybase, VP *p)
{
	/* TODO: more efficient. */
	return arraybase_getunitbyindex(arraybase, 0, p);
}

EXPORT Bool arraybase_getunitlast(arraybase_t *arraybase, VP *p)
{
	/* TODO: more efficient. */
	return arraybase_getunitbyindex(arraybase, arraybase->datanum - 1, p);
}

EXPORT W arraybase_appendunit(arraybase_t *arraybase, VP p)
{
	arraybase_datanode_t *node;
	W i_data;

	i_data = arraybase->datanum % arraybase->denom;
	if ((i_data == 0)&&(arraybase->datanum > 0)) {
		node = arraybase_datanode_new(arraybase->unitsize, arraybase->denom);
		if (node == NULL) {
			return -1; /* TODO */
		}
		arraybase_datanode_insert(node, &arraybase->datalist);
	} else {
		node = arraybase_datanode_prev(&arraybase->datalist);
	}

	arraybase_datanode_writebyindex(node, arraybase->unitsize, i_data, p);

	arraybase->datanum++;

	return 0;
}

EXPORT VOID arraybase_truncate(arraybase_t *arraybase, W newlength)
{
	W i, i_num;
	arraybase_datanode_t *node, *newlast;

	if (newlength >= arraybase->datanum) {
		return;
	}

	i_num = (newlength - 1) / arraybase->denom;
	node = &arraybase->datalist;
	for (i = 0; i < i_num; i++) {
		node = arraybase_datanode_next(node);
	}
	newlast = node;

	for (;;) {
		node = arraybase_datanode_prev(&arraybase->datalist);
		if (node == newlast) {
			break;
		}
		arraybase_datanode_delete(node);
	}

	arraybase->datanum = newlength;
}

EXPORT W arraybase_length(arraybase_t *arraybase)
{
	return arraybase->datanum;
}

EXPORT W arraybase_initialize(arraybase_t *arraybase, W unitsize, W denom)
{
	W err;

	err = arraybase_datanode_initialize(&arraybase->datalist, unitsize, denom);
	if (err < 0) {
		return err;
	}
	arraybase->unitsize = unitsize;
	arraybase->denom = denom;
	arraybase->datanum = 0;

	return 0;
}

EXPORT VOID arraybase_finalize(arraybase_t *arraybase)
{
	arraybase_datanode_t *node;

	for (;;) {
		node = arraybase_datanode_prev(&arraybase->datalist);
		if (node == &arraybase->datalist) {
			break;
		}
		arraybase_datanode_delete(node);
	}
	arraybase_datanode_finalize(&arraybase->datalist);
}
