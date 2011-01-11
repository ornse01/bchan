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
#include	<tstring.h>
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

EXPORT Bool wordlist_searchwordbyindex(wordlist_t *list, W index, TC **str, W *len)
{
	ngwordlist_node_t *node;
	W i;

	i = 0;
	node = ngwordlist_node_next(&list->node);
	for (; node != &list->node;) {
		if (i == index) {
			*str = node->str;
			*len = node->len;
			return True;
		}
		node = ngwordlist_node_next(node);
		i++;
	}

	return False;
}

LOCAL Bool wordlist_searchnodebyword(wordlist_t *list, TC *str, W len, ngwordlist_node_t **node)
{
	ngwordlist_node_t *node0;
	W result;

	node0 = ngwordlist_node_next(&list->node);
	for (; node0 != &list->node;) {
		if (node0->len == len) {
			result = tc_strncmp(node0->str, str, len);
			if (result == 0) {
				*node = node0;
				return True;
			}
		}
		node0 = ngwordlist_node_next(node0);
	}
	return False;
}

EXPORT Bool wordlist_checkexistbyword(wordlist_t *list, TC *str, W len)
{
	ngwordlist_node_t *node;
	return wordlist_searchnodebyword(list, str, len, &node);
}

EXPORT W wordlist_appendword(wordlist_t *list, TC *str, W len)
{
	ngwordlist_node_t *newnode;

	newnode = ngwordlist_node_new(str, len);
	if (newnode == NULL) {
		return -1; /* TODO */
	}
	ngwordlist_node_insert(newnode, &list->node);
	return 0;
}

EXPORT Bool wordlist_removeword(wordlist_t *list, TC *str, W len)
{
	ngwordlist_node_t *node;
	Bool found;
	found = wordlist_searchnodebyword(list, str, len, &node);
	if (found == True) {
		ngwordlist_node_delete(node);
	}
	return found;
}

EXPORT W wordlist_initialize(wordlist_t *list)
{
	QueInit(&(list->node.queue));
	return True;
}

EXPORT VOID wordlist_finalize(wordlist_t *list)
{
	ngwordlist_node_t *node;

	node = ngwordlist_node_next(&list->node);
	for (; node != &list->node;) {
		ngwordlist_node_delete(node);
		node = ngwordlist_node_next(&list->node);
	}
}

EXPORT Bool wordlist_iterator_next(wordlist_iterator_t *iter, TC **str, W *len)
{
	if (iter->node == &iter->wordlist->node) {
		return False;
	}

	*str = iter->node->str;
	*len = iter->node->len;

	iter->node = ngwordlist_node_next(iter->node);

	return True;
}

EXPORT VOID wordlist_iterator_initialize(wordlist_iterator_t *iter, wordlist_t *target)
{
	iter->wordlist = target;
	iter->node = ngwordlist_node_next(&target->node);
}

EXPORT VOID wordlist_iterator_finalize(wordlist_iterator_t *iter)
{
}
