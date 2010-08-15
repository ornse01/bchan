/*
 * tadimf.h
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

#include    "tadlib.h"

#ifndef __TADIMF_H__
#define __TADIMF_H__

enum TIMFPARSER_STATE_T_ {
	TIMFPARSER_STATE_SEARCH_HEADER,
	TIMFPARSER_STATE_SKIP_HEADER,
	TIMFPARSER_STATE_READ_HEADER_FIELDNAME,
	TIMFPARSER_STATE_SEARCH_HEADER_FIELDVALUE,
	TIMFPARSER_STATE_READ_HEADER_FIELDVALUE,
	TIMFPARSER_STATE_READ_VALUE_MESSAGE
};
typedef enum TIMFPARSER_STATE_T_ TIMFPARSER_STATE_T;

typedef struct timfparser_t_ timfparser_t;
struct timfparser_t_ {
	taditerator_t iterator;
	tctokenchecker_t headerchecker;
	TIMFPARSER_STATE_T state;
	Bool ishankaku;
	W headertype;
	TC chratio[5];
	TC buf[6];
};

enum TIMFPARSER_RESULT_T_ {
	TIMFPARSER_RESULT_HEADERVALUE,
	TIMFPARSER_RESULT_BODY,
	TIMFPARSER_RESULT_END
};
typedef enum TIMFPARSER_RESULT_T_ TIMFPARSER_RESULT_T;

IMPORT VOID timfparser_initialize(timfparser_t *timf, tctokenchecker_valuetuple_t *val, W val_len, TC *bin, W strlen);
IMPORT VOID timfparser_finalize(timfparser_t *timf);
IMPORT VOID timfparser_clear(timfparser_t *timf);
IMPORT W timfparser_next(timfparser_t *timf, W *headername, UB **bin, W *len);

#endif
