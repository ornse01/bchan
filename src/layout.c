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
#include	<tstring.h>
#include	<tcode.h>
#include	<btron/btron.h>
#include	<btron/dp.h>
#include	<btron/libapp.h>
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

LOCAL TC dec[] = {TK_0,TK_1,TK_2,TK_3,TK_4,TK_5,TK_6,TK_7,TK_8,TK_9};

LOCAL W datlayout_res_UW_to_str(UW num, TC *dest)
{
	W i = 0, digit, draw = 0;
	TC dummy_str[10];

	if (dest == NULL) {
		dest = dummy_str;
	}

	digit = num / 1000 % 10;
	if ((digit != 0)||(draw != 0)) {
		dest[i++] = dec[digit];
		draw = 1;
	}
	digit = num / 100 % 10;
	if ((digit != 0)||(draw != 0)) {
		dest[i++] = dec[digit];
		draw = 1;
	}
	digit = num / 10 % 10;
	if ((digit != 0)||(draw != 0)) {
		dest[i++] = dec[digit];
		draw = 1;
	}
	digit = num % 10;
	if ((digit != 0)||(draw != 0)) {
		dest[i++] = dec[digit];
		draw = 1;
	}

	return i;
}

LOCAL W datlayout_res_totraytextdata_write_zenkaku(TC *str)
{
	TADSEG *seg = (TADSEG*)str;

	if (str == NULL) {
		return 10;
	}

	seg->id = 0xFF00|TS_TFONT;
	seg->len = 6;
	*(UH*)(str + 2) = 3 << 8;
	*(RATIO*)(str + 3) = 0x0101;
	*(RATIO*)(str + 4) = 0x0101;

	return 10;
}

