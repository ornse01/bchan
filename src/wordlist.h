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

struct wordlist_node_t_ {
	QUEUE queue;
	TC *str;
	W len;
};
typedef struct wordlist_node_t_ wordlist_node_t;

struct wordlist_t_ {
	wordlist_node_t node;
};
typedef struct wordlist_t_ wordlist_t;

IMPORT W wordlist_initialize(wordlist_t *list);
IMPORT VOID wordlist_finalize(wordlist_t *list);
IMPORT Bool wordlist_searchwordbyindex(wordlist_t *list, W index, TC **str, W *len);
IMPORT Bool wordlist_checkexistbyword(wordlist_t *list, TC *str, W len);
IMPORT W wordlist_appendword(wordlist_t *list, TC *str, W len);
IMPORT Bool wordlist_removeword(wordlist_t *list, TC *str, W len);
IMPORT Bool wordlist_isempty(wordlist_t *list);

struct wordlist_iterator_t_ {
	wordlist_t *wordlist;
	wordlist_node_t *node;
};
typedef struct wordlist_iterator_t_ wordlist_iterator_t;

IMPORT VOID wordlist_iterator_initialize(wordlist_iterator_t *iter, wordlist_t *target);
IMPORT VOID wordlist_iterator_finalize(wordlist_iterator_t *iter);
IMPORT Bool wordlist_iterator_next(wordlist_iterator_t *iter, TC **str, W *len);

#endif
