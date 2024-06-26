/*
 * tadlib.c
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

#include    "tadlib.h"

#include	<bstdio.h>
#include	<bstdlib.h>
#include	<tstring.h>
#include	<tcode.h>
#include	<btron/btron.h>
#include	<btron/dp.h>
#include	<btron/libapp.h>
#include	<tad.h>
#include	<bsys/queue.h>

#ifdef BCHAN_CONFIG_DEBUG
# define DP(arg) printf arg
# define DP_ER(msg, err) printf("%s (%d/%x)\n", msg, err>>16, err)
#else
# define DP(arg) /**/
# define DP_ER(msg, err) /**/
#endif

typedef W (*iterate_callback_ch)(VP arg, TC ch);
typedef W (*iterate_callback_br)(VP arg);
typedef W (*iterate_callback_chratio)(VP arg, RATIO w_ratio, RATIO h_ratio);
typedef W (*iterate_callback_chcolor)(VP arg, COLOR color);
typedef W (*iterate_callback_bchanappl)(VP arg, UB *seg, UB subid);

typedef struct iterate_callbacks_t_ iterate_callbacks_t;
struct iterate_callbacks_t_ {
	iterate_callback_chcolor callback_chcolor;
	iterate_callback_chratio callback_chratio;
	iterate_callback_ch callback_ch;
	iterate_callback_br callback_br;
	iterate_callback_bchanappl callback_bchanappl;
};

LOCAL VOID parse_fusen_chration(UB attr, UB *data, iterate_callbacks_t *callbacks, VP arg)
{
	RATIO w_ratio,h_ratio;

	w_ratio = *(RATIO*)(data + 4);
	h_ratio = *(RATIO*)(data + 2);

	(*callbacks->callback_chratio)(arg, w_ratio, h_ratio);
}

LOCAL VOID parse_fusen_chcolor(UB attr, UB *data, iterate_callbacks_t *callbacks, VP arg)
{
	COLOR color;

	color = *(COLOR*)(data + 2);

	(*callbacks->callback_chcolor)(arg, color);
}

LOCAL VOID parse_fusen_TS_TAPPL(LTADSEG *seg, UB *data, iterate_callbacks_t *callbacks, VP arg)
{
	TT_BCHAN *fsn = (TT_BCHAN*)data;

	if ((fsn->appl[0] == 0x8000)&&(fsn->appl[1] == 0xC053)&&(fsn->appl[2] == 0x8000)) {
		(*callbacks->callback_bchanappl)(arg, (UB*)seg, fsn->subid);		
	}
}

LOCAL VOID parse_fusen(LTADSEG *seg, iterate_callbacks_t *callbacks, VP arg)
{
	UB *data;
	UB segid;
	UB subid;
	UB attr;

	segid = seg->id & 0xFF;

	if (seg->len == 0xffff) {
		data = ((UB*)seg) + 8;
	} else {
		data = ((UB*)seg) + 4;
	}

	subid = *(UH*)data >> 8;
	attr = *(UH*)data & 0xff;

	if (segid == TS_TFONT) {
		switch (subid) {
		case 3:
			parse_fusen_chration(attr, data, callbacks, arg);
			break;
		case 6:
			parse_fusen_chcolor(attr, data, callbacks, arg);
			break;
		}
	} else if (segid == TS_TAPPL) {
		parse_fusen_TS_TAPPL(seg, data, callbacks, arg);
	}
}

