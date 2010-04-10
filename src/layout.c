/*
 * layout.c
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

#include    "layout.h"
#include    "parser.h"
#include    "tadlib.h"

#include	<bstdio.h>
#include	<bstdlib.h>
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

typedef struct datlayout_box_t_ datlayout_box_t;
struct datlayout_box_t_ {
	W l,t,r,b;
};

typedef struct datlayout_res_t_ datlayout_res_t;
struct datlayout_res_t_ {
	datparser_res_t *parser_res;
	W index;
	struct {
		datlayout_box_t res;
		datlayout_box_t resheader;
		datlayout_box_t resmessage;
	} box;
	struct {
		actionlist_t *name;
		actionlist_t *mail;
		actionlist_t *date;
		actionlist_t *body;
	} action;
};

struct datlayout_t_ {
	GID target;
	struct {
		datlayout_style_t body;
		datlayout_style_t res;
		datlayout_style_t resheader;
		datlayout_style_t resmessage;
	} style;
	datlayout_box_t body;
	datlayout_res_t **layout_res; /* should be QUEUE? */
	W len;
};

//#define datlayout_fontconfig_class FTC_MINCHO
//#define datlayout_fontconfig_class 0x000000c0 /* gothic */
#define datlayout_fontconfig_class FTC_DEFAULT

LOCAL VOID datlayout_box_getoffsetrect(datlayout_box_t *box, datlayout_style_t *style, W *l, W *t, W *r, W *b)
{
	*l = box->l - (style->margin_width_left + style->border_width_left + style->padding_width_left);
	*t = box->t - (style->margin_width_top + style->border_width_top + style->padding_width_top);
	*r = box->r + (style->margin_width_right + style->border_width_right + style->padding_width_right);
	*b = box->b + (style->margin_width_bottom + style->border_width_bottom + style->padding_width_bottom);
}

LOCAL VOID datlayout_box_getcontentrect(datlayout_box_t *box, datlayout_style_t *style, W *l, W *t, W *r, W *b)
{
	*l = box->l;
	*t = box->t;
	*r = box->r;
	*b = box->b;
}

LOCAL datlayout_res_t* datlayout_res_new(datparser_res_t *res)
{
	datlayout_res_t *layout_res;

	layout_res = (datlayout_res_t*)malloc(sizeof(datlayout_res_t));
	if (layout_res == NULL) {
		return NULL;
	}
	layout_res->parser_res = res;
	layout_res->box.res.l = 0;
	layout_res->box.res.t = 0;
	layout_res->box.res.r = 0;
	layout_res->box.res.b = 0;
	layout_res->action.name = NULL;
	layout_res->action.mail = NULL;
	layout_res->action.date = NULL;
	layout_res->action.body = NULL;

	return layout_res;
}

LOCAL VOID datlayout_res_delete(datlayout_res_t *layout_res)
{
	if (layout_res->action.body != NULL) {
		actionlist_delete(layout_res->action.body);
	}
	if (layout_res->action.date != NULL) {
		actionlist_delete(layout_res->action.date);
	}
	if (layout_res->action.mail != NULL) {
		actionlist_delete(layout_res->action.mail);
	}
	if (layout_res->action.name != NULL) {
		actionlist_delete(layout_res->action.name);
	}
	datparser_res_delete(layout_res->parser_res);
	free(layout_res);
}

LOCAL W datlayout_res_setupgid(GID gid)
{
	FSSPEC spec;

	gget_fon(gid, &spec, NULL);
	spec.attr |= FT_PROP;
	spec.attr |= FT_GRAYSCALE;
	spec.fclass = datlayout_fontconfig_class;
	spec.size.h = 16;
	spec.size.v = 16;
	gset_fon(gid, &spec);

	return 0;
}

