/*
 * psvlexer.h
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

#ifndef __PSVLEXER_H__
#define __PSVLEXER_H__

struct psvlexer_result_t_ {
	enum {
		PSVLEXER_RESULT_VALUE,
		PSVLEXER_RESULT_SEPARATION,
		PSVLEXER_RESULT_FIELDEND,
		PSVLEXER_RESULT_ERROR,
	} type;
	UB str[2];
	W len;
};
typedef struct psvlexer_result_t_ psvlexer_result_t;

struct psvlexer_t_ {
	enum {
		PSVLEXER_STATE_READ_VALUE,
		PSVLEXER_STATE_LSTN,
	} state;
	 psvlexer_result_t buffer[2];
};
typedef struct psvlexer_t_ psvlexer_t;

IMPORT W psvlexer_initialize(psvlexer_t *lexer);
IMPORT VOID psvlexer_finalize(psvlexer_t *lexer);
IMPORT VOID psvlexer_inputchar(psvlexer_t *lexer, UB *ch, W ch_len, psvlexer_result_t **result, W *result_len);
IMPORT VOID psvlexer_endinput(psvlexer_t *lexer, psvlexer_result_t **result, W *result_len);

#endif
