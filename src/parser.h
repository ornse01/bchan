/*
 * parser.h
 *
 * Copyright (c) 2009 project bchan
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
#include	<btron/tf.h>

#include    "cache.h"

#ifndef __PARSER_H__
#define __PARSER_H__

typedef struct datparser_t_ datparser_t;
typedef struct datparser_res_t_ datparser_res_t;
struct datparser_res_t_ {
	TC *name;
	W name_len;
	TC *mail;
	W mail_len;
	TC *date;
	W date_len;
	TC *body;
	W body_len;
	TC *title;
	W title_len;
};

IMPORT datparser_t* datparser_new(datcache_t *parser);
IMPORT VOID datparser_delete(datparser_t *parser);
IMPORT W datparser_getnextres(datparser_t *parser, datparser_res_t **res);
IMPORT VOID datparser_clear(datparser_t *parser);

IMPORT datparser_res_t* datparser_res_new();
IMPORT VOID datparser_res_delete(datparser_res_t *res);

#endif