LOCAL W datlayout_res_calcresheaderdrawsize(datlayout_res_t *layout_res, GID gid, SIZE *sz)
{
	SIZE sz_name = {0,0}, sz_mail = {0,0}, sz_date = {0,0};

	datlayout_res_setupgid(gid);
	tadlib_calcdrawsize(layout_res->parser_res->name, layout_res->parser_res->name_len, gid, &sz_name, &layout_res->action.name);
	datlayout_res_setupgid(gid);
	tadlib_calcdrawsize(layout_res->parser_res->mail, layout_res->parser_res->mail_len, gid, &sz_mail, &layout_res->action.mail);
	datlayout_res_setupgid(gid);
	tadlib_calcdrawsize(layout_res->parser_res->date, layout_res->parser_res->date_len, gid, &sz_date, &layout_res->action.date);

	sz->h = sz_name.h + sz_mail.h + sz_date.h + 16*(4+1+1+1+1);
	sz->v = sz_name.v;
	if (sz_mail.v > sz->v) {
		sz->v = sz_mail.v;
	}
	if (sz_date.v > sz->v) {
		sz->v = sz_date.v;
	}

	return 0;
}

LOCAL W datlayout_res_calcbodydrawsize(datlayout_res_t *layout_res, GID gid, SIZE *sz)
{
	return tadlib_calcdrawsize(layout_res->parser_res->body, layout_res->parser_res->body_len, gid, sz, &layout_res->action.body);
}

LOCAL W datlayout_res_calcsize(datlayout_res_t *layout_res, datlayout_style_t *resstyle, datlayout_style_t *resheaderstyle, datlayout_style_t *resmessagestyle, GID gid, W left, W top)
{
	SIZE sz_body = {0, 0}, sz_resheader = {0, 0};
	W l,t,r,b,err;

	err = datlayout_res_calcbodydrawsize(layout_res, gid, &sz_body);
	if (err < 0) {
		return err;
	}
	err = datlayout_res_calcresheaderdrawsize(layout_res, gid, &sz_resheader);
	if (err < 0) {
		return err;
	}

	layout_res->box.res.t = top + resstyle->margin_width_top + resstyle->border_width_top + resstyle->padding_width_top;
	layout_res->box.res.l = left + resstyle->margin_width_left + resstyle->border_width_left + resstyle->padding_width_left;

	layout_res->box.resheader.t = layout_res->box.res.t + resheaderstyle->margin_width_top + resheaderstyle->border_width_top + resheaderstyle->padding_width_top;
	layout_res->box.resheader.l = layout_res->box.res.l + resheaderstyle->margin_width_left + resheaderstyle->border_width_left + resheaderstyle->padding_width_left;
	layout_res->box.resheader.b = layout_res->box.resheader.t + sz_resheader.v;
	layout_res->box.resheader.r = sz_resheader.h;

	datlayout_box_getoffsetrect(&(layout_res->box.resheader), resheaderstyle, &l, &t, &r, &b);
	layout_res->box.res.r = r;

	layout_res->box.resmessage.t = b + resmessagestyle->margin_width_top + resmessagestyle->border_width_top + resmessagestyle->padding_width_top;
	layout_res->box.resmessage.l = layout_res->box.res.l + resmessagestyle->margin_width_left + resmessagestyle->border_width_left + resmessagestyle->padding_width_left;
	layout_res->box.resmessage.b = layout_res->box.resmessage.t + sz_body.v;
	layout_res->box.resmessage.r = layout_res->box.resmessage.l + sz_body.h;

	datlayout_box_getoffsetrect(&(layout_res->box.resmessage), resmessagestyle, &l, &t, &r, &b);
	if (r > layout_res->box.res.r) {
		layout_res->box.res.r = r;
	}
	layout_res->box.res.b = b;

	return err;
}

EXPORT VOID datlayout_res_getviewrect(datlayout_res_t *res, datlayout_style_t *resstyle, W *l, W *t, W *r, W *b)
{
	datlayout_box_getoffsetrect(&(res->box.res), resstyle, l, t, r, b);
}

EXPORT VOID datlayout_resheader_getviewrect(datlayout_res_t *res, datlayout_style_t *resstyle, W *l, W *t, W *r, W *b)
{
	datlayout_box_getoffsetrect(&(res->box.resheader), resstyle, l, t, r, b);
}

EXPORT VOID datlayout_resheader_getcontentrect(datlayout_res_t *res, datlayout_style_t *resstyle, W *l, W *t, W *r, W *b)
{
	datlayout_box_getcontentrect(&(res->box.resheader), resstyle, l, t, r, b);
}

