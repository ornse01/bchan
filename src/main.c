/*
 * main.c
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
#include	<btron/vobj.h>
#include	<btron/libapp.h>
#include	<btron/bsocket.h>

#include	"postres.h"
#include	"poptray.h"
#include	"confirm.h"
#include	"window.h"
#include	"cache.h"
#include	"parser.h"
#include	"layout.h"
#include	"retriever.h"
#include	"submit.h"

#ifdef BCHAN_CONFIG_DEBUG
# define DP(arg) printf arg
# define DP_ER(msg, err) printf("%s (%d/%x)\n", msg, err>>16, err)
#else
# define DP(arg) /**/
# define DP_ER(msg, err) /**/
#endif

#define BCHAN_DBX_MENU_TEST 20
#define BCHAN_DBX_TEXT_MLIST0	21
#define BCHAN_DBX_TEXT_MLIST1	22
#define BCHAN_DBX_TEXT_MLIST2	23
#define BCHAN_DBX_MSGTEXT_RETRIEVING 24
#define BCHAN_DBX_MSGTEXT_NOTMODIFIED 25
#define BCHAN_DBX_MSGTEXT_POSTSUCCEED 26
#define BCHAN_DBX_MSGTEXT_POSTDENIED 27
#define BCHAN_DBX_MSGTEXT_POSTERROR	28
#define BCHAN_DBX_MS_CONFIRM_POST 29
#define BCHAN_DBX_MS_CONFIRM_CANCEL	30
#define BCHAN_DBX_TEXT_CONFIRM_TITLE 31
#define BCHAN_DBX_MSGTEXT_NONAUTHORITATIVE 32

#define BCHAN_MENU_WINDOW 3

typedef struct bchan_hmistate_t_ bchan_hmistate_t;
struct bchan_hmistate_t_ {
	PTRSTL ptr;

	TC *msg_retrieving;
	TC *msg_notmodified;
	TC *msg_nonauthoritative;
	TC *msg_postsucceed;
	TC *msg_postdenied;
	TC *msg_posterror;
};

LOCAL VOID bchan_hmistate_updateptrstyle(bchan_hmistate_t *hmistate, PTRSTL ptr)
{
	if (hmistate->ptr == ptr) {
		return;
	}
	hmistate->ptr = ptr;
	gset_ptr(hmistate->ptr, NULL, -1, -1);
}

typedef struct bchan_t_ bchan_t;
struct bchan_t_ {
	W wid;
	W gid;
	W taskid;
	W mbfid;
	VID vid;
	W exectype;

	Bool request_confirm_open; /* TODO: should be other implememt? */

	MENUITEM *mnitem;
	MNID mnid;

	bchan_hmistate_t hmistate;

	datretriever_t *retriever;
	datcache_t *cache;
	datparser_t *parser;
	datlayout_t *layout;
	datdraw_t *draw;
	datwindow_t *window;
	ressubmit_t *submit;
	cfrmwindow_t *confirm;

	postresdata_t *resdata;
};

#define BCHAN_MESSAGE_RETRIEVER_UPDATE   1
#define BCHAN_MESSAGE_RETRIEVER_RELAYOUT 2
#define BCHAN_MESSAGE_RETRIEVER_NOTMODIFIED 3
#define BCHAN_MESSAGE_RETRIEVER_NONAUTHORITATIVE 4
#define BCHAN_MESSAGE_RETRIEVER_ERROR -1

static	WEVENT	wev0;

void	killme(bchan_t *bchan)
{
	gset_ptr(PS_BUSY, NULL, -1, -1);
	pdsp_msg(NULL);
	if (bchan->exectype == EXECREQ) {
		oend_prc(bchan->vid, NULL, 0);
	}
	if (bchan->wid > 0) {
		wcls_wnd(bchan->wid, CLR);
	}
	ext_prc(0);
}

