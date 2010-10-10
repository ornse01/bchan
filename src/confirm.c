/*
 * confirm.c
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
#include	<bstring.h>
#include	<errcode.h>
#include	<tstring.h>
#include	<keycode.h>
#include	<tcode.h>
#include	<btron/btron.h>
#include	<btron/dp.h>
#include	<btron/hmi.h>

#include    "confirm.h"
#include	"window.h"
#include	"poptray.h"
#include	"postres.h"
#include	"tadlib.h"

#ifdef BCHAN_CONFIG_DEBUG
# define DP(arg) printf arg
# define DP_ER(msg, err) printf("%s (%d/%x)\n", msg, err>>16, err)
#else
# define DP(arg) /**/
# define DP_ER(msg, err) /**/
#endif

struct cfrmwindow_t_ {
	WID wid;
	GID gid;
	WID parent;
	PAID ms_post_id;
	PAID ms_cancel_id;
	RECT r;
	datwindow_t *base;
	postresdata_t *post;
	cfrmwindow_notifyclosecallback notifyclosecallback;
	VP arg;
	TC *windowtitle;
	W dnum_post;
	W dnum_cancel;
	SIZE sz_ms_post;
	SIZE sz_ms_cancel;

	/* left top of content rect */
	PNT pos_from;
	PNT pos_mail;
	PNT pos_message;
	PNT pos_cancel;
	PNT pos_post;
};

LOCAL VOID cfrmwindow_close2(cfrmwindow_t *window)
{
	WDSTAT stat;
	W err;

	stat.attr = WA_STD;
	err = wget_sts(window->wid, &stat, NULL);
	if (err >= 0) {
		window->r = stat.r;
	}
	wcls_wnd(window->wid, CLR);
	window->wid = -1;
	window->gid = -1;
	window->ms_post_id = -1;
	window->ms_cancel_id = -1;
}

LOCAL VOID cfrmwindow_scroll(VP arg, W dh, W dv)
{
	cfrmwindow_t *window = (cfrmwindow_t*)arg;
	wscr_wnd(window->wid, NULL, -dh, -dv, W_SCRL|W_RDSET);
}

LOCAL VOID cfrmwindow_resetgenv(cfrmwindow_t *window)
{
	FSSPEC spec;
	GID gid;

	gid = window->gid;

	gget_fon(gid, &spec, NULL);
	spec.attr |= FT_PROP;
	spec.attr |= FT_GRAYSCALE;
	spec.fclass = FTC_DEFAULT;
	spec.size.h = 16;
	spec.size.v = 16;
	gset_fon(gid, &spec);
	gset_chc(gid, 0x10000000, 0x10efefef);

	return;
}

LOCAL VOID cfrmwindow_draw(VP arg, RECT *r)
{
	cfrmwindow_t *window = (cfrmwindow_t*)arg;
	TC label_from[] = {0x4C3E, 0x4130, TK_COLN, TK_KSP, TNULL};
	TC label_mail[] = {TK_E, 0x213E, TK_m, TK_a, TK_i, TK_l, TK_COLN, TK_KSP, TNULL};
	GID gid;
	TC *from, *mail, *message;
	W from_len, mail_len, message_len;

	if (window->post == NULL) {
		return;
	}

	gid = window->gid;

	from = postresdata_getfromstring(window->post);
	from_len = postresdata_getfromstringlen(window->post);
	mail = postresdata_getmailstring(window->post);
	mail_len = postresdata_getmailstringlen(window->post);
	message = postresdata_getmessagestring(window->post);
	message_len = postresdata_getmessagestringlen(window->post);

	cfrmwindow_resetgenv(window);
	gdra_stp(gid, window->pos_from.x, window->pos_from.y + 16, label_from, 4, G_STORE);
	tadlib_drawtext(from, from_len, gid, 0, 0);
	cfrmwindow_resetgenv(window);
	gdra_stp(gid, window->pos_mail.x, window->pos_mail.y + 16, label_mail, 8, G_STORE);
	tadlib_drawtext(mail, mail_len, gid, 0, 0);
	cfrmwindow_resetgenv(window);
	gset_chp(gid, window->pos_message.x, window->pos_message.y + 16, True);
	tadlib_drawtext(message, message_len, gid, -window->pos_message.x, -window->pos_message.y);

	cdsp_pwd(window->wid, r, P_RDISP);
}

