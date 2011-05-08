/*
 * postres.c
 *
 * Copyright (c) 2009-2011 project bchan
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
#include	<bctype.h>
#include	<errcode.h>
#include	<tstring.h>
#include	<tcode.h>
#include	<btron/btron.h>
#include	<btron/hmi.h>
#include	<btron/tf.h>

#include    "postres.h"
#include    "sjisstring.h"
#include    "tadimf.h"

typedef struct tcstrbuffer_t_ tcstrbuffer_t;
struct tcstrbuffer_t_ {
	TC *buffer;
	W strlen;
};

LOCAL TC* tcstrbuffer_getbuffer(tcstrbuffer_t *tcstr)
{
	return tcstr->buffer;
}

LOCAL W tcstrbuffer_getstrlen(tcstrbuffer_t *tcstr)
{
	return tcstr->strlen;
}

LOCAL W tcstrbuffer_appendstring(tcstrbuffer_t *tcstr, TC *str, W len)
{
	tcstr->buffer = realloc(tcstr->buffer, sizeof(TC)*(tcstr->strlen + len + 1));
	if (tcstr->buffer == NULL) {
		tcstr->strlen = 0;
		return -1;
	}
	memcpy(tcstr->buffer+tcstr->strlen, str, sizeof(TC)*len);
	tcstr->strlen += len;
	(tcstr->buffer)[tcstr->strlen] = TNULL;

	return 0;
}

LOCAL W tcstrbuffer_initialize(tcstrbuffer_t *tcstr)
{
	tcstr->buffer = NULL;
	tcstr->strlen = 0;

	return 0;
}

LOCAL VOID tcstrbuffer_finalize(tcstrbuffer_t *tcstr)
{
	if (tcstr->buffer != NULL) {
		free(tcstr->buffer);
	}
}

struct postresdata_t_ {
	tcstrbuffer_t from;
	tcstrbuffer_t mail;
	tcstrbuffer_t message;
	UB *asc_from;
	W asc_from_len;
	UB *asc_mail;
	W asc_mail_len;
	UB *asc_message;
	W asc_message_len;
};

EXPORT TC* postresdata_getfromstring(postresdata_t *post)
{
	return tcstrbuffer_getbuffer(&post->from);
}

EXPORT W postresdata_getfromstringlen(postresdata_t *post)
{
	return tcstrbuffer_getstrlen(&post->from);
}

EXPORT TC* postresdata_getmailstring(postresdata_t *post)
{
	return tcstrbuffer_getbuffer(&post->mail);
}

EXPORT W postresdata_getmailstringlen(postresdata_t *post)
{
	return tcstrbuffer_getstrlen(&post->mail);
}

EXPORT TC* postresdata_getmessagestring(postresdata_t *post)
{
	return tcstrbuffer_getbuffer(&post->message);
}

EXPORT W postresdata_getmessagestringlen(postresdata_t *post)
{
	return tcstrbuffer_getstrlen(&post->message);
}

#define POSTRESDATA_VALID_FROM 1
#define POSTRESDATA_VALID_mail 2
LOCAL tctokenchecker_valuetuple_t postreadata_nametable[] = {
  {(TC[]){TK_F, TK_R, TK_O, TK_M, TNULL}, POSTRESDATA_VALID_FROM},
  {(TC[]){TK_m, TK_a, TK_i, TK_l, TNULL}, POSTRESDATA_VALID_mail},
};

LOCAL W postresdata_readtad(postresdata_t *post, TC *str, W len)
{
	timfparser_t timf;
	TIMFPARSER_RESULT_T cont;
	UB *bin;
	W rcvlen, val, err = 0;

	timfparser_initialize(&timf, postreadata_nametable, 2, str, len);

	for (;;) {
		cont = timfparser_next(&timf, &val, &bin, &rcvlen);
		if (cont == TIMFPARSER_RESULT_HEADERVALUE) {
			if (val == POSTRESDATA_VALID_FROM) {
				err = tcstrbuffer_appendstring(&(post->from), (TC*)bin, rcvlen/sizeof(TC));
				if (err < 0) {
					break;
				}
			} else if (val == POSTRESDATA_VALID_mail) {
				err = tcstrbuffer_appendstring(&(post->mail), (TC*)bin, rcvlen/sizeof(TC));
				if (err < 0) {
					break;
				}
			}
		} else if (cont == TIMFPARSER_RESULT_BODY) {
			err = tcstrbuffer_appendstring(&(post->message), (TC*)bin, rcvlen/sizeof(TC));
			if (err < 0) {
				break;
			}
		} else if (cont == TIMFPARSER_RESULT_END) {
			break;
		} else {
			break;
		}
	}

	timfparser_finalize(&timf);

	return err;
}

LOCAL W postresdata_tctosjis(TF_CTX *ctx, tcstrbuffer_t *src_tc, UB **dest, W *dlen)
{
	UB *buf;
	W slen, buf_len, err;
	TC *src;

	src = tcstrbuffer_getbuffer(src_tc);
	slen = tcstrbuffer_getstrlen(src_tc);

	if (slen == 0) {
		*dest = malloc(sizeof(UB));
		(*dest)[0] = '\0';
		*dlen = 0;
		return 0;
	}

	err = tf_tcstostr(*ctx, src, slen, 0, TF_ATTR_START, NULL, &buf_len);
	if (err < 0) {
		return err;
	}
	buf = malloc(buf_len + sizeof(UB));
	if (buf == NULL) {
		return -1; /* TODO */
	}
	err = tf_tcstostr(*ctx, src, slen, 0, TF_ATTR_START, buf, &buf_len);
	if (err < 0) {
		return err;
	}

	err = sjstring_appendurlencodestring(dest, dlen, buf, buf_len);
	free(buf);

	return err;
}

