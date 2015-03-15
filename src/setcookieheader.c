/*
 * setcookieheader.c
 *
 * Copyright (c) 2011-2015 project bchan
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

#include    "setcookieheader.h"
#include    "parselib.h"
#include    "httpdateparser.h"

#include	<basic.h>
#include	<bstdio.h>

#ifdef BCHAN_CONFIG_DEBUG
# define DP(arg) printf arg
# define DP_ER(msg, err) printf("%s (%d/%x)\n", msg, err>>16, err)
#else
# define DP(arg) /**/
# define DP_ER(msg, err) /**/
#endif

#if 0
#define DP_STATE(arg) printf arg
#else
#define DP_STATE(arg) /**/
#endif

enum HTTPCOOKIEGENERAL_RESULT_T_ {
	HTTPCOOKIEGENERAL_RESULT_NONE,
	HTTPCOOKIEGENERAL_RESULT_ATTR,
	HTTPCOOKIEGENERAL_RESULT_VALUE,
	HTTPCOOKIEGENERAL_RESULT_AVPAIR_END,
};
typedef enum HTTPCOOKIEGENERAL_RESULT_T_ HTTPCOOKIEGENERAL_RESULT_T;

LOCAL VOID httpcookiegeneral_inputchar(httpcookiegeneral_t *lexer, UB ch, HTTPCOOKIEGENERAL_RESULT_T *result)
{
	*result = HTTPCOOKIEGENERAL_RESULT_NONE;

	switch (lexer->state) {
	case HTTPCOOKIEGENERAL_STATE_SEARCH_ATTR:
		DP_STATE(("state = SEARCH_ATTR: %c[%02x]\n", ch, ch));
		if (ch == ' ') {
			break;
		}
		if (ch == ';') {
			break;
		}
		if (ch == '=') {
			/* TODO error */
			break;
		}
		lexer->state = HTTPCOOKIEGENERAL_STATE_READ_ATTR;
		*result = HTTPCOOKIEGENERAL_RESULT_ATTR;
		break;
	case HTTPCOOKIEGENERAL_STATE_READ_ATTR:
		DP_STATE(("state = READ_ATTR: %c[%02x]\n", ch, ch));
		if (ch == ' ') {
			lexer->state = HTTPCOOKIEGENERAL_STATE_SEARCH_CH_EQ;
			break;
		}
		if (ch == '=') {
			lexer->state = HTTPCOOKIEGENERAL_STATE_SEARCH_VALUE;
			break;
		}
		if (ch == ';') {
			lexer->state = HTTPCOOKIEGENERAL_STATE_SEARCH_ATTR;
			*result = HTTPCOOKIEGENERAL_RESULT_AVPAIR_END;
			break;
		}
		*result = HTTPCOOKIEGENERAL_RESULT_ATTR;
		break;
	case HTTPCOOKIEGENERAL_STATE_SEARCH_CH_EQ:
		DP_STATE(("state = SEARCH_CH_EQ: %c[%02x]\n", ch, ch));
		if (ch == '=') {
			lexer->state = HTTPCOOKIEGENERAL_STATE_SEARCH_VALUE;
		}
		if (ch == ';') {
			lexer->state = HTTPCOOKIEGENERAL_STATE_SEARCH_ATTR;
			*result = HTTPCOOKIEGENERAL_RESULT_AVPAIR_END;
			break;
		}
		if (ch != ' ') {
			/* TODO: error */
		}
		break;
	case HTTPCOOKIEGENERAL_STATE_SEARCH_VALUE:
		DP_STATE(("state = SEARCH_VALUE: %c[%02x]\n", ch, ch));
		if (ch == ' ') {
			break;
		}
		if (ch == ';') {
			lexer->state = HTTPCOOKIEGENERAL_STATE_SEARCH_ATTR;
			*result = HTTPCOOKIEGENERAL_RESULT_AVPAIR_END;
			break;
		}
		lexer->state = HTTPCOOKIEGENERAL_STATE_READ_VALUE;
		*result = HTTPCOOKIEGENERAL_RESULT_VALUE;
		break;
	case HTTPCOOKIEGENERAL_STATE_READ_VALUE:
		DP_STATE(("state = READ_VALUE: %c[%02x]\n", ch, ch));
		if (ch == ';') {
			lexer->state = HTTPCOOKIEGENERAL_STATE_SEARCH_ATTR;
			*result = HTTPCOOKIEGENERAL_RESULT_AVPAIR_END;
			break;
		}
		*result = HTTPCOOKIEGENERAL_RESULT_VALUE;
		break;
	}
}

