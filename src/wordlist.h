/*
 * wordlist.h
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

#include    <basic.h>
#include	<bsys/queue.h>

#ifndef __WORDLIST_H__
#define __WORDLIST_H__

struct ngwordlist_node_t_ {
	QUEUE queue;
	TC *str;
	W len;
};
typedef struct ngwordlist_node_t_ ngwordlist_node_t;

IMPORT ngwordlist_node_t* ngwordlist_node_new(TC *str, W strlen);
IMPORT VOID ngwordlist_node_delete(ngwordlist_node_t *node);
IMPORT VOID ngwordlist_node_insert(ngwordlist_node_t *entry, ngwordlist_node_t *node);
IMPORT ngwordlist_node_t* ngwordlist_node_next(ngwordlist_node_t *node);

#endif