LOCAL W postresdata_convertdata(postresdata_t *post)
{
	TF_CTX ctx;
	W err, ctx_id;

	err = tf_open_ctx(&ctx);
	if (err < 0) {
		return err;
	}
	ctx_id = tf_to_id(TF_ID_PROFSET_CONVERTTO, "Shift_JIS");
	if (ctx_id < 0) {
		tf_close_ctx(ctx);
		return err;
	}
	err = tf_set_profile(ctx, ctx_id);
	if (err < 0) {
		tf_close_ctx(ctx);
		return err;
	}

	err = postresdata_tctosjis(&ctx, &post->from, &post->asc_from, &post->asc_from_len);
	if (err < 0) {
		tf_close_ctx(ctx);
		return err;
	}
	err = postresdata_tctosjis(&ctx, &post->mail, &post->asc_mail, &post->asc_mail_len);
	if (err < 0) {
		tf_close_ctx(ctx);
		return err;
	}
	err = postresdata_tctosjis(&ctx, &post->message, &post->asc_message, &post->asc_message_len);
	if (err < 0) {
		tf_close_ctx(ctx);
		return err;
	}

	tf_close_ctx(ctx);

	return 0;
}

EXPORT W postresdata_readfile(postresdata_t *post, VLINK *vlnk)
{
	W fd, err, size;
	UB *buf;

	fd = opn_fil((LINK*)vlnk, F_READ, NULL);
	if (fd < 0) {
		return fd;
	}
	err = fnd_rec(fd, F_TOPEND, RM_TADDATA, 0, NULL);
	if (err < 0) {
		cls_fil(fd);
		return err;
	}
	err = rea_rec(fd, 0, NULL, 0, &size, NULL);
	if (err < 0) {
		cls_fil(fd);
		return err;
	}
	buf = malloc(size*sizeof(UB));
	if (buf == NULL) {
		cls_fil(fd);
		return -1; /* TODO */
	}
	err = rea_rec(fd, 0, buf, size, NULL, NULL);
	if (err < 0) {
		free(buf);
		cls_fil(fd);
		return err;
	}
	cls_fil(fd);

	err =  postresdata_readtad(post, (TC *)buf, size/sizeof(TC));
	free(buf);
	if (err < 0) {
		return err;
	}

	err = postresdata_convertdata(post);

	return err;
}

LOCAL W postresdata_appendasciistring(UB **dest, W *dest_len, UB *str, W len)
{
	return sjstring_appendasciistring(dest, dest_len, str, len);
}

struct postresdata_genbodycontext_t_ {
	postresdata_t *post;
	UB *board;
	W board_len;
	UB *thread;
	W thread_len;
	STIME time;
	enum {
		POSTRESDATA_GENBODYCONTEXT_STATE_NAME_BBS,
		POSTRESDATA_GENBODYCONTEXT_STATE_BOARD,
		POSTRESDATA_GENBODYCONTEXT_STATE_NAME_KEY,
		POSTRESDATA_GENBODYCONTEXT_STATE_THREAD,
		POSTRESDATA_GENBODYCONTEXT_STATE_NAME_TIME,
		POSTRESDATA_GENBODYCONTEXT_STATE_TIME,
		POSTRESDATA_GENBODYCONTEXT_STATE_NAME_FROM,
		POSTRESDATA_GENBODYCONTEXT_STATE_FROM,
		POSTRESDATA_GENBODYCONTEXT_STATE_NAME_MAIL,
		POSTRESDATA_GENBODYCONTEXT_STATE_MAIL,
		POSTRESDATA_GENBODYCONTEXT_STATE_NAME_MESSAGE,
		POSTRESDATA_GENBODYCONTEXT_STATE_MESSAGE,
		POSTRESDATA_GENBODYCONTEXT_STATE_NAME_SUBMIT,
		POSTRESDATA_GENBODYCONTEXT_STATE_END
	} state;
	UB timebuf[10];
} ;
typedef struct postresdata_genbodycontext_t_ postresdata_genbodycontext_t;