LOCAL VOID cfrmwindow_resize(VP arg)
{
	cfrmwindow_t *window = (cfrmwindow_t*)arg;
	RECT work;
	Bool workchange = False;

	wget_wrk(window->wid, &work);
	if (work.c.left < 0) {
		work.c.left = 0;
		workchange = True;
	}
	if (work.c.top < 0) {
		work.c.top = 0;
		workchange = True;
	}
	wset_wrk(window->wid, &work);
	gset_vis(window->gid, work);

	datwindow_setworkrect(window->base, work.c.left, work.c.top, work.c.right, work.c.bottom);

	if (workchange == True) {
		wera_wnd(window->wid, NULL);
		wreq_dsp(window->wid);
	}
}

LOCAL VOID cfrmwindow_butdn(VP arg, WEVENT *wev)
{
	cfrmwindow_t *window = (cfrmwindow_t*)arg;
	PAID id;
	W ret;

	ret = cfnd_par(window->wid, wev->s.pos, &id);
	if (ret <= 0) {
		return;
	}
	if (id == window->ms_post_id) {
		ret = cact_par(window->ms_post_id, wev);
		if ((ret & 0x5000) != 0x5000) {
			return;
		}
		cfrmwindow_close2(window);
		(*window->notifyclosecallback)(window->arg, 1);
		return;
	}
	if (id == window->ms_cancel_id) {
		ret = cact_par(window->ms_cancel_id, wev);
		if ((ret & 0x5000) != 0x5000) {
			return;
		}
		cfrmwindow_close2(window);
		(*window->notifyclosecallback)(window->arg, 0);
		return;
	}
}

LOCAL VOID cfrmwindow_close(VP arg)
{
	cfrmwindow_t *window = (cfrmwindow_t*)arg;

	cfrmwindow_close2(window);
	(*window->notifyclosecallback)(window->arg, 0);
}

#define CFRMWINDOW_LAYOUT_WHOLE_MARGIN 5
#define CFRMWINDOW_LAYOUT_FROM_MARGIN 0
#define CFRMWINDOW_LAYOUT_MAIL_MARGIN 0
#define CFRMWINDOW_LAYOUT_MESSAGE_MARGIN 16
#define CFRMWINDOW_LAYOUT_BUTTON_MARGIN 5

