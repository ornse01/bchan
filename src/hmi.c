/*
 * hmi.c
 *
 * Copyright (c) 2011 project bchan
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

#include    "hmi.h"
#include	"tadlib.h"
#include	"wordlist.h"

#include	<bstdio.h>
#include	<bstdlib.h>
#include	<tcode.h>
#include	<tstring.h>
#include	<btron/btron.h>
#include	<btron/hmi.h>
#include	<btron/vobj.h>

#ifdef BCHAN_CONFIG_DEBUG
# define DP(arg) printf arg
# define DP_ER(msg, err) printf("%s (%d/%x)\n", msg, err>>16, err)
#else
# define DP(arg) /**/
# define DP_ER(msg, err) /**/
#endif

typedef VOID (*windowscroll_scrollcalback)(VP arg, W dh, W dv);

struct windowscroll_t_ {
	PAID rbar;
	PAID bbar;
	windowscroll_scrollcalback scroll_callback;
	W draw_l,draw_t,draw_r,draw_b;
	W work_l,work_t,work_r,work_b;
	VP arg;
};
typedef struct windowscroll_t_ windowscroll_t;

LOCAL VOID windowscroll_callback_scroll(windowscroll_t *wscr, W dh, W dv)
{
	if (wscr->scroll_callback != NULL) {
		(*wscr->scroll_callback)(wscr->arg, -dh, -dv);
	}
}

LOCAL W windowscroll_updaterbar(windowscroll_t *wscr)
{
	W sbarval[4],err;

	sbarval[0] = wscr->work_t;
	sbarval[1] = wscr->work_b;
	sbarval[2] = 0;
	sbarval[3] = wscr->draw_b - wscr->draw_t;
	if (sbarval[1] > sbarval[3]) {
		sbarval[1] = sbarval[3];
	}
	if((err = cset_val(wscr->rbar, 4, sbarval)) < 0){
		DP(("windowscroll_updaterbar:cset_val rbar\n"));
		DP(("                       :%d %d %d %d\n", sbarval[0], sbarval[1], sbarval[2], sbarval[3]));
		return err;
	}

	return 0;
}

LOCAL W windowscroll_updatebbar(windowscroll_t *wscr)
{
	W sbarval[4],err;

	sbarval[0] = wscr->work_r;
	sbarval[1] = wscr->work_l;
	sbarval[2] = wscr->draw_r - wscr->draw_l;
	sbarval[3] = 0;
	if (sbarval[0] > sbarval[2]) {
		sbarval[0] = sbarval[2];
	}
	if((err = cset_val(wscr->bbar, 4, sbarval)) < 0){
		DP(("windowscroll_updatebbar:cset_val bbar\n"));
		DP(("                       :%d %d %d %d\n", sbarval[0], sbarval[1], sbarval[2], sbarval[3]));
		return err;
    }

	return 0;
}

EXPORT W windowscroll_updatebar(windowscroll_t *wscr)
{
	W err;

	err = windowscroll_updaterbar(wscr);
	if (err < 0) {
		return err;
	}

	err = windowscroll_updatebbar(wscr);
	if (err < 0) {
		return err;
	}

	return 0;
}

LOCAL W windowscroll_scrollbar(windowscroll_t *wscr, WEVENT *wev, PAID scrbarPAID)
{
	W i,err,barval[4];
	W *clo = barval,*chi = barval+1,*lo = barval+2,*hi = barval+3;
	RECT sbarRECT;
	W pressval,smoothscrolldiff;
	W dh,dv;
	WID wid;

	wid = wev->g.wid;

	for(;;) {
		if((i = cact_par(scrbarPAID, wev)) < 0) {
			DP_ER("cact_par error", i);
			return i;
		}
		cget_val(scrbarPAID, 4, barval);
		if((i & 0xc) == 0x8) { /*ジャンプ*/
			if (scrbarPAID == wscr->rbar) { /*右バー*/
				dh = 0;
				dv = -(*clo-wscr->work_t);
				wscr->work_t -= dv;
				wscr->work_b -= dv;
				windowscroll_callback_scroll(wscr, dh, dv);
			} else if (scrbarPAID == wscr->bbar) { /*下バー*/
				dh = -(*chi-wscr->work_l);
				dv = 0;
				wscr->work_l -= dh;
				wscr->work_r -= dh;
				windowscroll_callback_scroll(wscr, dh, dv);
			}
			if ((i & 0x6000) == 0x6000) {
				/*ジャンプ移動中の値の変更*/
				continue;
			}
			break;
		}
		switch(i) {
			/*スムーススクロール*/
			/*プレス位置とノブの位置に比例した速度でスクロール*/
		case 0x6000:	/*上へのスムーススクロールで中断*/
		case 0x6001:	/*下へのスムーススクロールで中断*/
			if ((err = cget_pos(scrbarPAID, &sbarRECT)) < 0) {
				continue;
			}
			pressval = (wev->s.pos.y - sbarRECT.c.top)*
				(*hi - *lo)/(sbarRECT.c.bottom-sbarRECT.c.top) + *lo;
			smoothscrolldiff = (pressval - (*chi+*clo)/2)/5;
			if (smoothscrolldiff == 0) {
				continue;
			}
			if ((*clo + smoothscrolldiff) < *lo) {
				if (*lo >= *clo) {
					continue;
				}
				dh = 0;
				dv = -(*lo - *clo);
			} else if ((*chi + smoothscrolldiff) > *hi) {
				if (*hi <= *chi) {
					continue;
				}
				dh = 0;
				dv = -(*hi - *chi);
			} else {
				dh = 0;
				dv = -smoothscrolldiff;
			}
			wscr->work_t -= dv;
			wscr->work_b -= dv;
			windowscroll_callback_scroll(wscr, dh, dv);
			windowscroll_updaterbar(wscr);
			continue;
		case 0x6002:	/*左へのスムーススクロールで中断*/
		case 0x6003:	/*右へのスムーススクロールで中断*/
			if ((err = cget_pos(scrbarPAID, &sbarRECT)) < 0) {
				continue;
			}
			pressval = (wev->s.pos.x - sbarRECT.c.left)*
				(*lo - *hi)/(sbarRECT.c.right-sbarRECT.c.left) + *hi;
			smoothscrolldiff = (pressval - (*clo+*chi)/2)/5;
			if (smoothscrolldiff == 0) {
				continue;
			}
			if ((*clo + smoothscrolldiff) > *lo) {
				if (*lo <= *clo) {
					continue;
				}
				dh = -(*lo - *clo);
				dv = 0;
			} else if ((*chi + smoothscrolldiff) < *hi) {
				if (*hi >= *chi) {
					continue;
				}
				dh = -(*hi - *chi);
				dv = 0;
			} else {
				dh = -smoothscrolldiff;
				dv = 0;
			}
			wscr->work_l -= dh;
			wscr->work_r -= dh;
			windowscroll_callback_scroll(wscr, dh, dv);
			windowscroll_updatebbar(wscr);
			continue;
		case 0x5004:	/*上へのエリアスクロールで終了*/
			if ((wscr->work_t - (*chi-*clo)) < *lo) {
				dh = 0;
				dv = -(*lo - wscr->work_t);
			} else {
				dh = 0;
				dv = (*chi - *clo);
			}
			wscr->work_t -= dv;
			wscr->work_b -= dv;
			windowscroll_callback_scroll(wscr, dh, dv);
			windowscroll_updaterbar(wscr);
			break;
		case 0x5005:	/*下へのエリアスクロールで終了*/
			if((wscr->work_b + (*chi-*clo)) > *hi){
				dh = 0;
				dv = -(*hi - wscr->work_b);
			} else {
				dh = 0;
				dv = -(*chi - *clo);
			}
			wscr->work_t -= dv;
			wscr->work_b -= dv;
			windowscroll_callback_scroll(wscr, dh, dv);
			windowscroll_updaterbar(wscr);
			break;
		case 0x5006:	/*左へのエリアスクロールで終了*/
			if((wscr->work_l - (*clo-*chi)) < *hi){
				dh = -(*hi - wscr->work_l);
				dv = 0;
			} else {
				dh = *clo - *chi;
				dv = 0;
			}
			wscr->work_l -= dh;
			wscr->work_r -= dh;
			windowscroll_callback_scroll(wscr, dh, dv);
			windowscroll_updatebbar(wscr);
			break;
		case 0x5007:	/*右へのエリアスクロールで終了*/
			if((wscr->work_r + (*clo-*chi)) > *lo){
				dh = -(*lo - wscr->work_r);
				dv = 0;
			} else {
				dh = -(*clo - *chi);
				dv = 0;
			}
			wscr->work_l -= dh;
			wscr->work_r -= dh;
			windowscroll_callback_scroll(wscr, dh, dv);
			windowscroll_updatebbar(wscr);
			break;
		}
		break;
	}

	return 0;
}