LOCAL UB name_bbs[] = "bbs=";
LOCAL UB name_key[] = "&key=";
LOCAL UB name_time[] = "&time=";
LOCAL UB name_FROM[] = "&FROM=";
LOCAL UB name_mail[] = "&mail=";
LOCAL UB name_MESSAGE[] = "&MESSAGE=";
LOCAL UB name_submit[] = "&submit=%8F%91%82%AB%8D%9E%82%DE";

LOCAL Bool postresdata_genbodycontext_next(postresdata_genbodycontext_t *ctx, UB **str, W *len)
{
	switch (ctx->state) {
	case POSTRESDATA_GENBODYCONTEXT_STATE_NAME_BBS:
		*str = name_bbs;
		*len = strlen(name_bbs);
		ctx->state = POSTRESDATA_GENBODYCONTEXT_STATE_BOARD;
		return True;
	case POSTRESDATA_GENBODYCONTEXT_STATE_BOARD:
		*str = ctx->board;
		*len = ctx->board_len;
		ctx->state = POSTRESDATA_GENBODYCONTEXT_STATE_NAME_KEY;
		return True;
	case POSTRESDATA_GENBODYCONTEXT_STATE_NAME_KEY:
		*str = name_key;
		*len = strlen(name_key);
		ctx->state = POSTRESDATA_GENBODYCONTEXT_STATE_THREAD;
		return True;
	case POSTRESDATA_GENBODYCONTEXT_STATE_THREAD:
		*str = ctx->thread;
		*len = ctx->thread_len;
		ctx->state = POSTRESDATA_GENBODYCONTEXT_STATE_NAME_TIME;
		return True;
	case POSTRESDATA_GENBODYCONTEXT_STATE_NAME_TIME:
		*str = name_time;
		*len = strlen(name_time);
		ctx->state = POSTRESDATA_GENBODYCONTEXT_STATE_TIME;
		return True;
	case POSTRESDATA_GENBODYCONTEXT_STATE_TIME:
		*str = ctx->timebuf;
		*len = sjstring_writeUWstring(ctx->timebuf, ctx->time + 473385600);
		ctx->state = POSTRESDATA_GENBODYCONTEXT_STATE_NAME_FROM;
		return True;
	case POSTRESDATA_GENBODYCONTEXT_STATE_NAME_FROM:
		*str = name_FROM;
		*len = strlen(name_FROM);
		ctx->state = POSTRESDATA_GENBODYCONTEXT_STATE_FROM;
		return True;
	case POSTRESDATA_GENBODYCONTEXT_STATE_FROM:
		*str = ctx->post->asc_from;
		*len = ctx->post->asc_from_len;
		ctx->state = POSTRESDATA_GENBODYCONTEXT_STATE_NAME_MAIL;
		return True;
	case POSTRESDATA_GENBODYCONTEXT_STATE_NAME_MAIL:
		*str = name_mail;
		*len = strlen(name_mail);
		ctx->state = POSTRESDATA_GENBODYCONTEXT_STATE_MAIL;
		return True;
	case POSTRESDATA_GENBODYCONTEXT_STATE_MAIL:
		*str = ctx->post->asc_mail;
		*len = ctx->post->asc_mail_len;
		ctx->state = POSTRESDATA_GENBODYCONTEXT_STATE_NAME_MESSAGE;
		return True;
	case POSTRESDATA_GENBODYCONTEXT_STATE_NAME_MESSAGE:
		*str = name_MESSAGE;
		*len = strlen(name_MESSAGE);
		ctx->state = POSTRESDATA_GENBODYCONTEXT_STATE_MESSAGE;
		return True;
	case POSTRESDATA_GENBODYCONTEXT_STATE_MESSAGE:
		*str = ctx->post->asc_message;
		*len = ctx->post->asc_message_len;
		ctx->state = POSTRESDATA_GENBODYCONTEXT_STATE_NAME_SUBMIT;
		return True;
	case POSTRESDATA_GENBODYCONTEXT_STATE_NAME_SUBMIT:
		*str = name_submit;
		*len = strlen(name_submit);
		ctx->state = POSTRESDATA_GENBODYCONTEXT_STATE_END;
		return True;
	case POSTRESDATA_GENBODYCONTEXT_STATE_END:
	default:
	}
	*str = NULL;
	*len = 0;
	return False;
}