LOCAL W parse_tad(TC *str, W len, iterate_callbacks_t *callbacks, VP arg)
{
	W i;
	TC ch;
	LTADSEG *seg;
	Bool skip;

	for (i=0;i<len;i++) {
		ch = str[i];

		/* temporary. should be move in convarting sjis? */
		skip = False;
		switch(ch) {
		case TK_LSTN:
			if (i + 3 <= len) {
				if ((str[i+1] == TK_b)
					&&(str[i+2] == TK_r)
					&&(str[i+3] == TK_GTTN)) {
					(*callbacks->callback_br)(arg);
					i+=3;
					skip = True;
				}
			}
			break;
		case TK_AND:
			if (i + 3 <= len) {
				if ((str[i+1] == TK_l)
					&&(str[i+2] == TK_t)
					&&(str[i+3] == TK_SCLN)) {
					(*callbacks->callback_ch)(arg, TK_LSTN);
					i+=3;
					skip = True;
				} else if ((str[i+1] == TK_g)
						   &&(str[i+2] == TK_t)
						   &&(str[i+3] == TK_SCLN)) {
					(*callbacks->callback_ch)(arg, TK_GTTN);
					i+=3;
					skip = True;
				} 
			}
		}

		if (skip != False) {
			continue;
		}

		if ((str[i] & 0xFF80) == 0xFF80) {
			seg = (LTADSEG*)(str + i);
			if (seg->len == 0xffff) {
				i += seg->llen / 2 + 3;
			} else {
				i += seg->len / 2 + 1;
			}
			parse_fusen(seg, callbacks, arg);
		} else if (str[i] == TK_NL) {
			(*callbacks->callback_br)(arg);
		} else {
			(*callbacks->callback_ch)(arg, str[i]);
		}
	}

	return 0;
}

typedef struct actionlist_data_t_ actionlist_data_t;
struct actionlist_data_t_ {
	QUEUE queue;
	RECT r;
	W type;
	UB *start;
	UB *end;
};

struct actionlist_t_ {
	actionlist_data_t sentinel;
};

LOCAL actionlist_data_t* actionlist_data_next(actionlist_data_t *data)
{
	return (actionlist_data_t*)data->queue.next;
}

LOCAL actionlist_data_t* actionlist_data_new(RECT r, W type, UB *start, UB *end)
{
	actionlist_data_t *alist_data;

	alist_data = malloc(sizeof(actionlist_data_t));
	if (alist_data == NULL) {
		return NULL;
	}
	alist_data->r = r;
	alist_data->type = type;
	alist_data->start = start;
	alist_data->end = end;

	return alist_data;
}

LOCAL VOID actionlist_data_delete(actionlist_data_t *cache_data)
{
	QueRemove(&(cache_data->queue));
	free(cache_data);
}

LOCAL W actionlist_appenddata(actionlist_t *alist, RECT r, W type, UB *start, UB *end)
{
	actionlist_data_t *alist_data;

	alist_data = actionlist_data_new(r, type, start, end);
	if (alist_data == NULL) {
		return -1; /* TODO */
	}
	QueInsert(&(alist_data->queue), &(alist->sentinel.queue));

	return 0; /* TODO */
}

EXPORT W actionlist_findboard(actionlist_t *alist, PNT pos, RECT *r, W *type, UB **start, W *len)
{
	actionlist_data_t *alist_data;
	W n;

	alist_data = &(alist->sentinel);
	for (;;) {
		alist_data = actionlist_data_next(alist_data);
		if (alist_data == &(alist->sentinel)) {
			break;
		}
		n = inrect(alist_data->r, pos);
		if (n == 1) {
			*r = alist_data->r;
			*type = alist_data->type;
			*start = alist_data->start;
			*len = alist_data->end - alist_data->start;
			return 1;
		}
	}

	return 0;
}

EXPORT actionlist_t* actionlist_new()
{
	actionlist_t *alist;

	alist = (actionlist_t*)malloc(sizeof(actionlist_t));
	if (alist == NULL) {
		return NULL;
	}
	QueInit(&(alist->sentinel.queue));
	return alist;
}

EXPORT VOID actionlist_delete(actionlist_t *alist)
{
	actionlist_data_t *alist_data;
	Bool ok;

	for (;;) {
		ok = isQueEmpty(&(alist->sentinel.queue));
		if (ok == True) {
			break;
		}
		alist_data = actionlist_data_next(&alist->sentinel);
		actionlist_data_delete(alist_data);
	}
	free(alist);
}