EXPORT VOID windowscroll_scrollbyvalue(windowscroll_t *wscr, W dh, W dv)
{
	wscr->work_l += dh;
	wscr->work_t += dv;
	wscr->work_r += dh;
	wscr->work_b += dv;
	windowscroll_callback_scroll(wscr, -dh, -dv);
	windowscroll_updatebar(wscr);
}

LOCAL W windowscroll_weventrbar(windowscroll_t *wscr, WEVENT *wev)
{
	return windowscroll_scrollbar(wscr, wev, wscr->rbar);
}

LOCAL W windowscroll_weventbbar(windowscroll_t *wscr, WEVENT *wev)
{
	return windowscroll_scrollbar(wscr, wev, wscr->bbar);
}

LOCAL W windowscroll_setworkrect(windowscroll_t *wscr, W l, W t, W r, W b)
{
	wscr->work_l = l;
	wscr->work_t = t;
	wscr->work_r = r;
	wscr->work_b = b;
	return windowscroll_updatebar(wscr);
}

LOCAL W windowscroll_setdrawrect(windowscroll_t *wscr, W l, W t, W r, W b)
{
	wscr->draw_l = l;
	wscr->draw_t = t;
	wscr->draw_r = r;
	wscr->draw_b = b;
	return windowscroll_updatebar(wscr);
}

LOCAL VOID windowscroll_settarget(windowscroll_t *wscr, WID target)
{
	if (target < 0) {
		wscr->rbar = -1;
		wscr->bbar = -1;
	} else {
		wget_bar(target, &(wscr->rbar), &(wscr->bbar), NULL);
		cchg_par(wscr->rbar, P_NORMAL|P_NOFRAME|P_ENABLE|P_ACT|P_DRAGBREAK);
		cchg_par(wscr->bbar, P_NORMAL|P_NOFRAME|P_ENABLE|P_ACT|P_DRAGBREAK);
	}
}

LOCAL W windowscroll_initialize(windowscroll_t *wscr, WID target, windowscroll_scrollcalback scrollcallback, VP arg)
{
	windowscroll_settarget(wscr, target);
	wscr->scroll_callback = scrollcallback;
	wscr->arg = arg;
	wscr->work_l = 0;
	wscr->work_t = 0;
	wscr->work_r = 0;
	wscr->work_b = 0;
	wscr->draw_l = 0;
	wscr->draw_t = 0;
	wscr->draw_r = 0;
	wscr->draw_b = 0;

	return 0;
}

LOCAL VOID windowscroll_finalize(windowscroll_t *wscr)
{
}

struct datwindow_t_ {
	WID wid;
	GID gid;
	datwindow_scrollcalback scroll_callback;
	windowscroll_t wscr;
	VP arg;
	WEVENT savedwev;
};

