/*
 * postres.c
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
#include	<bctype.h>
#include	<errcode.h>
#include	<tstring.h>
#include	<tcode.h>
#include	<btron/btron.h>
#include	<btron/hmi.h>
#include	<btron/tf.h>

#include    "postres.h"
#include    "sjisstring.h"

LOCAL W postresdata_appendstring(TC **dest, W *dest_len, TC *str, W len)
{
	*dest = realloc(*dest, sizeof(TC)*(*dest_len + len + 1));
	if (*dest == NULL) {
		*dest_len = 0;
		return -1;
	}
	memcpy((*dest)+*dest_len, str, sizeof(TC)*len);
	*dest_len += len;
	(*dest)[*dest_len] = TNULL;

	return 0;
}

LOCAL Bool is_chratio_fusen(LTADSEG *seg, RATIO *w_ratio)
{
	UB *data;
	UB subid;

	if ((seg->id & 0xFF) != TS_TFONT) {
		return False;
	}
	if (seg->len != 6) {
		return False;
	}
	data = ((UB*)seg) + 4;
	subid = *(UH*)data >> 8;
	if (subid != 3) {
		return False;
	}
	*w_ratio = *(RATIO*)(data + 4);
	return True;
}

LOCAL W postresdata_readtad(postresdata_t *post, TC *str, W len)
{
	W i;
	TC ch, lang = 0xFE21, nest = 0;
	UB segid;
	LTADSEG *seg;
	TC header_from[] = {TK_F, TK_R, TK_O, TK_M, TK_COLN, TNULL};
	TC header_mail[] = {TK_m, TK_a, TK_i, TK_l, TK_COLN, TNULL};
	TC read_fieldname_index = 0;
	UB chratio_seg[] = {0xA2, 0xFF, 0x06, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00};
	W chratio_seg_len = sizeof(chratio_seg);
	Bool is_hankaku = False, is_chratio;
	RATIO w_ratio;
	W ratio_a, ratio_b;
	enum {
		SEARCH_HEADER,
		SKIP_HEADER,
		READ_HEADER_FIELDNAME_FROM,
		READ_HEADER_FIELDVALUE_FROM,
		READ_HEADER_FIELDNAME_mail,
		READ_HEADER_FIELDVALUE_mail,
		READ_VALUE_MESSAGE
	} state = SEARCH_HEADER;

	for (i=0;i<len;i++) {
		ch = str[i];

		if ((ch & 0xFF80) == 0xFF80) {
			seg = (LTADSEG*)(str + i);
			if (seg->len == 0xffff) {
				i += seg->llen / 2 + 3;
			} else {
				i += seg->len / 2 + 1;
			}

			segid = seg->id & 0xff;

			if ((segid == TS_TEXT)||(segid == TS_FIG)) {
				nest++;
			} else if ((segid == TS_TEXTEND)||(segid == TS_FIGEND)) {
				nest--;
			}

			is_chratio = is_chratio_fusen(seg, &w_ratio);
			if (is_chratio == True) {
				memcpy(chratio_seg, seg, chratio_seg_len);
				ratio_a = w_ratio >> 8;
				ratio_b = w_ratio & 0xFF;
				if ((ratio_a * 2 > ratio_b)||(ratio_b == 0)) {
					is_hankaku = False;
				} else {
					is_hankaku = True;
				}
				switch (state) {
				case READ_HEADER_FIELDVALUE_FROM:
					postresdata_appendstring(&(post->from), &(post->from_len), (TC*)chratio_seg, chratio_seg_len/sizeof(TC));
					break;
				case READ_HEADER_FIELDVALUE_mail:
					postresdata_appendstring(&(post->mail), &(post->mail_len), (TC*)chratio_seg, chratio_seg_len/sizeof(TC));
					break;
				case READ_VALUE_MESSAGE:
					postresdata_appendstring(&(post->message), &(post->message_len), (TC*)chratio_seg, chratio_seg_len/sizeof(TC));
					break;
				case SEARCH_HEADER:
				case SKIP_HEADER:
				case READ_HEADER_FIELDNAME_FROM:
				case READ_HEADER_FIELDNAME_mail:
				default:
					break;
				}
			}

			continue;
		}

		if (nest > 1) {
			continue;
		}

		if ((ch & 0xFF00) == 0xFE00) {
			lang = ch;
		}
		if (lang != 0xFE21) {
			continue;
		}

		switch (state) {
		case SEARCH_HEADER:
			if (ch == TK_NL) {
				state = READ_VALUE_MESSAGE;
				if (is_hankaku == True) {
					postresdata_appendstring(&(post->message), &(post->message_len), (TC*)chratio_seg, chratio_seg_len/sizeof(TC));
				}
				break;
			}
			read_fieldname_index = 0;
			if ((ch == header_mail[0])&&(lang == 0xFE21)) {
				state = READ_HEADER_FIELDNAME_mail;
				read_fieldname_index++;
			} else if ((ch == header_from[0])&&(lang == 0xFE21)) {
				state = READ_HEADER_FIELDNAME_FROM;
				read_fieldname_index++;
			} else {
				state = SKIP_HEADER;
			}
			break;
		case SKIP_HEADER:
			if (ch == TK_NL) {
				state = SEARCH_HEADER;
			}
			break;
		case READ_HEADER_FIELDNAME_FROM:
			if (ch == TK_NL) {
				state = SEARCH_HEADER;
				break;
			}
			if (read_fieldname_index >= 5) {
				if ((ch == TK_KSP)&&(lang == 0xFE21)) {
					break;
				}
				state = READ_HEADER_FIELDVALUE_FROM;
				if (is_hankaku == True) {
					postresdata_appendstring(&(post->from), &(post->from_len), (TC*)chratio_seg, chratio_seg_len/sizeof(TC));
				}
				postresdata_appendstring(&(post->from), &(post->from_len), &ch, 1);
				break;
			}
			if ((ch == header_from[read_fieldname_index])&&(lang == 0xFE21)) {
				read_fieldname_index++;
				break;
			}
			state = SKIP_HEADER;
			break;
		case READ_HEADER_FIELDVALUE_FROM:
			if (ch == TK_NL) {
				state = SEARCH_HEADER;
				break;
			}
			postresdata_appendstring(&(post->from), &(post->from_len), &ch, 1);
			break;
		case READ_HEADER_FIELDNAME_mail:
			if (ch == TK_NL) {
				state = SEARCH_HEADER;
				break;
			}
			if (read_fieldname_index >= 5) {
				if ((ch == TK_KSP)&&(lang == 0xFE21)) {
					break;
				}
				state = READ_HEADER_FIELDVALUE_mail;
				if (is_hankaku == True) {
					postresdata_appendstring(&(post->mail), &(post->mail_len), (TC*)chratio_seg, chratio_seg_len/sizeof(TC));
				}
				postresdata_appendstring(&(post->mail), &(post->mail_len), &ch, 1);
				break;
			}
			if ((ch == header_mail[read_fieldname_index])&&(lang == 0xFE21)) {
				read_fieldname_index++;
				break;
			}
			state = SKIP_HEADER;
			break;
		case READ_HEADER_FIELDVALUE_mail:
			if (ch == TK_NL) {
				state = SEARCH_HEADER;
				break;
			}
			postresdata_appendstring(&(post->mail), &(post->mail_len), &ch, 1);
			break;
		case READ_VALUE_MESSAGE:
			postresdata_appendstring(&(post->message), &(post->message_len), &ch, 1);
			break;
		default:
			break;
		}
	}

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
	buf = malloc(size);
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

	return err;
}

LOCAL W postresdata_appendasciistring(UB **dest, W *dest_len, UB *str, W len)
{
	return sjstring_appendasciistring(dest, dest_len, str, len);
}

LOCAL W postesdata_appendconvertedstring(TF_CTX *ctx, UB **dest, W *dlen, TC *src, W slen)
{
	UB *buf;
	W buf_len, err;

	if (slen == 0) {
		return 0;
	}

	err = tf_tcstostr(*ctx, src, slen, 0, TF_ATTR_START, NULL, &buf_len);
	if (err < 0) {
		return err;
	}
	buf = malloc(buf_len);
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

LOCAL W postesdata_appendUWstring(UB **dest, W *dlen, UW n)
{
	return sjstring_appendUWstring(dest, dlen, n);
}

EXPORT W postresdata_genrequestbody(postresdata_t *post, UB *board, W board_len, UB *thread, W thread_len, STIME time, UB **body, W *body_len)
{
	TF_CTX ctx;
	UB *buf_ret = NULL;
	W err, ctx_id, buf_ret_len = 0;
	UB name_bbs[] = "bbs=";
	UB name_key[] = "&key=";
	UB name_time[] = "&time=";
	UB name_FROM[] = "&FROM=";
	UB name_mail[] = "&mail=";
	UB name_MESSAGE[] = "&MESSAGE=";
	UB name_submit[] = "&submit=%8F%91%82%AB%8D%9E%82%DE";

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
	err = postesdata_appendconvertedstring(&ctx, &buf_ret, &buf_ret_len, post->from, post->from_len);
	if (err < 0) {
		free(buf_ret);
		return err;
	}
	err = postresdata_appendasciistring(&buf_ret, &buf_ret_len, name_mail, strlen(name_mail));
	if (err < 0) {
		free(buf_ret);
		return err;
	}
	err = postesdata_appendconvertedstring(&ctx, &buf_ret, &buf_ret_len, post->mail, post->mail_len);
	if (err < 0) {
		free(buf_ret);
		return err;
	}
	err = postresdata_appendasciistring(&buf_ret, &buf_ret_len, name_MESSAGE, strlen(name_MESSAGE));
	if (err < 0) {
		free(buf_ret);
		return err;
	}
	err = postesdata_appendconvertedstring(&ctx, &buf_ret, &buf_ret_len, post->message, post->message_len);
	if (err < 0) {
		free(buf_ret);
		return err;
	}
	err = postresdata_appendasciistring(&buf_ret, &buf_ret_len, name_submit, strlen(name_submit));
	if (err < 0) {
		free(buf_ret);
		return err;
	}

	tf_close_ctx(ctx);

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
	post->from = NULL;
	post->from_len = 0;
	post->mail = NULL;
	post->mail_len = 0;
	post->message = NULL;
	post->message_len = 0;

	return post;
}

EXPORT VOID postresdata_delete(postresdata_t *post)
{
	if (post->message != NULL) {
		free(post->message);
	}
	if (post->mail != NULL) {
		free(post->mail);
	}
	if (post->from != NULL) {
		free(post->message);
	}
	free(post);
}