LOCAL W httpcookiegeneral_initialize(httpcookiegeneral_t *lexer)
{
	lexer->state = HTTPCOOKIEGENERAL_STATE_SEARCH_ATTR;
	return 0;
}

LOCAL VOID httpcookiegeneral_finalize(httpcookiegeneral_t *lexer)
{
}

LOCAL VOID setcookieparser_inputchar_resultNAME_ch(setcookieparser_t *parser, UB ch, setcookieparser_result_t *result)
{
	parser->buf_str[0] = ch;
	result->type = SETCOOKIEPARSER_RESULT_TYPE_NAMEATTR;
	result->val.name.str = parser->buf_str;
	result->val.name.len = 1;
}

LOCAL VOID setcookieparser_inputchar_resultVALUE_ch(setcookieparser_t *parser, UB ch, setcookieparser_result_t *result, SETCOOKIEPARSER_ATTR_T attr)
{
	result->type = SETCOOKIEPARSER_RESULT_TYPE_VALUE;
	parser->buf_str[0] = ch;
	result->val.value.str = parser->buf_str;
	result->val.value.len = 1;
	result->val.value.attr = attr;
}

EXPORT W setcookieparser_inputchar(setcookieparser_t *parser, UB ch, setcookieparser_result_t **result, W *result_len)
{
	W ret, val, err;
	HTTPCOOKIEGENERAL_RESULT_T res;

	*result_len = 0;

	httpcookiegeneral_inputchar(&parser->lexer, ch, &res);
	if (res == HTTPCOOKIEGENERAL_RESULT_NONE) {
		return SETCOOKIEPARSER_CONTINUE;
	}

	switch (parser->state) {
	case SETCOOKIEPARSER_STATE_SEARCH_ATTR:
		if (res != HTTPCOOKIEGENERAL_RESULT_ATTR) {
			return SETCOOKIEPARSER_ERROR;
		}
		parser->state = SETCOOKIEPARSER_STATE_READ_ATTR;
		tokenchecker_clear(&parser->attrchecker);
	case SETCOOKIEPARSER_STATE_READ_ATTR:
		if (res == HTTPCOOKIEGENERAL_RESULT_ATTR) {
			ret = tokenchecker_inputchar(&parser->attrchecker, ch, &val);
			if (ret == TOKENCHECKER_CONTINUE) {
				return SETCOOKIEPARSER_CONTINUE;
			}
			if (ret == TOKENCHECKER_CONTINUE_NOMATCH) {
				parser->state = SETCOOKIEPARSER_STATE_READ_NAMEATTR;
				parser->buffer[0].type = SETCOOKIEPARSER_RESULT_TYPE_NAMEATTR;
				tokenchecker_getlastmatchedstring(&parser->attrchecker, &parser->buffer[0].val.name.str, &parser->buffer[0].val.name.len);
				setcookieparser_inputchar_resultNAME_ch(parser, ch, parser->buffer + 1);
				*result = parser->buffer;
				*result_len = 2;
				return SETCOOKIEPARSER_CONTINUE;
			}
			parser->state = SETCOOKIEPARSER_STATE_SKIP_AVPAIR;
			return SETCOOKIEPARSER_CONTINUE;
		}
		ret = tokenchecker_endinput(&parser->attrchecker, &val);
		if (ret != TOKENCHECKER_DETERMINE) {
			parser->state = SETCOOKIEPARSER_STATE_VALUE_NAME;
			parser->buffer[0].type = SETCOOKIEPARSER_RESULT_TYPE_NAMEATTR;
			tokenchecker_getlastmatchedstring(&parser->attrchecker, &parser->buffer[0].val.name.str, &parser->buffer[0].val.name.len);
			setcookieparser_inputchar_resultNAME_ch(parser, ch, parser->buffer + 1);
			*result = parser->buffer;
			*result_len = 2;
			return SETCOOKIEPARSER_CONTINUE;
		}
		if (res == HTTPCOOKIEGENERAL_RESULT_VALUE) {
			if (val == SETCOOKIEPARSER_ATTR_SECURE) {
				parser->state = SETCOOKIEPARSER_STATE_VALUE_UNSUPPORTED;
			} else if (val == SETCOOKIEPARSER_ATTR_EXPIRES) {
				parser->state = SETCOOKIEPARSER_STATE_VALUE_EXPIRES;
				rfc733dateparser_initialize(&parser->dateparser);
				err = rfc733dateparser_inputchar(&parser->dateparser, ch, &parser->date);
				if (err != HTTPDATEPARSER_CONTINUE) {
					return SETCOOKIEPARSER_ERROR;
				}
			} else {
				parser->state = SETCOOKIEPARSER_STATE_VALUE_SUPPORTED;
				parser->attr = val;
				setcookieparser_inputchar_resultVALUE_ch(parser, ch, parser->buffer, parser->attr);
				*result = parser->buffer;
				*result_len = 1;
			}
			return SETCOOKIEPARSER_CONTINUE;
		}
		if (res == HTTPCOOKIEGENERAL_RESULT_AVPAIR_END) {
			parser->state = SETCOOKIEPARSER_STATE_SEARCH_ATTR;
			if (val == SETCOOKIEPARSER_ATTR_SECURE) {
				parser->buffer[0].type = SETCOOKIEPARSER_RESULT_TYPE_SECUREATTR;
				*result = parser->buffer;
				*result_len = 1;
			}
			return SETCOOKIEPARSER_CONTINUE;
		}
		break;
	case SETCOOKIEPARSER_STATE_READ_NAMEATTR:
		if (res == HTTPCOOKIEGENERAL_RESULT_VALUE) {
			parser->state = SETCOOKIEPARSER_STATE_VALUE_NAME;
			setcookieparser_inputchar_resultVALUE_ch(parser, ch, parser->buffer, SETCOOKIEPARSER_ATTR_NAME);
			*result = parser->buffer;
			*result_len = 1;
			break;
		} else if (res == HTTPCOOKIEGENERAL_RESULT_AVPAIR_END) {
			parser->state = SETCOOKIEPARSER_STATE_SEARCH_ATTR;
			break;
		}
		setcookieparser_inputchar_resultNAME_ch(parser, ch, parser->buffer);
		*result = parser->buffer;
		*result_len = 1;
		break;
	case SETCOOKIEPARSER_STATE_SKIP_AVPAIR:
	case SETCOOKIEPARSER_STATE_VALUE_UNSUPPORTED:
		if (res == HTTPCOOKIEGENERAL_RESULT_AVPAIR_END) {
			parser->state = SETCOOKIEPARSER_STATE_SEARCH_ATTR;
			break;
		}
		break;
	case SETCOOKIEPARSER_STATE_VALUE_SUPPORTED:
		if (res == HTTPCOOKIEGENERAL_RESULT_AVPAIR_END) {
			parser->state = SETCOOKIEPARSER_STATE_SEARCH_ATTR;
			break;
		}
		setcookieparser_inputchar_resultVALUE_ch(parser, ch, parser->buffer, parser->attr);
		*result = parser->buffer;
		*result_len = 1;
		break;
	case SETCOOKIEPARSER_STATE_VALUE_NAME:
		if (res == HTTPCOOKIEGENERAL_RESULT_AVPAIR_END) {
			parser->state = SETCOOKIEPARSER_STATE_SEARCH_ATTR;
			break;
		}
		setcookieparser_inputchar_resultVALUE_ch(parser, ch, parser->buffer, SETCOOKIEPARSER_ATTR_NAME);
		*result = parser->buffer;
		*result_len = 1;
		break;
	case SETCOOKIEPARSER_STATE_VALUE_EXPIRES:
		if (res == HTTPCOOKIEGENERAL_RESULT_AVPAIR_END) {
			parser->state = SETCOOKIEPARSER_STATE_SEARCH_ATTR;
			err = rfc733dateparser_endinput(&parser->dateparser, &parser->date);
			if (err == HTTPDATEPARSER_DETERMINE) {
				parser->buffer[0].type = SETCOOKIEPARSER_RESULT_TYPE_EXPIRESATTR;
				set_tod(&parser->date, &(parser->buffer[0].val.expires.time), 0);
				*result = parser->buffer;
				*result_len = 1;
			}
			rfc733dateparser_finalize(&parser->dateparser);
			break;
		}
		err = rfc733dateparser_inputchar(&parser->dateparser, ch, &parser->date);
		if (err != HTTPDATEPARSER_CONTINUE) {
			return SETCOOKIEPARSER_ERROR;
		}
		break;
	}

	return SETCOOKIEPARSER_CONTINUE;
}