struct cfrmwindow_t_ {
	WID wid;
	GID gid;
	WID parent;
	PAID ms_post_id;
	PAID ms_cancel_id;
	RECT r;
	windowscroll_t wscr;
	postresdata_t *post;
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

struct ngwordwindow_t_ {
	WID wid;
	GID gid;
	WID parent;
	RECT r;
	PAT bgpat;
	W dnum_list;
	W dnum_delete;
	W dnum_input;
	W dnum_append;
	PAID ss_list_id;
	PAID ms_delete_id;
	PAID tb_input_id;
	PAID ms_append_id;
	WEVENT savedwev;
	wordlist_t wordlist;
	TC strbuf[128];
	TC *selector;
};

#define DATHMI_FLAG_SWITCHBUTDN 0x00000001

struct dathmi_t_ {
	WEVENT wev;
	dathmievent_t evt;
	UW flag;
	datwindow_t *mainwindow;
	cfrmwindow_t *cfrmwindow;
	ngwordwindow_t *ngwordwindow;
};

EXPORT W datwindow_startredisp(datwindow_t *window, RECT *r)
{
	return wsta_dsp(window->wid, r, NULL);
}

EXPORT W datwindow_endredisp(datwindow_t *window)
{
	return wend_dsp(window->wid);
}

EXPORT W datwindow_eraseworkarea(datwindow_t *window, RECT *r)
{
	return wera_wnd(window->wid, r);
}

EXPORT W datwindow_scrollworkarea(datwindow_t *window, W dh, W dv)
{
	return wscr_wnd(window->wid, NULL, dh, dv, W_MOVE|W_RDSET);
}

EXPORT VOID datwindow_scrollbyvalue(datwindow_t *window, W dh, W dv)
{
	windowscroll_scrollbyvalue(&window->wscr, dh, dv);
}

EXPORT W datwindow_requestredisp(datwindow_t *window)
{
	return wreq_dsp(window->wid);
}

EXPORT GID datwindow_startdrag(datwindow_t *window)
{
	return wsta_drg(window->wid, 0);
}

EXPORT W datwindow_getdrag(datwindow_t *window, PNT *pos, WID *wid, PNT *pos_butup)
{
	W etype;

	etype = wget_drg(pos, &window->savedwev);
	*wid = window->savedwev.s.wid;
	if (etype == EV_BUTUP) {
		*pos_butup = window->savedwev.s.pos;
	}

	return etype;
}

EXPORT VOID datwindow_enddrag(datwindow_t *window)
{
	wend_drg();
}

EXPORT VOID datwindow_responsepasterequest(datwindow_t *window, W nak, PNT *pos)
{
	if (pos != NULL) {
		window->savedwev.r.r.p.rightbot.x = pos->x;
		window->savedwev.r.r.p.rightbot.y = pos->y;
	}
	wrsp_evt(&window->savedwev, nak);
}

EXPORT W datwindow_getworkrect(datwindow_t *window, RECT *r)
{
	return wget_wrk(window->wid, r);
}

EXPORT GID datwindow_getGID(datwindow_t *window)
{
	return wget_gid(window->wid);
}

EXPORT WID datwindow_getWID(datwindow_t *window)
{
	return window->wid;
}

EXPORT W datwindow_settitle(datwindow_t *window, TC *title)
{
	return wset_tit(window->wid, -1, title, 0);
}

EXPORT W datwindow_setdrawrect(datwindow_t *window, W l, W t, W r, W b)
{
	return windowscroll_setdrawrect(&window->wscr, l, t, r, b);
}

EXPORT W datwindow_setworkrect(datwindow_t *window, W l, W t, W r, W b)
{
	return windowscroll_setworkrect(&window->wscr, l, t, r, b);
}

LOCAL VOID dathmi_setswitchbutdnflag(dathmi_t *hmi)
{
	hmi->flag = hmi->flag | DATHMI_FLAG_SWITCHBUTDN;
}

LOCAL VOID dathmi_clearswitchbutdnflag(dathmi_t *hmi)
{
	hmi->flag = hmi->flag & ~DATHMI_FLAG_SWITCHBUTDN;
}

LOCAL Bool dathmi_issetswitchbutdnflag(dathmi_t *hmi)
{
	if ((hmi->flag & DATHMI_FLAG_SWITCHBUTDN) == 0) {
		return False;
	}
	return True;
}

LOCAL WID dathmi_getmainWID(dathmi_t *hmi)
{
	if (hmi->mainwindow == NULL) {
		return -1;
	}
	return hmi->mainwindow->wid;
}

LOCAL WID dathmi_getcfrmWID(dathmi_t *hmi)
{
	if (hmi->cfrmwindow == NULL) {
		return -1;
	}
	return hmi->cfrmwindow->wid;
}

LOCAL WID dathmi_getngwordWID(dathmi_t *hmi)
{
	if (hmi->ngwordwindow == NULL) {
		return -1;
	}
	return hmi->ngwordwindow->wid;
}

/* TODO: same as layoutstyle_resetgenvfont */
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
	windowscroll_settarget(&window->wscr, wid);

