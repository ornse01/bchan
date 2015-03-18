/*
 * postres.c
 *
 * Copyright (c) 2009-2015 project bchan
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

#include    <coding/htmlform_urlencoder.h>

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

LOCAL VOID tcstrbuffer_clear(tcstrbuffer_t *tcstr)
{
	if (tcstr->buffer != NULL) {
		free(tcstr->buffer);
		tcstr->buffer = NULL;
		tcstr->strlen = 0;
	}
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

	err = sjstring_appendasciistring(dest, dlen, buf, buf_len);
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

LOCAL VOID postresdata_cleardata(postresdata_t *post)
{
	tcstrbuffer_clear(&post->from);
	tcstrbuffer_clear(&post->mail);
	tcstrbuffer_clear(&post->message);
	if (post->asc_from != NULL) {
		free(post->asc_from);
		post->asc_from = NULL;
		post->asc_from_len = 0;
	}
	if (post->asc_mail != NULL) {
		free(post->asc_mail);
		post->asc_mail = NULL;
		post->asc_mail_len = 0;
	}
	if (post->asc_message != NULL) {
		free(post->asc_message);
		post->asc_message = NULL;
		post->asc_message_len = 0;
	}
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

	postresdata_cleardata(post);

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

#define POST_FIELDS (7)

struct postresdata_genbodycontext_t_ {
	postresdata_t *post;
	UB timebuf[10];
	htmlform_field fields[POST_FIELDS];
	htmlform_urlencoder_t encoder;
} ;
typedef struct postresdata_genbodycontext_t_ postresdata_genbodycontext_t;

LOCAL UB name_bbs[] = "bbs";
LOCAL UB name_key[] = "key";
LOCAL UB name_time[] = "time";
LOCAL UB name_FROM[] = "FROM";
LOCAL UB name_mail[] = "mail";
LOCAL UB name_MESSAGE[] = "MESSAGE";
LOCAL UB name_submit[] = "submit";
LOCAL UB value_submit[] = { 0x8F, 0x91, 0x82, 0xAB, 0x8D, 0x9E, 0x82, 0xDE };

LOCAL Bool postresdata_genbodycontext_next(postresdata_genbodycontext_t *ctx, UB **str, W *len)
{
	return htmlform_urlencoder_next(&ctx->encoder, str, len);
}

LOCAL VOID postresdata_genbodycontext_initialize(postresdata_genbodycontext_t *ctx, postresdata_t *post, UB *board, W board_len, UB *thread, W thread_len, STIME time)
{
	ctx->post = post;

	ctx->fields[0].name = name_bbs;
	ctx->fields[0].name_len = strlen(name_bbs);
	ctx->fields[0].value = board;
	ctx->fields[0].value_len = board_len;
	ctx->fields[1].name = name_key;
	ctx->fields[1].name_len = strlen(name_key);
	ctx->fields[1].value = thread;
	ctx->fields[1].value_len = thread_len;
	ctx->fields[2].name = name_time;
	ctx->fields[2].name_len = strlen(name_time);
	ctx->fields[2].value = ctx->timebuf;
	ctx->fields[2].value_len = sjstring_writeUWstring(ctx->timebuf, time + 473385600);
	ctx->fields[3].name = name_FROM;
	ctx->fields[3].name_len = strlen(name_FROM);
	ctx->fields[3].value = ctx->post->asc_from;
	ctx->fields[3].value_len = ctx->post->asc_from_len;
	ctx->fields[4].name = name_mail;
	ctx->fields[4].name_len = strlen(name_mail);
	ctx->fields[4].value = ctx->post->asc_mail;
	ctx->fields[4].value_len = ctx->post->asc_mail_len;
	ctx->fields[5].name = name_MESSAGE;
	ctx->fields[5].name_len = strlen(name_MESSAGE);
	ctx->fields[5].value = ctx->post->asc_message;
	ctx->fields[5].value_len = ctx->post->asc_message_len;
	ctx->fields[6].name = name_submit;
	ctx->fields[6].name_len = strlen(name_submit);
	ctx->fields[6].value = value_submit;
	ctx->fields[6].value_len = strlen(value_submit);

	htmlform_urlencoder_initialize(&ctx->encoder, ctx->fields, POST_FIELDS);
}

LOCAL VOID postresdata_genbodycontext_finalize(postresdata_genbodycontext_t *ctx)
{
	htmlform_urlencoder_finalize(&ctx->encoder);
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