EXPORT W setcookieparser_endinput(setcookieparser_t *parser, setcookieparser_result_t **result, W *result_len)
{
	W ret, val;

	*result_len = 0;

	switch (parser->state) {
	case SETCOOKIEPARSER_STATE_READ_ATTR:
		ret = tokenchecker_endinput(&parser->attrchecker, &val);
		if (ret != TOKENCHECKER_DETERMINE) {
			break;
		}
		if (val != SETCOOKIEPARSER_ATTR_SECURE) {
			break;
		}
		parser->buffer[0].type = SETCOOKIEPARSER_RESULT_TYPE_SECUREATTR;
		*result = parser->buffer;
		*result_len = 1;
		break;
	case SETCOOKIEPARSER_STATE_SEARCH_ATTR:
	case SETCOOKIEPARSER_STATE_READ_NAMEATTR:
	case SETCOOKIEPARSER_STATE_SKIP_AVPAIR:
	case SETCOOKIEPARSER_STATE_VALUE_NAME:
	case SETCOOKIEPARSER_STATE_VALUE_SUPPORTED:
	case SETCOOKIEPARSER_STATE_VALUE_UNSUPPORTED:
	default:
	}

	return SETCOOKIEPARSER_CONTINUE;
}

LOCAL tokenchecker_valuetuple_t nList_attr[] = {
	{"Comment", SETCOOKIEPARSER_ATTR_COMMENT},
	{"Domain", SETCOOKIEPARSER_ATTR_DOMAIN},
	{"Expires", SETCOOKIEPARSER_ATTR_EXPIRES},
	{"Max-age", SETCOOKIEPARSER_ATTR_MAX_AGE},
	{"Path", SETCOOKIEPARSER_ATTR_PATH},
	{"Secure", SETCOOKIEPARSER_ATTR_SECURE},
	{"Versions", SETCOOKIEPARSER_ATTR_VERSION},
	{"comment", SETCOOKIEPARSER_ATTR_COMMENT},
	{"domain", SETCOOKIEPARSER_ATTR_DOMAIN},
	{"expires", SETCOOKIEPARSER_ATTR_EXPIRES},
	{"max-age", SETCOOKIEPARSER_ATTR_MAX_AGE},
	{"path", SETCOOKIEPARSER_ATTR_PATH},
	{"secure", SETCOOKIEPARSER_ATTR_SECURE},
	{"versions", SETCOOKIEPARSER_ATTR_VERSION},
};
LOCAL B eToken_attr[] = " =;";

EXPORT W setcookieparser_initialize(setcookieparser_t *parser)
{
	parser->state = SETCOOKIEPARSER_STATE_SEARCH_ATTR;
	parser->attr = 0;
	tokenchecker_initialize(&(parser->attrchecker), nList_attr, 14, eToken_attr);
	return httpcookiegeneral_initialize(&parser->lexer);
}

EXPORT VOID setcookieparser_finalize(setcookieparser_t *parser)
{
	httpcookiegeneral_finalize(&parser->lexer);
}