EXPORT VOID datlayout_resmessage_getcontentrect(datlayout_res_t *res, datlayout_style_t *resstyle, W *l, W *t, W *r, W *b)
{
	datlayout_box_getcontentrect(&(res->box.resmessage), resstyle, l, t, r, b);
}

LOCAL W datlayout_setupgid(datlayout_t *layout, GID gid)
{
	FSSPEC spec;

	gget_fon(gid, &spec, NULL);
	spec.attr |= FT_PROP;
	spec.attr |= FT_GRAYSCALE;
	spec.fclass = datlayout_fontconfig_class;
	spec.size.h = 16;
	spec.size.v = 16;
	gset_fon(gid, &spec);

	return 0;
}

EXPORT W datlayout_appendres(datlayout_t *layout, datparser_res_t *parser_res)
{
	datlayout_res_t *layout_res;
	W l,t,r,b,len;

	layout_res = datlayout_res_new(parser_res);
	if (layout_res == NULL) {
		return -1; /* TODO */
	}

	len = layout->len + 1;
	layout->layout_res = (datlayout_res_t**)realloc(layout->layout_res, sizeof(datlayout_res_t*)*len);
	layout->layout_res[layout->len] = layout_res;
	layout->len = len;

	datlayout_setupgid(layout, layout->target);

	layout_res->index = len - 1;
	datlayout_res_calcsize(layout_res, &(layout->style.res), &(layout->style.resheader), &(layout->style.resmessage), layout->target, layout->body.l, layout->body.b);
	datlayout_res_getviewrect(layout_res, &(layout->style.res), &l, &t, &r, &b);

	/* orrect */
	if (layout->body.l > l) {
		layout->body.l = l;
	}
	if (layout->body.t > t) {
		layout->body.t = t;
	}
	if (layout->body.r < r) {
		layout->body.r = r;
	}
	if (layout->body.b < b) {
		layout->body.b = b;
	}

	return 0;
}

EXPORT VOID datlayout_getdrawrect(datlayout_t *layout, W *l, W *t, W *r, W *b)
{
	datlayout_box_getoffsetrect(&(layout->body), &(layout->style.body), l, t, r, b);
}

EXPORT TC* datlayout_gettitle(datlayout_t *layout)
{
	if (layout->len <= 0) {
		return NULL;
	}
	return layout->layout_res[0]->parser_res->title;
}

EXPORT W datlayout_gettitlelen(datlayout_t *layout)
{
	if (layout->len <= 0) {
		return NULL;
	}
	return layout->layout_res[0]->parser_res->title_len;
}

EXPORT VOID datlayout_clear(datlayout_t *layout)
{
	W i;

	if (layout->layout_res != NULL) {
		for (i=0;i<layout->len;i++) {
			datlayout_res_delete(layout->layout_res[i]);
		}
		free(layout->layout_res);
	}
	layout->layout_res = NULL;
	layout->len = 0;
	layout->body.l = layout->style.body.margin_width_left + layout->style.body.border_width_left + layout->style.body.padding_width_left;
	layout->body.t = layout->style.body.margin_width_top + layout->style.body.border_width_top + layout->style.body.padding_width_top;
	layout->body.r = layout->body.l;
	layout->body.b = layout->body.t;
}

EXPORT W datlayout_getthreadviewrectbyindex(datlayout_t *layout, W n, W *l, W *t, W *r, W *b)
{
	if ((n < 0)||(layout->len <= n)) {
		return 0;
	}

	datlayout_res_getviewrect(layout->layout_res[n], &(layout->style.res), l, t, r, b);

	return 1;
}

