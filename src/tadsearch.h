/*
 * tadsearch.h
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

#ifndef __TADSEARCH_H__
#define __TADSEARCH_H__

struct tcstrbuffer_t_ {
	TC *buffer;
	W strlen;
};
typedef struct tcstrbuffer_t_ tcstrbuffer_t;

struct tadlib_rk_result_t_ {
	W i;
};
typedef struct tadlib_rk_result_t_ tadlib_rk_result_t;

EXPORT W tadlib_rabinkarpsearch(TC *str, W len, tcstrbuffer_t *strset, W setlen, tadlib_rk_result_t *result);

#endif
