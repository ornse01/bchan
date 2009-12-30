/*
 * parselib.c
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
#include	<bstdlib.h>
#include	<bstdio.h>
#include	<bstring.h>
#include	<bctype.h>

#include    "parselib.h"

#ifdef BCHAN_CONFIG_DEBUG
# define DP(arg) printf arg
# define DP_ER(msg, err) printf("%s (%d/%z)\n", msg, err>>16, err)
#else
# define DP(arg) /**/
# define DP_ER(msg, err) /**/
#endif

EXPORT VOID tokenchecker_initialize(tokenchecker_t *checker, tokenchecker_valuetuple_t *namelist, B *endchars)
{
	W i;

	for (i=1;;i++) {
		if (namelist[i].name == NULL) {
			break;
		}
	}

	checker->NameList = namelist;
	checker->num_of_list = i;
	checker->pos_of_EachString = 0;
	checker->StartIndex_of_list = 1;
	checker->EndIndex_of_list = i;
	checker->flg_notexist = 0;
	checker->endtokens = endchars;
}

EXPORT VOID tokenchecker_resetstate(tokenchecker_t *checker)
{
	checker->pos_of_EachString = 0;
	checker->StartIndex_of_list = 1;
	checker->EndIndex_of_list = checker->num_of_list;
	checker->flg_notexist = 0;
}

EXPORT W tokenchecker_inputcharacter(tokenchecker_t *checker, B c)
{
	W i;
	tokenchecker_valuetuple_t *NameList = checker->NameList;

	for (i=0;;i++) {
		if ((checker->endtokens)[i] == '\0') {
			break;
		}
		if (c == (checker->endtokens)[i]) {
			if (checker->flg_notexist) {
				return TOKENCHECK_NOMATCH;
			}
			if ((NameList[checker->StartIndex_of_list]).name[checker->pos_of_EachString] == '\0') {
				/*List's Name End and receive EndToken = found match string*/
				return (NameList[checker->StartIndex_of_list]).val;
			}
			/*List's Name continue but receive endtoken.*/
			return TOKENCHECK_NOMATCH;
		}
	}

	if (checker->flg_notexist) {
		return TOKENCHECK_CONTINUE;
	}

	for (i=checker->StartIndex_of_list;i<checker->EndIndex_of_list;i++) {
		if ( (NameList[i]).name[checker->pos_of_EachString] == c ) {
			break;
		}
	}
	if (i==checker->EndIndex_of_list) { /*receive char is not matched.*/
		checker->flg_notexist = 1;
		return TOKENCHECK_CONTINUE;
	}
	checker->StartIndex_of_list = i;
	for (i=i+1;i<checker->EndIndex_of_list;i++) {
		if ( (NameList[i]).name[checker->pos_of_EachString] != c ) {
			break;
		}
	}
	checker->EndIndex_of_list = i;

	if ((NameList[checker->StartIndex_of_list]).name[checker->pos_of_EachString] == '\0') {
		/*Don't recive endtoken but List's Name is end.*/
		checker->flg_notexist = 1;
		return TOKENCHECK_CONTINUE;
	}
	checker->pos_of_EachString++;
	return TOKENCHECK_CONTINUE;
}

LOCAL tokenchecker_valuetuple_t nList_nameref[] = {
  {NULL,0},
  {"amp", '&'},
  {"gt", '>'},
  {"lt", '<'},
  {"quot", '"'},
  {NULL,0}
};
LOCAL B eToken_nameref[] = ";";

LOCAL W charreferparser_digitchartointeger(UB ch)
{
	return ch - '0';
}

LOCAL W charreferparser_hexchartointeger(UB ch)
{
	if(('a' <= ch)&&(ch <= 'h')){
		return ch - 'a' + 10;
	}
	if(('A' <= ch)&&(ch <= 'H')){
		return ch - 'A' + 10;
	}
	return charreferparser_digitchartointeger(ch);
}

EXPORT charreferparser_result_t charreferparser_parsechar(charreferparser_t *parser, UB ch)
{
	W err;

	switch (parser->state) {
	case START:
		if (ch != '&') {
			return CHARREFERPARSER_RESULT_INVALID;
		}
		parser->state = RECIEVE_AMP;
		return CHARREFERPARSER_RESULT_CONTINUE;
	case RECIEVE_AMP:
		if (ch == '#') {
			parser->state = RECIEVE_NUMBER;
			return CHARREFERPARSER_RESULT_CONTINUE;
		}
		if (ch == ';') {
			return CHARREFERPARSER_RESULT_INVALID;
		}
		/* TODO */
		parser->state = NAMED;
		tokenchecker_inputcharacter(&parser->named, ch);
		parser->charnumber = -1;
		return CHARREFERPARSER_RESULT_CONTINUE;
	case RECIEVE_NUMBER:
		if ((ch == 'x')||(ch == 'X')) {
			parser->state = NUMERIC_HEXADECIMAL;
			return CHARREFERPARSER_RESULT_CONTINUE;
		}
		if (isdigit(ch)) {
			parser->state = NUMERIC_DECIMAL;
			parser->charnumber = charreferparser_digitchartointeger(ch);
			return CHARREFERPARSER_RESULT_CONTINUE;
		}
		return CHARREFERPARSER_RESULT_INVALID;
	case NUMERIC_DECIMAL:
		if (isdigit(ch)) {
			parser->charnumber *= 10;
			parser->charnumber += charreferparser_digitchartointeger(ch);
			return CHARREFERPARSER_RESULT_CONTINUE;
		}
		if (ch == ';') {
			parser->state = DETERMINED;
			return CHARREFERPARSER_RESULT_DETERMINE;
		}
		return CHARREFERPARSER_RESULT_INVALID;
	case NUMERIC_HEXADECIMAL:
		if (isxdigit(ch)) {
			parser->charnumber *= 16;
			parser->charnumber += charreferparser_hexchartointeger(ch);
			return CHARREFERPARSER_RESULT_CONTINUE;
		}
		if (ch == ';') {
			parser->state = DETERMINED;
			return CHARREFERPARSER_RESULT_DETERMINE;
		}
		return CHARREFERPARSER_RESULT_INVALID;
	case NAMED:
		err = tokenchecker_inputcharacter(&parser->named, ch);
		if (ch == ';') {
			if (err > 0) {
				parser->charnumber = err;
			}
			parser->state = DETERMINED;
			return CHARREFERPARSER_RESULT_DETERMINE;
		}
		return CHARREFERPARSER_RESULT_CONTINUE;
	case INVALID:
		if (ch == ';') {
			parser->state = DETERMINED;
			return CHARREFERPARSER_RESULT_DETERMINE;
		}
		return CHARREFERPARSER_RESULT_CONTINUE;
	case DETERMINED:
		return CHARREFERPARSER_RESULT_INVALID;
	}

	return CHARREFERPARSER_RESULT_INVALID;
}

EXPORT W charreferparser_getcharnumber(charreferparser_t *parser)
{
	if (parser->state != DETERMINED) {
		return -1;
	}
	return parser->charnumber;
}

EXPORT VOID charreferparser_resetstate(charreferparser_t *parser)
{
	parser->state = START;
	parser->charnumber = 0;
	tokenchecker_resetstate(&(parser->named));
}

EXPORT W charreferparser_initialize(charreferparser_t *parser)
{
	parser->state = START;
	parser->charnumber = 0;
	tokenchecker_initialize(&(parser->named), nList_nameref, eToken_nameref);
	return 0;
}

EXPORT VOID charreferparser_finalize(charreferparser_t *parser)
{
}
