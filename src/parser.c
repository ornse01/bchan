/*
 * parser.c
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

#include    "cache.h"
#include    "parser.h"
#include    "parselib.h"

#include	<bstdio.h>
#include	<bstdlib.h>
#include	<btron/tf.h>

#ifdef BCHAN_CONFIG_DEBUG
# define DP(arg) printf arg
# define DP_ER(msg, err) printf("%s (%d/%z)\n", msg, err>>16, err)
#else
# define DP(arg) /**/
# define DP_ER(msg, err) /**/
#endif

typedef enum {
	datparser_elmname_a = 1,
	datparser_elmname_br,
	datparser_elmname_hr
} datparser_elmname_t;

struct datparser_t_ {
	datcache_t *cache;
	TF_CTX ctx;
	W i;
	/**/
	enum {
		STATE_NAME,
		STATE_MAIL,
		STATE_DATE,
		STATE_BODY,
		STATE_TITLE
	} state;
	enum {
		STRSTATE_START,
		STRSTATE_LSTN,
		STRSTATE_ELMNAME,
		STRSTATE_SEARCH_END,
		STRSTATE_CHARREF,
	} strstate;
	tokenchecker_t elmname;
	charreferparser_t charref;
	datparser_res_t *resbuffer;
};

EXPORT datparser_res_t* datparser_res_new()
{
	datparser_res_t *res;

	res = malloc(sizeof(datparser_res_t));
	if (res == NULL) {
		return NULL;
	}
	res->name = NULL;
	res->name_len = 0;
	res->mail = NULL;
	res->mail_len = 0;
	res->date = NULL;
	res->date_len = 0;
	res->body = NULL;
	res->body_len = 0;
	res->title = NULL;
	res->title_len = 0;

	return res;
}

LOCAL VOID datparsr_res_clear(datparser_res_t *res)
{
	if (res->title != NULL) {
		free(res->title);
	}
	if (res->body != NULL) {
		free(res->body);
	}
	if (res->date != NULL) {
		free(res->date);
	}
	if (res->mail != NULL) {
		free(res->mail);
	}
	if (res->name != NULL) {
		free(res->name);
	}
	res->name = NULL;
	res->name_len = 0;
	res->mail = NULL;
	res->mail_len = 0;
	res->date = NULL;
	res->date_len = 0;
	res->body = NULL;
	res->body_len = 0;
	res->title = NULL;
	res->title_len = 0;
}

EXPORT VOID datparser_res_delete(datparser_res_t *res)
{
	if (res->title != NULL) {
		free(res->title);
	}
	if (res->body != NULL) {
		free(res->body);
	}
	if (res->date != NULL) {
		free(res->date);
	}
	if (res->mail != NULL) {
		free(res->mail);
	}
	if (res->name != NULL) {
		free(res->name);
	}
	free(res);
}

LOCAL W datparser_convert_str(datparser_t *parser, const UB *src, W slen, UW attr, TC **dest, W *dlen)
{
	TC ch[20];
	W dest_len = 20, err;

	err = tf_strtotcs(parser->ctx, src, slen, attr, ch, &dest_len);
	if (err < 0) {
		DP_ER("tf_strtotcs error A", err);
		return err;
	}
	if (dest_len == 0) {
		return 0;
	}

	*dest = realloc(*dest, sizeof(TC)*(*dlen + dest_len + 1));
	memcpy((*dest)+*dlen, ch, sizeof(TC)*dest_len);
	*dlen += dest_len;
	(*dest)[*dlen] = TNULL;

	if (err == 0) {
		return 0;
	}

	for (;;) {
		dest_len = 1;
		err = tf_strtotcs(parser->ctx, NULL, slen, attr, ch, &dest_len);
		if (err < 0) {
			DP_ER("tf_strtotcs error B ", err);
			return err;
		}
		if (dest_len == 0) {
			return 0;
		}

		*dest = realloc(*dest, sizeof(TC)*(*dlen + dest_len + 1));
		memcpy((*dest)+*dlen, ch, sizeof(TC)*dest_len);
		*dlen += dest_len;
		(*dest)[*dlen] = TNULL;

		if (err == 0) {
			break;
		}
	}

	return 0;
}