LOCAL VOID bchan_scroll(VP arg, W dh, W dv)
{
	bchan_t *bchan = (bchan_t*)arg;
	datdraw_scrollviewrect(bchan->draw, dh, dv);
	wscr_wnd(bchan->wid, NULL, -dh, -dv, W_MOVE|W_RDSET);
}

LOCAL VOID bchan_draw(VP arg, RECT *r)
{
	bchan_t *bchan = (bchan_t*)arg;
	datdraw_draw(bchan->draw, r);
}

LOCAL VOID bchan_resize(VP arg)
{
	bchan_t *bchan = (bchan_t*)arg;
	W l,t,r,b;
	RECT work;
	Bool workchange = False;

	wget_wrk(bchan->wid, &work);
	if (work.c.left != 0) {
		work.c.left = 0;
		workchange = True;
	}
	if (work.c.top != 0) {
		work.c.top = 0;
		workchange = True;
	}
	wset_wrk(bchan->wid, &work);
	gset_vis(bchan->gid, work);

	datdraw_getviewrect(bchan->draw, &l, &t, &r, &b);

	r = l + work.c.right - work.c.left;
	b = t + work.c.bottom - work.c.top;

	datdraw_setviewrect(bchan->draw, l, t, r, b);
	datwindow_setworkrect(bchan->window, l, t, r, b);

	if (workchange == True) {
		wera_wnd(bchan->wid, NULL);
		wreq_dsp(bchan->wid);
	}
}

LOCAL VOID bchan_close(VP arg)
{
	bchan_t *bchan = (bchan_t*)arg;
	/* TODO: guard request event W_DELETE and W_FINISH. */
	datcache_writefile(bchan->cache);
	killme(bchan);
}

LOCAL W bchan_paste(VP arg, WEVENT *wev)
{
#ifdef BCHAN_CONFIG_DEBUG
	bchan_t *bchan = (bchan_t*)arg;
	W err;
	postresdata_t *post = NULL;

	err = poptray_gettraydata(&post);
	if (err < 0) {
		return 1; /* NACK */
	}

	wev->r.r.p.rightbot.x = 0x8000;
	wev->r.r.p.rightbot.y = 0x8000;

	if (post != NULL) {
		if (bchan->resdata != NULL) {
			postresdata_delete(bchan->resdata);
		}
		bchan->resdata = post;
		cfrmwindow_setpostresdata(bchan->confirm, post);
		bchan->request_confirm_open = True;
	}

	return 0; /* ACK */
#endif
	return 1; /* NACK */
}

LOCAL VOID bchan_recieveclose(VP arg, W send)
{
	bchan_t *bchan = (bchan_t*)arg;
	W err;

	DP(("bchan_recieveclose = %d\n", send));
	if (send == 1) {
		bchan_hmistate_updateptrstyle(&bchan->hmistate, PS_BUSY);
		err = ressubmit_respost(bchan->submit, bchan->resdata);
		bchan_hmistate_updateptrstyle(&bchan->hmistate, PS_SELECT);
		switch (err) {
		case RESSUBMIT_RESPOST_SUCCEED:
			pdsp_msg(bchan->hmistate.msg_postsucceed);
			break;
		case RESSUBMIT_RESPOST_DENIED:
			pdsp_msg(bchan->hmistate.msg_postdenied);
			break;
		case RESSUBMIT_RESPOST_ERROR_CLIENT:
		case RESSUBMIT_RESPOST_ERROR_STATUS:
		case RESSUBMIT_RESPOST_ERROR_CONTENT:
		default:
			pdsp_msg(bchan->hmistate.msg_posterror);
			break;
		}
	}
}