LOCAL W datlayout_res_totraytextdata(datlayout_res_t *res, B *data, W data_len)
{
	W i = 0, num, digit, draw = 0;
	TC *str = (TC*)data;

	if (data != NULL) {
		num = res->index + 1;
		i += datlayout_res_totraytextdata_write_zenkaku(str + i) / 2;
		digit = num / 1000 % 10;
		if ((digit != 0)||(draw != 0)) {
			str[i++] = dec[digit];
			draw = 1;
		}
		digit = num / 100 % 10;
		if ((digit != 0)||(draw != 0)) {
			str[i++] = dec[digit];
			draw = 1;
		}
		digit = num / 10 % 10;
		if ((digit != 0)||(draw != 0)) {
			str[i++] = dec[digit];
			draw = 1;
		}
		digit = num % 10;
		if ((digit != 0)||(draw != 0)) {
			str[i++] = dec[digit];
			draw = 1;
		}
		str[i++] = TK_KSP;
		i += tadlib_remove_TA_APPL(res->parser_res->name, res->parser_res->name_len, str + i, data_len - i / 2) / 2;
		str[i++] = TK_KSP;
		i += datlayout_res_totraytextdata_write_zenkaku(str + i) / 2;
		str[i++] = TK_LABR;
		i += tadlib_remove_TA_APPL(res->parser_res->mail, res->parser_res->mail_len, str + i, data_len - i / 2) / 2;
		i += datlayout_res_totraytextdata_write_zenkaku(str + i) / 2;
		str[i++] = TK_RABR;
		str[i++] = TK_KSP;
		i += tadlib_remove_TA_APPL(res->parser_res->date, res->parser_res->date_len, str + i, data_len - i / 2) / 2;
		str[i++] = TK_NL;
		i += datlayout_res_totraytextdata_write_zenkaku(str + i) / 2;
		i += tadlib_remove_TA_APPL(res->parser_res->body, res->parser_res->body_len, str + i, data_len - i / 2) / 2;
		str[i++] = TK_NL;
	} else {
		num = res->index + 1;
		i += datlayout_res_totraytextdata_write_zenkaku(NULL) / 2;
		if ((0 <= num)&&(num < 10)) {
			i += 1;
		} else if ((10 <= num)&&(num < 100)) {
			i += 2;
		} else if ((100 <= num)&&(num < 1000)) {
			i += 3;
		} else if (1000 <= num) {
			i += 4;
		}
		i++; /* TK_KSP */
		i += tadlib_remove_TA_APPL_calcsize(res->parser_res->name, res->parser_res->name_len) / 2;
		i++; /* TK_KSP */
		i += datlayout_res_totraytextdata_write_zenkaku(NULL) / 2;
		i++; /* TO_LABR */
		i += tadlib_remove_TA_APPL_calcsize(res->parser_res->mail, res->parser_res->mail_len) / 2;
		i += datlayout_res_totraytextdata_write_zenkaku(NULL) / 2;
		i++; /* TO_RABR */
		i++; /* TK_KSP */
		i += tadlib_remove_TA_APPL_calcsize(res->parser_res->date, res->parser_res->date_len) / 2;
		i++; /* TK_NL */
		i += datlayout_res_totraytextdata_write_zenkaku(NULL) / 2;
		i += tadlib_remove_TA_APPL_calcsize(res->parser_res->body, res->parser_res->body_len) / 2;
		i++; /* TK_NL */
	}

	return i * sizeof(TC);
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
	SIZE sz_number = {0,0}, sz_name = {0,0}, sz_mail = {0,0}, sz_date = {0,0}, sz_id = {0,0}, sz_beid = {0,0};
	actionlist_t *alist_tmp;
	TC numstr[10];
	W numstr_len, chwidth;
	H id_left = 0;

	numstr_len = datlayout_res_UW_to_str(layout_res->index + 1, numstr);

	datlayout_res_setupgid(gid);
	tadlib_calcdrawsize(numstr, numstr_len, gid, &sz_number, &alist_tmp);
	actionlist_delete(alist_tmp);

	tadlib_calcdrawsize(layout_res->parser_res->name, layout_res->parser_res->name_len, gid, &sz_name, &layout_res->action.name);

	datlayout_res_setupgid(gid);
	tadlib_calcdrawsize(layout_res->parser_res->mail, layout_res->parser_res->mail_len, gid, &sz_mail, &layout_res->action.mail);
	datlayout_res_setupgid(gid);
	chwidth = gget_stw(gid, (TC[]){TK_LABR, TK_RABR}, 2, NULL, NULL);
	if (chwidth > 0) { /* should calc here ? */
		sz_mail.h += chwidth;
	}

	datlayout_res_setupgid(gid);
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

EXPORT VOID datlayout_res_getviewrect(datlayout_res_t *res, datlayout_style_t *resstyle, W *l, W *t, W *r, W *b)
{
	datlayout_box_getoffsetrect(&(res->box.res), resstyle, l, t, r, b);
}

EXPORT VOID datlayout_res_getcontentrect(datlayout_res_t *res, datlayout_style_t *resstyle, W *l, W *t, W *r, W *b)
{
	datlayout_box_getcontentrect(&(res->box.res), resstyle, l, t, r, b);
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

EXPORT W datlayout_resindextotraytextdata(datlayout_t *layout, W n, B *data, W data_len)
{
	datlayout_res_t *res;
	Bool exist;
	exist = datlayoutarray_getresbyindex(layout->boxarray, n, &res);
	if (exist == False) {
		return -1; /* TODO */
	}
	return datlayout_res_totraytextdata(res, data, data_len);
}

EXPORT W datlayout_idtotraytextdata(datlayout_t *layout, TC *id, W id_len, B *data, W data_len)
{
	W i, len, result, sum = 0;
	Bool haveid;
	datlayout_res_t *res;

	len = datlayoutarray_length(layout->boxarray);
	for (i = 0; i < len; i++) {
		datlayoutarray_getresbyindex(layout->boxarray, i, &res);

		haveid = datlayout_res_issameid(res, id, id_len);
		if (haveid == True) {
			if (data == NULL) {
				result = datlayout_res_totraytextdata(res, NULL, 0);
				if (result < 0) {
					return result;
				}
			} else {
				result = datlayout_res_totraytextdata(res, data + sum, data_len - sum);
				if (result < 0) {
					return result;
				}
			}
			sum += result;
		}
	}

	return sum;
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

	datlayout_setupgid(layout, layout->target);

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

LOCAL VOID datlayout_new_setdefaultstyle(datlayout_t *layout)
{
	layout->style->body.margin_width_left = 0;
	layout->style->body.margin_width_top = 0;
	layout->style->body.margin_width_right = 0;
	layout->style->body.margin_width_bottom = 0;
	layout->style->body.border_width_left = 0;
	layout->style->body.border_width_top = 0;
	layout->style->body.border_width_right = 0;
	layout->style->body.border_width_bottom = 0;
	layout->style->body.padding_width_left = 0;
	layout->style->body.padding_width_top = 0;
	layout->style->body.padding_width_right = 0;
	layout->style->body.padding_width_bottom = 0;
	layout->style->body.border_color_left = 0x10ffffff;
	layout->style->body.border_color_top = 0x10ffffff;
	layout->style->body.border_color_right = 0x10ffffff;
	layout->style->body.border_color_bottom = 0x10ffffff;
	layout->style->res.margin_width_left = 0;
	layout->style->res.margin_width_top = 3;
	layout->style->res.margin_width_right = 0;
	layout->style->res.margin_width_bottom = 3;
	layout->style->res.border_width_left = 0;
	layout->style->res.border_width_top = 0;
	layout->style->res.border_width_right = 0;
	layout->style->res.border_width_bottom = 0;
	layout->style->res.padding_width_left = 2;
	layout->style->res.padding_width_top = 2;
	layout->style->res.padding_width_right = 2;
	layout->style->res.padding_width_bottom = 5;
	layout->style->res.border_color_left = 0x10ffffff;
	layout->style->res.border_color_top = 0x10fffff;
	layout->style->res.border_color_right = 0x10ffffff;
	layout->style->res.border_color_bottom = 0x10ffffff;
	layout->style->resheader.margin_width_left = 3;
	layout->style->resheader.margin_width_top = 3;
	layout->style->resheader.margin_width_right = 3;
	layout->style->resheader.margin_width_bottom = 0;
	layout->style->resheader.border_width_left = 0;
	layout->style->resheader.border_width_top = 0;
	layout->style->resheader.border_width_right = 0;
	layout->style->resheader.border_width_bottom = 0;
	layout->style->resheader.padding_width_left = 2;
	layout->style->resheader.padding_width_top = 2;
	layout->style->resheader.padding_width_right = 2;
	layout->style->resheader.padding_width_bottom = 0;
	layout->style->resheader.border_color_left = 0x10ffffff;
	layout->style->resheader.border_color_top = 0x10ffffff;
	layout->style->resheader.border_color_right = 0x10ffffff;
	layout->style->resheader.border_color_bottom = 0x10ffffff;
	layout->style->resmessage.margin_width_left = 25;
	layout->style->resmessage.margin_width_top = 3;
	layout->style->resmessage.margin_width_right = 3;
	layout->style->resmessage.margin_width_bottom = 3;
	layout->style->resmessage.border_width_left = 0;
	layout->style->resmessage.border_width_top = 0;
	layout->style->resmessage.border_width_right = 0;
	layout->style->resmessage.border_width_bottom = 0;
	layout->style->resmessage.padding_width_left = 0;
	layout->style->resmessage.padding_width_top = 0;
	layout->style->resmessage.padding_width_right = 0;
	layout->style->resmessage.padding_width_bottom = 0;
	layout->style->resmessage.border_color_left = 0x10ffffff;
	layout->style->resmessage.border_color_top = 0x10ffffff;
	layout->style->resmessage.border_color_right = 0x10ffffff;
	layout->style->resmessage.border_color_bottom = 0x10ffffff;
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

	datlayout_new_setdefaultstyle(layout);

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

LOCAL W datdraw_entrydraw_hankakuchsize(GID gid)
{
	FSSPEC spec;

	gget_fon(gid, &spec, NULL);
	spec.attr |= FT_PROP;
	spec.attr |= FT_GRAYSCALE;
	spec.fclass = datlayout_fontconfig_class;
	spec.size.h = 8;
	spec.size.v = 16;
	gset_fon(gid, &spec);

	return 0;
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
	datdraw_entrydraw_resetchsize(target);
	gdra_chr(target, TK_LABR, G_STORE);
	err = datdraw_entrydraw_drawmail(entry, target, dh, dv);
	if (err < 0) {
		return err;
	}
	datdraw_entrydraw_resetchsize(target);
	gdra_chr(target, TK_RABR, G_STORE);

	err = datdraw_entrydraw_drawdateidbeid(entry, target, dh, dv);
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
		err = datdraw_entrydraw_hankakuchsize(target);
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
		datlayout_res_setupgid(draw->target); /* Ugh! */
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