LOCAL VOID cfrmwindow_calclayout(cfrmwindow_t *window, SIZE *sz)
{
	SIZE sz_from = {0,0}, sz_mail = {0,0}, sz_msg = {0,0}, sz_button = {0,0};
	TC label_from[] = {0x4C3E, 0x4130, TK_COLN, TK_KSP, TNULL};
	TC label_mail[] = {TK_E, 0x213E, TK_m, TK_a, TK_i, TK_l, TK_COLN, TK_KSP, TNULL};
	GID gid;
	W width_from, width_mail;
	actionlist_t *alist_from = NULL, *alist_mail = NULL, *alist_message = NULL;
	TC *from, *mail, *message;
	W from_len, mail_len, message_len;

	sz->h = 0;
	sz->v = 0;

	if (window->post == NULL) {
		return;
	}

	gid = window->gid;

	from = postresdata_getfromstring(window->post);
	from_len = postresdata_getfromstringlen(window->post);
	mail = postresdata_getmailstring(window->post);
	mail_len = postresdata_getmailstringlen(window->post);
	message = postresdata_getmessagestring(window->post);
	message_len = postresdata_getmessagestringlen(window->post);

	cfrmwindow_resetgenv(window);
	width_from = gget_stw(gid, label_from, 4, NULL, NULL);
	if (width_from < 0) {
		return;
	}
	tadlib_calcdrawsize(from, from_len, gid, &sz_from, &alist_from);
	sz_from.h += width_from + CFRMWINDOW_LAYOUT_FROM_MARGIN * 2;
	sz_from.v += CFRMWINDOW_LAYOUT_FROM_MARGIN * 2;

	if (alist_from != NULL) {
		actionlist_delete(alist_from);
	}
	cfrmwindow_resetgenv(window);
	width_mail = gget_stw(gid, label_mail, 4, NULL, NULL);
	if (width_mail < 0) {
		return;
	}
	tadlib_calcdrawsize(mail, mail_len, gid, &sz_mail, &alist_mail);
	sz_mail.h += width_mail + CFRMWINDOW_LAYOUT_MAIL_MARGIN * 2;
	sz_mail.v += CFRMWINDOW_LAYOUT_MAIL_MARGIN * 2;

	if (alist_mail != NULL) {
		actionlist_delete(alist_mail);
	}
	cfrmwindow_resetgenv(window);
	tadlib_calcdrawsize(message, message_len, gid, &sz_msg, &alist_message);
	if (alist_message != NULL) {
		actionlist_delete(alist_message);
	}
	sz_msg.h += CFRMWINDOW_LAYOUT_MESSAGE_MARGIN * 2;
	sz_msg.v += CFRMWINDOW_LAYOUT_MESSAGE_MARGIN * 2;

	sz_button.h = window->sz_ms_post.h + window->sz_ms_cancel.h + CFRMWINDOW_LAYOUT_BUTTON_MARGIN * 4;
	sz_button.v = (window->sz_ms_post.v > window->sz_ms_cancel.v) ? window->sz_ms_post.v : window->sz_ms_cancel.v + CFRMWINDOW_LAYOUT_BUTTON_MARGIN * 2;

	sz->v += sz_from.v;
	if (sz->h < sz_from.h) {
		sz->h = sz_from.h;
	}
	sz->v += sz_mail.v;
	if (sz->h < sz_mail.h) {
		sz->h = sz_mail.h;
	}
	sz->v += sz_msg.v;
	if (sz->h < sz_msg.h) {
		sz->h = sz_msg.h;
	}
	sz->v += sz_button.v;
	if (sz->h < sz_button.h) {
		sz->h = sz_button.h;
	}
	sz->h += CFRMWINDOW_LAYOUT_WHOLE_MARGIN * 2;
	sz->v += CFRMWINDOW_LAYOUT_WHOLE_MARGIN * 2;

	window->pos_from.x = CFRMWINDOW_LAYOUT_WHOLE_MARGIN + CFRMWINDOW_LAYOUT_FROM_MARGIN;
	window->pos_from.y = CFRMWINDOW_LAYOUT_WHOLE_MARGIN + CFRMWINDOW_LAYOUT_FROM_MARGIN;
	window->pos_mail.x = CFRMWINDOW_LAYOUT_WHOLE_MARGIN + CFRMWINDOW_LAYOUT_MAIL_MARGIN;
	window->pos_mail.y = CFRMWINDOW_LAYOUT_WHOLE_MARGIN + sz_from.v + CFRMWINDOW_LAYOUT_MAIL_MARGIN;
	window->pos_message.x = CFRMWINDOW_LAYOUT_WHOLE_MARGIN + CFRMWINDOW_LAYOUT_MESSAGE_MARGIN;
	window->pos_message.y = CFRMWINDOW_LAYOUT_WHOLE_MARGIN + sz_from.v + sz_mail.v + CFRMWINDOW_LAYOUT_MESSAGE_MARGIN;
	window->pos_cancel.x = CFRMWINDOW_LAYOUT_WHOLE_MARGIN + CFRMWINDOW_LAYOUT_BUTTON_MARGIN;
	window->pos_cancel.y = CFRMWINDOW_LAYOUT_WHOLE_MARGIN + sz_from.v + sz_mail.v + sz_msg.v + CFRMWINDOW_LAYOUT_BUTTON_MARGIN;
	window->pos_post.x = window->pos_cancel.x + window->sz_ms_cancel.h + CFRMWINDOW_LAYOUT_BUTTON_MARGIN;
	window->pos_post.y = window->pos_cancel.y;
}

EXPORT W cfrmwindow_open(cfrmwindow_t* window)
{
	static	PAT	pat0 = {{
		0,
		16, 16,
		0x10efefef,
		0x10efefef,
		FILL100
	}};
	WID wid;
	SIZE sz;

	if (window->wid > 0) {
		return 0;
	}

	wid = wopn_wnd(WA_SUBW|WA_SIZE|WA_HHDL|WA_VHDL|WA_BBAR|WA_RBAR, window->parent, &(window->r), NULL, 2, window->windowtitle, &pat0, NULL);
	if (wid < 0) {
		DP_ER("wopn_wnd: confirm error", wid);
		return wid;
	}
	window->wid = wid;
	window->gid = wget_gid(wid);
	datwindow_setwid(window->base, wid);

	cfrmwindow_calclayout(window, &sz);
	datwindow_setdrawrect(window->base, 0, 0, sz.h, sz.v);
	datwindow_setworkrect(window->base, 0, 0, window->r.c.right, window->r.c.bottom);

	window->ms_cancel_id = copn_par(wid, window->dnum_cancel, &window->pos_cancel);
	if (window->ms_cancel_id < 0) {
		DP_ER("copn_par error:", window->ms_cancel_id);
	}

	window->ms_post_id = copn_par(wid, window->dnum_post, &window->pos_post);
	if (window->ms_post_id < 0) {
		DP_ER("copn_par error:", window->ms_post_id);
	}

	wreq_dsp(wid);

	return 0;
}

EXPORT VOID cfrmwindow_weventbutdn(cfrmwindow_t *window, WEVENT *wev)
{
	datwindow_weventbutdn(window->base, wev);
}