	cfrmwindow_calclayout(window, &sz);
	windowscroll_setdrawrect(&window->wscr, 0, 0, sz.h, sz.v);
	windowscroll_setworkrect(&window->wscr, 0, 0, window->r.c.right, window->r.c.bottom);

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

LOCAL W ngwordwindow_writeswitchselecterstring(ngwordwindow_t *window, TC *str)
{
	wordlist_iterator_t iter;
	Bool cont;
	TC *nodestr;
	W size = 0, nodelen;

	wordlist_iterator_initialize(&iter, &window->wordlist);
	if (str == NULL) {
		for (;;) {
			cont = wordlist_iterator_next(&iter, &nodestr, &nodelen);
			if (cont == False) {
				break;
			}
			size += 1; /* MC_STR */
			size += nodelen;
		}
		if (size == 0) {
			size = 1; /* MC_STR */
		}
		size++; /* TNULL */
	} else {
		for (;;) {
			cont = wordlist_iterator_next(&iter, &nodestr, &nodelen);
			if (cont == False) {
				break;
			}

			str[size++] = MC_STR;
			memcpy(str + size, nodestr, nodelen * sizeof(TC));
			size += nodelen;
		}
		if (size == 0) {
			str[size++] = MC_STR;
		}
		str[size++] = TNULL;
	}

	wordlist_iterator_finalize(&iter);
	return size + 1;
}

LOCAL W ngwordwindow_updatescrollselector(ngwordwindow_t *window)
{
	W len;

	len = ngwordwindow_writeswitchselecterstring(window, NULL);
	window->selector = realloc(window->selector, len*sizeof(TC));
	if (window->selector == NULL) {
		return -1; /* TODO */
	}
	ngwordwindow_writeswitchselecterstring(window, window->selector);
	cset_dat(window->ss_list_id, (W)window->selector);

	return 0;
}

EXPORT W ngwordwindow_open(ngwordwindow_t *window)
{
	TC test[] = {TK_N, TK_G, 0x256F, 0x213C, 0x2549, 0x306C, 0x4D77,TNULL};
	WID wid;
	PNT pos;

	if (window->wid > 0) {
		return 0;
	}

	wid = wopn_wnd(WA_SUBW, window->parent, &(window->r), NULL, 2, test, &window->bgpat, NULL);
	if (wid < 0) {
		DP_ER("wopn_wnd: ngword error", wid);
		return wid;
	}
	window->wid = wid;
	window->gid = wget_gid(wid);

	pos.x = 0;
	pos.y = 0;
	window->ss_list_id = copn_par(wid, window->dnum_list, &pos);
	if (window->ss_list_id < 0) {
		DP_ER("copn_par list error:", window->ss_list_id);
	}
	pos.x = 0;
	pos.y = 100;
	window->ms_delete_id = copn_par(wid, window->dnum_delete, &pos);
	if (window->ms_delete_id < 0) {
		DP_ER("copn_par delete error:", window->ms_delete_id);
	}
	pos.x = 0;
	pos.y = 130;
	window->tb_input_id = copn_par(wid, window->dnum_input, &pos);
	if (window->tb_input_id < 0) {
		DP_ER("copn_par input error:", window->tb_input_id);
	}
	pos.x = 0;
	pos.y = 160;
	window->ms_append_id = copn_par(wid, window->dnum_append, &pos);
	if (window->ms_append_id < 0) {
		DP_ER("copn_par append error:", window->ms_append_id);
	}

	ngwordwindow_updatescrollselector(window);

	wreq_dsp(wid);

	return 0;
}

LOCAL Bool ngwordwindow_searchwordbyindex(ngwordwindow_t *window, W index, TC **str, W *len)
{
	return wordlist_searchwordbyindex(&window->wordlist, index, str, len);
}

EXPORT W ngwordwindow_appendword(ngwordwindow_t *window, TC *str, W len)
{
	W err;

	err = wordlist_appendword(&window->wordlist, str, len);
	if (err < 0) {
		return err;
	}
	return ngwordwindow_updatescrollselector(window);
}

EXPORT W ngwordwindow_removeword(ngwordwindow_t *window, TC *str, W len)
{
	Bool removed;

	removed = wordlist_removeword(&window->wordlist, str, len);
	if (removed == True) {
		return ngwordwindow_updatescrollselector(window);
	}
	return 0;
}

EXPORT Bool ngwordwindow_isopen(ngwordwindow_t *window)
{
	if (window->wid < 0) {
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
		windowscroll_setdrawrect(&window->wscr, 0, 0, sz.h, sz.v);

		work.c.left = 0;
		work.c.top = 0;
		wset_wrk(window->wid, &work);
		windowscroll_setworkrect(&window->wscr, work.c.left, work.c.top, work.c.right, work.c.bottom);

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

LOCAL VOID cfrmwindow_draw(cfrmwindow_t *window, RECT *r)
{
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

LOCAL VOID cfrmwindow_redisp(cfrmwindow_t *window)
{
	RECT r;
	do {
		if (wsta_dsp(window->wid, &r, NULL) == 0) {
			break;
		}
		wera_wnd(window->wid, &r);
		cfrmwindow_draw(window, &r);
	} while (wend_dsp(window->wid) > 0);
}

LOCAL VOID cfrmwindow_close2(cfrmwindow_t *window)
{
	WDSTAT stat;
	W err;

	windowscroll_settarget(&window->wscr, -1);
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

LOCAL VOID cfrmwindow_close(cfrmwindow_t *window, dathmievent_t *evt)
{
	cfrmwindow_close2(window);
	evt->type = DATHMIEVENT_TYPE_CONFIRM_CLOSE;
	evt->data.confirm_close.send = False;
}

EXPORT VOID ngwordwindow_close(ngwordwindow_t *window)
{
	WDSTAT stat;
	W err;

	if (window->wid < 0) {
		return;
	}

	stat.attr = WA_STD;
	err = wget_sts(window->wid, &stat, NULL);
	if (err >= 0) {
		window->r = stat.r;
	}
	cdel_pwd(window->wid, NOCLR);
	wcls_wnd(window->wid, CLR);
	window->wid = -1;
	window->gid = -1;
}

LOCAL VOID ngwordwindow_draw(ngwordwindow_t *window, RECT *r)
{
	cdsp_pwd(window->wid, r, P_RDISP);
}

LOCAL VOID ngwordwindow_redisp(ngwordwindow_t *window)
{
	RECT r;
	do {
		if (wsta_dsp(window->wid, &r, NULL) == 0) {
			break;
		}
		wera_wnd(window->wid, &r);
		ngwordwindow_draw(window, &r);
	} while (wend_dsp(window->wid) > 0);
}

LOCAL VOID dathmi_weventrequest(dathmi_t *hmi, WEVENT *wev, dathmievent_t *evt)
{
	WID main_wid, cfrm_wid, ngword_wid;

	main_wid = dathmi_getmainWID(hmi);
	cfrm_wid = dathmi_getcfrmWID(hmi);
	ngword_wid = dathmi_getngwordWID(hmi);

	switch (wev->g.cmd) {
	case	W_REDISP:	/*再表示要求*/
		if (wev->g.wid == main_wid) {
			evt->type = DATHMIEVENT_TYPE_THREAD_DRAW;
		} else if (wev->g.wid == cfrm_wid) {
			cfrmwindow_redisp(hmi->cfrmwindow);
		} else if (wev->g.wid == ngword_wid) {
			ngwordwindow_redisp(hmi->ngwordwindow);
		}
		break;
	case	W_PASTE:	/*貼込み要求*/
		if (wev->g.wid == main_wid) {
			evt->type = DATHMIEVENT_TYPE_THREAD_PASTE;
			memcpy(&hmi->mainwindow->savedwev, wev, sizeof(WEVENT));
		} else if (wev->g.wid == cfrm_wid) {
			wrsp_evt(wev, 1); /*NACK*/
		} else if (wev->g.wid == ngword_wid) {
			wrsp_evt(wev, 1); /*NACK*/
		}
		break;
	case	W_DELETE:	/*保存終了*/
		wrsp_evt(wev, 0);	/*ACK*/
		if (wev->g.wid == main_wid) {
			evt->type = DATHMIEVENT_TYPE_THREAD_CLOSE;
			evt->data.main_close.save = True;
		} else if (wev->g.wid == cfrm_wid) {
			cfrmwindow_close(hmi->cfrmwindow, evt);
		} else if (wev->g.wid == ngword_wid) {
			ngwordwindow_close(hmi->ngwordwindow);
			evt->type = DATHMIEVENT_TYPE_NGWORD_CLOSE;
		}
		break;
	case	W_FINISH:	/*廃棄終了*/
		wrsp_evt(wev, 0);	/*ACK*/
		if (wev->g.wid == main_wid) {
			evt->type = DATHMIEVENT_TYPE_THREAD_CLOSE;
			evt->data.main_close.save = False;
		} else if (wev->g.wid == cfrm_wid) {
			cfrmwindow_close(hmi->cfrmwindow, evt);
		} else if (wev->g.wid == ngword_wid) {
			ngwordwindow_close(hmi->ngwordwindow);
			evt->type = DATHMIEVENT_TYPE_NGWORD_CLOSE;
		}
		break;
	}
}

LOCAL VOID cfrmwindow_resize(cfrmwindow_t *window)
{
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

	windowscroll_setworkrect(&window->wscr, work.c.left, work.c.top, work.c.right, work.c.bottom);

	if (workchange == True) {
		wera_wnd(window->wid, NULL);
		wreq_dsp(window->wid);
	}
}

LOCAL VOID datwindow_resize(datwindow_t *window, SIZE *sz)
{
	RECT work;
	Bool workchange = False;

	wget_wrk(window->wid, &work);
	if (work.c.left != 0) {
		work.c.left = 0;
		workchange = True;
	}
	if (work.c.top != 0) {
		work.c.top = 0;
		workchange = True;
	}
	wset_wrk(window->wid, &work);
	gset_vis(window->gid, work);

	if (workchange == True) {
		wera_wnd(window->wid, NULL);
		wreq_dsp(window->wid);
	}

	sz->v = work.c.bottom - work.c.top;
	sz->h = work.c.right - work.c.left;
}

LOCAL VOID cfrmwindow_butdnwork(cfrmwindow_t *window, WEVENT *wev, dathmievent_t *evt)
{
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
		evt->type = DATHMIEVENT_TYPE_CONFIRM_CLOSE;
		evt->data.confirm_close.send = True;
		return;
	}
	if (id == window->ms_cancel_id) {
		ret = cact_par(window->ms_cancel_id, wev);
		if ((ret & 0x5000) != 0x5000) {
			return;
		}
		cfrmwindow_close2(window);
		evt->type = DATHMIEVENT_TYPE_CONFIRM_CLOSE;
		evt->data.confirm_close.send = False;
		return;
	}
}

EXPORT W ngwordwindow_starttextboxaction(ngwordwindow_t *window)
{
	return 0;
}

EXPORT W ngwordwindow_gettextboxaction(ngwordwindow_t *window, TC *key, TC **val, W *len)
{
	W ret, len0;
	WEVENT *wev;

	wev = &window->savedwev;

	for (;;) {
		ret = cact_par(window->tb_input_id, wev);
		if (ret < 0) {
			DP_ER("cact_par tb_input_id error:", ret);
			return ret;
		}
		switch (ret & 0xefff) {
		case P_EVENT:
			switch (wev->s.type) {
			case EV_INACT:
			case EV_REQUEST:
				wugt_evt(wev);
				return NGWORDWINDOW_GETTEXTBOXACTION_FINISH;
			case EV_DEVICE:
				oprc_dev(&wev->e, NULL, 0);
				break;
			}
			wev->s.type = EV_NULL;
			continue;
		case P_MENU:
			wev->s.type = EV_NULL;
			if ((wev->s.type == EV_KEYDWN)&&(wev->s.stat & ES_CMD)) {
				*key = wev->e.data.key.code;
				return NGWORDWINDOW_GETTEXTBOXACTION_KEYMENU;
			}
			return NGWORDWINDOW_GETTEXTBOXACTION_MENU;
		case (0x4000|P_MOVE):
			return NGWORDWINDOW_GETTEXTBOXACTION_MOVE;
		case (0x4000|P_COPY):
			return NGWORDWINDOW_GETTEXTBOXACTION_COPY;
		case (0x4000|P_NL):
			len0 = cget_val(window->tb_input_id, 128, (W*)window->strbuf);
			if (len0 <= 0) {
				return NGWORDWINDOW_GETTEXTBOXACTION_FINISH;
			}
			*val = window->strbuf;
			*len = len0;
			ret = cset_val(window->tb_input_id, 0, NULL);
			if (ret < 0) {
				DP_ER("cset_val tb_input_id error", ret);
				return ret;
			}
			return NGWORDWINDOW_GETTEXTBOXACTION_APPEND;
		case (0x4000|P_TAB):
			return NGWORDWINDOW_GETTEXTBOXACTION_FINISH;
		case (0x4000|P_BUT):
			wugt_evt(wev);
			return NGWORDWINDOW_GETTEXTBOXACTION_FINISH;
		default:
			return NGWORDWINDOW_GETTEXTBOXACTION_FINISH;
		}
	}

	return NGWORDWINDOW_GETTEXTBOXACTION_FINISH;
}

EXPORT W ngwordwindow_endtextboxaction(ngwordwindow_t *window)
{
	return 0;
}

LOCAL VOID ngwordwindow_butdnwork(ngwordwindow_t *window, WEVENT *wev, dathmievent_t *evt)
{
	PAID id;
	W ret, len, val, err;
	Bool found;
	TC *str;

	ret = cfnd_par(window->wid, wev->s.pos, &id);
	if (ret <= 0) {
		return;
	}
	if (id == window->ss_list_id) {
		cact_par(window->ss_list_id, wev);
	}
	if (id == window->ms_delete_id) {
		ret = cact_par(window->ms_delete_id, wev);
		if ((ret & 0x5000) != 0x5000) {
			return;
		}
		err = cget_val(window->ss_list_id, 1, (W*)&val);
		if (err < 0) {
			DP_ER("cget_val ss_list_id error:", err);
			return;
		}
		if (val < 0) {
			return;
		}
		found = ngwordwindow_searchwordbyindex(window, val - 1, &str, &len);
		if (found == False) {
			DP(("not found in ngwordwindow parts\n"));
			return;
		}
		memcpy(window->strbuf, str, len * sizeof(TC));
		evt->type = DATHMIEVENT_TYPE_NGWORD_REMOVE;
		evt->data.ngword_append.str = window->strbuf;
		evt->data.ngword_append.len = len;
		return;
	}
	if (id == window->tb_input_id) {
		memcpy(&window->savedwev, wev, sizeof(WEVENT));
		evt->type = DATHMIEVENT_TYPE_NGWORD_TEXTBOX;
		return;
	}
	if (id == window->ms_append_id) {
		ret = cact_par(window->ms_append_id, wev);
		if ((ret & 0x5000) != 0x5000) {
			return;
		}
		len = cget_val(window->tb_input_id, 128, (W*)window->strbuf);
		if (len <= 0) {
			return;
		}
		evt->type = DATHMIEVENT_TYPE_NGWORD_APPEND;
		evt->data.ngword_append.str = window->strbuf;
		evt->data.ngword_append.len = len;
		len = cset_val(window->tb_input_id, 0, NULL);
		if (len < 0) {
			DP_ER("cset_val tb_input_id error", len);
		}
		return;
	}
}

LOCAL VOID dathmi_weventbutdn(dathmi_t *hmi, WEVENT *wev, dathmievent_t *evt)
{
	W i;
	WID main_wid, cfrm_wid, ngword_wid;

	main_wid = dathmi_getmainWID(hmi);
	cfrm_wid = dathmi_getcfrmWID(hmi);
	ngword_wid = dathmi_getngwordWID(hmi);

	switch	(wev->s.cmd) {
	case	W_PICT:
		switch (wchk_dck(wev->s.time)) {
		case	W_DCLICK:
			if (wev->s.wid == main_wid) {
				evt->type = DATHMIEVENT_TYPE_THREAD_CLOSE;
				evt->data.main_close.save = True; /* TODO: tmp value. */
			} else if (wev->s.wid == cfrm_wid) {
				cfrmwindow_close2(hmi->cfrmwindow);
				evt->type = DATHMIEVENT_TYPE_CONFIRM_CLOSE;
				evt->data.confirm_close.send = False;
			} else if (wev->s.wid == ngword_wid) {
				ngwordwindow_close(hmi->ngwordwindow);
				evt->type = DATHMIEVENT_TYPE_NGWORD_CLOSE;
			}
			return;
		case	W_PRESS:
			break;
		default:
			return;
		}
	case	W_FRAM:
	case	W_TITL:
		if (wmov_drg(wev, NULL) > 0) {
			if (wev->s.wid == main_wid) {
				evt->type = DATHMIEVENT_TYPE_THREAD_DRAW;
			} else if (wev->s.wid == cfrm_wid) {
				cfrmwindow_redisp(hmi->cfrmwindow);
			} else if (wev->s.wid == ngword_wid) {
				ngwordwindow_redisp(hmi->ngwordwindow);
			}
		}
		return;
	case	W_LTHD:
	case	W_RTHD:
	case	W_LBHD:
	case	W_RBHD:
		switch (wchk_dck(wev->s.time)) {
		case	W_DCLICK:
			i = wchg_wnd(wev->s.wid, NULL, W_MOVE);
			break;
		case	W_PRESS:
			i = wrsz_drg(wev, NULL, NULL);
			break;
		default:
			return;
		}

		if (wev->s.wid == main_wid) {
			evt->type = DATHMIEVENT_TYPE_THREAD_RESIZE;
			datwindow_resize(hmi->mainwindow, &evt->data.main_resize.work_sz);
			windowscroll_updatebar(&hmi->mainwindow->wscr);
			if (i > 0) {
				//evt->type = DATHMIEVENT_TYPE_THREAD_DRAW;
				/* TODO: queueing */
				evt->data.main_resize.needdraw = True;
			} else {
				evt->data.main_resize.needdraw = False;
			}
		} else if (wev->s.wid == cfrm_wid) {
			cfrmwindow_resize(hmi->cfrmwindow);
			windowscroll_updatebar(&hmi->cfrmwindow->wscr);
			if (i > 0) {
				cfrmwindow_redisp(hmi->cfrmwindow);
			}
		}
		/* ngword window need not do nothing. */
		return;
	case	W_RBAR:
		if (wev->s.wid == main_wid) {
			windowscroll_weventrbar(&hmi->mainwindow->wscr, wev);
		} else if (wev->s.wid == cfrm_wid) {
			windowscroll_weventrbar(&hmi->cfrmwindow->wscr, wev);
		}
		/* ngword window need not do nothing. */
		return;
	case	W_BBAR:
		if (wev->s.wid == main_wid) {
			windowscroll_weventbbar(&hmi->mainwindow->wscr, wev);
		} else if (wev->s.wid == cfrm_wid) {
			windowscroll_weventbbar(&hmi->cfrmwindow->wscr, wev);
		}
		/* ngword window need not do nothing. */
		return;
	case	W_WORK:
		if (wev->s.wid == main_wid) {
			evt->type = DATHMIEVENT_TYPE_THREAD_BUTDN;
			evt->data.main_butdn.type = wchk_dck(wev->s.time);
			evt->data.main_butdn.pos = wev->s.pos;
			memcpy(&hmi->mainwindow->savedwev, wev, sizeof(WEVENT));
		} else if (wev->s.wid == cfrm_wid) {
			cfrmwindow_butdnwork(hmi->cfrmwindow, wev, evt);
		} else if (wev->s.wid == ngword_wid) {
			ngwordwindow_butdnwork(hmi->ngwordwindow, wev, evt);
		}
		return;
	}

	return;
}

LOCAL VOID dathmi_weventswitch(dathmi_t *hmi, WEVENT *wev, dathmievent_t *evt)
{
	evt->type = DATHMIEVENT_TYPE_THREAD_SWITCH;
	evt->data.main_switch.needdraw = False;
	dathmi_setswitchbutdnflag(hmi);
}

LOCAL VOID dathmi_weventreswitch(dathmi_t *hmi, WEVENT *wev, dathmievent_t *evt)
{
	WID main_wid, cfrm_wid, ngword_wid;

	main_wid = dathmi_getmainWID(hmi);
	cfrm_wid = dathmi_getcfrmWID(hmi);
	ngword_wid = dathmi_getngwordWID(hmi);

	if (wev->s.wid == main_wid) {
		evt->type = DATHMIEVENT_TYPE_THREAD_SWITCH;
		evt->data.main_switch.needdraw = True;
	} else if (wev->s.wid == cfrm_wid) {
		cfrmwindow_redisp(hmi->cfrmwindow);
	} else if (wev->s.wid == ngword_wid) {
		ngwordwindow_redisp(hmi->ngwordwindow);
	}
	dathmi_setswitchbutdnflag(hmi);
}

LOCAL VOID dathmi_receivemessage(dathmi_t *hmi, dathmievent_t *evt)
{
	MESSAGE msg;
	W err;

    err = rcv_msg(MM_ALL, &msg, sizeof(MESSAGE), WAIT|NOCLR);
	if (err >= 0) {
		if (msg.msg_type == MS_TMOUT) { /* should be use other type? */
			evt->type = DATHMIEVENT_TYPE_COMMON_TIMEOUT;
			evt->data.common_timeout.code = msg.msg_body.TMOUT.code;
		}
	}
	clr_msg(MM_ALL, MM_ALL);
}

EXPORT W dathmi_getevent(dathmi_t *hmi, dathmievent_t **evt)
{
	WEVENT	*wev0;
	WID main_wid, cfrm_wid, ngword_wid;
	Bool open, ok;

	main_wid = dathmi_getmainWID(hmi);
	cfrm_wid = dathmi_getcfrmWID(hmi);
	ngword_wid = dathmi_getngwordWID(hmi);

	hmi->evt.type = DATHMIEVENT_TYPE_NONE;
	wev0 = &hmi->wev;

	ok = dathmi_issetswitchbutdnflag(hmi);
	if (ok == True) {
		dathmi_weventbutdn(hmi, wev0, &hmi->evt);
		dathmi_clearswitchbutdnflag(hmi);
		return 0;
	}

	wget_evt(wev0, WAIT);
	switch (wev0->s.type) {
	case	EV_NULL:
		cidl_par(wev0->s.wid, &wev0->s.pos);
		if ((wev0->s.wid != main_wid)&&(wev0->s.wid != cfrm_wid)&&(wev0->s.wid != ngword_wid)) {
			hmi->evt.type = DATHMIEVENT_TYPE_COMMON_MOUSEMOVE;
			hmi->evt.data.common_mousemove.pos = wev0->s.pos;
			break;		/*ウィンドウ外*/
		}
		if (wev0->s.cmd != W_WORK)
			break;		/*作業領域外*/
		if (wev0->s.stat & ES_CMD)
			break;	/*命令キーが押されている*/
		if (wev0->s.wid == main_wid) {
			hmi->evt.type = DATHMIEVENT_TYPE_THREAD_MOUSEMOVE;
			hmi->evt.data.main_mousemove.pos = wev0->s.pos;
			hmi->evt.data.main_mousemove.stat = wev0->s.stat;
		}
		break;
	case	EV_REQUEST:
		dathmi_weventrequest(hmi, wev0, &hmi->evt);
		break;
	case	EV_RSWITCH:
		dathmi_weventreswitch(hmi, wev0, &hmi->evt);
		break;
	case	EV_SWITCH:
		dathmi_weventswitch(hmi, wev0, &hmi->evt);
		break;
	case	EV_BUTDWN:
		dathmi_weventbutdn(hmi, wev0, &hmi->evt);
		break;
	case	EV_KEYDWN:
	case	EV_AUTKEY:
		open = ngwordwindow_isopen(hmi->ngwordwindow);
		if (open == True) {
			memcpy(&hmi->ngwordwindow->savedwev, wev0, sizeof(WEVENT));
			hmi->evt.type = DATHMIEVENT_TYPE_NGWORD_TEXTBOX;
			break;
		}
		hmi->evt.type = DATHMIEVENT_TYPE_COMMON_KEYDOWN;
		hmi->evt.data.common_keydown.keycode = wev0->e.data.key.code;
		hmi->evt.data.common_keydown.keytop = wev0->e.data.key.keytop;
		hmi->evt.data.common_keydown.stat = wev0->e.stat;
		break;
	case	EV_INACT:
		pdsp_msg(NULL);
		break;
	case	EV_DEVICE:
		oprc_dev(&wev0->e, NULL, 0);
		break;
	case	EV_MSG:
		dathmi_receivemessage(hmi, &hmi->evt);
		break;
	case	EV_MENU:
		hmi->evt.type = DATHMIEVENT_TYPE_COMMON_MENU;
		hmi->evt.data.common_menu.pos = wev0->s.pos;
		break;
	}

	*evt = &hmi->evt;

	return 0;
}

LOCAL VOID datwindow_scroll(VP arg, W dh, W dv)
{
	datwindow_t *window = (datwindow_t*)arg;
	(*window->scroll_callback)(window->arg, dh, dv);
}

LOCAL datwindow_t* datwindow_new(RECT *r, TC *title, PAT *bgpat, datwindow_scrollcalback scrollcallback, VP arg)
{
	datwindow_t* window;
	W err;

	window = malloc(sizeof(datwindow_t));
	if (window == NULL) {
		return NULL;
	}

	window->wid = wopn_wnd(WA_SIZE|WA_HHDL|WA_VHDL|WA_BBAR|WA_RBAR, 0, r, NULL, 1, title, bgpat, NULL);
	if (window->wid < 0) {
		free(window);
		return NULL;
	}
	err = windowscroll_initialize(&window->wscr, window->wid, datwindow_scroll, (VP)window);
	if (err < 0) {
		wcls_wnd(window->wid, CLR);
		free(window);
		return NULL;
	}
	window->gid = wget_gid(window->wid);
	window->scroll_callback = scrollcallback;
	window->arg = arg;

	return window;
}

LOCAL VOID datwindow_delete(datwindow_t *window)
{
	wcls_wnd(window->wid, CLR);
	windowscroll_finalize(&window->wscr);
	free(window);
}

EXPORT datwindow_t* dathmi_newmainwindow(dathmi_t *hmi, RECT *r, TC *title, PAT *bgpat, datwindow_scrollcalback scrollcallback, VP arg)
{
	hmi->mainwindow = datwindow_new(r, title, bgpat, scrollcallback, arg);
	return hmi->mainwindow;
}

EXPORT VOID dathmi_deletemainwindow(dathmi_t *hmi, datwindow_t *window)
{
	datwindow_delete(window);
	hmi->mainwindow = NULL;
}

LOCAL VOID cfrmwindow_scroll(VP arg, W dh, W dv)
{
	cfrmwindow_t *window = (cfrmwindow_t*)arg;
	wscr_wnd(window->wid, NULL, -dh, -dv, W_SCRL|W_RDSET);
	cfrmwindow_redisp(window);
}

EXPORT cfrmwindow_t* cfrmwindow_new(RECT *r, WID parent, W dnum_title, W dnum_post, W dnum_cancel)
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
	window->post = NULL;
	err = windowscroll_initialize(&window->wscr, window->wid, cfrmwindow_scroll, (VP)window);
	if (err < 0) {
		free(window);
		return NULL;
	}

	err = dget_dtp(TEXT_DATA, dnum_title, (void**)&window->windowtitle);
	if (err < 0) {
		DP_ER("dget_dtp: message retrieving error", err);
		windowscroll_finalize(&window->wscr);
		free(window);
		return NULL;
	}

	err = dget_dtp(PARTS_DATA, dnum_post, (void**)&sw);
	if (err < 0) {
		DP_ER("dget_dtp: message retrieving error", err);
		windowscroll_finalize(&window->wscr);
		free(window);
		return NULL;
	}
	window->dnum_post = dnum_post;
	window->sz_ms_post.h = sw->r.c.right - sw->r.c.left;
	window->sz_ms_post.v = sw->r.c.bottom - sw->r.c.top;

	err = dget_dtp(PARTS_DATA, dnum_cancel, (void**)&sw);
	if (err < 0) {
		DP_ER("dget_dtp: message retrieving error", err);
		windowscroll_finalize(&window->wscr);
		free(window);
		return NULL;
	}
	window->dnum_cancel = dnum_cancel;
	window->sz_ms_cancel.h = sw->r.c.right - sw->r.c.left;
	window->sz_ms_cancel.v = sw->r.c.bottom - sw->r.c.top;

	return window;
}

EXPORT cfrmwindow_t *dathmi_newconfirmwindow(dathmi_t *hmi, RECT *r, W dnum_title, W dnum_post, W dnum_cancel)
{
	W main_wid;
	main_wid = dathmi_getmainWID(hmi);
	if (main_wid < 0) {
		DP_ER("main window not exist", 0);
		return NULL;
	}
	hmi->cfrmwindow = cfrmwindow_new(r, main_wid, dnum_title, dnum_post, dnum_cancel);
	return hmi->cfrmwindow;
}

LOCAL VOID cfrmwindow_delete(cfrmwindow_t *window)
{
	if (window->wid > 0) {
		wcls_wnd(window->wid, CLR);
	}
	windowscroll_finalize(&window->wscr);
	free(window);
}

EXPORT VOID dathmi_deleteconfirmwindow(dathmi_t *hmi, cfrmwindow_t *window)
{
	cfrmwindow_delete(window);
	hmi->cfrmwindow = NULL;
}

LOCAL ngwordwindow_t* ngwordwindow_new(PNT *p, WID parent, W dnum_list, W dnum_delete, W dnum_input, W dnum_append)
{
	ngwordwindow_t *window;
	W err;

	window = (ngwordwindow_t*)malloc(sizeof(ngwordwindow_t));
	if (window == NULL) {
		DP_ER("malloc error:", 0);
		return NULL;
	}
	window->wid = -1;
	window->gid = -1;
	window->parent = parent;
	window->r.p.lefttop = *p;
	window->r.p.rightbot.x = p->x + 300 + 7;
	window->r.p.rightbot.y = p->y + 200 + 30;
	err = wget_inf(WI_PANELBACK, &window->bgpat, sizeof(PAT));
	if (err != sizeof(PAT)) {
		DP_ER("wget_inf error:", err);
		window->bgpat.spat.kind = 0;
		window->bgpat.spat.hsize = 16;
		window->bgpat.spat.vsize = 16;
		window->bgpat.spat.fgcol = 0x10ffffff;
		window->bgpat.spat.bgcol = 0;
		window->bgpat.spat.mask = FILL100;
	}
	window->dnum_list = dnum_list;
	window->dnum_delete = dnum_delete;
	window->dnum_input = dnum_input;
	window->dnum_append = dnum_append;
	window->ss_list_id = -1;
	window->ms_delete_id = -1;
	window->tb_input_id = -1;
	window->ms_append_id = -1;
	wordlist_initialize(&window->wordlist);
	window->selector = NULL;

	return window;
}

LOCAL VOID ngwordwindow_delete(ngwordwindow_t *window)
{
	wordlist_finalize(&window->wordlist);
	if (window->wid > 0) {
		cdel_pwd(window->wid, NOCLR);
		wcls_wnd(window->wid, CLR);
	}
	free(window);
}

EXPORT ngwordwindow_t *dathmi_newngwordwindow(dathmi_t *hmi, PNT *p, W dnum_list, W dnum_delete, W dnum_input, W dnum_append)
{
	W main_wid;
	main_wid = dathmi_getmainWID(hmi);
	if (main_wid < 0) {
		DP_ER("main window not exist", 0);
		return NULL;
	}
	hmi->ngwordwindow = ngwordwindow_new(p, main_wid, dnum_list, dnum_delete, dnum_input, dnum_append);
	return hmi->ngwordwindow;
}

EXPORT VOID dathmi_deletengwordwindow(dathmi_t *hmi, ngwordwindow_t *window)
{
	ngwordwindow_delete(window);
	hmi->ngwordwindow = NULL;
}

EXPORT dathmi_t* dathmi_new()
{
	dathmi_t *hmi;

	hmi = (dathmi_t *)malloc(sizeof(dathmi_t));
	if (hmi == NULL) {
		return NULL;
	}
	hmi->mainwindow = NULL;
	hmi->cfrmwindow = NULL;
	hmi->ngwordwindow = NULL;

	return hmi;
}

EXPORT VOID dathmi_delete(dathmi_t *hmi)
{
	if (hmi->ngwordwindow != NULL) {
		ngwordwindow_delete(hmi->ngwordwindow);
	}
	if (hmi->cfrmwindow != NULL) {
		cfrmwindow_delete(hmi->cfrmwindow);
	}
	if (hmi->mainwindow != NULL) {
		datwindow_delete(hmi->mainwindow);
	}
	free(hmi);
}
