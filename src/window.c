/*
 * window.c
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

#include    "window.h"

#include	<basic.h>
#include	<bstdio.h>
#include	<bstdlib.h>
#include	<btron/hmi.h>

#ifdef BCHAN_CONFIG_DEBUG
# define DP(arg) printf arg
# define DP_ER(msg, err) printf("%s (%d/%x)\n", msg, err>>16, err)
#else
# define DP(arg) /**/
# define DP_ER(msg, err) /**/
#endif

struct datwindow_t_ {
	WID wid;
	GID gid;
	PAID rbar;
	PAID bbar;
	datwindow_scrollcalback scroll_callback;
	datwindow_drawcallback draw_callback;
	datwindow_resizecallback resize_callback;
	datwindow_closecallback close_callback;
	datwindow_butdncallback butdn_callback;
	datwindow_pastecallback paste_callback;
	VP arg;
	W draw_l,draw_t,draw_r,draw_b;
	W work_l,work_t,work_r,work_b;
};

LOCAL VOID datwindow_callback_draw(datwindow_t *window, RECT *r)
{
	if (window->draw_callback != NULL) {
		(*window->draw_callback)(window->arg, r);
	}
}

LOCAL VOID datwindow_callback_resize(datwindow_t *window)
{
	if (window->resize_callback != NULL) {
		(*window->resize_callback)(window->arg);
	}
}

LOCAL VOID datwindow_callback_scroll(datwindow_t *window, W dh, W dv)
{
	if (window->scroll_callback != NULL) {
		(*window->scroll_callback)(window->arg, -dh, -dv); /* TODO */
	}
}

LOCAL VOID datwindow_callback_close(datwindow_t *window)
{
	if (window->close_callback != NULL) {
		(*window->close_callback)(window->arg);
	}
}

LOCAL VOID datwindow_callback_butdn(datwindow_t *window, WEVENT *wev)
{
	if (window->butdn_callback != NULL) {
		(*window->butdn_callback)(window->arg, wev);
	}
}

LOCAL W datwindow_callback_paste(datwindow_t *window, WEVENT *wev)
{
	if (window->close_callback != NULL) {
		return (*window->paste_callback)(window->arg, wev);
	}
	return 1; /* nack */
}

EXPORT VOID datwindow_setwid(datwindow_t *window, WID target)
{
	if (target < 0) {
		window->wid = -1;
		window->gid = -1;
		window->rbar = -1;
		window->bbar = -1;
	} else {
		window->wid = target;
		window->gid = wget_gid(window->wid);
		wget_bar(target, &(window->rbar), &(window->bbar), NULL);
		cchg_par(window->rbar, P_NORMAL|P_NOFRAME|P_ENABLE|P_ACT|P_DRAGBREAK);
		cchg_par(window->bbar, P_NORMAL|P_NOFRAME|P_ENABLE|P_ACT|P_DRAGBREAK);
	}
}

EXPORT datwindow_t* datwindow_new(WID target, datwindow_scrollcalback scrollcallback, datwindow_drawcallback drawcallback, datwindow_resizecallback resizecallback, datwindow_closecallback closecallback, datwindow_butdncallback butdncallback, datwindow_pastecallback pastecallback, VP arg)
{
	datwindow_t* wscr;

	wscr = malloc(sizeof(datwindow_t));
	if (wscr == NULL) {
		return NULL;
	}

	datwindow_setwid(wscr, target);
	wscr->scroll_callback = scrollcallback;
	wscr->draw_callback = drawcallback;
	wscr->resize_callback = resizecallback;
	wscr->close_callback = closecallback;
	wscr->butdn_callback = butdncallback;
	wscr->paste_callback = pastecallback;
	wscr->arg = arg;

	wscr->work_l = 0;
	wscr->work_t = 0;
	wscr->work_r = 0;
	wscr->work_b = 0;
	wscr->draw_l = 0;
	wscr->draw_t = 0;
	wscr->draw_r = 0;
	wscr->draw_b = 0;

	return wscr;
}

EXPORT VOID datwindow_delete(datwindow_t *wscr)
{
	free(wscr);

}

EXPORT W datwindow_setworkrect(datwindow_t *wscr, W l, W t, W r, W b)
{
	wscr->work_l = l;
	wscr->work_t = t;
	wscr->work_r = r;
	wscr->work_b = b;
	return datwindow_updatebar(wscr);
}

LOCAL VOID datwindow_redisp(datwindow_t *window)
{
	RECT r;
	do {
		if (wsta_dsp(window->wid, &r, NULL) == 0) {
			break;
		}
		wera_wnd(window->wid, &r);
		datwindow_callback_draw(window, &r);
	} while (wend_dsp(window->wid) > 0);
}

LOCAL W datwindow_updaterbar(datwindow_t *wscr)
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
		DP(("datwindow_updaterbar:cset_val rbar\n"));
		DP(("                       :%d %d %d %d\n", sbarval[0], sbarval[1], sbarval[2], sbarval[3]));
		return err;
	}

	return 0;
}

LOCAL W datwindow_updatebbar(datwindow_t *wscr)
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
		DP(("datwindow_updatebbar:cset_val bbar\n"));
		DP(("                       :%d %d %d %d\n", sbarval[0], sbarval[1], sbarval[2], sbarval[3]));
		return err;
    }

	return 0;
}

