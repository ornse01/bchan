/*
 * wordlist.c
 *
 * Copyright (c) 2011 project bchan
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

#include    "wordlist.h"

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

EXPORT VOID ngwordlist_node_insert(ngwordlist_node_t *entry, ngwordlist_node_t *node)
{
	QueInsert(&(entry->queue), &(node->queue));
}

EXPORT ngwordlist_node_t* ngwordlist_node_next(ngwordlist_node_t *node)
{
	return (ngwordlist_node_t *)(node->queue.next);
}

EXPORT ngwordlist_node_t* ngwordlist_node_new(TC *str, W strlen)
{
	ngwordlist_node_t *node;

	node = (ngwordlist_node_t*)malloc(sizeof(ngwordlist_node_t));
	if (node == NULL) {
		return NULL;
	}

	node->str = malloc(sizeof(TC)*strlen);
	if (node->str == NULL) {
		free(node);
		return NULL;
	}
	memcpy(node->str, str, sizeof(TC)*strlen);
	node->len = strlen;
	QueInit(&(node->queue));

	return node;
}

EXPORT VOID ngwordlist_node_delete(ngwordlist_node_t *node)
{
	QueRemove(&(node->queue));
	free(node->str);
	free(node);
}