LOCAL VOID datlayout_new_setdefaultstyle(datlayout_t *layout)
{
	layout->style.body.margin_width_left = 0;
	layout->style.body.margin_width_top = 0;
	layout->style.body.margin_width_right = 0;
	layout->style.body.margin_width_bottom = 0;
	layout->style.body.border_width_left = 0;
	layout->style.body.border_width_top = 0;
	layout->style.body.border_width_right = 0;
	layout->style.body.border_width_bottom = 0;
	layout->style.body.padding_width_left = 0;
	layout->style.body.padding_width_top = 0;
	layout->style.body.padding_width_right = 0;
	layout->style.body.padding_width_bottom = 0;
	layout->style.body.border_color_left = 0x10ffffff;
	layout->style.body.border_color_top = 0x10ffffff;
	layout->style.body.border_color_right = 0x10ffffff;
	layout->style.body.border_color_bottom = 0x10ffffff;
	layout->style.res.margin_width_left = 0;
	layout->style.res.margin_width_top = 3;
	layout->style.res.margin_width_right = 0;
	layout->style.res.margin_width_bottom = 3;
	layout->style.res.border_width_left = 0;
	layout->style.res.border_width_top = 0;
	layout->style.res.border_width_right = 0;
	layout->style.res.border_width_bottom = 0;
	layout->style.res.padding_width_left = 2;
	layout->style.res.padding_width_top = 2;
	layout->style.res.padding_width_right = 2;
	layout->style.res.padding_width_bottom = 5;
	layout->style.res.border_color_left = 0x10ffffff;
	layout->style.res.border_color_top = 0x10fffff;
	layout->style.res.border_color_right = 0x10ffffff;
	layout->style.res.border_color_bottom = 0x10ffffff;
	layout->style.resheader.margin_width_left = 3;
	layout->style.resheader.margin_width_top = 3;
	layout->style.resheader.margin_width_right = 3;
	layout->style.resheader.margin_width_bottom = 0;
	layout->style.resheader.border_width_left = 0;
	layout->style.resheader.border_width_top = 0;
	layout->style.resheader.border_width_right = 0;
	layout->style.resheader.border_width_bottom = 0;
	layout->style.resheader.padding_width_left = 2;
	layout->style.resheader.padding_width_top = 2;
	layout->style.resheader.padding_width_right = 2;
	layout->style.resheader.padding_width_bottom = 0;
	layout->style.resheader.border_color_left = 0x10ffffff;
	layout->style.resheader.border_color_top = 0x10ffffff;
	layout->style.resheader.border_color_right = 0x10ffffff;
	layout->style.resheader.border_color_bottom = 0x10ffffff;
	layout->style.resmessage.margin_width_left = 25;
	layout->style.resmessage.margin_width_top = 3;
	layout->style.resmessage.margin_width_right = 3;
	layout->style.resmessage.margin_width_bottom = 3;
	layout->style.resmessage.border_width_left = 0;
	layout->style.resmessage.border_width_top = 0;
	layout->style.resmessage.border_width_right = 0;
	layout->style.resmessage.border_width_bottom = 0;
	layout->style.resmessage.padding_width_left = 0;
	layout->style.resmessage.padding_width_top = 0;
	layout->style.resmessage.padding_width_right = 0;
	layout->style.resmessage.padding_width_bottom = 0;
	layout->style.resmessage.border_color_left = 0x10ffffff;
	layout->style.resmessage.border_color_top = 0x10ffffff;
	layout->style.resmessage.border_color_right = 0x10ffffff;
	layout->style.resmessage.border_color_bottom = 0x10ffffff;
}

EXPORT datlayout_t* datlayout_new(GID gid)
{
	datlayout_t *layout;

	layout = (datlayout_t*)malloc(sizeof(datlayout_t));
	if (layout == NULL) {
		return NULL;
	}
	layout->target = gid;
	layout->layout_res = NULL;
	layout->len = 0;

	datlayout_new_setdefaultstyle(layout);

	layout->body.l = layout->style.body.margin_width_left + layout->style.body.border_width_left + layout->style.body.padding_width_left;
	layout->body.t = layout->style.body.margin_width_top + layout->style.body.border_width_top + layout->style.body.padding_width_top;
	layout->body.r = layout->body.l;
	layout->body.b = layout->body.t;

	return layout;
}

EXPORT VOID datlayout_delete(datlayout_t *layout)
{
	W i;

	if (layout->layout_res != NULL) {
		for (i=0;i<layout->len;i++) {
			datlayout_res_delete(layout->layout_res[i]);
		}
		free(layout->layout_res);
	}
	free(layout);
}