LOCAL W datparser_parsechar(datparser_t *parser, UB ch, datparser_res_t *res)
{
	TC **str;
	W *len, len0, ret;
	UW attr = 0;
	UB chref;
	charreferparser_result_t chref_result;

	switch (parser->state) {
	case STATE_NAME:
		str = &(res->name);
		len = &(res->name_len);
		break;
	case STATE_MAIL:
		str = &(res->mail);
		len = &(res->mail_len);
		break;
	case STATE_DATE:
		str = &(res->date);
		len = &(res->date_len);
		attr = TF_ATTR_SUPPRESS_FUSEN;
		break;
	case STATE_BODY:
		str = &(res->body);
		len = &(res->body_len);
		break;
	case STATE_TITLE:
		str = &(res->title);
		len = &(res->title_len);
		attr = TF_ATTR_SUPPRESS_FUSEN;
		break;
	default:
		DP(("error state\n"));
		return -1; /* TODO */
	}

	switch(ch) {
	  case '\n':
		if (parser->state == STATE_TITLE) {
		}
		parser->state = STATE_NAME;
		datparser_convert_str(parser, NULL, 0, TF_ATTR_CONT|attr, str, len);
		datparser_convert_str(parser, NULL, 0, TF_ATTR_START|attr, NULL, &len0);
		return 1; /* TODO */
		break;
	  case '<':
		parser->strstate = STRSTATE_LSTN;
		break;
	  case '>':
		if (parser->strstate == STRSTATE_START) {
			datparser_convert_str(parser, ">", 1, TF_ATTR_CONT|attr, str, len);
			break;
		} else if (parser->strstate == STRSTATE_ELMNAME) {
			parser->strstate = STRSTATE_START;
			ret = tokenchecker_inputcharacter(&(parser->elmname), ch);
			if (ret == TOKENCHECK_NOMATCH) {
				break;
			}
			if (ret == datparser_elmname_br) {
				datparser_convert_str(parser, "\r\n", 2, TF_ATTR_CONT|attr, str, len);
			}
			if (ret == datparser_elmname_hr) {
				datparser_convert_str(parser, "\r\n\r\n", 4, TF_ATTR_CONT|attr, str, len);
			}
			break;
		} else if (parser->strstate == STRSTATE_SEARCH_END) {
			parser->strstate = STRSTATE_START;
			break;
		}
		/* STRSTATE_LSTN */
		switch (parser->state) {
		case STATE_NAME:
			parser->state = STATE_MAIL;
			break;
		case STATE_MAIL:
			parser->state = STATE_DATE;
			break;
		case STATE_DATE:
			parser->state = STATE_BODY;
			break;
		case STATE_BODY:
			parser->state = STATE_TITLE;
			break;
		case STATE_TITLE:
			parser->state = STATE_NAME;
			break;
		}
		datparser_convert_str(parser, NULL, 0, TF_ATTR_CONT|attr, str, len);
		datparser_convert_str(parser, NULL, 0, TF_ATTR_START|attr, NULL, &len0);

		parser->strstate = STRSTATE_START;
		break;
	  case '&':
		if (parser->strstate == STRSTATE_START) {
			charreferparser_resetstate(&(parser->charref));
			chref_result = charreferparser_parsechar(&(parser->charref), ch);
			if (chref_result != CHARREFERPARSER_RESULT_CONTINUE) {
				break;
			}
			parser->strstate = STRSTATE_CHARREF;
			break;
		}
	  default:
		if (parser->strstate == STRSTATE_LSTN) {
			parser->strstate = STRSTATE_ELMNAME;
			tokenchecker_resetstate(&(parser->elmname));
			ret = tokenchecker_inputcharacter(&(parser->elmname), ch);
			if (ret == TOKENCHECK_NOMATCH) {
				parser->strstate = STRSTATE_SEARCH_END;
				break;
			}
			if (ret == TOKENCHECK_CONTINUE) {
				break;
			}
			break;
		} else if (parser->strstate == STRSTATE_START) {
			datparser_convert_str(parser, &ch, 1, TF_ATTR_CONT|attr, str, len);
		} else if (parser->strstate == STRSTATE_ELMNAME) {
			ret = tokenchecker_inputcharacter(&(parser->elmname), ch);
			if (ret == TOKENCHECK_NOMATCH) {
				parser->strstate = STRSTATE_SEARCH_END;
				break;
			}
			if (ret == TOKENCHECK_CONTINUE) {
				break;
			}
			if (ret == datparser_elmname_br) {
				datparser_convert_str(parser, "\r\n", 2, TF_ATTR_CONT|attr, str, len);
			}
			if (ret == datparser_elmname_hr) {
				datparser_convert_str(parser, "\r\n\r\n", 4, TF_ATTR_CONT|attr, str, len);
			}
		} else if (parser->strstate == STRSTATE_CHARREF) {
			chref_result = charreferparser_parsechar(&(parser->charref), ch);
			if (chref_result == CHARREFERPARSER_RESULT_DETERMINE) {
				chref = charreferparser_getcharnumber(&(parser->charref));
				datparser_convert_str(parser, &chref, 1, TF_ATTR_CONT|attr, str, len);
				parser->strstate = STRSTATE_START;
			}
		}
	}

	return 0; /* TODO */
}

