/*
 * render.c
 *
 * Copyright (c) 2010 project bchan
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

#include    "render.h"
#include    "parser.h"
#include    "layoutarray.h"
#include    "layoutstyle.h"
#include    "tadlib.h"

#include	<bstdio.h>
#include	<bstdlib.h>
#include	<tstring.h>
#include	<tcode.h>
#include	<btron/btron.h>
#include	<btron/dp.h>
#include	<btron/libapp.h>
#include	<tad.h>

struct datdraw_t_ {
	GID target;
	datlayoutstyle_t *style;
	datlayoutarray_t *array;
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

LOCAL W datdraw_entrydraw_drawdateidbeid(datlayout_res_t *entry, GID gid, W dh, W dv)
{
	TC *str;
	W len, err;

	if (entry->parser_res->dateinfo.date != NULL) {
		err = gset_chp(gid, 16, 0, 0);
		if (err < 0) {
			return err;
		}
		str = entry->parser_res->dateinfo.date;
		len = entry->parser_res->dateinfo.date_len;
		err = tadlib_drawtext(str, len, gid, dh, dv);
		if (err < 0) {
			return err;
		}
	}
	if (entry->parser_res->dateinfo.id != NULL) {
		err = gset_chp(gid, 16, 0, 0);
		if (err < 0) {
			return err;
		}
		str = entry->parser_res->dateinfo.id;
		len = entry->parser_res->dateinfo.id_len;
		err = tadlib_drawtext(str, len, gid, dh, dv);
		if (err < 0) {
			return err;
		}
	}
	if (entry->parser_res->dateinfo.beid != NULL) {
		err = gset_chp(gid, 16, 0, 0);
		if (err < 0) {
			return err;
		}
		str = entry->parser_res->dateinfo.beid;
		len = entry->parser_res->dateinfo.beid_len;
		err = tadlib_drawtext(str, len, gid, dh, dv);
		if (err < 0) {
			return err;
		}
	}
	return 0;
#if 0
	TC *str = entry->parser_res->date;
	W len = entry->parser_res->date_len;
	return tadlib_drawtext(str, len, gid, dh, dv);
#endif
}

LOCAL TC dec[] = {TK_0,TK_1,TK_2,TK_3,TK_4,TK_5,TK_6,TK_7,TK_8,TK_9}; /* TODO: layout.c */

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