struct datdraw_t_ {
	datlayout_t *layout;
	W view_l, view_t, view_r, view_b;
};

LOCAL W datdraw_entrydraw_drawbody(datlayout_res_t *entry, GID gid, W dh, W dv)
{
	TC *str = entry->parser_res->body;
	W len = entry->parser_res->body_len;
	return tadlib_drawtext(str, len, gid, dh, dv);
}

LOCAL W datdraw_entrydraw_drawname(datlayout_res_t *entry, GID gid, W dh, W dv)
{
	TC *str = entry->parser_res->name;
	W len = entry->parser_res->name_len;
	return tadlib_drawtext(str, len, gid, dh, dv);
}

LOCAL W datdraw_entrydraw_drawmail(datlayout_res_t *entry, GID gid, W dh, W dv)
{
	TC *str = entry->parser_res->mail;
	W len = entry->parser_res->mail_len;
	return tadlib_drawtext(str, len, gid, dh, dv);
}

LOCAL W datdraw_entrydraw_drawdate(datlayout_res_t *entry, GID gid, W dh, W dv)
{
	TC *str = entry->parser_res->date;
	W len = entry->parser_res->date_len;
	return tadlib_drawtext(str, len, gid, dh, dv);
}

LOCAL TC dec[] = {TK_0,TK_1,TK_2,TK_3,TK_4,TK_5,TK_6,TK_7,TK_8,TK_9};

LOCAL W datdraw_entrydraw_resnumber(datlayout_res_t *entry, W resnum, GID target)
{
	W err,digit,draw = 0;

	digit = resnum / 1000 % 10;
	if ((digit != 0)||(draw != 0)) {
		err = gdra_chr(target, dec[digit], G_STORE);
		if (err < 0) {
			return err;
		}
		draw = 1;
	}
	digit = resnum / 100 % 10;
	if ((digit != 0)||(draw != 0)) {
		err = gdra_chr(target, dec[digit], G_STORE);
		if (err < 0) {
			return err;
		}
		draw = 1;
	}
	digit = resnum / 10 % 10;
	if ((digit != 0)||(draw != 0)) {
		err = gdra_chr(target, dec[digit], G_STORE);
		if (err < 0) {
			return err;
		}
		draw = 1;
	}
	digit = resnum % 10;
	if ((digit != 0)||(draw != 0)) {
		err = gdra_chr(target, dec[digit], G_STORE);
		if (err < 0) {
			return err;
		}
		draw = 1;
	}

	return 0;
}

LOCAL int sectrect_tmp(RECT a, W left, W top, W right, W bottom)
{
	return (a.c.left<right && left<a.c.right && a.c.top<bottom && top<a.c.bottom);
}

LOCAL W andrect_tmp(W *l_dest, W *t_dest, W *r_dest, W *b_dest, W l2, W t2, W r2, W b2)
{
	if (!(*l_dest<r2 && l2<*r_dest && *t_dest<b2 && t2<*b_dest)) {
		*l_dest = 0;
		*t_dest = 0;
		*r_dest = 0;
		*b_dest = 0;
		return 0;
	}

	if (*l_dest < l2) {
		*l_dest = l2;
	}
	if (*t_dest < t2) {
		*t_dest = t2;
	}
	if (*r_dest > r2) {
		*r_dest = r2;
	}
	if (*b_dest > b2) {
		*b_dest = b2;
	}

	return 1;
}

LOCAL W datdraw_entrydraw_resetchsize(GID gid)
{
	FSSPEC spec;

	gget_fon(gid, &spec, NULL);
	spec.attr |= FT_PROP;
	spec.attr |= FT_GRAYSCALE;
	spec.fclass = datlayout_fontconfig_class;
	spec.size.h = 16;
	spec.size.v = 16;
	gset_fon(gid, &spec);

	return 0;
}