EXPORT W datwindow_updatebar(datwindow_t *wscr)
{
	W err;

	err = datwindow_updaterbar(wscr);
	if (err < 0) {
		return err;
	}

	err = datwindow_updatebbar(wscr);
	if (err < 0) {
		return err;
	}

	return 0;
}

LOCAL W datwindow_scrollbar(datwindow_t *wscr, WEVENT *wev, PAID scrbarPAID)
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
				datwindow_callback_scroll(wscr, dh, dv);
			} else if (scrbarPAID == wscr->bbar) { /*下バー*/
				dh = -(*chi-wscr->work_l);
				dv = 0;
				wscr->work_l -= dh;
				wscr->work_r -= dh;
				datwindow_callback_scroll(wscr, dh, dv);
			}
			datwindow_redisp(wscr);
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
			datwindow_callback_scroll(wscr, dh, dv);
			datwindow_redisp(wscr);
			datwindow_updaterbar(wscr);
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
			datwindow_callback_scroll(wscr, dh, dv);
			datwindow_redisp(wscr);
			datwindow_updatebbar(wscr);
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
			datwindow_callback_scroll(wscr, dh, dv);
			datwindow_redisp(wscr);
			datwindow_updaterbar(wscr);
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
			datwindow_callback_scroll(wscr, dh, dv);
			datwindow_redisp(wscr);
			datwindow_updaterbar(wscr);
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
			datwindow_callback_scroll(wscr, dh, dv);
			datwindow_redisp(wscr);
			datwindow_updatebbar(wscr);
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
			datwindow_callback_scroll(wscr, dh, dv);
			datwindow_redisp(wscr);
			datwindow_updatebbar(wscr);
			break;
		}
		break;
	}

	return 0;
}

EXPORT VOID datwindow_scrollbyvalue(datwindow_t *wscr, W dh, W dv)
{
	wscr->work_l += dh;
	wscr->work_t += dv;
	wscr->work_r += dh;
	wscr->work_b += dv;
	datwindow_callback_scroll(wscr, -dh, -dv);
	datwindow_redisp(wscr);
	datwindow_updatebar(wscr);
}

LOCAL W datwindow_weventrbar(datwindow_t *wscr, WEVENT *wev)
{
	return datwindow_scrollbar(wscr, wev, wscr->rbar);
}

LOCAL W datwindow_weventbbar(datwindow_t *wscr, WEVENT *wev)
{
	return datwindow_scrollbar(wscr, wev, wscr->bbar);
}

EXPORT W datwindow_setdrawrect(datwindow_t *wscr, W l, W t, W r, W b)
{
	wscr->draw_l = l;
	wscr->draw_t = t;
	wscr->draw_r = r;
	wscr->draw_b = b;
	return datwindow_updatebar(wscr);
}

EXPORT VOID datwindow_weventbutdn(datwindow_t *wscr, WEVENT *wev)
{
	W i;

	switch	(wev->s.cmd) {
		case	W_PICT:
			switch (wchk_dck(wev->s.time)) {
				case	W_DCLICK:
					datwindow_callback_close(wscr);
				case	W_PRESS:
					break;
				default:
					return;
			}
		case	W_FRAM:
		case	W_TITL:
			if (wmov_drg(wev, NULL) > 0) {
				datwindow_redisp(wscr);
			}
			return;
		case	W_LTHD:
		case	W_RTHD:
		case	W_LBHD:
		case	W_RBHD:
			switch (wchk_dck(wev->s.time)) {
				case	W_DCLICK:
					i = wchg_wnd(wscr->wid, NULL, W_MOVE);
					break;
				case	W_PRESS:
					i = wrsz_drg(wev, NULL, NULL);
					break;
				default:
					return;
			}

			datwindow_callback_resize(wscr);
			datwindow_updatebar(wscr);
			if (i > 0) {
				datwindow_redisp(wscr);
			}
			return;
		case	W_RBAR:
			datwindow_weventrbar(wscr, wev);
			return;
		case	W_BBAR:
			datwindow_weventbbar(wscr, wev);
			return;
		case	W_WORK:
			datwindow_callback_butdn(wscr, wev);
			return;
		
	}
	return;
}

EXPORT VOID datwindow_weventrequest(datwindow_t *wscr, WEVENT *wev)
{
	W nak;

	switch (wev->g.cmd) {
	case	W_REDISP:	/*再表示要求*/
		datwindow_redisp(wscr);
		break;
	case	W_PASTE:	/*貼込み要求*/
		nak = datwindow_callback_paste(wscr, wev);
		wrsp_evt(wev, nak);
		break;
	case	W_DELETE:	/*保存終了*/
	case	W_FINISH:	/*廃棄終了*/
		wrsp_evt(wev, 0);	/*ACK*/
		datwindow_callback_close(wscr);
	}
}

EXPORT VOID datwindow_weventswitch(datwindow_t *wscr, WEVENT *wev)
{
	datwindow_weventbutdn(wscr, wev);
}

EXPORT VOID datwindow_weventreswitch(datwindow_t *wscr, WEVENT *wev)
{
	datwindow_redisp(wscr);
	datwindow_weventbutdn(wscr, wev);
}