EXPORT W datparser_getnextres(datparser_t *parser, datparser_res_t **res)
{
	datcache_datareadcontext_t *context;
	Bool cont;
	UB *bin_cache;
	W len_cache, i, err = 0;

	*res = NULL;

	context = datcache_startdataread(parser->cache, parser->i);
	if (context == NULL) {
		return -1; /* TODO */
	}

	for (;;) {
		cont = datcache_datareadcontext_nextdata(context, &bin_cache, &len_cache);
		if (cont == False) {
			break;
		}

		for (i = 0; i < len_cache; i++) {
			err = datparser_parsechar(parser, bin_cache[i], parser->resbuffer);
			if (err < 0) {
				i++;
				break;
			}
			if (err == 1) {
				i++;
				break;
			}
		}

		parser->i += i;
		if (err < 0) {
			break;
		}
		if (err == 1) {
			*res = parser->resbuffer;
			parser->resbuffer = datparser_res_new();
			break;
		}
	}

	datcache_enddataread(parser->cache, context);

	return err;
}

EXPORT VOID datparser_clear(datparser_t *parser)
{
	TC ch;
	W len = 1, err;

	err = tf_strtotcs(parser->ctx, NULL, 0, TF_ATTR_START, &ch, &len);
	if (err != 0) {
		DP_ER("tf_strtotcs:", err);
	}

	parser->i = 0;
	parser->state = STATE_NAME;
	parser->strstate = STRSTATE_START;

	datparsr_res_clear(parser->resbuffer);
}

LOCAL tokenchecker_valuetuple_t nList_elmname[] = {
	{NULL,0},
	{"A", datparser_elmname_a},
	{"BR", datparser_elmname_br},
	{"HR", datparser_elmname_hr},
	{"a", datparser_elmname_a},
	{"br", datparser_elmname_br},
	{"hr", datparser_elmname_hr},
	{NULL,0}
};
LOCAL B eToken_elmname[] = " >";

EXPORT datparser_t* datparser_new(datcache_t *cache)
{
	datparser_t *parser;
	W err, ctx_id;

	parser = malloc(sizeof(datparser_t));
	if (parser == NULL) {
		return NULL;
	}
	parser->cache = cache;

	tf_open_ctx(&parser->ctx);
	ctx_id = tf_to_id(TF_ID_PROFSET_CONVERTFROM, "Shift_JIS");
	if (ctx_id < 0) {
		tf_close_ctx(parser->ctx);
		free(parser);
		return NULL;
	}
	err = tf_set_profile(parser->ctx, ctx_id);
	if (err < 0) {
		tf_close_ctx(parser->ctx);
		free(parser);
		return NULL;
	}

	parser->i = 0;
	parser->state = STATE_NAME;
	parser->strstate = STRSTATE_START;

	parser->resbuffer = datparser_res_new();
	if (parser->resbuffer == NULL) {
		tf_close_ctx(parser->ctx);
		free(parser);
		return NULL;
	}
	tokenchecker_initialize(&(parser->elmname), nList_elmname, eToken_elmname);
	charreferparser_initialize(&(parser->charref));

	return parser;
}

EXPORT VOID datparser_delete(datparser_t *parser)
{
	charreferparser_finalize(&(parser->charref));
	datparser_res_delete(parser->resbuffer);
	tf_close_ctx(parser->ctx);
	free(parser);
}