LOCAL W datdraw_fillrect(datdraw_t *draw, RECT *rect, W l, W t, W r, W b)
{
	datlayout_t *layout;
	static	PAT	pat0 = {{
		0,
		16, 16,
		0x10000000,
		0x10000000,
		FILL100
	}};
	W err, sect, dh, dv;
	RECT border;

	layout = draw->layout;

	sect = andrect_tmp(&l, &t, &r, &b, draw->view_l, draw->view_t, draw->view_r, draw->view_b);
	if (sect == 0) {
		return 0;
	}

	dh = draw->view_l;
	dv = draw->view_t;

	sect = sectrect_tmp(*rect, l - dh, t - dv, r - dh, b - dv);
	if (sect == 0) {
		return 0;
	}

	border.c.left = l - dh;
	border.c.top = t - dv;
	border.c.right = r - dh;
	border.c.bottom = b - dv;

	err = gfil_rec(layout->target, border, &pat0, 0, G_STORE);
	if (err < 0) {
		return err;
	}

	return 0;
}

LOCAL W datlayout_box_drawleftborder(datlayout_box_t *box, datlayout_style_t *style, datdraw_t *draw, RECT *rect)
{
	W l,t,r,b;

	if (style->border_width_left <= 0) {
		return 0;
	}

	l = box->l - style->padding_width_left - style->border_width_left;
	t = box->t - style->padding_width_top - style->border_width_top;
	r = box->l - style->padding_width_left;
	b = box->b + style->padding_width_bottom + style->border_width_bottom;

	return datdraw_fillrect(draw, rect, l, t, r, b);
}

LOCAL W datlayout_box_drawtopborder(datlayout_box_t *box, datlayout_style_t *style, datdraw_t *draw, RECT *rect)
{
	W l,t,r,b;

	if (style->border_width_top <= 0) {
		return 0;
	}

	l = box->l - style->padding_width_left - style->border_width_left;
	t = box->t - style->padding_width_top - style->border_width_top;
	r = box->r + style->padding_width_right + style->border_width_right;
	b = box->t - style->padding_width_bottom;

	return datdraw_fillrect(draw, rect, l, t, r, b);
}

LOCAL W datlayout_box_drawrightborder(datlayout_box_t *box, datlayout_style_t *style, datdraw_t *draw, RECT *rect)
{
	W l,t,r,b;

	if (style->border_width_right <= 0) {
		return 0;
	}

	l = box->r + style->padding_width_right;
	t = box->t - style->padding_width_top - style->border_width_top;
	r = box->r + style->padding_width_right + style->border_width_right;
	b = box->b + style->padding_width_bottom + style->border_width_bottom;

	return datdraw_fillrect(draw, rect, l, t, r, b);
}

LOCAL W datlayout_box_drawbottomborder(datlayout_box_t *box, datlayout_style_t *style, datdraw_t *draw, RECT *rect)
{
	W l,t,r,b;

	if (style->border_width_top <= 0) {
		return 0;
	}

	l = box->l - style->padding_width_left - style->border_width_left;
	t = box->b + style->padding_width_top;
	r = box->r + style->padding_width_right + style->border_width_right;
	b = box->b + style->padding_width_bottom + style->border_width_bottom;

	return datdraw_fillrect(draw, rect, l, t, r, b);
}

LOCAL W datlayout_box_drawborder(datlayout_box_t *box, datlayout_style_t *style, datdraw_t *draw, RECT *r)
{
	W err;

	err = datlayout_box_drawleftborder(box, style, draw, r);
	if (err < 0) {
		return err;
	}
	err = datlayout_box_drawtopborder(box, style, draw, r);
	if (err < 0) {
		return err;
	}
	err = datlayout_box_drawrightborder(box, style, draw, r);
	if (err < 0) {
		return err;
	}
	err = datlayout_box_drawbottomborder(box, style, draw, r);
	if (err < 0) {
		return err;
	}
	return 0;
}

