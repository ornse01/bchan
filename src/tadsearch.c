/*
 * tadsearch.c
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

#include    "tadsearch.h"
#include    "tadlib.h"

#include	<bstdio.h>
#include	<bstdlib.h>
#include	<tstring.h>

/* http://ja.wikipedia.org/wiki/%E3%83%A9%E3%83%93%E3%83%B3-%E3%82%AB%E3%83%BC%E3%83%97%E6%96%87%E5%AD%97%E5%88%97%E6%A4%9C%E7%B4%A2%E3%82%A2%E3%83%AB%E3%82%B4%E3%83%AA%E3%82%BA%E3%83%A0 */

/* Rabin-Karp String Search Algorithm */

struct tcloopbuf_t_ {
	W len;
	W pos;
	TC buf[1];
};
typedef struct tcloopbuf_t_ tcloopbuf_t;

LOCAL TC tcloopbuf_getbackchar(tcloopbuf_t *loopbuf, W n)
{
	return loopbuf->buf[(loopbuf->pos - 1 - n) % loopbuf->len];
}

LOCAL Bool tcloopbuf_compair(tcloopbuf_t *loopbuf, tcstrbuffer_t *str)
{
	W n, result;

	n = loopbuf->pos % loopbuf->len;

	result = tc_strncmp(loopbuf->buf, str->buffer + str->strlen - n, n);
	if (result != 0) {
		return False;
	}
	result = tc_strncmp(loopbuf->buf + str->strlen - n, str->buffer, str->strlen - n);
	if (result != 0) {
		return False;
	}
	return True;
}

LOCAL VOID tcloopbuf_putchar(tcloopbuf_t *loopbuf, TC ch)
{
	loopbuf->buf[loopbuf->pos++ % loopbuf->len] = ch;
}

LOCAL tcloopbuf_t *tcloopbuf_new(W len)
{
	tcloopbuf_t *loopbuf;

	loopbuf = malloc(sizeof(tcloopbuf_t) + sizeof(W)*len);
	if (loopbuf == NULL) {
		return NULL;
	}

	loopbuf->len = len + 1;
	loopbuf->pos = 0;

	return loopbuf;
}

LOCAL VOID tcloopbuf_delete(tcloopbuf_t *loopbuf)
{
	free(loopbuf);
}

struct tadlib_rk_hash_t_ {
	tcstrbuffer_t *orig;
	W expectedhash;
	W currenthash;
};
typedef struct tadlib_rk_hash_t_ tadlib_rk_hash_t;

struct tadlib_rk_hashset_t_ {
	W setlen;
	tadlib_rk_hash_t array[1];
};
typedef struct tadlib_rk_hashset_t_ tadlib_rk_hashset_t;

LOCAL W make_expectedhash(tcstrbuffer_t *buffer)
{
	W i, sum = 0;

	for (i = 0; i < buffer->strlen; i++) {
		sum += buffer->buffer[i];
	}

	return sum;
}

LOCAL Bool rk_hash_count(tadlib_rk_hash_t *hash, TC ch, W pos, tcloopbuf_t *loopbuf)
{
	TC prev_ch;
	Bool ok;

	if (pos < hash->orig->strlen) {
		hash->currenthash += ch;
		return False;
	}

	if (pos == hash->orig->strlen) {
		hash->currenthash += ch;
	} else {
		prev_ch = tcloopbuf_getbackchar(loopbuf, hash->orig->strlen);
		hash->currenthash += ch - prev_ch;
	}

	if (hash->currenthash == hash->expectedhash) {
		ok = tcloopbuf_compair(loopbuf, hash->orig);
		if (ok == True) {
			return True;
		}
	}

	return False;
}

LOCAL W rk_hashset_count(tadlib_rk_hashset_t *hashset, TC ch, W pos, tcloopbuf_t *loopbuf)
{
	W i;
	Bool hit;

	for (i = 0; i < hashset->setlen; i++) {
		hit = rk_hash_count(hashset->array + i, ch, pos, loopbuf);
		if (hit == True) {
			return i;
		}
	}

	return -1;
}

LOCAL tadlib_rk_hashset_t* rk_hashset_new(tcstrbuffer_t *strset, W setlen)
{
	tadlib_rk_hashset_t *hashset;
	W i;

	hashset = malloc(sizeof(W) + (sizeof(tadlib_rk_hash_t) * setlen));
	if (hashset == NULL) {
		return NULL;
	}
	hashset->setlen = setlen;
	for (i = 0; i < setlen; i++) {
		hashset->array[i].orig = strset + i;
		hashset->array[i].expectedhash = make_expectedhash(strset + i);
		hashset->array[i].currenthash = 0;
	}

	return hashset;
}

LOCAL VOID rk_hashset_delete(tadlib_rk_hashset_t *hashset)
{
	free(hashset);
}

LOCAL W tadlib_rk_getmaxlenfromstrset(tcstrbuffer_t *strset, W setlen)
{
	W i, max;

	max = -1;
	for (i = 0; i < setlen; i++) {
		if (strset[i].strlen > max) {
			max = strset[i].strlen;
		}
	}

	return max;
}

EXPORT W tadlib_rabinkarpsearch(TC *str, W len, tcstrbuffer_t *strset, W setlen, tadlib_rk_result_t *result)
{
	taditerator_t iter;
	TADITERATOR_RESULT_T iter_result;
	TC ch, *pos;
	LTADSEG *seg;
	W size;
	UB *data0;
	W found, max, ret = 0, n;
	tcloopbuf_t *loopbuf;
	tadlib_rk_hashset_t *hashset;

	max = tadlib_rk_getmaxlenfromstrset(strset, setlen);

	loopbuf = tcloopbuf_new(max);
	if (loopbuf == NULL) {
		return -1;
	}
	hashset = rk_hashset_new(strset, setlen);
	if (hashset == NULL) {
		tcloopbuf_delete(loopbuf);
		return -1;
	}

	taditerator_initialize(&iter, str, len);
	n = 0;
	for (;;) {
		iter_result = taditerator_next(&iter, &pos, &ch, &seg, &size, &data0);
		if (iter_result == TADITERATOR_RESULT_END) {
			break;
		}
		if (iter_result != TADITERATOR_RESULT_CHARCTOR) {
			continue;
		}
		n++;
		tcloopbuf_putchar(loopbuf, ch);
		found = rk_hashset_count(hashset, ch, n, loopbuf);
		if (found >= 0) {
			result->i = found;
			ret = 1;
			break;
		}
	}
	taditerator_finalize(&iter);

	rk_hashset_delete(hashset);
	tcloopbuf_delete(loopbuf);

	return ret;
}
