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
#include	<tstring.h>
#include	<tcode.h>
#include	<btron/btron.h>
#include	<btron/dp.h>
#include	<tad.h>

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

typedef struct iterate_callbacks_t_ iterate_callbacks_t;
struct iterate_callbacks_t_ {
	iterate_callback_chcolor callback_chcolor;
	iterate_callback_chratio callback_chratio;
	iterate_callback_ch callback_ch;
	iterate_callback_br callback_br;
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

LOCAL VOID parse_fusen(LTADSEG *seg, iterate_callbacks_t *callbacks, VP arg)
{
	UB *data;
	UB segid;
	UB subid;
	UB attr;

	segid = seg->id & 0xFF;
	if (segid != TS_TFONT) {
		return;
	}

	if (seg->len == 0xffff) {
		data = ((UB*)seg) + 8;
	} else {
		data = ((UB*)seg) + 4;
	}

	subid = *(UH*)data >> 8;
	attr = *(UH*)data & 0xff;

	switch (subid) {
	case 3:
		parse_fusen_chration(attr, data, callbacks, arg);
		break;
	case 6:
		parse_fusen_chcolor(attr, data, callbacks, arg);
		break;
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

typedef struct tadlib_calcdrawsize_t_ tadlib_calcdrawsize_t;
struct tadlib_calcdrawsize_t_ {
	GID gid;
	SIZE sz;
	H ln_width;
	Bool isHankaku;
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
	ctx->sz.v += 16;
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

EXPORT W tadlib_calcdrawsize(TC *str, W len, GID gid, SIZE *sz)
{
	W err;
	tadlib_calcdrawsize_t ctx;
	iterate_callbacks_t callbacks;

	ctx.gid = gid;
	ctx.isHankaku = 0;
	ctx.sz.v = 16;
	ctx.sz.h = 0;
	ctx.ln_width = 0;

	callbacks.callback_chcolor = tadlib_calcdrawsize_chcolor;
	callbacks.callback_chratio = tadlib_calcdrawsize_chratio;
	callbacks.callback_ch = tadlib_calcdrawsize_ch;
	callbacks.callback_br = tadlib_calcdrawsize_br;

	err = parse_tad(str, len, &callbacks, &ctx);
	if (err < 0) {
		return err;
	}

	if (ctx.ln_width > ctx.sz.h) {
		ctx.sz.h = ctx.ln_width;
	}
	*sz = ctx.sz;

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
			gset_chp(ctx->gid, 4, 0, 0);
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
	gset_chp(ctx->gid, - ctx->dh, y+16, 1);

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

	return parse_tad(str, len, &callbacks, &ctx);
}