typedef struct tadlib_calcdrawsize_t_ tadlib_calcdrawsize_t;
struct tadlib_calcdrawsize_t_ {
	GID gid;
	SIZE sz;
	H ln_width;
	Bool isHankaku;
	actionlist_t *alist;
	PNT astart;
	UB *startseg;
};

LOCAL W tadlib_calcdrawsize_ch(VP arg, TC ch)
{
	W width;

	tadlib_calcdrawsize_t *ctx;
	ctx = (tadlib_calcdrawsize_t*)arg;

	width = gget_chw(ctx->gid, ch);
	if (width < 0) {
		return 0;
	}
	ctx->ln_width += width;

	return 0;
}

LOCAL W tadlib_calcdrawsize_br(VP arg)
{
	tadlib_calcdrawsize_t *ctx;
	ctx = (tadlib_calcdrawsize_t*)arg;
	ctx->sz.v += 18;
	if (ctx->ln_width > ctx->sz.h) {
		ctx->sz.h = ctx->ln_width;
	}
	ctx->ln_width = 0;
	return 0;
}

LOCAL W tadlib_calcdrawsize_chratio(VP arg, RATIO w_ratio, RATIO h_ratio)
{
	tadlib_calcdrawsize_t *ctx;
	GID gid;
	FSSPEC spec;
	W ratio_a, ratio_b;

	ctx = (tadlib_calcdrawsize_t*)arg;
	gid = ctx->gid;

	ratio_a = w_ratio >> 8;
	ratio_b = w_ratio & 0xFF;

	if ((ratio_a * 2 > ratio_b)||(ratio_b == 0)) {
		gget_fon(gid, &spec, NULL);
		spec.attr |= FT_PROP;
		spec.size.h = 16;
		spec.size.v = 16;
		gset_fon(gid, &spec);
		ctx->isHankaku = False;
	} else {
		gget_fon(gid, &spec, NULL);
		spec.attr |= FT_PROP;
		spec.size.h = 8;
		spec.size.v = 16;
		gset_fon(gid, &spec);
		ctx->isHankaku = True;
	}

	return 0;
}

LOCAL W tadlib_calcdrawsize_chcolor(VP arg, COLOR color)
{
	return 0;
}

LOCAL W tadlib_calcdrawsize_bchanappl(VP arg, UB *seg, UB subid)
{
	tadlib_calcdrawsize_t *ctx;
	RECT r;
	W err = 0;

	ctx = (tadlib_calcdrawsize_t*)arg;

	switch (subid) {
	case TT_BCHAN_SUBID_ANCHOR_START:
		ctx->astart.x = ctx->ln_width;
		ctx->astart.y = ctx->sz.v;
		ctx->startseg = seg;
		break;
	case TT_BCHAN_SUBID_ANCHOR_END:
		r.c.left = ctx->astart.x;
		r.c.top = ctx->astart.y - 16;
		r.c.right = ctx->ln_width;
		r.c.bottom = ctx->astart.y;
		err = actionlist_appenddata(ctx->alist, r, ACTIONLIST_ACTIONTYPE_ANCHOR, ctx->startseg, seg);
		break;
	case TT_BCHAN_SUBID_URL_START:
		ctx->astart.x = ctx->ln_width;
		ctx->astart.y = ctx->sz.v;
		ctx->startseg = seg;
		break;
	case TT_BCHAN_SUBID_URL_END:
		r.c.left = ctx->astart.x;
		r.c.top = ctx->astart.y - 16;
		r.c.right = ctx->ln_width;
		r.c.bottom = ctx->astart.y;
		err = actionlist_appenddata(ctx->alist, r, ACTIONLIST_ACTIONTYPE_URL, ctx->startseg, seg);
		break;
	}

	return err;
}