LOCAL W datdraw_entrydraw(datlayout_res_t *entry, datlayout_style_t *resstyle, datlayout_style_t *resheaderstyle, datlayout_style_t *resmessagestyle, W index, datdraw_t *draw, GID target, RECT *r, W dh, W dv)
{
	W sect, err;
	W rv_l, rv_t, rv_r, rv_b;

	datlayout_res_getviewrect(entry, resstyle, &rv_l, &rv_t, &rv_r, &rv_b);

	/* sectrect */
	sect = sectrect_tmp(*r, rv_l - dh, rv_t - dv, rv_r - dh, rv_b - dv);
	if (sect == 0) {
		return 0;
	}

	err = datlayout_box_drawborder(&(entry->box.res), resstyle, draw, r);
	if (err < 0) {
		return err;
	}
	err = datlayout_box_drawborder(&(entry->box.resheader), resheaderstyle, draw, r);
	if (err < 0) {
		return err;
	}
	err = datlayout_box_drawborder(&(entry->box.resmessage), resmessagestyle, draw, r);
	if (err < 0) {
		return err;
	}

	gset_chc(target, 0x10000000, 0x10efefef);
	err = gset_chp(target, entry->box.resheader.l - dh, entry->box.resheader.t + 16 - dv, 1);
	if (err < 0) {
		return err;
	}
	err = datdraw_entrydraw_resnumber(entry, index+1, target);
	if (err < 0) {
		return err;
	}
	err = gset_chp(target, 16, 0, 0);
	if (err < 0) {
		return err;
	}
	if (entry->parser_res->mail_len != 0) {
		gset_chc(target, 0x100000ff, 0x10efefef);
	} else {
		gset_chc(target, 0x10008000, 0x10efefef);
	}
	err = datdraw_entrydraw_drawname(entry, target, dh, dv);
	if (err < 0) {
		return err;
	}
	err = gset_chp(target, 16, 0, 0);
	if (err < 0) {
		return err;
	}
	gset_chc(target, 0x10000000, 0x10efefef);
	datdraw_entrydraw_resetchsize(target);
	gdra_chr(target, TK_LABR, G_STORE);
	err = datdraw_entrydraw_drawmail(entry, target, dh, dv);
	if (err < 0) {
		return err;
	}
	datdraw_entrydraw_resetchsize(target);
	gdra_chr(target, TK_RABR, G_STORE);
	err = gset_chp(target, 16, 0, 0);
	if (err < 0) {
		return err;
	}
	err = datdraw_entrydraw_drawdate(entry, target, dh, dv);
	if (err < 0) {
		return err;
	}
	err = gset_chp(target, entry->box.resmessage.l - dh, entry->box.resmessage.t + 16 - dv, 1);
	if (err < 0) {
		return err;
	}
	datdraw_entrydraw_resetchsize(target);
	err = datdraw_entrydraw_drawbody(entry, target, dh - entry->box.resmessage.l/* Ugh! */, dv);
	if (err < 0) {
		return err;
	}

	return 0;
}

LOCAL W datdraw_bodyborderdraw(datdraw_t *draw, RECT *r)
{
	datlayout_t *layout;
	layout = draw->layout;
	return datlayout_box_drawborder(&(layout->body), &(layout->style.body), draw, r);
}

EXPORT W datdraw_draw(datdraw_t *draw, RECT *r)
{
	W i,err;
	GID target;
	datlayout_t *layout;

	layout = draw->layout;
	target = layout->target;

	for (i=0;i < layout->len;i++) {
		datlayout_setupgid(layout, layout->target);
		err = datdraw_entrydraw(layout->layout_res[i], &(layout->style.res), &(layout->style.resheader), &(layout->style.resmessage), i, draw, target, r, draw->view_l, draw->view_t);
		if (err < 0) {
			return err;
		}
	}

	err = datdraw_bodyborderdraw(draw, r);
	if (err < 0) {
		return err;
	}

	return 0;
}