LOCAL VOID bchan_hmistate_initialize(bchan_hmistate_t *hmistate)
{
	W err;

	hmistate->ptr = PS_SELECT;

	err = dget_dtp(TEXT_DATA, BCHAN_DBX_MSGTEXT_RETRIEVING, (void**)&hmistate->msg_retrieving);
	if (err < 0) {
		DP_ER("dget_dtp: message retrieving error", err);
		hmistate->msg_retrieving = NULL;
	}
	err = dget_dtp(TEXT_DATA, BCHAN_DBX_MSGTEXT_NOTMODIFIED, (void**)&hmistate->msg_notmodified);
	if (err < 0) {
		DP_ER("dget_dtp: message not modified error", err);
		hmistate->msg_notmodified = NULL;
	}
	err = dget_dtp(TEXT_DATA, BCHAN_DBX_MSGTEXT_NONAUTHORITATIVE, (void**)&hmistate->msg_nonauthoritative);
	if (err < 0) {
		DP_ER("dget_dtp: message non-authoritative error", err);
		hmistate->msg_nonauthoritative = NULL;
	}
	err = dget_dtp(TEXT_DATA, BCHAN_DBX_MSGTEXT_POSTSUCCEED, (void**)&hmistate->msg_postsucceed);
	if (err < 0) {
		DP_ER("dget_dtp: message post succeed error", err);
		hmistate->msg_postsucceed = NULL;
	}
	err = dget_dtp(TEXT_DATA, BCHAN_DBX_MSGTEXT_POSTDENIED, (void**)&hmistate->msg_postdenied);
	if (err < 0) {
		DP_ER("dget_dtp: message post denied error", err);
		hmistate->msg_postdenied = NULL;
	}
	err = dget_dtp(TEXT_DATA, BCHAN_DBX_MSGTEXT_POSTERROR, (void**)&hmistate->msg_posterror);
	if (err < 0) {
		DP_ER("dget_dtp: message post error error", err);
		hmistate->msg_posterror = NULL;
	}
}

LOCAL W bchan_initialize(bchan_t *bchan, VID vid, WID wid, W exectype)
{
	GID gid;
	datcache_t *cache;
	datparser_t *parser;
	datlayout_t *layout;
	datdraw_t *draw;
	datwindow_t *window;
	datretriever_t *retriever;
	ressubmit_t *submit;
	cfrmwindow_t *confirm;
	MENUITEM *mnitem_dbx, *mnitem;
	MNID mnid;
	static	RECT	r0 = {{200, 100, 400+7, 200+30}};
	RECT w_work;
	W len, err;

	gid = wget_gid(wid);

	cache = datcache_new(vid);
	if (cache == NULL) {
		DP_ER("datcache_new error", 0);
		goto error_cache;
	}
	parser = datparser_new(cache);
	if (parser == NULL) {
		DP_ER("datparser_new error", 0);
		goto error_parser;
	}
	layout = datlayout_new(gid);
	if (layout == NULL) {
		DP_ER("datlayout_new error", 0);
		goto error_layout;
	}
	draw = datdraw_new(layout);
	if (draw == NULL) {
		DP_ER("datdraw_new error", 0);
		goto error_draw;
	}
	window = datwindow_new(wid, bchan_scroll, bchan_draw, bchan_resize, bchan_close, NULL, bchan_paste, bchan);
	if (window == NULL) {
		DP_ER("datwindow_new error", 0);
		goto error_window;
	}
	retriever = datretriever_new(cache);
	if (retriever == NULL) {
		DP_ER("datretriever_new error", 0);
		goto error_retriever;
	}
	submit = ressubmit_new(cache);
	if (submit == NULL) {
		DP_ER("ressubmit_new error", 0);
		goto error_submit;
	}
	confirm = cfrmwindow_new(&r0, wid, bchan_recieveclose, bchan, BCHAN_DBX_TEXT_CONFIRM_TITLE, BCHAN_DBX_MS_CONFIRM_POST, BCHAN_DBX_MS_CONFIRM_CANCEL);
	if (confirm == NULL) {
		DP_ER("dfrmwindow_new error", 0);
		goto error_confirm;
	}
	err = dget_dtp(8, BCHAN_DBX_MENU_TEST, (void**)&mnitem_dbx);
	if (err < 0) {
		DP_ER("dget_dtp error:", err);
		goto error_dget_dtp;
	}
	len = dget_siz((B*)mnitem_dbx);
	mnitem = malloc(len);
	if (mnitem == NULL) {
		DP_ER("mallod error", err);
		goto error_mnitem;
	}
	memcpy(mnitem, mnitem_dbx, len);
	mnid = mcre_men(BCHAN_MENU_WINDOW+2, mnitem, NULL);
	if (mnid < 0) {
		DP_ER("mcre_men error", mnid);
		goto error_mcre_men;
	}

	bchan_hmistate_initialize(&bchan->hmistate);

	wget_wrk(wid, &w_work);
	datdraw_setviewrect(draw, 0, 0, w_work.c.right, w_work.c.bottom);
	datwindow_setworkrect(window, 0, 0, w_work.c.right, w_work.c.bottom);

	if (exectype == EXECREQ) {
		osta_prc(vid, wid);
	}

	bchan->wid = wid;
	bchan->gid = gid;
	bchan->taskid = -1;
	bchan->mbfid = -1;
	bchan->vid = vid;
	bchan->exectype = exectype;
	bchan->request_confirm_open = False;
	bchan->cache = cache;
	bchan->parser = parser;
	bchan->layout = layout;
	bchan->draw = draw;
	bchan->window = window;
	bchan->retriever = retriever;
	bchan->submit = submit;
	bchan->confirm = confirm;
	bchan->resdata = NULL;
	bchan->mnitem = mnitem;
	bchan->mnid = mnid;

	return 0;

error_mcre_men:
	free(mnitem);
error_mnitem:
error_dget_dtp:
	cfrmwindow_delete(confirm);
error_confirm:
	ressubmit_delete(submit);
error_submit:
	datretriever_delete(retriever);
error_retriever:
	datwindow_delete(window);
error_window:
	datdraw_delete(draw);
error_draw:
	datlayout_delete(layout);
error_layout:
	datparser_delete(parser);
error_parser:
	datcache_delete(cache);
error_cache:
	return -1; /* TODO */
}

