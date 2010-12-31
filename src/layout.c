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
#include    "layoutarray.h"
#include    "tadlib.h"

#include	<bstdio.h>
#include	<bstdlib.h>
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

struct datlayout_t_ {
	GID target;
	datlayoutstyle_t *style;
	datlayoutarray_t *boxarray;
};

LOCAL W datlayout_res_calcresheaderdrawsize(datlayout_res_t *layout_res, GID gid, SIZE *sz)
{
	SIZE sz_number = {0,0}, sz_name = {0,0}, sz_mail = {0,0}, sz_date = {0,0}, sz_id = {0,0}, sz_beid = {0,0};
	actionlist_t *alist_tmp;
	TC numstr[10];
	W numstr_len, chwidth;
	H id_left = 0;

	numstr_len = tadlib_UW_to_str(layout_res->index + 1, numstr, 10);

	layoutstyle_resetgenvfont(gid);
	tadlib_calcdrawsize(numstr, numstr_len, gid, &sz_number, &alist_tmp);
	actionlist_delete(alist_tmp);

	tadlib_calcdrawsize(layout_res->parser_res->name, layout_res->parser_res->name_len, gid, &sz_name, &layout_res->action.name);

	layoutstyle_resetgenvfont(gid);
	tadlib_calcdrawsize(layout_res->parser_res->mail, layout_res->parser_res->mail_len, gid, &sz_mail, &layout_res->action.mail);
	layoutstyle_resetgenvfont(gid);
	chwidth = gget_stw(gid, (TC[]){TK_LABR, TK_RABR}, 2, NULL, NULL);
	if (chwidth > 0) { /* should calc here ? */
		sz_mail.h += chwidth;
	}

	layoutstyle_resetgenvfont(gid);
	tadlib_calcdrawsize(layout_res->parser_res->dateinfo.date, layout_res->parser_res->dateinfo.date_len, gid, &sz_date, &alist_tmp);
	actionlist_delete(alist_tmp);
	tadlib_calcdrawsize(layout_res->parser_res->dateinfo.id, layout_res->parser_res->dateinfo.id_len, gid, &sz_id, &alist_tmp);
	actionlist_delete(alist_tmp);
	tadlib_calcdrawsize(layout_res->parser_res->dateinfo.beid, layout_res->parser_res->dateinfo.beid_len, gid, &sz_beid, &alist_tmp);
	actionlist_delete(alist_tmp);

	sz->h = sz_number.h + sz_name.h + sz_mail.h + 16*(1+1);
	if (layout_res->parser_res->dateinfo.date != NULL) {
		sz->h += 16 + sz_date.h;
	}
	if (layout_res->parser_res->dateinfo.id != NULL) {
		id_left = sz->h + 16;
		sz->h += 16 + sz_id.h;
	}
	if (layout_res->parser_res->dateinfo.beid != NULL) {
		sz->h += 16 + sz_beid.h;
	}

	sz->v = sz_number.v;
	if (sz_name.v > sz->v) {
		sz->v = sz_name.v;
	}
	if (sz_mail.v > sz->v) {
		sz->v = sz_mail.v;
	}
	if (sz_date.v > sz->v) {
		sz->v = sz_date.v;
	}

	layout_res->headerinfo.rel_number_pos.c.left = 0;
	layout_res->headerinfo.rel_number_pos.c.top = 0;
	layout_res->headerinfo.rel_number_pos.c.right = sz_number.h;
	layout_res->headerinfo.rel_number_pos.c.bottom = sz_number.v;
	layout_res->headerinfo.rel_id_pos.c.left = id_left;
	layout_res->headerinfo.rel_id_pos.c.top = 0;
	layout_res->headerinfo.rel_id_pos.c.right = id_left + sz_id.h;
	layout_res->headerinfo.rel_id_pos.c.bottom = sz_id.v;

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

EXPORT W datlayout_appendres(datlayout_t *layout, datparser_res_t *parser_res)
{
	datlayout_res_t *layout_res;
	datlayout_box_t box;
	W l,t,r,b,len,err;
	Bool exist;

	err = datlayoutarray_appendres(layout->boxarray, parser_res);
	if (err < 0) {
		return err;
	}
	exist = datlayoutarray_getreslast(layout->boxarray, &layout_res);
	if (exist == False) {
		return -1; /* TODO or assertion */
	}
	len = datlayoutarray_length(layout->boxarray);

	layoutstyle_resetgenvfont(layout->target);

	layout_res->index = len - 1;
	datlayoutarray_getbodybox(layout->boxarray, &box);
	datlayout_res_calcsize(layout_res, &(layout->style->res), &(layout->style->resheader), &(layout->style->resmessage), layout->target, box.l, box.b);
	datlayout_res_getviewrect(layout_res, &(layout->style->res), &l, &t, &r, &b);

	datlayoutarray_orrectbodybox(layout->boxarray, l, t, r, b);

	return 0;
}

EXPORT VOID datlayout_getdrawrect(datlayout_t *layout, W *l, W *t, W *r, W *b)
{
	datlayout_box_t box;
	datlayoutarray_getbodybox(layout->boxarray, &box);
	datlayout_box_getoffsetrect(&box, &(layout->style->body), l, t, r, b);
}

EXPORT TC* datlayout_gettitle(datlayout_t *layout)
{
	datlayout_res_t *layout_res;
	Bool exist;
	exist = datlayoutarray_getresfirst(layout->boxarray, &layout_res);
	if (exist == False) {
		return NULL;
	}
	return layout_res->parser_res->title;
}

EXPORT W datlayout_gettitlelen(datlayout_t *layout)
{
	datlayout_res_t *layout_res;
	Bool exist;
	exist = datlayoutarray_getresfirst(layout->boxarray, &layout_res);
	if (exist == False) {
		return NULL;
	}
	return layout_res->parser_res->title_len;
}

EXPORT VOID datlayout_clear(datlayout_t *layout)
{
	datlayout_box_t newbody;

	datlayoutarray_clear(layout->boxarray);

	newbody.l = layout->style->body.margin_width_left + layout->style->body.border_width_left + layout->style->body.padding_width_left;
	newbody.t = layout->style->body.margin_width_top + layout->style->body.border_width_top + layout->style->body.padding_width_top;
	newbody.r = newbody.l;
	newbody.b = newbody.t;
	datlayoutarray_setbodybox(layout->boxarray, &newbody);
}

EXPORT W datlayout_getthreadviewrectbyindex(datlayout_t *layout, W n, W *l, W *t, W *r, W *b)
{
	datlayout_res_t *layout_res;
	Bool exist;

	exist = datlayoutarray_getresbyindex(layout->boxarray, n, &layout_res);
	if (exist == False) {
		return 0;
	}

	datlayout_res_getviewrect(layout_res, &(layout->style->res), l, t, r, b);

	return 1;
}

EXPORT datlayout_t* datlayout_new(GID gid, datlayoutstyle_t *style, datlayoutarray_t *layoutarray)
{
	datlayout_t *layout;
	datlayout_box_t bodybox;

	layout = (datlayout_t*)malloc(sizeof(datlayout_t));
	if (layout == NULL) {
		return NULL;
	}
	layout->boxarray = layoutarray;
	layout->target = gid;
	layout->style = style;

	bodybox.l = layout->style->body.margin_width_left + layout->style->body.border_width_left + layout->style->body.padding_width_left;
	bodybox.t = layout->style->body.margin_width_top + layout->style->body.border_width_top + layout->style->body.padding_width_top;
	bodybox.r = bodybox.l;
	bodybox.b = bodybox.t;
	datlayoutarray_setbodybox(layout->boxarray, &bodybox);

	return layout;
}

EXPORT VOID datlayout_delete(datlayout_t *layout)
{
	free(layout);
}