LOCAL VOID postresdata_genbodycontext_initialize(postresdata_genbodycontext_t *ctx, postresdata_t *post, UB *board, W board_len, UB *thread, W thread_len, STIME time)
{
	ctx->post = post;
	ctx->board = board;
	ctx->board_len = board_len;
	ctx->thread = thread;
	ctx->thread_len = thread_len;
	ctx->time = time;
	ctx->state = POSTRESDATA_GENBODYCONTEXT_STATE_NAME_BBS;
}

LOCAL VOID postresdata_genbodycontext_finalize(postresdata_genbodycontext_t *ctx)
{
}

EXPORT W postresdata_genrequestbody(postresdata_t *post, UB *board, W board_len, UB *thread, W thread_len, STIME time, UB **body, W *body_len)
{
	postresdata_genbodycontext_t ctx;
	UB *str, *buf_ret;
	W len, buf_ret_len, buf_written;
	Bool cont;

	postresdata_genbodycontext_initialize(&ctx, post, board, board_len, thread, thread_len, time);
	buf_ret_len = 0;
	for (;;) {
		cont = postresdata_genbodycontext_next(&ctx, &str, &len);
		if (cont == False) {
			break;
		}
		buf_ret_len += len;
	}
	postresdata_genbodycontext_finalize(&ctx);

	buf_ret = malloc((buf_ret_len + 1) * sizeof(UB));
	if (buf_ret == NULL) {
		return -1; /* TODO */
	}

	postresdata_genbodycontext_initialize(&ctx, post, board, board_len, thread, thread_len, time);
	buf_written = 0;
	for (;;) {
		cont = postresdata_genbodycontext_next(&ctx, &str, &len);
		if (cont == False) {
			break;
		}
		memcpy(buf_ret + buf_written, str, len);
		buf_written += len;
	}
	postresdata_genbodycontext_finalize(&ctx);
	buf_ret[buf_ret_len] = '\0';

	*body = buf_ret;
	*body_len = buf_ret_len;

	return 0;
}

EXPORT W postresdata_gennamemail(postresdata_t *post, UB **name, W *name_len, UB **mail, W *mail_len)
{
	UB *buf_name = NULL, *buf_mail = NULL;
	W err, buf_name_len = 0, buf_mail_len = 0;

	buf_name = malloc(sizeof(UB));
	if (buf_name == NULL) {
		return -1;
	}
	buf_name[0] = '\0';
	buf_mail = malloc(sizeof(UB));
	if (buf_mail == NULL) {
		free(buf_name);
		return -1;
	}
	buf_mail[0] = '\0';

	err = postresdata_appendasciistring(&buf_name, &buf_name_len, post->asc_from, post->asc_from_len);
	if (err < 0) {
		free(buf_mail);
		free(buf_name);
		return err;
	}
	err = postresdata_appendasciistring(&buf_mail, &buf_mail_len, post->asc_mail, post->asc_mail_len);
	if (err < 0) {
		free(buf_mail);
		free(buf_name);
		return err;
	}

	*name = buf_name;
	*name_len = buf_name_len;
	*mail = buf_mail;
	*mail_len = buf_mail_len;

	return 0;
}

EXPORT postresdata_t* postresdata_new()
{
	postresdata_t *post;

	post = (postresdata_t*)malloc(sizeof(postresdata_t));
	if (post == NULL) {
		return NULL;
	}
	tcstrbuffer_initialize(&post->from);
	tcstrbuffer_initialize(&post->mail);
	tcstrbuffer_initialize(&post->message);
	post->asc_from = NULL;
	post->asc_from_len = 0;
	post->asc_mail = NULL;
	post->asc_mail_len = 0;
	post->asc_message = NULL;
	post->asc_message_len = 0;

	return post;
}

EXPORT VOID postresdata_delete(postresdata_t *post)
{
	if (post->asc_message != NULL) {
		free(post->asc_message);
	}
	if (post->asc_mail != NULL) {
		free(post->asc_mail);
	}
	if (post->asc_from != NULL) {
		free(post->asc_from);
	}
	tcstrbuffer_finalize(&post->message);
	tcstrbuffer_finalize(&post->mail);
	tcstrbuffer_finalize(&post->from);
	free(post);
}