#define BCHAN_LAYOUT_MAXBLOCKING 20

LOCAL VOID bchan_relayout(bchan_t *bchan)
{
	datparser_res_t *res = NULL;
	RECT w_work;
	W i, err, l, t, r, b;
	TC *title;

	datlayout_clear(bchan->layout);
	datparser_clear(bchan->parser);

	for (i=0;;i++) {
		if (i >= BCHAN_LAYOUT_MAXBLOCKING) {
			req_tmg(0, BCHAN_MESSAGE_RETRIEVER_UPDATE);
			break;
		}

		err = datparser_getnextres(bchan->parser, &res);
		if (err != 1) {
			break;
		}
		if (res == NULL) {
			break;
		}
		datlayout_appendres(bchan->layout, res);
	}

	wget_wrk(bchan->wid, &w_work);

	datlayout_getdrawrect(bchan->layout, &l, &t, &r, &b);
	datwindow_setdrawrect(bchan->window, l, t, r, b);

	title = datlayout_gettitle(bchan->layout);
	wset_tit(bchan->wid, -1, title, 0);

	wreq_dsp(bchan->wid);
}

LOCAL VOID bchan_update(bchan_t *bchan)
{
	datparser_res_t *res = NULL;
	RECT w_work;
	W i, err, l, t, r, b;

	for (i=0;;i++) {
		if (i >= BCHAN_LAYOUT_MAXBLOCKING) {
			req_tmg(0, BCHAN_MESSAGE_RETRIEVER_UPDATE);
			break;
		}

		err = datparser_getnextres(bchan->parser, &res);
		if (err != 1) {
			break;
		}
		if (res == NULL) {
			break;
		}
		datlayout_appendres(bchan->layout, res);
	}

	wget_wrk(bchan->wid, &w_work);

	datlayout_getdrawrect(bchan->layout, &l, &t, &r, &b);
	datwindow_setdrawrect(bchan->window, l, t, r, b);

	wreq_dsp(bchan->wid);
}

