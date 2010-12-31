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
#include	<tcode.h>
#include	<btron/btron.h>
#include	<btron/libapp.h>
#include	<btron/dp.h>

struct datrender_t_ {
	GID target;
	datlayoutstyle_t *style;
	datlayoutarray_t *array;
	W view_l, view_t, view_r, view_b;
};

LOCAL W datrender_entrydraw_drawbody(datlayout_res_t *entry, GID gid, W dh, W dv)
{
	TC *str = entry->parser_res->body;
	W len = entry->parser_res->body_len;
	return tadlib_drawtext(str, len, gid, dh, dv);
}

LOCAL W datrender_entrydraw_drawname(datlayout_res_t *entry, GID gid, W dh, W dv)
{
	TC *str = entry->parser_res->name;
	W len = entry->parser_res->name_len;
	return tadlib_drawtext(str, len, gid, dh, dv);
}

LOCAL W datrender_entrydraw_drawmail(datlayout_res_t *entry, GID gid, W dh, W dv)
{
	TC *str = entry->parser_res->mail;
	W len = entry->parser_res->mail_len;
	return tadlib_drawtext(str, len, gid, dh, dv);
}

LOCAL W datrender_entrydraw_drawdateidbeid(datlayout_res_t *entry, GID gid, W dh, W dv)
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

