/*
 * confirm.c
 *
 * Copyright (c) 2009 project bchan
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
# define DP_ER(msg, err) printf("%s (%d/%z)\n", msg, err>>16, err)
#else
# define DP(arg) /**/
# define DP_ER(msg, err) /**/
#endif

struct cfrmwindow_t_ {
	WID wid;
	GID gid;
	PAID tw_id;
	RECT r;
	datwindow_t *base;
	postresdata_t *post;
	cfrmwindow_notifyclosecallback notifyclosecallback;
	VP arg;
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
	window->tw_id = -1;
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

	if (window->post == NULL) {
		return;
	}

	gid = window->gid;

	cfrmwindow_resetgenv(window);
	gdra_stp(gid, 0, 16, label_from, 4, G_STORE);
	tadlib_drawtext(window->post->from, window->post->from_len, gid, 0, 0);
	cfrmwindow_resetgenv(window);
	gdra_stp(gid, 0, 32, label_mail, 8, G_STORE);
	tadlib_drawtext(window->post->mail, window->post->mail_len, gid, 0, 0);
	cfrmwindow_resetgenv(window);
	gset_chp(gid, 0, 64, True);
	tadlib_drawtext(window->post->message, window->post->message_len, gid, 0, 0);

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
	W ret;

	ret = cchk_par(window->tw_id, wev->s.pos);
	if (ret <= 0) {
		return;
	}
	ret = cact_par(window->tw_id, wev);
	if ((ret & 0x5000) != 0x5000) {
		return;
	}
	cfrmwindow_close2(window);
	(*window->notifyclosecallback)(window->arg, 1);
}

LOCAL VOID cfrmwindow_close(VP arg)
{
	cfrmwindow_t *window = (cfrmwindow_t*)arg;

	cfrmwindow_close2(window);
	(*window->notifyclosecallback)(window->arg, 0);
}

LOCAL VOID cfrmwindow_calcdrawsize(cfrmwindow_t *window, SIZE *sz)
{
	SIZE sz_from = {0,0}, sz_mail = {0,0}, sz_msg = {0,0};
	TC label_from[] = {0x4C3E, 0x4130, TK_COLN, TK_KSP, TNULL};
	TC label_mail[] = {TK_E, 0x213E, TK_m, TK_a, TK_i, TK_l, TK_COLN, TK_KSP, TNULL};
	GID gid;
	W width_from, width_mail;

	sz->h = 0;
	sz->v = 0;

	if (window->post == NULL) {
		return;
	}

	gid = window->gid;

	cfrmwindow_resetgenv(window);
	width_from = gget_stw(gid, label_from, 4, NULL, NULL);
	if (width_from < 0) {
		return;
	}
	tadlib_calcdrawsize(window->post->from, window->post->from_len, gid, &sz_from);
	cfrmwindow_resetgenv(window);
	width_mail = gget_stw(gid, label_mail, 4, NULL, NULL);
	if (width_mail < 0) {
		return;
	}
	tadlib_calcdrawsize(window->post->mail, window->post->mail_len, gid, &sz_mail);
	cfrmwindow_resetgenv(window);
	tadlib_calcdrawsize(window->post->message, window->post->message_len, gid, &sz_msg);

	sz->v += sz_from.v;
	if (sz->h < sz_from.h + width_from) {
		sz->h = sz_from.h + width_from;
	}
	sz->v += sz_mail.v;
	if (sz->h < sz_mail.h + width_mail) {
		sz->h = sz_mail.h + width_mail;
	}
	sz->v += sz_msg.v;
	if (sz->h < sz_msg.v) {
		sz->h = sz_msg.v;
	}
	sz->v += 16 + 24;
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
	static	TC	tit0[] = {TK_T, TK_e, TK_s, TK_t, TNULL};
	static	TC	tit1[] = {MC_STR, TK_T, TK_e, TK_s, TK_t, TNULL};
	WID wid;
	RECT r = {{10, 112, 60, 136}};
	SIZE sz;

	if (window->wid > 0) {
		return 0;
	}

	wid = wopn_wnd(WA_SIZE|WA_HHDL|WA_VHDL|WA_BBAR|WA_RBAR, 0, &(window->r), NULL, 2, tit0, &pat0, NULL);
	if (wid < 0) {
		return wid;
	}
	window->wid = wid;
	window->gid = wget_gid(wid);
	datwindow_setwid(window->base, wid);

	cfrmwindow_calcdrawsize(window, &sz);
	datwindow_setdrawrect(window->base, 0, 0, sz.h, sz.v);
	datwindow_setworkrect(window->base, 0, 0, window->r.c.right, window->r.c.bottom);

	r.c.left = 10;
	r.c.top = sz.v - 24;
	r.c.right = 60;
	r.c.bottom = r.c.top + 24;
	window->tw_id = ccre_msw(wid, MS_PARTS|P_DCLICK, &r, tit1, NULL);
	if (window->tw_id < 0) {
		DP_ER("ccre_msw error:", window->tw_id);
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
	window->post = post;
	if (window->wid > 0) {
		wreq_dsp(window->wid);
	}
}

EXPORT cfrmwindow_t* cfrmwindow_new(RECT *r, cfrmwindow_notifyclosecallback proc, VP arg)
{
	cfrmwindow_t *window;

	window = (cfrmwindow_t*)malloc(sizeof(cfrmwindow_t));
	if (window == NULL) {
		return NULL;
	}
	window->wid = -1;
	window->gid = -1;
	window->tw_id = -1;
	window->r = *r;
	window->base = datwindow_new(-1, cfrmwindow_scroll, cfrmwindow_draw, cfrmwindow_resize, cfrmwindow_close, cfrmwindow_butdn, NULL, window);
	if (window->base == NULL) {
		free(window);
		return NULL;
	}
	window->post = NULL;
	window->notifyclosecallback = proc;
	window->arg = arg;

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