LOCAL VOID bchan_retriever_task(W arg)
{
	bchan_t *bchan;
	datretriever_t *retr;
	W msg,err;

	bchan = (bchan_t*)arg;
	retr = bchan->retriever;

	for (;;) {
		DP(("before rcv_mbf %d\n", bchan->mbfid));
		err = rcv_mbf(bchan->mbfid, (VP)&msg, T_FOREVER);
		DP_ER("rcv_mbf", err);
		if (err != 4) {
			continue;
		}

		err = datretriever_request(retr);

		switch (err) {
		case DATRETRIEVER_REQUEST_ALLRELOAD:
			req_tmg(0, BCHAN_MESSAGE_RETRIEVER_RELAYOUT);
			break;
		case DATRETRIEVER_REQUEST_PARTIAL_CONTENT:
			req_tmg(0, BCHAN_MESSAGE_RETRIEVER_UPDATE);
			break;
		case DATRETRIEVER_REQUEST_NOT_MODIFIED:
			req_tmg(0, BCHAN_MESSAGE_RETRIEVER_NOTMODIFIED);
			break;
		case DATRETRIEVER_REQUEST_NON_AUTHORITATIVE:
			req_tmg(0, BCHAN_MESSAGE_RETRIEVER_NONAUTHORITATIVE);
			break;
		default:
			DP_ER("datretreiver_request error:",err);
			DATRETRIEVER_DP(retr);
			req_tmg(0, BCHAN_MESSAGE_RETRIEVER_ERROR);
			break;
		}
	}

	ext_tsk();
}

LOCAL W bchan_prepare_network(bchan_t *bchan)
{
	if (bchan->retriever == NULL) {
		return 0;
	}

	bchan->mbfid = cre_mbf(sizeof(W), sizeof(W), DELEXIT);
	if (bchan->mbfid < 0) {
		DP_ER("cre_mbf error:", bchan->mbfid);
		return -1;
	}
	bchan->taskid = cre_tsk(bchan_retriever_task, -1, (W)bchan);
	if (bchan->taskid < 0) {
		del_mbf(bchan->mbfid);
		bchan->mbfid = -1;
		DP_ER("cre_tsk error:", bchan->taskid);
		return -1;
	}

	return 0;
}

LOCAL W bchan_networkrequest(bchan_t *bchan)
{
	W msg = 1, err;
	static UW lastrequest = 0;
	UW etime;

	if (bchan->mbfid < 0) {
		return 0;
	}

	err = get_etm(&etime);
	if (err < 0) {
		DP_ER("get_etm error:", err);
		return err;
	}
	if (lastrequest + 10000 > etime) {
		return 0;
	}
	lastrequest = etime;

	err = snd_mbf(bchan->mbfid, &msg, sizeof(W), T_FOREVER);
	if (err < 0) {
		DP_ER("snd_mbf error:", err);
		return err;
	}

	bchan_hmistate_updateptrstyle(&bchan->hmistate, PS_BUSY);
	pdsp_msg(bchan->hmistate.msg_retrieving);

	return 0;
}

LOCAL VOID keydwn(bchan_t *bchan, UH keycode, TC ch)
{
	W l,t,r,b,l1,t1,r1,b1,scr;

	switch (ch) {
	case KC_CC_U:
		datdraw_getviewrect(bchan->draw, &l, &t, &r, &b);
		if (t < 16) {
			scr = -t;
		} else {
			scr = -16;
		}
		datwindow_scrollbyvalue(bchan->window, 0, scr);
		break;
	case KC_CC_D:
		datdraw_getviewrect(bchan->draw, &l, &t, &r, &b);
		datlayout_getdrawrect(bchan->layout, &l1, &t1, &r1, &b1);
		if (b + 16 > b1) {
			scr = b1 - b;
		} else {
			scr = 16;
		}
		datwindow_scrollbyvalue(bchan->window, 0, scr);
		break;
	case KC_CC_R:
		datdraw_getviewrect(bchan->draw, &l, &t, &r, &b);
		datlayout_getdrawrect(bchan->layout, &l1, &t1, &r1, &b1);
		if (r + 16 > r1) {
			scr = r1 - r;
		} else {
			scr = 16;
		}
		datwindow_scrollbyvalue(bchan->window, scr, 0);
		break;
	case KC_CC_L:
		datdraw_getviewrect(bchan->draw, &l, &t, &r, &b);
		if (l < 16) {
			scr = -l;
		} else {
			scr = -16;
		}
		datwindow_scrollbyvalue(bchan->window, scr, 0);
		break;
	case KC_PG_U:
	case KC_PG_D:
	case KC_PG_R:
	case KC_PG_L:
		/* TODO: area scroll */
		break;
	case KC_PF5:
		bchan_networkrequest(bchan);
		break;
	}
}