LOCAL W datrender_entrydraw_resnumber(datlayout_res_t *entry, W resnum, GID target)
{
	W err,numstr_len;
	TC numstr[10];

	numstr_len = tadlib_UW_to_str(resnum, numstr, 10);
	err = gdra_str(target, numstr, numstr_len, G_STORE);
	if (err < 0) {
		return err;
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

LOCAL W datrender_fillrect(datrender_t *render, RECT *rect, W l, W t, W r, W b)
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

	sect = andrect_tmp(&l, &t, &r, &b, render->view_l, render->view_t, render->view_r, render->view_b);
	if (sect == 0) {
		return 0;
	}

	dh = render->view_l;
	dv = render->view_t;

	sect = sectrect_tmp(*rect, l - dh, t - dv, r - dh, b - dv);
	if (sect == 0) {
		return 0;
	}

	border.c.left = l - dh;
	border.c.top = t - dv;
	border.c.right = r - dh;
	border.c.bottom = b - dv;

	err = gfil_rec(render->target, border, &pat0, 0, G_STORE);
	if (err < 0) {
		return err;
	}

	return 0;
}

LOCAL W datlayout_box_drawleftborder(datlayout_box_t *box, datlayout_style_t *style, datrender_t *render, RECT *rect)
{
	W l,t,r,b;

	if (style->border_width_left <= 0) {
		return 0;
	}

	l = box->l - style->padding_width_left - style->border_width_left;
	t = box->t - style->padding_width_top - style->border_width_top;
	r = box->l - style->padding_width_left;
	b = box->b + style->padding_width_bottom + style->border_width_bottom;

	return datrender_fillrect(render, rect, l, t, r, b);
}

LOCAL W datlayout_box_drawtopborder(datlayout_box_t *box, datlayout_style_t *style, datrender_t *render, RECT *rect)
{
	W l,t,r,b;

	if (style->border_width_top <= 0) {
		return 0;
	}

	l = box->l - style->padding_width_left - style->border_width_left;
	t = box->t - style->padding_width_top - style->border_width_top;
	r = box->r + style->padding_width_right + style->border_width_right;
	b = box->t - style->padding_width_bottom;

	return datrender_fillrect(render, rect, l, t, r, b);
}

LOCAL W datlayout_box_drawrightborder(datlayout_box_t *box, datlayout_style_t *style, datrender_t *render, RECT *rect)
{
	W l,t,r,b;

	if (style->border_width_right <= 0) {
		return 0;
	}

	l = box->r + style->padding_width_right;
	t = box->t - style->padding_width_top - style->border_width_top;
	r = box->r + style->padding_width_right + style->border_width_right;
	b = box->b + style->padding_width_bottom + style->border_width_bottom;

	return datrender_fillrect(render, rect, l, t, r, b);
}

LOCAL W datlayout_box_drawbottomborder(datlayout_box_t *box, datlayout_style_t *style, datrender_t *render, RECT *rect)
{
	W l,t,r,b;

	if (style->border_width_top <= 0) {
		return 0;
	}

	l = box->l - style->padding_width_left - style->border_width_left;
	t = box->b + style->padding_width_top;
	r = box->r + style->padding_width_right + style->border_width_right;
	b = box->b + style->padding_width_bottom + style->border_width_bottom;

	return datrender_fillrect(render, rect, l, t, r, b);
}

LOCAL W datlayout_box_drawborder(datlayout_box_t *box, datlayout_style_t *style, datrender_t *render, RECT *r)
{
	W err;

	err = datlayout_box_drawleftborder(box, style, render, r);
	if (err < 0) {
		return err;
	}
	err = datlayout_box_drawtopborder(box, style, render, r);
	if (err < 0) {
		return err;
	}
	err = datlayout_box_drawrightborder(box, style, render, r);
	if (err < 0) {
		return err;
	}
	err = datlayout_box_drawbottomborder(box, style, render, r);
	if (err < 0) {
		return err;
	}
	return 0;
}

LOCAL W datrender_drawresborder(datrender_t *render, datlayout_res_t *layout_res, RECT *r)
{
	W err;

	err = datlayout_box_drawborder(&(layout_res->box.res), &render->style->res, render, r);
	if (err < 0) {
		return err;
	}
	err = datlayout_box_drawborder(&(layout_res->box.resheader), &render->style->resheader, render, r);
	if (err < 0) {
		return err;
	}
	err = datlayout_box_drawborder(&(layout_res->box.resmessage), &render->style->resmessage, render, r);
	if (err < 0) {
		return err;
	}

	return 0;
}

LOCAL W datrender_entrydrawnormal(datlayout_res_t *entry, datlayout_style_t *resstyle, datlayout_style_t *resheaderstyle, datlayout_style_t *resmessagestyle, W index, datrender_t *render, GID target, RECT *r, W dh, W dv)
{
	W sect, err;
	W rv_l, rv_t, rv_r, rv_b;

	datlayout_res_getviewrect(entry, resstyle, &rv_l, &rv_t, &rv_r, &rv_b);

	/* sectrect */
	sect = sectrect_tmp(*r, rv_l - dh, rv_t - dv, rv_r - dh, rv_b - dv);
	if (sect == 0) {
		return 0;
	}

	err = datrender_drawresborder(render, entry, r);
	if (err < 0) {
		return err;
	}

	gset_chc(target, 0x10000000, 0x10efefef);
	err = gset_chp(target, entry->box.resheader.l - dh, entry->box.resheader.t + 16 - dv, 1);
	if (err < 0) {
		return err;
	}
	err = datrender_entrydraw_resnumber(entry, index+1, target);
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
	err = datrender_entrydraw_drawname(entry, target, dh, dv);
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
	err = datrender_entrydraw_drawmail(entry, target, dh, dv);
	if (err < 0) {
		return err;
	}
	layoutstyle_resetgenvfont(target);
	gdra_chr(target, TK_RABR, G_STORE);

	err = datrender_entrydraw_drawdateidbeid(entry, target, dh, dv);
	if (err < 0) {
		return err;
	}

	err = gset_chp(target, entry->box.resmessage.l - dh, entry->box.resmessage.t + 16 - dv, 1);
	if (err < 0) {
		return err;
	}
	layoutstyle_resetgenvfont(target);
	err = datrender_entrydraw_drawbody(entry, target, dh - entry->box.resmessage.l/* Ugh! */, dv);
	if (err < 0) {
		return err;
	}

	return 0;
}

LOCAL W datrender_entrydraw_drawNGrect(datrender_t *render, datlayout_res_t *entry, RECT *r)
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

	datlayout_res_getcontentrect(entry, &render->style->res, &rv_l, &rv_t, &rv_r, &rv_b);

	fr.c.left = rv_l - render->view_l;
	fr.c.top = rv_t - render->view_t;
	fr.c.right = rv_r - render->view_l + 1;
	fr.c.bottom = rv_b - render->view_t + 1;
	err = gfra_rec(render->target, fr, 1, &pat0, 0, G_STORE);
	if (err < 0) {
		return err;
	}

	p0.x = rv_l - render->view_l;
	p0.y = rv_t - render->view_t;
	p1.x = rv_r - render->view_l;
	p1.y = rv_b - render->view_t;
	err = gdra_lin(render->target, p0, p1, 1, &pat0, G_STORE);
	if (err < 0) {
		return err;
	}

	p0.x = rv_l - render->view_l;
	p0.y = rv_b - render->view_t;
	p1.x = rv_r - render->view_l;
	p1.y = rv_t - render->view_t;
	err = gdra_lin(render->target, p0, p1, 1, &pat0, G_STORE);
	if (err < 0) {
		return err;
	}

	return 0;
}