EXPORT W tadlib_calcdrawsize(TC *str, W len, GID gid, SIZE *sz, actionlist_t **alist)
{
	W err;
	tadlib_calcdrawsize_t ctx;
	iterate_callbacks_t callbacks;

	ctx.gid = gid;
	ctx.isHankaku = 0;
	ctx.sz.v = 16;
	ctx.sz.h = 0;
	ctx.ln_width = 0;
	ctx.alist = actionlist_new();
	if (ctx.alist == NULL) {
		return -1; /* TODO */
	}

	callbacks.callback_chcolor = tadlib_calcdrawsize_chcolor;
	callbacks.callback_chratio = tadlib_calcdrawsize_chratio;
	callbacks.callback_ch = tadlib_calcdrawsize_ch;
	callbacks.callback_br = tadlib_calcdrawsize_br;
	callbacks.callback_bchanappl = tadlib_calcdrawsize_bchanappl;

	err = parse_tad(str, len, &callbacks, &ctx);
	if (err < 0) {
		return err;
	}

	if (ctx.ln_width > ctx.sz.h) {
		ctx.sz.h = ctx.ln_width;
	}
	*sz = ctx.sz;
	*alist = ctx.alist;

	return 0;
}

typedef struct tadlib_drawtext_t_ tadlib_drawtext_t;
struct tadlib_drawtext_t_ {
	GID gid;
	Bool isHankaku;
	W dh,dv;
};

LOCAL W tadlib_drawtext_ch(VP arg, TC ch)
{
	tadlib_drawtext_t *ctx;
	ctx = (tadlib_drawtext_t*)arg;

	if (ch == TK_USCR) {
		if (ctx->isHankaku == True) {
			//gset_chp(ctx->gid, 4, 0, 0);
			gdra_chr(ctx->gid, ch, G_STORE);
		} else {
			gdra_chr(ctx->gid, ch, G_STORE);
		}
	} else {
		gdra_chr(ctx->gid, ch, G_STORE);
	}

	return 0;
}

LOCAL W tadlib_drawtext_br(VP arg)
{
	W x,y;
	tadlib_drawtext_t *ctx;
	ctx = (tadlib_drawtext_t*)arg;

	gget_chp(ctx->gid, &x, &y);
	gset_chp(ctx->gid, - ctx->dh, y+18, 1);

	return 0;
}

LOCAL W tadlib_drawtext_chratio(VP arg, RATIO w_ratio, RATIO h_ratio)
{
	tadlib_drawtext_t *ctx;
	GID gid;
	FSSPEC spec;
	W ratio_a, ratio_b;

	ctx = (tadlib_drawtext_t*)arg;
	gid = ctx->gid;

	ratio_a = w_ratio >> 8;
	ratio_b = w_ratio & 0xFF;

	if ((ratio_a * 2 > ratio_b)||(ratio_b == 0)) {
		gget_fon(gid, &spec, NULL);
		spec.attr |= FT_PROP;
		spec.size.h = 16;
		spec.size.v = 16;
		gset_fon(gid, &spec);
		ctx->isHankaku = False;
	} else {
		gget_fon(gid, &spec, NULL);
		spec.attr |= FT_PROP;
		spec.size.h = 8;
		spec.size.v = 16;
		gset_fon(gid, &spec);
		ctx->isHankaku = True;
	}

	return 0;
}

LOCAL W tadlib_drawtext_chcolor(VP arg, COLOR color)
{
	tadlib_drawtext_t *ctx;
	GID gid;

	ctx = (tadlib_drawtext_t*)arg;
	gid = ctx->gid;

	return gset_chc(gid, color, /*tmp*/0x10efefef);
}