LOCAL W datdraw_findentryaction(datlayout_res_t *entry, datlayout_style_t *resstyle, datlayout_style_t *resheaderstyle, datlayout_style_t *resmessagestyle, W abs_x, W abs_y, W *al, W *at, W *ar, W *ab, W *type, UB **start, W *len)
{
	W l,t,r,b,resnum;
	PNT pos;
	W fnd;
	RECT rect;

	datlayout_res_getviewrect(entry, resstyle, &l, &t, &r, &b);
	if (!((l <= abs_x)&&(abs_x < r)&&(t <= abs_y)&&(abs_y < b))) {
		return 0;
	}
	datlayout_resheader_getcontentrect(entry, resheaderstyle, &l, &t, &r, &b);
	if ((l <= abs_x)&&(abs_x < r)&&(t <= abs_y)&&(abs_y < b)) {
		pos.x = abs_x - l;
		pos.y = abs_y - t;
		if (pos.y < 0) {
			DP(("A\n"));
			return 0;
		}
		if (pos.y > 16) {
			DP(("B\n"));
			return 0;
		}
		if (pos.x < 0) {
			DP(("C\n"));
			return 0;
		}
		resnum = entry->index + 1;
		printf("resnum = %d, pos = {%d, %d}\n", resnum, pos.x, pos.y);
		if ((0 <= resnum)&&(resnum < 10)) {
			DP(("D\n"));
			if (pos.x > 16) {
				return 0;
			}
		} else if ((10 <= resnum)&&(resnum < 100)) {
			DP(("E\n"));
			if (pos.x > 16*2) {
				return 0;
			}
		} else if ((100 <= resnum)&&(resnum < 1000)) {
			DP(("F\n"));
			if (pos.x > 16*3) {
				return 0;
			}
		} else if ((1000 <= resnum)&&(resnum < 10000)) {
			DP(("G\n"));
			if (pos.x > 16*4) {
				return 0;
			}
		} else {
			/* should be handling over 10000 res? */
			DP(("H\n"));
			return 0;
		}
		*type = DATDRAW_FINDACTION_TYPE_NUMBER;
		DP(("I\n"));
		return 1;
	}
	datlayout_resmessage_getcontentrect(entry, resmessagestyle, &l, &t, &r, &b);
	if ((l <= abs_x)&&(abs_x < r)&&(t <= abs_y)&&(abs_y < b)) {
		pos.x = abs_x - l;
		pos.y = abs_y - t;
		fnd = actionlist_findboard(entry->action.body, pos, &rect, type, start, len);
		if (fnd != 0) {
			*al = rect.c.left + l;
			*at = rect.c.top + t;
			*ar = rect.c.right + l;
			*ab = rect.c.bottom + t;
			return fnd;
		}
	}
	return 0;
}

EXPORT W datdraw_findaction(datdraw_t *draw, PNT rel_pos, RECT *rect, W *type, UB **start, W *len, W *resindex)
{
	W i,abs_x,abs_y,fnd;
	W l,t,r,b;
	datlayout_t *layout;
	datlayout_res_t *res;

	layout = draw->layout;
	abs_x = rel_pos.x + draw->view_l;
	abs_y = rel_pos.y + draw->view_t;

	for (i=0;i < layout->len;i++) {
		res = layout->layout_res[i];
		fnd = datdraw_findentryaction(res, &(layout->style.res), &(layout->style.resheader), &(layout->style.resmessage), abs_x, abs_y, &l, &t, &r, &b, type, start, len);
		if (fnd == 1) {
			rect->c.left = l - draw->view_l;
			rect->c.top = t - draw->view_t;
			rect->c.right = r - draw->view_l;
			rect->c.bottom = b - draw->view_t;
			*resindex = i;
			return 1;
		}
	}

	return 0;
}

EXPORT VOID datdraw_setviewrect(datdraw_t *draw, W l, W t, W r, W b)
{
	draw->view_l = l;
	draw->view_t = t;
	draw->view_r = r;
	draw->view_b = b;
}

EXPORT VOID datdraw_getviewrect(datdraw_t *draw, W *l, W *t, W *r, W *b)
{
	*l = draw->view_l;
	*t = draw->view_t;
	*r = draw->view_r;
	*b = draw->view_b;
}

EXPORT VOID datdraw_scrollviewrect(datdraw_t *draw, W dh, W dv)
{
	draw->view_l += dh;
	draw->view_t += dv;
	draw->view_r += dh;
	draw->view_b += dv;
}

EXPORT datdraw_t* datdraw_new(datlayout_t *layout)
{
	datdraw_t *draw;

	draw = (datdraw_t*)malloc(sizeof(datdraw_t));
	if (draw == NULL) {
		return NULL;
	}
	draw->layout = layout;
	draw->view_l = 0;
	draw->view_t = 0;
	draw->view_r = 0;
	draw->view_b = 0;

	return draw;
}

EXPORT VOID datdraw_delete(datdraw_t *draw)
{
	free(draw);
}