LOCAL W datrender_entrydrawNG(datrender_t *render, datlayout_res_t *entry, W index, RECT *r, W dh, W dv, Bool is_display_id)
{
	W sect, err, len;
	W rv_l, rv_t, rv_r, rv_b;
	datlayout_style_t *resstyle;
	GID target;
	TC *str;

	target = render->target;
	resstyle = &render->style->res;

	datlayout_res_getviewrect(entry, resstyle, &rv_l, &rv_t, &rv_r, &rv_b);

	/* sectrect */
	sect = sectrect_tmp(*r, rv_l - dh, rv_t - dv, rv_r - dh, rv_b - dv);
	if (sect == 0) {
		return 0;
	}

	err = datrender_entrydraw_drawNGrect(render, entry, r);
	if (err < 0) {
		return err;
	}

	err = datrender_drawresborder(render, entry, r);
	if (err < 0) {
		return err;
	}

	gset_chc(target, 0x10000000, 0x10efefef);
	err = gset_chp(target, entry->box.resheader.l - dh, entry->box.resheader.t + 16 - dv, 1);
	if (err < 0) {
		return err;
	}
	err = datrender_entrydraw_resnumber(entry, index+1, target);
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

LOCAL W datrender_entrydraw(datlayout_res_t *entry, datlayout_style_t *resstyle, datlayout_style_t *resheaderstyle, datlayout_style_t *resmessagestyle, W index, datrender_t *render, GID target, RECT *r, W dh, W dv)
{
	Bool isNG;

	isNG = datlayout_res_isenableindexNG(entry);
	if (isNG == True) {
		return datrender_entrydrawNG(render, entry, index, r, dh, dv, False);
	}
	isNG = datlayout_res_isenableidNG(entry);
	if (isNG == True) {
		return datrender_entrydrawNG(render, entry, index, r, dh, dv, True);
	}
	return datrender_entrydrawnormal(entry, resstyle, resheaderstyle, resmessagestyle, index, render, target, r, dh, dv);
}

LOCAL W datrender_bodyborderdraw(datrender_t *render, RECT *r)
{
	datlayout_box_t bodybox;
	datlayoutarray_getbodybox(render->array, &bodybox);
	return datlayout_box_drawborder(&bodybox, &(render->style->body), render, r);
}

EXPORT W datrender_draw(datrender_t *render, RECT *r)
{
	W i,len,err;
	GID target;
	datlayout_res_t *layout_res;
	Bool exist;

	target = render->target;
	len = datlayoutarray_length(render->array);

	for (i = 0; i < len; i++) {
		exist = datlayoutarray_getresbyindex(render->array, i, &layout_res);
		if (exist == False) {
			break;
		}
		layoutstyle_resetgenvfont(render->target);
		err = datrender_entrydraw(layout_res, &(render->style->res), &(render->style->resheader), &(render->style->resmessage), i, render, target, r, render->view_l, render->view_t);
		if (err < 0) {
			return err;
		}
	}

	err = datrender_bodyborderdraw(render, r);
	if (err < 0) {
		return err;
	}

	return 0;
}

LOCAL W datrender_findentryaction(datlayout_res_t *entry, datlayout_style_t *resstyle, datlayout_style_t *resheaderstyle, datlayout_style_t *resmessagestyle, W abs_x, W abs_y, W *al, W *at, W *ar, W *ab, W *type, UB **start, W *len)
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
			*type = DATRENDER_FINDACTION_TYPE_NUMBER;
			return 1;
		}
		in = inrect(entry->headerinfo.rel_id_pos, pos);
		if (in == 1) {
			*type = DATRENDER_FINDACTION_TYPE_RESID;
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

EXPORT W datrender_findaction(datrender_t *render, PNT rel_pos, RECT *rect, W *type, UB **start, W *len, W *resindex)
{
	W i,abs_x,abs_y,fnd,layout_len;
	W l,t,r,b;
	Bool exist;
	datlayout_res_t *res;

	abs_x = rel_pos.x + render->view_l;
	abs_y = rel_pos.y + render->view_t;
	layout_len = datlayoutarray_length(render->array);

	for (i = 0; i < layout_len; i++) {
		exist = datlayoutarray_getresbyindex(render->array, i, &res);
		if (exist == False) {
			break;
		}

		fnd = datrender_findentryaction(res, &(render->style->res), &(render->style->resheader), &(render->style->resmessage), abs_x, abs_y, &l, &t, &r, &b, type, start, len);
		if (fnd == 1) {
			rect->c.left = l - render->view_l;
			rect->c.top = t - render->view_t;
			rect->c.right = r - render->view_l;
			rect->c.bottom = b - render->view_t;
			*resindex = i;
			return 1;
		}
	}

	return 0;
}

EXPORT VOID datrender_setviewrect(datrender_t *render, W l, W t, W r, W b)
{
	render->view_l = l;
	render->view_t = t;
	render->view_r = r;
	render->view_b = b;
}

EXPORT VOID datrender_getviewrect(datrender_t *render, W *l, W *t, W *r, W *b)
{
	*l = render->view_l;
	*t = render->view_t;
	*r = render->view_r;
	*b = render->view_b;
}

EXPORT VOID datrender_scrollviewrect(datrender_t *render, W dh, W dv)
{
	render->view_l += dh;
	render->view_t += dv;
	render->view_r += dh;
	render->view_b += dv;
}

EXPORT datrender_t* datrender_new(GID target, datlayoutstyle_t *style, datlayoutarray_t *layoutarray)
{
	datrender_t *render;

	render = (datrender_t*)malloc(sizeof(datrender_t));
	if (render == NULL) {
		return NULL;
	}
	render->target = target;
	render->style = style;
	render->array = layoutarray;
	render->view_l = 0;
	render->view_t = 0;
	render->view_r = 0;
	render->view_b = 0;

	return render;
}

EXPORT VOID datrender_delete(datrender_t *render)
{
	free(render);
}