LOCAL VOID bchan_setupmenu(bchan_t *bchan)
{
	wget_dmn(&(bchan->mnitem[BCHAN_MENU_WINDOW].ptr));
	mset_itm(bchan->mnid, BCHAN_MENU_WINDOW, bchan->mnitem+BCHAN_MENU_WINDOW);
	oget_men(0, NULL, &(bchan->mnitem[BCHAN_MENU_WINDOW+1].ptr), NULL, NULL);
	mset_itm(bchan->mnid, BCHAN_MENU_WINDOW+1, bchan->mnitem+BCHAN_MENU_WINDOW+1);
}

LOCAL VOID bchan_selectmenu(bchan_t *bchan, W i)
{
	switch(i >> 8) {
	case 0: /* [終了] */
		killme(bchan);
		break;
	case 1: /* [表示] */
		switch(i & 0xff) {
		case 1: /* [再表示] */
			wreq_dsp(bchan->wid);
			break;
		}
		break;
	case 2:	/* [操作] */
		switch(i & 0xff) {
		case 1: /* [スレッド取得] */
			bchan_networkrequest(bchan);
			break;
		}
		break;
	case BCHAN_MENU_WINDOW: /* [ウィンドウ] */
		wexe_dmn(i);
		break;
	case BCHAN_MENU_WINDOW+1: /* [小物] */
	    oexe_apg(0, i);
		break;
	}
	return;
}

LOCAL VOID bchan_popupmenu(bchan_t *bchan, PNT pos)
{
	W	i;

	bchan_setupmenu(bchan);
	gset_ptr(PS_SELECT, NULL, -1, -1);
	i = msel_men(bchan->mnid, pos);
	if (i > 0) {
		bchan_selectmenu(bchan, i);
	}
}

LOCAL VOID receive_message(bchan_t *bchan)
{
	MESSAGE msg;
	W code, err;

    err = rcv_msg(MM_ALL, &msg, sizeof(MESSAGE), WAIT|NOCLR);
	if (err >= 0) {
		if (msg.msg_type == MS_TMOUT) { /* should be use other type? */
			code = msg.msg_body.TMOUT.code;
			switch (code) {
			case BCHAN_MESSAGE_RETRIEVER_UPDATE:
				bchan_update(bchan);
				bchan_hmistate_updateptrstyle(&bchan->hmistate, PS_SELECT);
				pdsp_msg(NULL);
				break;
			case BCHAN_MESSAGE_RETRIEVER_RELAYOUT:
				bchan_relayout(bchan);
				bchan_hmistate_updateptrstyle(&bchan->hmistate, PS_SELECT);
				pdsp_msg(NULL);
				break;
			case BCHAN_MESSAGE_RETRIEVER_NOTMODIFIED:
				bchan_hmistate_updateptrstyle(&bchan->hmistate, PS_SELECT);
				pdsp_msg(bchan->hmistate.msg_notmodified);
				break;
			case BCHAN_MESSAGE_RETRIEVER_NONAUTHORITATIVE:
				bchan_hmistate_updateptrstyle(&bchan->hmistate, PS_SELECT);
				pdsp_msg(bchan->hmistate.msg_nonauthoritative);
				break;
			case BCHAN_MESSAGE_RETRIEVER_ERROR:
				bchan_hmistate_updateptrstyle(&bchan->hmistate, PS_SELECT);
				pdsp_msg(NULL);
				break;
			}
		}
	}
	clr_msg(MM_ALL, MM_ALL);
}

