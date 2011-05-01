/*
 * psvlexer.c
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

#include    "psvlexer.h"

#include	<basic.h>
#include	<bstdio.h>
#include	<bstring.h>

#ifdef BCHAN_CONFIG_DEBUG
# define DP(arg) printf arg
# define DP_ER(msg, err) printf("%s (%d/%x)\n", msg, err>>16, err)
#else
# define DP(arg) /**/
# define DP_ER(msg, err) /**/
#endif

/* Parenthesis-Separeted Values */

EXPORT VOID psvlexer_inputchar(psvlexer_t *lexer, UB *ch, W ch_len, psvlexer_result_t **result, W *result_len)
{
	switch (lexer->state) {
	case PSVLEXER_STATE_READ_VALUE:
		if ((ch[0] == '<')&&(ch_len == 1)) {
			lexer->state = PSVLEXER_STATE_LSTN;
			*result = NULL;
			*result_len = 0;
		} else if ((ch[0] == '\n')&&(ch_len == 1)) {
			lexer->buffer[0].type = PSVLEXER_RESULT_FIELDEND;
			*result = lexer->buffer;
			*result_len = 1;
		} else {
			lexer->buffer[0].type = PSVLEXER_RESULT_VALUE;
			memcpy(lexer->buffer[0].str, ch, ch_len);
			lexer->buffer[0].len = ch_len;
			*result = lexer->buffer;
			*result_len = 1;
		}
		break;
	case PSVLEXER_STATE_LSTN:
		if ((ch[0] == '>')&&(ch_len == 1)) {
			lexer->state = PSVLEXER_STATE_READ_VALUE;
			lexer->buffer[0].type = PSVLEXER_RESULT_SEPARATION;
			*result = lexer->buffer;
			*result_len = 1;
		} else if ((ch[0] == '\n')&&(ch_len == 1)) {
			lexer->state = PSVLEXER_STATE_READ_VALUE;
			lexer->buffer[0].type = PSVLEXER_RESULT_VALUE;
			lexer->buffer[0].str[0] = '<';
			lexer->buffer[0].len = 1;
			lexer->buffer[1].type = PSVLEXER_RESULT_FIELDEND;
			*result = lexer->buffer;
			*result_len = 2;
		} else if ((ch[0] == '<')&&(ch_len == 1)) {
			lexer->buffer[0].type = PSVLEXER_RESULT_VALUE;
			lexer->buffer[0].str[0] = '<';
			lexer->buffer[0].len = 1;
			*result = lexer->buffer;
			*result_len = 1;
		} else {
			lexer->state = PSVLEXER_STATE_READ_VALUE;
			lexer->buffer[0].type = PSVLEXER_RESULT_VALUE;
			lexer->buffer[0].str[0] = '<';
			lexer->buffer[0].len = 1;
			lexer->buffer[1].type = PSVLEXER_RESULT_VALUE;
			memcpy(lexer->buffer[1].str, ch, ch_len);
			lexer->buffer[1].len = ch_len;
			*result = lexer->buffer;
			*result_len = 2;
		}
		break;
	}
}

EXPORT VOID psvlexer_endinput(psvlexer_t *lexer, psvlexer_result_t **result, W *result_len)
{
	switch (lexer->state) {
	case PSVLEXER_STATE_READ_VALUE:
		lexer->buffer[0].type = PSVLEXER_RESULT_FIELDEND;
		*result = lexer->buffer;
		*result_len = 1;
		break;
	case PSVLEXER_STATE_LSTN:
		lexer->state = PSVLEXER_STATE_READ_VALUE;
		lexer->buffer[0].type = PSVLEXER_RESULT_VALUE;
		lexer->buffer[0].str[0] = '<';
		lexer->buffer[0].len = 1;
		lexer->buffer[1].type = PSVLEXER_RESULT_FIELDEND;
		*result = lexer->buffer;
		*result_len = 2;
		break;
	}
}

EXPORT W psvlexer_initialize(psvlexer_t *lexer)
{
	lexer->state = PSVLEXER_STATE_READ_VALUE;
	return 0;
}

EXPORT VOID psvlexer_finalize(psvlexer_t *lexer)
{
}