EXPORT VOID cfrmwindow_weventrequest(cfrmwindow_t *window, WEVENT *wev)
{
	datwindow_weventrequest(window->base, wev);
}

EXPORT VOID cfrmwindow_weventswitch(cfrmwindow_t *window, WEVENT *wev)
{
	datwindow_weventswitch(window->base, wev);
}

EXPORT VOID cfrmwindow_weventreswitch(cfrmwindow_t *window, WEVENT *wev)
{
	datwindow_weventreswitch(window->base, wev);
}

EXPORT Bool cfrmwindow_compairwid(cfrmwindow_t *window, WID wid)
{
	if (window->wid < 0) {
		return False;
	}
	if (window->wid != wid) {
		return False;
	}
	return True;
}

EXPORT VOID cfrmwindow_setpostresdata(cfrmwindow_t *window, postresdata_t *post)
{
	SIZE sz;
	RECT r;
	RECT work;
	W err;

	window->post = post;
	if (window->wid > 0) {
		cfrmwindow_calclayout(window, &sz);
		datwindow_setdrawrect(window->base, 0, 0, sz.h, sz.v);

		work.c.left = 0;
		work.c.top = 0;
		wset_wrk(window->wid, &work);
		datwindow_setworkrect(window->base, work.c.left, work.c.top, work.c.right, work.c.bottom);

		r.c.left = window->pos_cancel.x;
		r.c.top = window->pos_cancel.y;
		r.c.right = r.c.left + window->sz_ms_cancel.h;
		r.c.bottom = r.c.top + window->sz_ms_cancel.v;
		err = cset_pos(window->ms_cancel_id, &r);
		if (err < 0) {
			DP_ER("cset_pos error cancel button", err);
		}
		r.c.left = window->pos_post.x;
		r.c.top = window->pos_post.y;
		r.c.right = r.c.left + window->sz_ms_post.h;
		r.c.bottom = r.c.top + window->sz_ms_post.v;
		err = cset_pos(window->ms_post_id, &r);
		if (err < 0) {
			DP_ER("cset_pos error post button", err);
		}

		wreq_dsp(window->wid);
	}
}

EXPORT cfrmwindow_t* cfrmwindow_new(RECT *r, WID parent, cfrmwindow_notifyclosecallback proc, VP arg, W dnum_title, W dnum_post, W dnum_cancel)
{
	cfrmwindow_t *window;
	W err;
	SWSEL *sw;

	window = (cfrmwindow_t*)malloc(sizeof(cfrmwindow_t));
	if (window == NULL) {
		return NULL;
	}
	window->wid = -1;
	window->gid = -1;
	window->parent = parent;
	window->ms_post_id = -1;
	window->ms_cancel_id = -1;
	window->r = *r;
	window->base = datwindow_new(-1, cfrmwindow_scroll, cfrmwindow_draw, cfrmwindow_resize, cfrmwindow_close, cfrmwindow_butdn, NULL, window);
	if (window->base == NULL) {
		free(window);
		return NULL;
	}
	window->post = NULL;
	window->notifyclosecallback = proc;
	window->arg = arg;

	err = dget_dtp(TEXT_DATA, dnum_title, (void**)&window->windowtitle);
	if (err < 0) {
		DP_ER("dget_dtp: message retrieving error", err);
		datwindow_delete(window->base);
		free(window);
		return NULL;
	}

	err = dget_dtp(PARTS_DATA, dnum_post, (void**)&sw);
	if (err < 0) {
		DP_ER("dget_dtp: message retrieving error", err);
		datwindow_delete(window->base);
		free(window);
		return NULL;
	}
	window->dnum_post = dnum_post;
	window->sz_ms_post.h = sw->r.c.right - sw->r.c.left;
	window->sz_ms_post.v = sw->r.c.bottom - sw->r.c.top;

	err = dget_dtp(PARTS_DATA, dnum_cancel, (void**)&sw);
	if (err < 0) {
		DP_ER("dget_dtp: message retrieving error", err);
		datwindow_delete(window->base);
		free(window);
		return NULL;
	}
	window->dnum_cancel = dnum_cancel;
	window->sz_ms_cancel.h = sw->r.c.right - sw->r.c.left;
	window->sz_ms_cancel.v = sw->r.c.bottom - sw->r.c.top;

	return window;
}

EXPORT VOID cfrmwindow_delete(cfrmwindow_t *window)
{
	if (window->wid > 0) {
		wcls_wnd(window->wid, CLR);
	}
	datwindow_delete(window->base);
	free(window);
}
