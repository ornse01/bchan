/*
 * parselib.h
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

#include	<basic.h>

#ifndef __PARSELIB_H__
#define __PARSELIB_H__

typedef struct tokenchecker_valuetuple_t_ tokenchecker_valuetuple_t;
struct tokenchecker_valuetuple_t_ {
	B *name;
	W val;
};

typedef struct  tokenchecker_t_ tokenchecker_t;
struct  tokenchecker_t_ {
	tokenchecker_valuetuple_t *NameList;
	W num_of_list;
	W pos_of_EachString;
	W StartIndex_of_list;
	W EndIndex_of_list;
	W flg_notexist;
	B *endtokens;
};

#define TOKENCHECK_CONTINUE 0
#define TOKENCHECK_NOMATCH -1

IMPORT VOID tokenchecker_initialize(tokenchecker_t *checker, tokenchecker_valuetuple_t *namelist, B *endchars);
IMPORT VOID tokenchecker_resetstate(tokenchecker_t *checker);
IMPORT W tokenchecker_inputcharacter(tokenchecker_t *checker, B c);

typedef struct charreferparser_t_ charreferparser_t;
struct charreferparser_t_ {
	enum {
		START,
		RECIEVE_AMP,
		RECIEVE_NUMBER,
		NUMERIC_DECIMAL,
		NUMERIC_HEXADECIMAL,
		NAMED,
		INVALID,
		DETERMINED
	} state;
	tokenchecker_t named;
	W charnumber;
};

typedef enum {
	CHARREFERPARSER_RESULT_CONTINUE,
	CHARREFERPARSER_RESULT_DETERMINE,
	CHARREFERPARSER_RESULT_INVALID
} charreferparser_result_t;

IMPORT W charreferparser_initialize(charreferparser_t *parser);
IMPORT VOID charreferparser_finalize(charreferparser_t *parser);
IMPORT charreferparser_result_t charreferparser_parsechar(charreferparser_t *parser, UB ch);
IMPORT W charreferparser_getcharnumber(charreferparser_t *parser);
IMPORT W charreferparser_parsestr(charreferparser_t *parser, UB *str, W len);
IMPORT VOID charreferparser_resetstate(charreferparser_t *parser);

#endif
