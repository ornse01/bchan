/*
 * httpheaderlexer.h
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

#ifndef __HTTPHEADERLEXER_H__
#define __HTTPHEADERLEXER_H__

/* see rfc2616 4.2 Message Headers. */

struct httpheaderlexer_t_ {
	enum {
		HTTPHEADERLEXER_STATE_SEARCH_HEADER,
		HTTPHEADERLEXER_STATE_SEARCH_HEADER_CR,
		HTTPHEADERLEXER_STATE_READ_HEADER_FIELDNAME,
		HTTPHEADERLEXER_STATE_READ_HEADER_FIELDVALUE,
		HTTPHEADERLEXER_STATE_READ_HEADER_FIELDVALUE_LWS,
		HTTPHEADERLEXER_STATE_HERDER_CR,
		HTTPHEADERLEXER_STATE_READ_VALUE_MESSAGE,
	} state;
};
typedef struct httpheaderlexer_t_ httpheaderlexer_t;

/* should implement field-value? but difficult. */
enum HTTPHEADERLEXER_RESULT_T_ {
	HTTPHEADERLEXER_RESULT_NONE,
	HTTPHEADERLEXER_RESULT_FIELDNAME,
	HTTPHEADERLEXER_RESULT_FIELDNAME_END,
	HTTPHEADERLEXER_RESULT_FIELDCONTENT,
	HTTPHEADERLEXER_RESULT_LWS,
	HTTPHEADERLEXER_RESULT_MESSAGEHEADER_END,
};
typedef enum HTTPHEADERLEXER_RESULT_T_ HTTPHEADERLEXER_RESULT_T;

IMPORT W httpheaderlexer_initialize(httpheaderlexer_t *lexer);
IMPORT VOID httpheaderlexer_finalize(httpheaderlexer_t *lexer);
IMPORT VOID httpheaderlexer_inputchar(httpheaderlexer_t *lexer, UB ch, HTTPHEADERLEXER_RESULT_T *result);

#endif
