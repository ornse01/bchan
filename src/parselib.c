/*
 * parselib.c
 *
 * Copyright (c) 2009-2010 project bchan
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
# define DP_ER(msg, err) printf("%s (%d/%x)\n", msg, err>>16, err)
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

EXPORT W tokenchecker_inputcharacter(tokenchecker_t *checker, UB c)
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
		return TOKENCHECK_NOMATCH;
	}

	for (i=checker->StartIndex_of_list;i<checker->EndIndex_of_list;i++) {
		if ( (NameList[i]).name[checker->pos_of_EachString] == c ) {
			break;
		}
	}
	if (i==checker->EndIndex_of_list) { /*receive char is not matched.*/
		checker->flg_notexist = 1;
		return TOKENCHECK_NOMATCH;
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
		return TOKENCHECK_NOMATCH;
	}
	checker->pos_of_EachString++;
	return TOKENCHECK_CONTINUE;
}

EXPORT VOID tokenchecker_getparsingstring(tokenchecker_t *checker, UB **str, W *len)
{
	*str = (checker->NameList[checker->StartIndex_of_list]).name;
	*len = checker->pos_of_EachString + 1;
}

#define TOKENCHECKER2_FLAG_NOTEXIST 0x00000001
#define TOKENCHECKER2_FLAG_AFTERENDCHAR 0x00000002

EXPORT VOID tokenchecker2_initialize(tokenchecker2_t *checker, tokenchecker_valuetuple_t *namelist, W namelistnum, B *endchars)
{
	checker->namelist = namelist;
	checker->namelistnum = namelistnum;
	checker->endtokens = endchars;
	checker->stringindex = 0;
	checker->listindex_start = 0;
	checker->listindex_end = checker->namelistnum;
	checker->flag = 0;
}

EXPORT VOID tokenchecker2_clear(tokenchecker2_t *checker)
{
	checker->stringindex = 0;
	checker->listindex_start = 0;
	checker->listindex_end = checker->namelistnum;
	checker->flag = 0;
}

EXPORT W tokenchecker2_inputchar(tokenchecker2_t *checker, UB c, W *val)
{
	W i;
	tokenchecker_valuetuple_t *namelist = checker->namelist;

	if ((checker->flag & TOKENCHECKER2_FLAG_AFTERENDCHAR) != 0) {
		return TOKENCHECKER2_AFTER_END;
	}

	for (i = 0;; i++) {
		if ((checker->endtokens)[i] == '\0') {
			break;
		}
		if (c == (checker->endtokens)[i]) {
			checker->flag |= TOKENCHECKER2_FLAG_AFTERENDCHAR;
			if ((checker->flag & TOKENCHECKER2_FLAG_NOTEXIST) != 0) {
				return TOKENCHECKER2_NOMATCH;
			}
			if ((namelist[checker->listindex_start]).name[checker->stringindex] == '\0') {
				/*List's Name End and receive EndToken = found match string*/
				*val = (namelist[checker->listindex_start]).val;
				return TOKENCHECKER2_DETERMINE;
			}
			/*List's Name continue but receive endtoken.*/
			return TOKENCHECKER2_NOMATCH;
		}
	}

	if ((checker->flag & TOKENCHECKER2_FLAG_NOTEXIST) != 0) {
		return TOKENCHECKER2_CONTINUE_NOMATCH;
	}

	for (i = checker->listindex_start; i < checker->listindex_end; i++) {
		if ((namelist[i]).name[checker->stringindex] == c) {
			break;
		}
	}
	if (i == checker->listindex_end) { /*receive char is not matched.*/
		checker->flag &= TOKENCHECKER2_FLAG_NOTEXIST;
		return TOKENCHECKER2_CONTINUE_NOMATCH;
	}
	checker->listindex_start = i;
	for (i = i+1; i < checker->listindex_end; i++) {
		if ((namelist[i]).name[checker->stringindex] != c) {
			break;
		}
	}
	checker->listindex_end = i;

	if ((namelist[checker->listindex_start]).name[checker->stringindex] == '\0') {
		/*Don't recive endtoken but List's Name is end.*/
		checker->flag |= TOKENCHECKER2_FLAG_NOTEXIST;
		return TOKENCHECKER2_CONTINUE_NOMATCH;
	}
	checker->stringindex++;

	return TOKENCHECKER2_CONTINUE;
}

EXPORT VOID tokenchecker2_getlastmatchedstring(tokenchecker2_t *checker, UB **str, W *len)
{
	*str = (checker->namelist[checker->listindex_start]).name;
	*len = checker->stringindex;
}

LOCAL tokenchecker_valuetuple_t nList_nameref[] = {
  {"amp", '&'},
  {"gt", '>'},
  {"lt", '<'},
  {"quot", '"'},
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
	W ret, val;

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
		tokenchecker2_inputchar(&parser->named, ch, &val);
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
		ret = tokenchecker2_inputchar(&parser->named, ch, &val);
		if (ch == ';') {
			if (ret == TOKENCHECKER2_DETERMINE) {
				parser->charnumber = val;
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
	tokenchecker2_clear(&(parser->named));
}

EXPORT W charreferparser_initialize(charreferparser_t *parser)
{
	parser->state = START;
	parser->charnumber = 0;
	tokenchecker2_initialize(&(parser->named), nList_nameref, 4, eToken_nameref);
	return 0;
}

EXPORT VOID charreferparser_finalize(charreferparser_t *parser)
{
}
