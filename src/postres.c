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

LOCAL W postesdata_appendUWstring(UB **dest, W *dlen, UW n)
{
	return sjstring_appendUWstring(dest, dlen, n);
}

EXPORT W postresdata_genrequestbody(postresdata_t *post, UB *board, W board_len, UB *thread, W thread_len, STIME time, UB **body, W *body_len)
{
	UB *buf_ret = NULL;
	W err, buf_ret_len = 0;
	UB name_bbs[] = "bbs=";
	UB name_key[] = "&key=";
	UB name_time[] = "&time=";
	UB name_FROM[] = "&FROM=";
	UB name_mail[] = "&mail=";
	UB name_MESSAGE[] = "&MESSAGE=";
	UB name_submit[] = "&submit=%8F%91%82%AB%8D%9E%82%DE";

	buf_ret = malloc(sizeof(UB));
	if (buf_ret == NULL) {
		return -1; /* TODO */
	}
	buf_ret[0] = '\0';

	err = postresdata_appendasciistring(&buf_ret, &buf_ret_len, name_bbs, strlen(name_bbs));
	if (err < 0) {
		free(buf_ret);
		return err;
	}
	err = postresdata_appendasciistring(&buf_ret, &buf_ret_len, board, board_len);
	if (err < 0) {
		free(buf_ret);
		return err;
	}
	err = postresdata_appendasciistring(&buf_ret, &buf_ret_len, name_key, strlen(name_key));
	if (err < 0) {
		free(buf_ret);
		return err;
	}
	err = postresdata_appendasciistring(&buf_ret, &buf_ret_len, thread, thread_len);
	if (err < 0) {
		free(buf_ret);
		return err;
	}
	err = postresdata_appendasciistring(&buf_ret, &buf_ret_len, name_time, strlen(name_time));
	if (err < 0) {
		free(buf_ret);
		return err;
	}
	err = postesdata_appendUWstring(&buf_ret, &buf_ret_len, time + 473385600);
	if (err < 0) {
		free(buf_ret);
		return err;
	}
	err = postresdata_appendasciistring(&buf_ret, &buf_ret_len, name_FROM, strlen(name_FROM));
	if (err < 0) {
		free(buf_ret);
		return err;
	}
	err = postresdata_appendasciistring(&buf_ret, &buf_ret_len, post->asc_from, post->asc_from_len);
	if (err < 0) {
		free(buf_ret);
		return err;
	}
	err = postresdata_appendasciistring(&buf_ret, &buf_ret_len, name_mail, strlen(name_mail));
	if (err < 0) {
		free(buf_ret);
		return err;
	}
	err = postresdata_appendasciistring(&buf_ret, &buf_ret_len, post->asc_mail, post->asc_mail_len);
	if (err < 0) {
		free(buf_ret);
		return err;
	}
	err = postresdata_appendasciistring(&buf_ret, &buf_ret_len, name_MESSAGE, strlen(name_MESSAGE));
	if (err < 0) {
		free(buf_ret);
		return err;
	}
	err = postresdata_appendasciistring(&buf_ret, &buf_ret_len, post->asc_message, post->asc_message_len);
	if (err < 0) {
		free(buf_ret);
		return err;
	}
	err = postresdata_appendasciistring(&buf_ret, &buf_ret_len, name_submit, strlen(name_submit));
	if (err < 0) {
		free(buf_ret);
		return err;
	}

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