LOCAL W tadlib_drawtext_bchanappl(VP arg, UB *seg, UB subid)
{
	tadlib_drawtext_t *ctx;
	GID gid;
	W err = 0;

	ctx = (tadlib_drawtext_t*)arg;
	gid = ctx->gid;

	switch (subid) {
	case TT_BCHAN_SUBID_ANCHOR_START:
		err = gset_chc(gid, 0x100000ff, /*tmp*/0x10efefef);
		break;
	case TT_BCHAN_SUBID_ANCHOR_END:
		err = gset_chc(gid, 0x10000000, /*tmp*/0x10efefef);
		break;
	case TT_BCHAN_SUBID_URL_START:
		err = gset_chc(gid, 0x100000ff, /*tmp*/0x10efefef);
		break;
	case TT_BCHAN_SUBID_URL_END:
		err = gset_chc(gid, 0x10000000, /*tmp*/0x10efefef);
		break;
	}
	return err;
}

EXPORT W tadlib_drawtext(TC *str, W len, GID gid, W dh, W dv)
{
	tadlib_drawtext_t ctx;
	iterate_callbacks_t callbacks;

	ctx.gid = gid;
	ctx.isHankaku = 0;
	ctx.dh = dh;
	ctx.dv = dv;

	callbacks.callback_chcolor = tadlib_drawtext_chcolor;
	callbacks.callback_chratio = tadlib_drawtext_chratio;
	callbacks.callback_ch = tadlib_drawtext_ch;
	callbacks.callback_br = tadlib_drawtext_br;
	callbacks.callback_bchanappl = tadlib_drawtext_bchanappl;

	return parse_tad(str, len, &callbacks, &ctx);
}

EXPORT W taditerator_next(taditerator_t *iterator, TC **pos, TC *segment, LTADSEG **seg, W *segsize, UB **data)
{
	TC *ch, *dummy_pos;
	LTADSEG *seg0;

	ch = iterator->bin + iterator->index;
	if (pos == NULL) {
		pos = &dummy_pos;
	}

	if (iterator->index >= iterator->len) {
		*pos = ch;
		return TADITERATOR_RESULT_END;
	}

	if ((*ch & 0xFF80) == 0xFF80) {
		*pos = ch;
		*segment = *ch & 0xFF;
		seg0 = (LTADSEG*)ch;
		if (seg0->len == 0xffff) {
			iterator->index += 1 + seg0->llen / 2 + 3;
			*segsize = seg0->llen;
			*data = ((UB*)seg0) + 8;
			*seg = seg0;
		} else {
			iterator->index += 1 + seg0->len / 2 + 1;
			*segsize = seg0->len;
			*data = ((UB*)seg0) + 4;
			*seg = seg0;
		}
		return TADITERATOR_RESULT_SEGMENT;
	} else {
		*pos = ch;
		*segment = *ch;
		*seg = NULL;
		*segsize = 2;
		*data = NULL;
		iterator->index++;
		return TADITERATOR_RESULT_CHARCTOR;
	}
}

EXPORT VOID taditerator_initialize(taditerator_t *iterator, TC *bin, W strlen)
{
	iterator->bin = bin;
	iterator->len = strlen;
	iterator->index = 0;
}

EXPORT VOID taditerator_finalize(taditerator_t *iterator)
{
}

EXPORT W tadlib_remove_TA_APPL_calcsize(TC *str, W len)
{
	taditerator_t iter;
	TC ch;
	LTADSEG *seg;
	W size;
	UB *data;
	TADITERATOR_RESULT_T ret;
	W allsize;

	taditerator_initialize(&iter, str, len);

	allsize = 0;
	for (;;) {
		ret = taditerator_next(&iter, NULL, &ch, &seg, &size, &data);
		if (ret == TADITERATOR_RESULT_CHARCTOR) {
			allsize += size;
		} else if (ret == TADITERATOR_RESULT_SEGMENT) {
			if (ch != TS_TAPPL) {
				if (seg->len == 0xffff) {
					allsize += seg->llen + 8;
				} else {
					allsize += seg->len + 4;
				}
			}
		} else if (ret == TADITERATOR_RESULT_END) {
			break;
		}
	}

	taditerator_finalize(&iter);

	return allsize;
}