LOCAL W datdraw_fillrect(datdraw_t *draw, RECT *rect, W l, W t, W r, W b)
{
	static	PAT	pat0 = {{
		0,
		16, 16,
		0x10000000,
		0x10000000,
		FILL100
	}};
	W err, sect, dh, dv;
	RECT border;

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

	err = gfil_rec(draw->target, border, &pat0, 0, G_STORE);
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

LOCAL W datdraw_drawresborder(datdraw_t *draw, datlayout_res_t *layout_res, RECT *r)
{
	W err;

	err = datlayout_box_drawborder(&(layout_res->box.res), &draw->style->res, draw, r);
	if (err < 0) {
		return err;
	}
	err = datlayout_box_drawborder(&(layout_res->box.resheader), &draw->style->resheader, draw, r);
	if (err < 0) {
		return err;
	}
	err = datlayout_box_drawborder(&(layout_res->box.resmessage), &draw->style->resmessage, draw, r);
	if (err < 0) {
		return err;
	}

	return 0;
}

LOCAL W datdraw_entrydrawnormal(datlayout_res_t *entry, datlayout_style_t *resstyle, datlayout_style_t *resheaderstyle, datlayout_style_t *resmessagestyle, W index, datdraw_t *draw, GID target, RECT *r, W dh, W dv)
{
	W sect, err;
	W rv_l, rv_t, rv_r, rv_b;

	datlayout_res_getviewrect(entry, resstyle, &rv_l, &rv_t, &rv_r, &rv_b);

	/* sectrect */
	sect = sectrect_tmp(*r, rv_l - dh, rv_t - dv, rv_r - dh, rv_b - dv);
	if (sect == 0) {
		return 0;
	}

	err = datdraw_drawresborder(draw, entry, r);
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
	layoutstyle_resetgenvfont(target);
	gdra_chr(target, TK_LABR, G_STORE);
	err = datdraw_entrydraw_drawmail(entry, target, dh, dv);
	if (err < 0) {
		return err;
	}
	layoutstyle_resetgenvfont(target);
	gdra_chr(target, TK_RABR, G_STORE);

	err = datdraw_entrydraw_drawdateidbeid(entry, target, dh, dv);
	if (err < 0) {
		return err;
	}

	err = gset_chp(target, entry->box.resmessage.l - dh, entry->box.resmessage.t + 16 - dv, 1);
	if (err < 0) {
		return err;
	}
	layoutstyle_resetgenvfont(target);
	err = datdraw_entrydraw_drawbody(entry, target, dh - entry->box.resmessage.l/* Ugh! */, dv);
	if (err < 0) {
		return err;
	}

	return 0;
}

LOCAL W datdraw_entrydraw_drawNGrect(datdraw_t *draw, datlayout_res_t *entry, RECT *r)
{
	W rv_l, rv_t, rv_r, rv_b;
	W err;
	static PAT pat0 = {{
		0,
		16, 16,
		0x10000000,
		0x10000000,
		FILL100
	}};
	RECT fr;
	PNT p0, p1;

	datlayout_res_getcontentrect(entry, &draw->style->res, &rv_l, &rv_t, &rv_r, &rv_b);

	fr.c.left = rv_l - draw->view_l;
	fr.c.top = rv_t - draw->view_t;
	fr.c.right = rv_r - draw->view_l + 1;
	fr.c.bottom = rv_b - draw->view_t + 1;
	err = gfra_rec(draw->target, fr, 1, &pat0, 0, G_STORE);
	if (err < 0) {
		return err;
	}

	p0.x = rv_l - draw->view_l;
	p0.y = rv_t - draw->view_t;
	p1.x = rv_r - draw->view_l;
	p1.y = rv_b - draw->view_t;
	err = gdra_lin(draw->target, p0, p1, 1, &pat0, G_STORE);
	if (err < 0) {
		return err;
	}

	p0.x = rv_l - draw->view_l;
	p0.y = rv_b - draw->view_t;
	p1.x = rv_r - draw->view_l;
	p1.y = rv_t - draw->view_t;
	err = gdra_lin(draw->target, p0, p1, 1, &pat0, G_STORE);
	if (err < 0) {
		return err;
	}

	return 0;
}

LOCAL W datdraw_entrydrawNG(datdraw_t *draw, datlayout_res_t *entry, W index, RECT *r, W dh, W dv, Bool is_display_id)
{
	W sect, err, len;
	W rv_l, rv_t, rv_r, rv_b;
	datlayout_style_t *resstyle;
	GID target;
	TC *str;

	target = draw->target;
	resstyle = &draw->style->res;

	datlayout_res_getviewrect(entry, resstyle, &rv_l, &rv_t, &rv_r, &rv_b);

	/* sectrect */
	sect = sectrect_tmp(*r, rv_l - dh, rv_t - dv, rv_r - dh, rv_b - dv);
	if (sect == 0) {
		return 0;
	}

	err = datdraw_entrydraw_drawNGrect(draw, entry, r);
	if (err < 0) {
		return err;
	}

	err = datdraw_drawresborder(draw, entry, r);
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

	if ((is_display_id == True) && (entry->parser_res->dateinfo.id != NULL)) {
		err = gset_chp(target, entry->box.resheader.l - dh + entry->headerinfo.rel_id_pos.c.left, entry->box.resheader.t + 16 - dv, 1);
		if (err < 0) {
			return err;
		}
		err = layoutstyle_sethankakufont(target);
		if (err < 0) {
			return err;
		}
		str = entry->parser_res->dateinfo.id;
		len = entry->parser_res->dateinfo.id_len;
		err = tadlib_drawtext(str, len, target, dh, dv);
		if (err < 0) {
			return err;
		}
	}

	return 0;
}

LOCAL W datdraw_entrydraw(datlayout_res_t *entry, datlayout_style_t *resstyle, datlayout_style_t *resheaderstyle, datlayout_style_t *resmessagestyle, W index, datdraw_t *draw, GID target, RECT *r, W dh, W dv)
{
	Bool isNG;

	isNG = datlayout_res_isenableindexNG(entry);
	if (isNG == True) {
		return datdraw_entrydrawNG(draw, entry, index, r, dh, dv, False);
	}
	isNG = datlayout_res_isenableidNG(entry);
	if (isNG == True) {
		return datdraw_entrydrawNG(draw, entry, index, r, dh, dv, True);
	}
	return datdraw_entrydrawnormal(entry, resstyle, resheaderstyle, resmessagestyle, index, draw, target, r, dh, dv);
}

LOCAL W datdraw_bodyborderdraw(datdraw_t *draw, RECT *r)
{
	datlayout_box_t bodybox;
	datlayoutarray_getbodybox(draw->array, &bodybox);
	return datlayout_box_drawborder(&bodybox, &(draw->style->body), draw, r);
}

EXPORT W datdraw_draw(datdraw_t *draw, RECT *r)
{
	W i,len,err;
	GID target;
	datlayout_res_t *layout_res;
	Bool exist;

	target = draw->target;
	len = datlayoutarray_length(draw->array);

	for (i = 0; i < len; i++) {
		exist = datlayoutarray_getresbyindex(draw->array, i, &layout_res);
		if (exist == False) {
			break;
		}
		layoutstyle_resetgenvfont(draw->target);
		err = datdraw_entrydraw(layout_res, &(draw->style->res), &(draw->style->resheader), &(draw->style->resmessage), i, draw, target, r, draw->view_l, draw->view_t);
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
	W l,t,r,b,in;
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
			return 0;
		}
		if (pos.y > 16) {
			return 0;
		}
		if (pos.x < 0) {
			return 0;
		}
		in = inrect(entry->headerinfo.rel_number_pos, pos);
		if (in == 1) {
			*type = DATDRAW_FINDACTION_TYPE_NUMBER;
			return 1;
		}
		in = inrect(entry->headerinfo.rel_id_pos, pos);
		if (in == 1) {
			*type = DATDRAW_FINDACTION_TYPE_RESID;
			return 1;
		}
		return 0;
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
	W i,abs_x,abs_y,fnd,layout_len;
	W l,t,r,b;
	Bool exist;
	/*datlayout_t *layout;*/
	datlayout_res_t *res;

	/*layout = draw->layout;*/
	abs_x = rel_pos.x + draw->view_l;
	abs_y = rel_pos.y + draw->view_t;
	layout_len = datlayoutarray_length(draw->array);

	for (i = 0; i < layout_len; i++) {
		exist = datlayoutarray_getresbyindex(draw->array, i, &res);
		if (exist == False) {
			break;
		}

		fnd = datdraw_findentryaction(res, &(draw->style->res), &(draw->style->resheader), &(draw->style->resmessage), abs_x, abs_y, &l, &t, &r, &b, type, start, len);
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

EXPORT datdraw_t* datdraw_new(GID target, datlayoutstyle_t *style, datlayoutarray_t *layoutarray)
{
	datdraw_t *draw;

	draw = (datdraw_t*)malloc(sizeof(datdraw_t));
	if (draw == NULL) {
		return NULL;
	}
	draw->target = target;
	draw->style = style;
	draw->array = layoutarray;
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
