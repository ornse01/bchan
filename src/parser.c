/*
 * parser.c
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

#include    "cache.h"
#include    "parser.h"
#include    "parselib.h"
#include    "sjisstring.h"
#include    "tadlib.h"

#include	<bstdio.h>
#include	<bstdlib.h>
#include	<errcode.h>
#include	<btron/tf.h>

#ifdef BCHAN_CONFIG_DEBUG
# define DP(arg) printf arg
# define DP_ER(msg, err) printf("%s (%d/%x)\n", msg, err>>16, err)
#else
# define DP(arg) /**/
# define DP_ER(msg, err) /**/
#endif

typedef enum {
	datparser_elmname_a_open = 1,
	datparser_elmname_a_close,
	datparser_elmname_br,
	datparser_elmname_hr
} datparser_elmname_t;

typedef enum {
	datparser_urlscheme_http = 1,
	datparser_urlscheme_sssp,
	datparser_urlscheme_ttp,
} datparser_urlscheme_t;

struct datparser_t_ {
	datcache_t *cache;
	TF_CTX ctx;
	W i;
	/**/
	enum {
		STATE_START,
		STATE_LSTN,
		STATE_ELEMENT,
		STATE_CHARREF,
		STATE_URL,
	} state;
	Bool issjissecondbyte;
	enum {
		COLUMN_NAME,
		COLUMN_MAIL,
		COLUMN_DATE,
		COLUMN_BODY,
		COLUMN_TITLE
	} col;
	enum {
		STATE_ELEMENT_SUBSTATE_ELMNAME,
		STATE_ELEMENT_SUBSTATE_REM,
	} state_element_substate;
	enum {
		STATE_URL_SUBSTATE_SCHEME,
		STATE_URL_SUBSTATE_REM,
	} state_url_substate;
	tokenchecker_t elmname;
	tokenchecker_t urlscheme;
	charreferparser_t charref;
	datparser_res_t *resbuffer;
	W elmnameid;
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

LOCAL W datparser_outputconvertingstring(datparser_t *parser, UB *str, W len, datparser_res_t *res)
{
	switch (parser->col) {
	case COLUMN_NAME:
		return datparser_convert_str(parser, str, len, TF_ATTR_CONT, &(res->name), &(res->name_len));
	case COLUMN_MAIL:
		return datparser_convert_str(parser, str, len, TF_ATTR_CONT, &(res->mail), &(res->mail_len));
	case COLUMN_DATE:
		return datparser_convert_str(parser, str, len, TF_ATTR_CONT|TF_ATTR_SUPPRESS_FUSEN, &(res->date), &(res->date_len));
	case COLUMN_BODY:
		return datparser_convert_str(parser, str, len, TF_ATTR_CONT, &(res->body), &(res->body_len));
	case COLUMN_TITLE:
		return datparser_convert_str(parser, str, len, TF_ATTR_CONT|TF_ATTR_SUPPRESS_FUSEN, &(res->title), &(res->title_len));
	}
	return -1; /* TODO */
}

LOCAL W datparser_flushstring(datparser_t *parser, datparser_res_t *res)
{
	W len0, err;

	switch (parser->col) {
	case COLUMN_NAME:
		err = datparser_convert_str(parser, NULL, 0, TF_ATTR_CONT, &(res->name), &(res->name_len));
		if (err < 0) {
			return err;
		}
		return datparser_convert_str(parser, NULL, 0, TF_ATTR_START, NULL, &len0);
	case COLUMN_MAIL:
		err = datparser_convert_str(parser, NULL, 0, TF_ATTR_CONT, &(res->mail), &(res->mail_len));
		if (err < 0) {
			return err;
		}
		return datparser_convert_str(parser, NULL, 0, TF_ATTR_START, NULL, &len0);
	case COLUMN_DATE:
		err = datparser_convert_str(parser, NULL, 0, TF_ATTR_CONT|TF_ATTR_SUPPRESS_FUSEN, &(res->date), &(res->date_len));
		if (err < 0) {
			return err;
		}
		return datparser_convert_str(parser, NULL, 0, TF_ATTR_START|TF_ATTR_SUPPRESS_FUSEN, NULL, &len0);
	case COLUMN_BODY:
		err = datparser_convert_str(parser, NULL, 0, TF_ATTR_CONT, &(res->body), &(res->body_len));
		if (err < 0) {
			return err;
		}
		return datparser_convert_str(parser, NULL, 0, TF_ATTR_START, NULL, &len0);
	case COLUMN_TITLE:
		err =  datparser_convert_str(parser, NULL, 0, TF_ATTR_CONT|TF_ATTR_SUPPRESS_FUSEN, &(res->title), &(res->title_len));
		if (err < 0) {
			return err;
		}
		return datparser_convert_str(parser, NULL, 0, TF_ATTR_START|TF_ATTR_SUPPRESS_FUSEN, NULL, &len0);
	}
	return -1; /* TODO */
}

LOCAL W datparser_appendbinary(datparser_t *parser, UB *data, W len, datparser_res_t *res)
{
	TC **str;
	W *len0;

	switch (parser->col) {
	case COLUMN_NAME:
		str = &(res->name);
		len0 = &(res->name_len);
		break;
	case COLUMN_MAIL:
		str = &(res->mail);
		len0 = &(res->mail_len);
		break;
	case COLUMN_DATE:
		str = &(res->date);
		len0 = &(res->date_len);
		break;
	case COLUMN_BODY:
		str = &(res->body);
		len0 = &(res->body_len);
		break;
	case COLUMN_TITLE:
		str = &(res->title);
		len0 = &(res->title_len);
		break;
	default:
		return -1; /* TODO*/
	}

	*str = realloc(*str, sizeof(TC)*(*len0 + len/sizeof(TC) + 1));
	if (*str == NULL) {
		return ER_NOMEM;
	}
	memcpy((*str)+*len0, data, len);
	*len0 += len/sizeof(TC);
	(*str)[*len0] = TNULL;

	return 0;
}

LOCAL W datparser_appendchcolorfusen(datparser_t *parser, COLOR color, datparser_res_t *res)
{
	UB data[10];
	TADSEG *seg = (TADSEG*)data;

	seg->id = 0xFF00|TS_TFONT;
	seg->len = 6;
	*(UH*)(data + 4) = 6 << 8;
	*(COLOR*)(data + 6) = color;

	return datparser_appendbinary(parser, data, 10, res);
}

LOCAL W datparser_appendbchanapplfusen(datparser_t *parser, UB subid, datparser_res_t *res)
{
	UB data[12];
	TADSEG *seg = (TADSEG*)data;
	TT_BCHAN *fsn = (TT_BCHAN*)(data + 4);

	seg->id = 0xFF00|TS_TAPPL;
	seg->len = 8;
	fsn->attr = 0;
	fsn->subid = subid;
	fsn->appl[0] = 0x8000;
	fsn->appl[1] = 0xC053;
	fsn->appl[2] = 0x8000;

	return datparser_appendbinary(parser, data, 12, res);
}

LOCAL W datparser_appendconvertedelementinfo(datparser_t *parser, W tagid, datparser_res_t *res)
{
	switch (tagid) {
	case datparser_elmname_br:
		return datparser_outputconvertingstring(parser, "\r\n", 2, res);
	case datparser_elmname_hr:
		return datparser_outputconvertingstring(parser, "\r\n\r\n", 4, res);
	case datparser_elmname_a_open:
		return datparser_appendbchanapplfusen(parser, TT_BCHAN_SUBID_ANCHOR_START, res);
	case datparser_elmname_a_close:
		return datparser_appendbchanapplfusen(parser, TT_BCHAN_SUBID_ANCHOR_END, res);
	}
	return 0;
}

#define DATPARSER_PARSECHAR_CONTINUE 0
#define DATPARSER_PARSECHAR_RESRESOLVED 1

LOCAL W datparser_parsechar_start_trigger(datparser_t *parser, UB ch, Bool issecondbyte, datparser_res_t *res)
{
	W err, ret, val;
	charreferparser_result_t chref_result;

	if ((ch == '\n')&&(issecondbyte == False)) {
		err = datparser_flushstring(parser, res);
		if (err < 0) {
			return err;
		}
		parser->col = COLUMN_NAME;
		return DATPARSER_PARSECHAR_RESRESOLVED;
	} else if ((ch == '<')&&(issecondbyte == False)) {
		parser->state = STATE_LSTN;
	} else if ((ch == '&')&&(issecondbyte == False)) {
		charreferparser_resetstate(&(parser->charref));
		chref_result = charreferparser_parsechar(&(parser->charref), ch);
		if (chref_result != CHARREFERPARSER_RESULT_CONTINUE) {
			return -1; /* TODO */
		}
		parser->state = STATE_CHARREF;
	} else {
		ret = tokenchecker_inputchar(&(parser->urlscheme), ch, &val);
		if ((ret == TOKENCHECKER_NOMATCH)||(ret == TOKENCHECKER_CONTINUE_NOMATCH)) {
			tokenchecker_clear(&(parser->urlscheme));
		} else if (ret == TOKENCHECKER_CONTINUE) {
			parser->state = STATE_URL;
			parser->state_url_substate = STATE_URL_SUBSTATE_SCHEME;
			return DATPARSER_PARSECHAR_CONTINUE;
		}
		err = datparser_outputconvertingstring(parser, &ch, 1, res);
		if (err < 0) {
			return err;
		}
	}

	return DATPARSER_PARSECHAR_CONTINUE;
}

LOCAL W datparser_parsechar_lstn_trigger(datparser_t *parser, UB ch, Bool issecondbyte, datparser_res_t *res)
{
	W ret, err, val;

	if ((ch == '>')&&(issecondbyte == False)) {
		err = datparser_flushstring(parser, res);
		if (err < 0) {
			return err;
		}
		parser->state = STATE_START;
		switch (parser->col) {
		case COLUMN_NAME:
			parser->col = COLUMN_MAIL;
			break;
		case COLUMN_MAIL:
			parser->col = COLUMN_DATE;
			break;
		case COLUMN_DATE:
			parser->col = COLUMN_BODY;
			break;
		case COLUMN_BODY:
			parser->col = COLUMN_TITLE;
			break;
		case COLUMN_TITLE:
			parser->col = COLUMN_NAME;
			break;
		}
	} else {
		parser->state = STATE_ELEMENT;
		parser->state_element_substate = STATE_ELEMENT_SUBSTATE_ELMNAME;
		tokenchecker_clear(&(parser->elmname));
		ret = tokenchecker_inputchar(&(parser->elmname), ch, &val);
		if (ret == TOKENCHECKER_NOMATCH) {
			parser->state_element_substate = STATE_ELEMENT_SUBSTATE_REM;
			return DATPARSER_PARSECHAR_CONTINUE;
		}
		if (!((ret == TOKENCHECKER_CONTINUE)
			  ||(ret == TOKENCHECKER_CONTINUE_NOMATCH))) {
			return -1; /* TODO */
		}
	}

	return DATPARSER_PARSECHAR_CONTINUE;
}

LOCAL W datparser_parsechar_element_trigger(datparser_t *parser, UB ch, Bool issecondbyte, datparser_res_t *res)
{
	W ret, err, val;

	if (parser->state_element_substate != STATE_ELEMENT_SUBSTATE_ELMNAME) {
		if ((ch == '>')&&(issecondbyte == False)) {
			err = datparser_appendconvertedelementinfo(parser, parser->elmnameid, res);
			if (err < 0) {
				return err;
			}
			parser->state = STATE_START;
		}
		return DATPARSER_PARSECHAR_CONTINUE;
	}

	ret = tokenchecker_inputchar(&(parser->elmname), ch, &val);
	if (ret == TOKENCHECKER_NOMATCH) {
		if ((ch == '>')&&(issecondbyte == False)) {
			parser->state = STATE_START;
		} else {
			parser->state_element_substate = STATE_ELEMENT_SUBSTATE_REM;
			parser->elmnameid = -1;
		}
		return DATPARSER_PARSECHAR_CONTINUE;
	}
	if ((ret == TOKENCHECKER_CONTINUE)
		||(ret == TOKENCHECKER_CONTINUE_NOMATCH)) {
		return DATPARSER_PARSECHAR_CONTINUE;
	}
	if (ret == TOKENCHECKER_DETERMINE) {
		if ((ch == '>')&&(issecondbyte == False)) {
			err = datparser_appendconvertedelementinfo(parser, val, res);
			if (err < 0) {
				return err;
			}
			parser->state = STATE_START;
		} if ((ch == ' ')&&(issecondbyte == False)) {
			parser->elmnameid = val;
			parser->state_element_substate = STATE_ELEMENT_SUBSTATE_REM;
		}
	}

	return DATPARSER_PARSECHAR_CONTINUE;
}

LOCAL W datparser_parsechar_charref_trigger(datparser_t *parser, UB ch, Bool issecondbyte, datparser_res_t *res)
{
	W err, len;
	charreferparser_result_t chref_result;
	UB chref, *str;

	chref_result = charreferparser_parsechar(&(parser->charref), ch);
	if (chref_result == CHARREFERPARSER_RESULT_DETERMINE) {
		chref = charreferparser_getcharnumber(&(parser->charref));
		err = datparser_outputconvertingstring(parser, &chref, 1, res);
		if (err < 0) {
			return err;
		}
		parser->state = STATE_START;
	} else if (chref_result == CHARREFERPARSER_RESULT_INVALID) {
		chref = '&';
		err = datparser_outputconvertingstring(parser, &chref, 1, res);
		if (err < 0) {
			return err;
		}
		charreferparser_getlastmatchedstring(&(parser->charref), &str, &len);
		err = datparser_outputconvertingstring(parser, str, len, res);
		if (err < 0) {
			return err;
		}
		parser->state = STATE_START;
		return datparser_parsechar_start_trigger(parser, ch, issecondbyte, res);
	}

	return DATPARSER_PARSECHAR_CONTINUE;
}

LOCAL W datparser_parsechar_url_trigger(datparser_t *parser, UB ch, Bool issecondbyte, datparser_res_t *res)
{
	W ret, err, len, val;
	UB *str;
	Bool usable;

	if (parser->state_url_substate == STATE_URL_SUBSTATE_SCHEME) {
		ret = tokenchecker_inputchar(&(parser->urlscheme), ch, &val);
		if ((ret == TOKENCHECKER_NOMATCH)||(ret == TOKENCHECKER_CONTINUE_NOMATCH)) {
			tokenchecker_getlastmatchedstring(&(parser->urlscheme), &str, &len);
			err = datparser_outputconvertingstring(parser, str, len, res);
			if (err < 0) {
				return err;
			}
			tokenchecker_clear(&(parser->urlscheme));
			if (ch == '<') {
				parser->state = STATE_LSTN;
				return DATPARSER_PARSECHAR_CONTINUE;
			}
			parser->state = STATE_START;
			return datparser_parsechar_start_trigger(parser, ch, issecondbyte, res);
		}
		if (ret == TOKENCHECKER_CONTINUE) {
			return DATPARSER_PARSECHAR_CONTINUE;
		}

		err = datparser_appendbchanapplfusen(parser, TT_BCHAN_SUBID_URL_START, res);
		if (err < 0) {
			return err;
		}
		tokenchecker_getlastmatchedstring(&(parser->urlscheme), &str, &len);
		err = datparser_outputconvertingstring(parser, str, len, res);
		if (err < 0) {
			return err;
		}
		err = datparser_outputconvertingstring(parser, &ch, 1, res);
		if (err < 0) {
			return err;
		}

		tokenchecker_clear(&(parser->urlscheme));
		parser->state_url_substate = STATE_URL_SUBSTATE_REM;
	} else if (parser->state_url_substate == STATE_URL_SUBSTATE_REM) {
		usable = sjstring_isurlusablecharacter(ch);
		if (usable == False) {
			err = datparser_appendbchanapplfusen(parser, TT_BCHAN_SUBID_URL_END, res);
			if (err < 0) {
				return err;
			}
			parser->state = STATE_START;
			return datparser_parsechar_start_trigger(parser, ch, issecondbyte, res);
		}
		err = datparser_outputconvertingstring(parser, &ch, 1, res);
		if (err < 0) {
			return err;
		}
	}

	return DATPARSER_PARSECHAR_CONTINUE;
}

LOCAL W datparser_parsechar(datparser_t *parser, UB ch, datparser_res_t *res)
{
	Bool secondbyte;

	if (parser->issjissecondbyte == True) {
		secondbyte = True;
		parser->issjissecondbyte = False;
	} else {
		if (((0x81 <= ch)&&(ch <= 0x9f))||((0xe0 <= ch)&&(ch <= 0xef))) {
			parser->issjissecondbyte = True;
		}
		secondbyte = False;
	}

	switch (parser->state) {
	case STATE_START:
		return datparser_parsechar_start_trigger(parser, ch, secondbyte, res);
	case STATE_LSTN:
		return datparser_parsechar_lstn_trigger(parser, ch, secondbyte, res);
	case STATE_ELEMENT:
		return datparser_parsechar_element_trigger(parser, ch, secondbyte, res);
	case STATE_CHARREF:
		return datparser_parsechar_charref_trigger(parser, ch, secondbyte, res);
	case STATE_URL:
		return datparser_parsechar_url_trigger(parser, ch, secondbyte, res);
	default:
		return -1; /* TODO */
	}
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
	parser->state = STATE_START;

	datparsr_res_clear(parser->resbuffer);
	tokenchecker_clear(&(parser->elmname));
	tokenchecker_clear(&(parser->urlscheme));
}

LOCAL tokenchecker_valuetuple_t nList_elmname[] = {
	{"/A", datparser_elmname_a_close},
	{"/a", datparser_elmname_a_close},
	{"A", datparser_elmname_a_open},
	{"BR", datparser_elmname_br},
	{"HR", datparser_elmname_hr},
	{"a", datparser_elmname_a_open},
	{"br", datparser_elmname_br},
	{"hr", datparser_elmname_hr},
};
LOCAL B eToken_elmname[] = " >";

LOCAL tokenchecker_valuetuple_t nList_urlscheme[] = {
	{"http", datparser_urlscheme_http},
	{"sssp", datparser_urlscheme_sssp},
	{"ttp", datparser_urlscheme_ttp},
};
LOCAL B eToken_urlscheme[] = ":";

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
	parser->issjissecondbyte = False;
	parser->state = STATE_START;
	parser->col = COLUMN_NAME;

	parser->resbuffer = datparser_res_new();
	if (parser->resbuffer == NULL) {
		tf_close_ctx(parser->ctx);
		free(parser);
		return NULL;
	}
	tokenchecker_initialize(&(parser->elmname), nList_elmname, 8, eToken_elmname);
	tokenchecker_initialize(&(parser->urlscheme), nList_urlscheme, 3, eToken_urlscheme);
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