typedef struct _arg {
	W ac;
	TC **argv;
} CLI_arg;

LOCAL    CLI_arg   MESSAGEtoargv(const MESSAGE *src)
{
	W len,i,ac;
	TC *str;
	TC **argv;
	CLI_arg ret;

	len = src->msg_size / sizeof(TC);
	str = (TC*)(src->msg_body.ANYMSG.msg_str);
	ac = 0;
	for(i=0;i<len;i++){
		if(str[i] == TK_KSP){
			str[i] = TNULL;
			continue;
		}
		ac++;
		for(;i<len;i++){
			if(str[i] == TK_KSP){
				i--;
				break;
			}
		}
	}

	argv = (TC**)malloc(sizeof(TC*)*ac);

	ac = 0;
	for(i=0;i<len;i++){
		if(str[i] == TNULL){
			str[i] = TNULL;
			continue;
		}
		argv[ac++] = str+i;
		for(;i<len;i++){
			if(str[i] == TNULL){
				i--;
				break;
			}
		}
	}

	ret.ac = ac;
	ret.argv = argv;

	return ret;
}

EXPORT	W	MAIN(MESSAGE *msg)
{
	static	RECT	r0 = {{100, 100, 650+7, 400+30}};
	static	TC	tit0[21];
	static	PAT	pat0 = {{
		0,
		16, 16,
		0x10efefef,
		0x10efefef,
		FILL100
	}};
	W	i, err, size;
	WID wid;
	VID vid;
	LINK lnk, dbx;
	CLI_arg arg;
	bchan_t bchan;
	datwindow_t *window;
	VOBJSEG vseg = {
		{{0,0,100,20}},
		16, 16,
		0x10000000, 0x10000000, 0x10FFFFFF, 0x10FFFFFF,
		0
	};

	err = dopn_dat(NULL);
	if (err < 0) {
		ext_prc(0);
	}

	switch (msg->msg_type) {
	case 0: /* CLI */
		arg = MESSAGEtoargv(msg);
		if (arg.ac <= 1) {
			ext_prc(0);
		}
		err = get_lnk(arg.argv[1], &lnk, F_NORM);
		if (err < 0) {
			DP_ER("get_lnk error", err);
			ext_prc(0);
		}
		vid = oreg_vob((VLINK*)&lnk, (VP)&vseg, -1, V_NODISP);
		if (vid < 0) {
			DP_ER("error oreq_vob", vid);
			ext_prc(0);
		}
		err = get_lnk((TC[]){TK_b, TK_c, TK_h, TK_a, TK_n, TK_PROD, TK_d, TK_b, TK_x,TNULL}, &dbx, F_NORM);
		if (err < 0) {
			DP_ER("get_lnk error", err);
			ext_prc(0);
		}
		err = dopn_dat(&dbx);
		if (err < 0) {
			DP_ER("dopn_dat error", err);
			ext_prc(0);
		}
		fil_sts(&lnk, tit0, NULL, NULL);
		break;
	case DISPREQ:
		oend_req(((M_DISPREQ*)msg)->vid, -1);
		ext_prc(0);
		break;
	case PASTEREQ:
		oend_req(((M_PASTEREQ*)msg)->vid, -1);
		ext_prc(0);
		break;
	case EXECREQ:
		if ((((M_EXECREQ*)msg)->mode & 2) != 0) {
			ext_prc(0);
		}
		err = dopn_dat(&((M_EXECREQ*)msg)->self);
		if (err < 0) {
			ext_prc(0);
		}
		fil_sts(&((M_EXECREQ*)msg)->lnk, tit0, NULL, NULL);
		vid = ((M_EXECREQ*)msg)->vid;
		break;
	default:
		ext_prc(0);
		break;
	}

	wid = wopn_wnd(WA_SIZE|WA_HHDL|WA_VHDL|WA_BBAR|WA_RBAR, 0, &r0, NULL, 1, tit0, &pat0, NULL);
	if (wid < 0) {
		ext_prc(0);
	}
	
	err = bchan_initialize(&bchan, vid, wid, msg->msg_type);
	if (err < 0) {
		DP_ER("bchan_initialize error", err);
		ext_prc(0);
	}
	window = bchan.window;

	err = bchan_prepare_network(&bchan);
	if (err < 0) {
		DP_ER("bchan_prepare_network error:", err);
		killme(&bchan);
	}

	size = datcache_datasize(bchan.cache);
	if ((msg->msg_type == EXECREQ)&&(size == 0)) {
		bchan_networkrequest(&bchan);
	} else {
		req_tmg(0, BCHAN_MESSAGE_RETRIEVER_RELAYOUT);
	}

	wreq_dsp(bchan.wid);

	/*イベントループ*/
	for (;;) {
		wget_evt(&wev0, WAIT);
		switch (wev0.s.type) {
			case	EV_NULL:
				if ((wev0.s.wid != wid)
					&&(cfrmwindow_compairwid(bchan.confirm, wev0.s.wid) == False)) {
					gset_ptr(bchan.hmistate.ptr, NULL, -1, -1);
					break;		/*ウィンドウ外*/
				}
				if (wev0.s.cmd != W_WORK)
					break;		/*作業領域外*/
				if (wev0.s.stat & ES_CMD)
					break;	/*命令キーが押されている*/
				gset_ptr(bchan.hmistate.ptr, NULL, -1, -1);
				break;
			case	EV_REQUEST:
				if (wev0.g.wid == wid) {
					datwindow_weventrequest(window, &wev0);
				}
				if (cfrmwindow_compairwid(bchan.confirm, wev0.s.wid)) {
					cfrmwindow_weventrequest(bchan.confirm, &wev0);
				}
				break;
			case	EV_RSWITCH:
				if (wev0.s.wid == wid) {
					datwindow_weventreswitch(window, &wev0);
					if (bchan.request_confirm_open == True) {
						cfrmwindow_open(bchan.confirm);
						bchan.request_confirm_open = False;
					}
				}
				if (cfrmwindow_compairwid(bchan.confirm, wev0.s.wid)) {
					cfrmwindow_weventreswitch(bchan.confirm, &wev0);
				}
				break;
			case	EV_SWITCH:
				if (wev0.s.wid == wid) {
					datwindow_weventswitch(window, &wev0);
					if (bchan.request_confirm_open == True) {
						cfrmwindow_open(bchan.confirm);
						bchan.request_confirm_open = False;
					}
				}
				if (cfrmwindow_compairwid(bchan.confirm, wev0.s.wid)) {
					cfrmwindow_weventswitch(bchan.confirm, &wev0);
				}
				break;
			case	EV_BUTDWN:
				if (wev0.g.wid == wid) {
					datwindow_weventbutdn(window, &wev0);
				}
				if (cfrmwindow_compairwid(bchan.confirm, wev0.s.wid)) {
					cfrmwindow_weventbutdn(bchan.confirm, &wev0);
				}
				break;
			case	EV_KEYDWN:
				if (wev0.s.stat & ES_CMD) {	/*命令キー*/
					bchan_setupmenu(&bchan);
					i = mfnd_key(bchan.mnid, wev0.e.data.key.code);
					if (i >= 0) {
						bchan_selectmenu(&bchan, i);
						break;
					}
				}
			case	EV_AUTKEY:
				keydwn(&bchan, wev0.e.data.key.keytop, wev0.e.data.key.code);
				break;
			case	EV_INACT:
				pdsp_msg(NULL);
				break;
			case	EV_DEVICE:
				oprc_dev(&wev0.e, NULL, 0);
				break;
			case	EV_MSG:
				receive_message(&bchan);
				break;
			case	EV_MENU:
				bchan_popupmenu(&bchan, wev0.s.pos);
				break;
		}
	}

	return 0;
}