EXPORT W tadlib_remove_TA_APPL(TC *str, W len, TC *data, W data_len)
{
	taditerator_t iter;
	TC ch;
	LTADSEG *seg;
	W size, i;
	UB *data0, *dest;
	TADITERATOR_RESULT_T ret;

	taditerator_initialize(&iter, str, len);

	i = 0;
	dest = (UB*)data;
	for (;;) {
		ret = taditerator_next(&iter, NULL, &ch, &seg, &size, &data0);
		if (ret == TADITERATOR_RESULT_CHARCTOR) {
			memcpy(dest + i, &ch, size);
			i += size;
		} else if (ret == TADITERATOR_RESULT_SEGMENT) {
			if (ch != TS_TAPPL) {
				if (seg->len == 0xffff) {
					size = seg->llen + 8;
				} else {
					size = seg->len + 4;
				}
				memcpy(dest + i, seg, size);
				i += size;
			}
		} else if (ret == TADITERATOR_RESULT_END) {
			break;
		}
	}

	taditerator_finalize(&iter);

	return i;
}

LOCAL TC dec[] = {TK_0,TK_1,TK_2,TK_3,TK_4,TK_5,TK_6,TK_7,TK_8,TK_9};

EXPORT W tadlib_UW_to_str(UW val, TC *dest, W dest_len)
{
	W i = 0, digit, draw = 0;
	TC dummy_str[10];

	if (dest == NULL) {
		dest = dummy_str;
	}

	digit = val / 1000 % 10;
	if ((digit != 0)||(draw != 0)) {
		dest[i++] = dec[digit];
		draw = 1;
	}
	digit = val / 100 % 10;
	if ((digit != 0)||(draw != 0)) {
		dest[i++] = dec[digit];
		draw = 1;
	}
	digit = val / 10 % 10;
	if ((digit != 0)||(draw != 0)) {
		dest[i++] = dec[digit];
		draw = 1;
	}
	digit = val % 10;
	if ((digit != 0)||(draw != 0)) {
		dest[i++] = dec[digit];
		draw = 1;
	}

	return i;
}

EXPORT VOID tadlib_separete_datepart(TC *str, W len, TC **date, W *date_len, TC **id, W *id_len, TC **beid, W *beid_len)
{
	taditerator_t iter;
	TC ch, *pos;
	LTADSEG *seg;
	W size;
	UB *data0;
	TADITERATOR_RESULT_T ret;
	Bool checktype = False;
	enum {
		PARSEDATE_DATE,
		PARSEDATE_ID,
		PARSEDATE_BEID
	} target = PARSEDATE_DATE;

	*date = str;
	*date_len = 0;
	*id = NULL;
	*id_len = 0;
	*beid = NULL;
	*beid_len = 0;

	taditerator_initialize(&iter, str, len);

	for (;;) {
		ret = taditerator_next(&iter, &pos, &ch, &seg, &size, &data0);
		if (ret == TADITERATOR_RESULT_CHARCTOR) {
			if (checktype == True) {
				if (ch == TK_I) {
					switch (target) {
					case PARSEDATE_DATE:
						*date_len = pos - *date - 1;
						break;
					case PARSEDATE_ID:
						*id_len = pos - *id - 1;
						break;
					case PARSEDATE_BEID:
						*beid_len = pos - *beid - 1;
						break;
					}
					*id = pos;
					target = PARSEDATE_ID;
				} else if (ch == TK_B) {
					switch (target) {
					case PARSEDATE_DATE:
						*date_len = pos - *date - 1;
						break;
					case PARSEDATE_ID:
						*id_len = pos - *id - 1;
						break;
					case PARSEDATE_BEID:
						*beid_len = pos - *beid - 1;
						break;
					}
					*beid = pos;
					target = PARSEDATE_BEID;
				}
				checktype = False;
			} else {
				if (ch == TK_KSP) {
					checktype = True;
				}
			}
		} else if (ret == TADITERATOR_RESULT_SEGMENT) {
		} else if (ret == TADITERATOR_RESULT_END) {
			switch (target) {
			case PARSEDATE_DATE:
				*date_len = pos - *date;
				break;
			case PARSEDATE_ID:
				*id_len = pos - *id;
				break;
			case PARSEDATE_BEID:
				*beid_len = pos - *beid;
				break;
			}
			break;
		}
	}

	taditerator_finalize(&iter);
}

#define TCTOKENCHECKER_FLAG_NOTEXIST 0x00000001
#define TCTOKENCHECKER_FLAG_AFTERENDCHAR 0x00000002

EXPORT VOID tctokenchecker_clear(tctokenchecker_t *checker)
{
	checker->stringindex = 0;
	checker->listindex_start = 0;
	checker->listindex_end = checker->namelistnum;
	checker->flag = 0;
}

EXPORT W tctokenchecker_inputchar(tctokenchecker_t *checker, TC c, W *val)
{
	W i;
	tctokenchecker_valuetuple_t *namelist = checker->namelist;

	if ((checker->flag & TCTOKENCHECKER_FLAG_AFTERENDCHAR) != 0) {
		return TCTOKENCHECKER_AFTER_END;
	}

	for (i = 0;; i++) {
		if ((checker->endtokens)[i] == TNULL) {
			break;
		}
		if (c == (checker->endtokens)[i]) {
			checker->flag |= TCTOKENCHECKER_FLAG_AFTERENDCHAR;
			if ((checker->flag & TCTOKENCHECKER_FLAG_NOTEXIST) != 0) {
				return TCTOKENCHECKER_NOMATCH;
			}
			if ((namelist[checker->listindex_start]).name[checker->stringindex] == TNULL) {
				/*List's Name End and receive EndToken = found match string*/
				*val = (namelist[checker->listindex_start]).val;
				return TCTOKENCHECKER_DETERMINE;
			}
			/*List's Name continue but receive endtoken.*/
			return TCTOKENCHECKER_NOMATCH;
		}
	}

	if ((checker->flag & TCTOKENCHECKER_FLAG_NOTEXIST) != 0) {
		return TCTOKENCHECKER_CONTINUE_NOMATCH;
	}

	for (i = checker->listindex_start; i < checker->listindex_end; i++) {
		if ((namelist[i]).name[checker->stringindex] == c) {
			break;
		}
	}
	if (i == checker->listindex_end) { /*receive char is not matched.*/
		checker->flag &= TCTOKENCHECKER_FLAG_NOTEXIST;
		return TCTOKENCHECKER_CONTINUE_NOMATCH;
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
		checker->flag |= TCTOKENCHECKER_FLAG_NOTEXIST;
		return TCTOKENCHECKER_CONTINUE_NOMATCH;
	}
	checker->stringindex++;

	return TCTOKENCHECKER_CONTINUE;
}

EXPORT VOID tctokenchecker_getlastmatchedstring(tctokenchecker_t *checker, TC **str, W *len)
{
	*str = (checker->namelist[checker->listindex_start]).name;
	*len = checker->stringindex;
}

EXPORT VOID tctokenchecker_initialize(tctokenchecker_t *checker, tctokenchecker_valuetuple_t *namelist, W namelistnum, TC *endchars)
{
	checker->namelist = namelist;
	checker->namelistnum = namelistnum;
	checker->endtokens = endchars;
	checker->stringindex = 0;
	checker->listindex_start = 0;
	checker->listindex_end = checker->namelistnum;
	checker->flag = 0;
}

EXPORT VOID tctokenchecker_finalize(tctokenchecker_t *checker)
{
}
