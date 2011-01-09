/*
 * main.c
 *
 * Copyright (c) 2009-2011 project bchan
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
#include	<tctype.h>
#include	<btron/btron.h>
#include	<btron/dp.h>
#include	<btron/hmi.h>
#include	<btron/vobj.h>
#include	<btron/libapp.h>
#include	<btron/bsocket.h>

#include	"postres.h"
#include	"poptray.h"
#include	"hmi.h"
#include	"cache.h"
#include	"parser.h"
#include	"layoutarray.h"
#include	"layoutstyle.h"
#include	"layout.h"
#include	"render.h"
#include	"traydata.h"
#include	"retriever.h"
#include	"submit.h"
#include	"tadurl.h"
#include	"sjisstring.h"
#include	"bchan_vobj.h"
#include	"bchan_panels.h"
#include	"bchan_menus.h"

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
#define BCHAN_DBX_MSGTEXT_NETWORKERROR	33
#define BCHAN_DBX_FFUSEN_BBB 34
#define BCHAN_DBX_FFUSEN_TEXEDIT 35
#define BCHAN_DBX_FFUSEN_VIEWER 36
#define BCHAN_DBX_MSGTEXT_NOTFOUND	37
#define BCHAN_DBX_MSGTEXT_CANTRETRIEVE 44
#define BCHAN_DBX_GMENU_RESNUMBER 45
#define BCHAN_DBX_GMENU_RESID 47
#define BCHAN_DBX_SS_NGWORD_LIST 49
#define BCHAN_DBX_MS_NGWORD_DELETE 50
#define BCHAN_DBX_TB_NGWORD_APPEND 51
#define BCHAN_DBX_MS_NGWORD_APPEND 52

typedef struct bchan_hmistate_t_ bchan_hmistate_t;
struct bchan_hmistate_t_ {
	PTRSTL ptr;

	TC *msg_retrieving;
	TC *msg_notmodified;
	TC *msg_nonauthoritative;
	TC *msg_postsucceed;
	TC *msg_postdenied;
	TC *msg_posterror;
	TC *msg_networkerror;
	TC *msg_notfound;
	TC *msg_cantretrieve;
};

LOCAL VOID bchan_hmistate_updateptrstyle(bchan_hmistate_t *hmistate, PTRSTL ptr)
{
	if (hmistate->ptr == ptr) {
		return;
	}
	hmistate->ptr = ptr;
	gset_ptr(hmistate->ptr, NULL, -1, -1);
}

LOCAL VOID bchan_hmistate_display_postdenied(bchan_hmistate_t *hmistate, TC *str, W len)
{
	W size, msg_len;
	TC *msg;

	size = tc_strlen(hmistate->msg_postdenied);
	msg_len = size + len + 2;
	msg = malloc((msg_len + 1)*sizeof(TC));
	if (msg == NULL) {
		pdsp_msg(hmistate->msg_postdenied);
	}

	tc_strcpy(msg, hmistate->msg_postdenied);
	msg[size] = TK_LPAR;
	memcpy((UB*)(msg + size + 1), str, len * sizeof(TC));
	msg[msg_len - 1] = TK_RPAR;
	msg[msg_len] = TNULL;

	pdsp_msg(msg);

	free(msg);
}

typedef struct bchan_t_ bchan_t;
struct bchan_t_ {
	W wid; /* tmp */
	W gid;
	W taskid;
	W mbfid;
	VID vid;
	W exectype;

	Bool request_confirm_open; /* TODO: should be other implememt? */

	bchan_hmistate_t hmistate;
	bchan_mainmenu_t mainmenu;
	bchan_resnumbermenu_t resnumbermenu;
	bchan_residmenu_t residmenu;

	datretriever_t *retriever;
	datcache_t *cache;
	datparser_t *parser;
	datlayoutarray_t *layoutarray;
	datlayoutstyle_t style;
	datlayout_t *layout;
	datrender_t *render;
	dattraydata_t *traydata;
	dathmi_t *hmi;
	datwindow_t *window;
	ressubmit_t *submit;
	cfrmwindow_t *confirm;
	ngwordwindow_t *ngword;

	postresdata_t *resdata;
};

#define BCHAN_MESSAGE_RETRIEVER_UPDATE   1
#define BCHAN_MESSAGE_RETRIEVER_RELAYOUT 2
#define BCHAN_MESSAGE_RETRIEVER_NOTMODIFIED 3
#define BCHAN_MESSAGE_RETRIEVER_NONAUTHORITATIVE 4
#define BCHAN_MESSAGE_RETRIEVER_NOTFOUND 5
#define BCHAN_MESSAGE_RETRIEVER_ERROR -1

void	killme(bchan_t *bchan)
{
	gset_ptr(PS_BUSY, NULL, -1, -1);
	pdsp_msg(NULL);
	bchan_residmenu_finalize(&bchan->residmenu);
	bchan_resnumbermenu_finalize(&bchan->resnumbermenu);
	if (bchan->exectype == EXECREQ) {
		oend_prc(bchan->vid, NULL, 0);
	}
	dathmi_delete(bchan->hmi);
	ext_prc(0);
}

LOCAL VOID bchan_draw(bchan_t *bchan)
{
	RECT r;
	do {
		if (datwindow_startredisp(bchan->window, &r) == 0) {
			break;
		}
		datwindow_eraseworkarea(bchan->window, &r);
		datrender_draw(bchan->render, &r);
	} while (datwindow_endredisp(bchan->window) > 0);
}

LOCAL VOID bchan_scroll(VP arg, W dh, W dv)
{
	bchan_t *bchan = (bchan_t*)arg;
	datrender_scrollviewrect(bchan->render, dh, dv);
	datwindow_scrollworkarea(bchan->window, -dh, -dv);
	bchan_draw(bchan);
}

LOCAL VOID bchan_resize(bchan_t *bchan, SIZE newsize)
{
	W l,t,r,b;

	datrender_getviewrect(bchan->render, &l, &t, &r, &b);

	r = l + newsize.h;
	b = t + newsize.v;

	datrender_setviewrect(bchan->render, l, t, r, b);
	datwindow_setworkrect(bchan->window, l, t, r, b);

	bchan_draw(bchan);
}

LOCAL VOID bchan_close(bchan_t *bchan, Bool save)
{
	/* TODO: guard request event W_DELETE and W_FINISH. */
	datcache_writefile(bchan->cache);
	killme(bchan);
}

LOCAL VOID bchan_pushstringtotray(TC *str, W len)
{
	W err;
	TRAYREC trayrec[2];
	UB bin[4+24];
	TADSEG *base = (TADSEG*)bin;
	TEXTSEG *textseg = (TEXTSEG*)(bin + 4);

	base->id = 0xFFE1;
	base->len = 24;
	textseg->view = (RECT){{0, 0, 0, 0}};
	textseg->draw = (RECT){{0, 0, 0, 0}};
	textseg->h_unit = -120;
	textseg->v_unit = -120;
	textseg->lang = 0x21;
	textseg->bgpat = 0;

	trayrec[0].id = 0xE1;
	trayrec[0].len = 28;
	trayrec[0].dt = bin;
	trayrec[1].id = TR_TEXT;
	trayrec[1].len = len * sizeof(TC);
	trayrec[1].dt = (B*)str;

	err = tpsh_dat(trayrec, 2, NULL);
	if (err < 0) {
		DP_ER("tpsh_dat", err);
	}
}

LOCAL VOID bchan_pushthreadtitle(bchan_t *bchan)
{
	W len;
	TC *str;

	str = datlayout_gettitle(bchan->layout);
	len = datlayout_gettitlelen(bchan->layout);
	bchan_pushstringtotray(str, len);
}

LOCAL VOID bchan_pushthreadurl(bchan_t *bchan)
{
	W host_len, board_len, thread_len, len, i, ret;
	UB *host, *board, *thread;
	TC *str;

	datcache_gethost(bchan->cache, &host, &host_len);
	datcache_getborad(bchan->cache, &board, &board_len);
	datcache_getthread(bchan->cache, &thread, &thread_len);

	len = 7 + host_len + 15 + board_len + 1 + thread_len + 1;
	str = malloc(sizeof(TC)*len);

	str[0] = TK_h;
	str[1] = TK_t;
	str[2] = TK_t;
	str[3] = TK_p;
	str[4] = TK_COLN;
	str[5] = TK_SLSH;
	str[6] = TK_SLSH;
	for (i = 0; i < host_len; i++) {
		ret = sjtotc(str + 7 + i, host + i);
		if (ret != 1) {
			DP(("invalid charactor\n"));
			free(str);
			return;
		}
	}
	str[7 + host_len] = TK_SLSH;
	str[7 + host_len + 1] = TK_t;
	str[7 + host_len + 2] = TK_e;
	str[7 + host_len + 3] = TK_s;
	str[7 + host_len + 4] = TK_t;
	str[7 + host_len + 5] = TK_SLSH;
	str[7 + host_len + 6] = TK_r;
	str[7 + host_len + 7] = TK_e;
	str[7 + host_len + 8] = TK_a;
	str[7 + host_len + 9] = TK_d;
	str[7 + host_len + 10] = TK_PROD;
	str[7 + host_len + 11] = TK_c;
	str[7 + host_len + 12] = TK_g;
	str[7 + host_len + 13] = TK_i;
	str[7 + host_len + 14] = TK_SLSH;
	for (i = 0; i < board_len; i++) {
		ret = sjtotc(str + 7 + host_len + 15 + i, board + i);
		if (ret != 1) {
			DP(("invalid charactor\n"));
			free(str);
			return;
		}
	}
	str[7 + host_len + 15 + board_len] = TK_SLSH;
	for (i = 0; i < thread_len; i++) {
		ret = sjtotc(str + 7 + host_len + 15 + board_len + 1 + i, thread + i);
		if (ret != 1) {
			DP(("invalid charactor\n"));
			free(str);
			return;
		}
	}
	str[7 + host_len + 15 + board_len + 1 + thread_len] = TK_SLSH;

	bchan_pushstringtotray(str, len);

	free(str);
}

LOCAL Bool bchan_is_bbs_url(UB *data, W data_len)
{
	TC *str;
	W str_len, cmp, i, n_slsh = 0;

	if ((tadurl_cmpscheme(data, data_len, "http", 4) != 0)
		&&(tadurl_cmpscheme(data, data_len, "ttp", 3) != 0)) {
		return False;
	}

	cmp = tadurl_cmppath(data, data_len, "test/read.cgi/", 14);
	if (cmp != 0) {
		return False;
	}

	str = (TC*)data;
	str_len = data_len/2;

	for (i=0;; i++) {
		if (i >= str_len) {
			return False;
		}
		if (n_slsh == 6) {
			break;
		}
		if (str[i] == TK_SLSH) {
			n_slsh++;
		}
	}

	for (;; i++) {
		if (i >= str_len) {
			return False;
		}
		if (str[i] == TK_SLSH) {
			break;
		}
		if (tc_isdigit(str[i]) == 0) {
			return False;
		}
	}

	return True;
}

LOCAL W bchan_createvobj_allocate_url(UB *data, W data_len, TC **url, W *url_len)
{
	TC *str;
	W len;

	if (tadurl_cmpscheme(data, data_len, "http", 4) == 0) {
		len = data_len;
		str = malloc(len);
		if (str == NULL) {
			return -1; /* TODO */
		}
		memcpy(str, data, data_len);
	} else if (tadurl_cmpscheme(data, data_len, "ttp", 3) == 0) {
		len = data_len + sizeof(TC)*1;
		str = malloc(len);
		if (str == NULL) {
			return -1; /* TODO */
		}
		memcpy(str + 1, data, data_len);
		str[0] = TK_h;
	} else if (tadurl_cmpscheme(data, data_len, "sssp", 4) == 0) {
		len = data_len;
		str = malloc(len);
		if (str == NULL) {
			return -1; /* TODO */
		}
		memcpy(str + 4, data + 8, data_len - 8);
		str[0] = TK_h;
		str[1] = TK_t;
		str[2] = TK_t;
		str[3] = TK_p;
	} else {
		return -1; /* TODO */
	}

	*url = str;
	*url_len = len/sizeof(TC);

	return 0;
}

LOCAL VOID bchan_separete_bbs_url(UB *url, W url_len, UB **host, W *host_len, UB **board, W *board_len, UB **thread, W *thread_len)
{
	W i,n_slsh = 0;

	for (i=0; i < url_len; i++) {
		if (n_slsh == 2) {
			break;
		}
		if (url[i] == '/') {
			n_slsh++;
		}
	}
	*host = url + i;
	*host_len = 0;
	for (; i < url_len; i++) {
		if (n_slsh == 3) {
			break;
		}
		if (url[i] == '/') {
			n_slsh++;
		} else {
			(*host_len)++;
		}
	}
	for (; i < url_len; i++) {
		if (n_slsh == 5) {
			break;
		}
		if (url[i] == '/') {
			n_slsh++;
		}
	}
	*board = url + i;
	*board_len = 0;
	for (; i < url_len; i++) {
		if (n_slsh == 6) {
			break;
		}
		if (url[i] == '/') {
			n_slsh++;
		} else {
			(*board_len)++;
		}
	}
	*thread = url + i;
	*thread_len = 0;
	for (; i < url_len; i++) {
		if (n_slsh == 7) {
			break;
		}
		if (url[i] == '/') {
			n_slsh++;
		} else {
			(*thread_len)++;
		}
	}
}

#define BCHAN_CREATEVOBJ_CANCELED 0
#define BCHAN_CREATEVOBJ_CREATED 1

LOCAL W bchan_createvobj(UB *data, W data_len, VOBJSEG *vseg, LINK *lnk)
{
	W err, fsn_bbb_len, fsn_texedit_len, fsn_viewer_len, url_len, ascii_url_len;
	TC *url;
	void *fsn_bbb, *fsn_texedit, *fsn_viewer;
	UB *ascii_url;
	UB *host, *board, *thread;
	W host_len, board_len, thread_len;
	LTADSEG *lseg;
	Bool is_bbs;
	TC title[] = {TK_T, TK_h, TK_r, TK_e, TK_a, TK_d, TNULL}; /* tmp */

	err = dget_dtp(64, BCHAN_DBX_FFUSEN_BBB, (void**)&fsn_bbb);
	if (err < 0) {
		DP_ER("dget_dtp: BCHAN_DBX_FFUSEN_BBB", err);
		return err;
	}
	fsn_bbb_len = dget_siz((B*)fsn_bbb);
	err = dget_dtp(64, BCHAN_DBX_FFUSEN_TEXEDIT, (void**)&fsn_texedit);
	if (err < 0) {
		DP_ER("dget_dtp: BCHAN_DBX_FFUSEN_TEXEDIT", err);
		return err;
	}
	fsn_texedit_len = dget_siz((B*)fsn_texedit);
	err = dget_dtp(64, BCHAN_DBX_FFUSEN_VIEWER, (void**)&fsn_viewer);
	if (err < 0) {
		DP_ER("dget_dtp: BCHAN_DBX_FFUSEN_VIEWER", err);
		return err;
	}
	fsn_viewer_len = dget_siz((B*)fsn_viewer);

	for (;;) {
		if(((*(UH*)data) & 0xFF80) == 0xFF80) {
			lseg = (LTADSEG*)(data);
			if (lseg->len == 0xffff) {
				data += lseg->llen + 8;
				data_len -= lseg->llen + 8;
			} else {
				data += lseg->len + 4;
				data_len -= lseg->len + 4;
			}
		} else {
			break;
		}
	}

	is_bbs = bchan_is_bbs_url(data, data_len);
	if (is_bbs == True) {
		ascii_url = NULL;
		ascii_url_len = 0;
		err = sjstring_appendconvartingTCstring(&ascii_url, &ascii_url_len, (TC*)data, data_len/2);
		if (err < 0) {
			return 0;
		}

		bchan_separete_bbs_url(ascii_url, ascii_url_len, &host, &host_len, &board, &board_len, &thread, &thread_len);
		err = bchan_createviewervobj(title, fsn_viewer, fsn_viewer_len, host, host_len, board, board_len, thread, thread_len, vseg, lnk);
		if (err < 0) {
			return 0;
		}

		free(ascii_url);
	} else {
		err = bchan_createvobj_allocate_url(data, data_len, &url, &url_len);
		if (err < 0) {
			DP_ER("bchan_createvobj_allocate_url", err);
			return err;
		}

		err = bchan_createbbbvobj(fsn_bbb, fsn_bbb_len, fsn_texedit, fsn_texedit_len, (UB*)url, url_len*2, vseg, lnk);
		if (err < 0) {
			DP_ER("bchan_createbbbvobj", err);
			return err;
		}

		free(url);
	}

	return BCHAN_CREATEVOBJ_CREATED;
}

LOCAL VOID bchan_scrollbyahcnor(bchan_t *bchan, UB *data, W data_len)
{
	LTADSEG *lseg;
	W num, len;
	TC *str;
	W fnd;
	W cl, ct, cr, cb, tl, tt, tr, tb;

	for (;;) {
		if(((*(UH*)data) & 0xFF80) == 0xFF80) {
			lseg = (LTADSEG*)(data);
			if (lseg->len == 0xffff) {
				data += lseg->llen + 8;
				data_len -= lseg->llen + 8;
			} else {
				data += lseg->len + 4;
				data_len -= lseg->len + 4;
			}
		} else {
			break;
		}
	}

	str = (TC*)data;
	len = data_len;

	num = tc_atoi(str + 2);
	DP(("num = %d\n", num));

	fnd = datlayout_getthreadviewrectbyindex(bchan->layout, num - 1, &tl, &tt, &tr, &tb);
	if (fnd == 0) {
		return;
	}
	datrender_getviewrect(bchan->render, &cl, &ct, &cr, &cb);
	datwindow_scrollbyvalue(bchan->window, 0 - cl, tt - ct);
}

LOCAL VOID bchan_layout_res_updateattrbyindex(datlayout_res_t *layout_res, UW attr, COLOR color)
{
	if ((attr & DATCACHE_RESINDEXDATA_FLAG_NG) != 0) {
		datlayout_res_enableindexNG(layout_res);
	} else {
		datlayout_res_disableindexNG(layout_res);
	}
	if ((attr & DATCACHE_RESINDEXDATA_FLAG_COLOR) != 0) {
		datlayout_res_enableindexcolor(layout_res);
	} else {
		datlayout_res_disableindexcolor(layout_res);
	}
	datlayout_res_setindexcolor(layout_res, color);
}

LOCAL VOID bchan_layout_res_updateattrbyid(datlayout_res_t *layout_res, UW attr, COLOR color)
{
	if ((attr & DATCACHE_RESIDDATA_FLAG_NG) != 0) {
		datlayout_res_enableidNG(layout_res);
	} else {
		datlayout_res_disableidNG(layout_res);
	}
	if ((attr & DATCACHE_RESIDDATA_FLAG_COLOR) != 0) {
		datlayout_res_enableidcolor(layout_res);
	} else {
		datlayout_res_disableidcolor(layout_res);
	}
	datlayout_res_setidcolor(layout_res, color);
}

LOCAL VOID bchan_butdn_updatedisplayinfobyindex(bchan_t *bchan, W resindex, UW attr, COLOR color)
{
	datlayout_res_t *layout_res;
	Bool found;

	found = datlayoutarray_getresbyindex(bchan->layoutarray, resindex, &layout_res);
	if (found == False) {
		return;
	}

	bchan_layout_res_updateattrbyindex(layout_res, attr, color);

	datwindow_requestredisp(bchan->window);
}

LOCAL VOID bchan_butdn_updatedisplayinfobyid(bchan_t *bchan, TC *id, W id_len, UW attr, COLOR color)
{
	datlayout_res_t *layout_res;
	W i, len;
	Bool found, issame;

	len = datlayoutarray_length(bchan->layoutarray);
	for (i = 0; i < len; i++) {
		found = datlayoutarray_getresbyindex(bchan->layoutarray, i, &layout_res);
		if (found == False) {
			break;
		}

		issame = datlayout_res_issameid(layout_res, id, id_len);
		if (issame != True) {
			continue;
		}

		bchan_layout_res_updateattrbyid(layout_res, attr, color);
	}

	datwindow_requestredisp(bchan->window);
}

LOCAL VOID bchan_butdn_pressnumber(bchan_t *bchan, PNT evpos, W resindex)
{
	W size, err;
	PNT pos;
	B *data;
	COLOR color;
	UW attr = 0;
	Bool ok;
	datlayout_res_t *layout_res;

	DP(("press DATRENDER_FINDACTION_TYPE_NUMBER: %d\n", resindex + 1));

	ok = datlayoutarray_getresbyindex(bchan->layoutarray, resindex, &layout_res);
	if (ok == False) {
		return;
	}

	ok = datlayout_res_isenableidNG(layout_res);
	if (ok == True) {
		return;
	}

	err = datcache_searchresindexdata(bchan->cache, resindex, &attr, &color);
	if (err == DATCACHE_SEARCHRESINDEXDATA_FOUND) {
		if ((attr & DATCACHE_RESINDEXDATA_FLAG_NG) != 0) {
			err = bchan_resnumbermenu_setngselected(&bchan->resnumbermenu, True);
		} else {
			err = bchan_resnumbermenu_setngselected(&bchan->resnumbermenu, False);
		}
	} else {
		attr = 0;
		err = bchan_resnumbermenu_setngselected(&bchan->resnumbermenu, False);
	}

	pos.x = evpos.x;
	pos.y = evpos.y;
	gcnv_abs(bchan->gid, &pos);
	err = bchan_resnumbermenu_select(&bchan->resnumbermenu, pos);
	if (err == BCHAN_RESNUMBERMENU_SELECT_PUSHTRAY) {
		size = dattraydata_resindextotraytextdata(bchan->traydata, resindex, NULL, 0);
		data = malloc(size);
		if (data == NULL) {
			return;
		}
		dattraydata_resindextotraytextdata(bchan->traydata, resindex, data, size);
		bchan_pushstringtotray((TC*)data, size/2);
		free(data);
	} if (err == BCHAN_RESNUMBERMENU_SELECT_NG) {
		if ((attr & DATCACHE_RESINDEXDATA_FLAG_NG) != 0) {
			datcache_removeresindexdata(bchan->cache, resindex);
			attr = 0;
		} else {
			err = datcache_addresindexdata(bchan->cache, resindex, DATCACHE_RESINDEXDATA_FLAG_NG, 0x10000000);
			attr = DATCACHE_RESINDEXDATA_FLAG_NG;
		}
		bchan_butdn_updatedisplayinfobyindex(bchan, resindex, attr, color);
	}
}

LOCAL VOID bchan_butdn_pressresheaderid(bchan_t *bchan, PNT evpos, W resindex)
{
	W id_len, size, err;
	TC *id;
	PNT pos;
	B *data;
	COLOR color;
	UW attr = 0;
	Bool ok;
	datlayout_res_t *layout_res;

	DP(("press DATRENDER_FINDACTION_TYPE_ID\n"));

	ok = datlayoutarray_getresbyindex(bchan->layoutarray, resindex, &layout_res);
	if (ok == False) {
		DP(("      id is not exist\n"));
		return;
	}

	datlayout_res_getid(layout_res, &id, &id_len);
	if (id == NULL) {
		DP(("      id is not exist\n"));
		return;
	}

	ok = datlayout_res_isenableindexNG(layout_res);
	if (ok == True) {
		return;
	}

	err = datcache_searchresiddata(bchan->cache, id, id_len, &attr, &color);
	if (err == DATCACHE_SEARCHRESIDDATA_FOUND) {
		if ((attr & DATCACHE_RESIDDATA_FLAG_NG) != 0) {
			err = bchan_residmenu_setngselected(&bchan->residmenu, True);
		} else {
			err = bchan_residmenu_setngselected(&bchan->residmenu, False);
		}
	} else {
		attr = 0;
		err = bchan_residmenu_setngselected(&bchan->residmenu, False);
	}

	pos.x = evpos.x;
	pos.y = evpos.y;
	gcnv_abs(bchan->gid, &pos);
	err = bchan_residmenu_select(&bchan->residmenu, pos);
	if (err == BCHAN_RESIDMENU_SELECT_PUSHTRAY) {
		size = dattraydata_idtotraytextdata(bchan->traydata, id, id_len, NULL, 0);
		data = malloc(size);
		if (data == NULL) {
			return;
		}
		dattraydata_idtotraytextdata(bchan->traydata, id, id_len, data, size);
		bchan_pushstringtotray((TC*)data, size/2);
		free(data);
	} if (err == BCHAN_RESIDMENU_SELECT_NG) {
		if ((attr & DATCACHE_RESIDDATA_FLAG_NG) != 0) {
			datcache_removeresiddata(bchan->cache, id, id_len);
			attr = 0;
		} else {
			err = datcache_addresiddata(bchan->cache, id, id_len, DATCACHE_RESIDDATA_FLAG_NG, 0x10000000);
			attr = DATCACHE_RESIDDATA_FLAG_NG;
		}
		bchan_butdn_updatedisplayinfobyid(bchan, id, id_len, attr, color);
	}
}

LOCAL VOID bchan_butdn(bchan_t *bchan, W dck, PNT evpos)
{
	W fnd, event_type, type, len, dx, dy, err, size, resindex;
	WID wid_butup;
	GID gid;
	RECT r, r0;
	UB *start;
	PNT p1, pos, pos_butup;
	TR_VOBJREC vrec;
	TRAYREC tr_rec;
	WEVENT paste_ev;
	SEL_RGN	sel;

	switch (dck) {
	case	W_CLICK:
		fnd = datrender_findaction(bchan->render, evpos, &r, &type, &start, &len, &resindex);
		if (fnd == 0) {
			return;
		}
		if (type != DATRENDER_FINDACTION_TYPE_ANCHOR) {
			return;
		}
		bchan_scrollbyahcnor(bchan, start, len);
		return;
	case	W_DCLICK:
	case	W_QPRESS:
	default:
		return;
	case	W_PRESS:
	}

	fnd = datrender_findaction(bchan->render, evpos, &r, &type, &start, &len, &resindex);
	if (fnd == 0) {
		return;
	}
	if (type == DATRENDER_FINDACTION_TYPE_NUMBER) {
		bchan_butdn_pressnumber(bchan, evpos, resindex);
		return;
	}
	if (type == DATRENDER_FINDACTION_TYPE_RESID) {
		bchan_butdn_pressresheaderid(bchan, evpos, resindex);
		return;
	}
	if (type != DATRENDER_FINDACTION_TYPE_URL) {
		return;
	}

	gid = datwindow_startdrag(bchan->window);
	if (gid < 0) {
		DP_ER("wsta_drg error:", gid);
		return;
	}

	gget_fra(gid, &r0);
	gset_vis(gid, r0);

	dx = r.c.left - evpos.x;
	dy = r.c.top - evpos.y;

	p1 = evpos;
	sel.sts = 0;
	sel.rgn.r.c.left = r.c.left;
	sel.rgn.r.c.top = r.c.top;
	sel.rgn.r.c.right = r.c.right;
	sel.rgn.r.c.bottom = r.c.bottom;
	adsp_sel(gid, &sel, 1);

	gset_ptr(PS_GRIP, NULL, -1, -1);
	for (;;) {
		event_type = datwindow_getdrag(bchan->window, &pos, &wid_butup, &pos_butup);
		if (event_type == EV_BUTUP) {
			break;
		}
		if (event_type != EV_NULL) {
			continue;
		}
		if ((pos.x == p1.x)&&(pos.y == p1.y)) {
			continue;
		}
		adsp_sel(gid, &sel, 0);
		sel.rgn.r.c.left += pos.x - p1.x;
		sel.rgn.r.c.top += pos.y - p1.y;
		sel.rgn.r.c.right += pos.x - p1.x;
		sel.rgn.r.c.bottom += pos.y - p1.y;
		adsp_sel(gid, &sel, 1);
		p1 = pos;
	}
	gset_ptr(PS_SELECT, NULL, -1, -1);
	adsp_sel(gid, &sel, 0);
	datwindow_enddrag(bchan->window);

	/* BUTUP on self window or no window or system message panel */
	if ((wid_butup == bchan->wid)||(wid_butup == 0)||(wid_butup == -1)) {
		return;
	}

	err = oget_vob(-wid_butup, &vrec.vlnk, NULL, 0, &size);
	if (err < 0) {
		return;
	}

	err = bchan_createvobj(start, len, &vrec.vseg, (LINK*)&vrec.vlnk);
	if (err < 0) {
		DP_ER("bchan_createvobj error", err);
		return;
	}
	if (err == BCHAN_CREATEVOBJ_CANCELED) {
		DP(("canceled\n"));
		return;
	}

	tr_rec.id = TR_VOBJ;
	tr_rec.len = sizeof(TR_VOBJREC);
	tr_rec.dt = (B*)&vrec;
	err = tset_dat(&tr_rec, 1);
	if (err < 0) {
		err = del_fil(NULL, (LINK*)&vrec.vlnk, 0);
		if (err < 0) {
			DP_ER("error del_fil:", err);
		}
		return;
	}

	paste_ev.r.type = EV_REQUEST;
	paste_ev.r.r.p.rightbot.x = pos_butup.x + dx;
	paste_ev.r.r.p.rightbot.y = pos_butup.y + dy;
	paste_ev.r.cmd = W_PASTE;
	paste_ev.r.wid = wid_butup;
	err = wsnd_evt(&paste_ev);
	if (err < 0) {
		tset_dat(NULL, 0);
		err = del_fil(NULL, (LINK*)&vrec.vlnk, 0);
		if (err < 0) {
			DP_ER("error del_fil:", err);
		}
		return;
	}
	err = wwai_rsp(NULL, W_PASTE, 60000);
	if (err != W_ACK) {
		tset_dat(NULL, 0);
		err = del_fil(NULL, (LINK*)&vrec.vlnk, 0);
		if (err < 0) {
			DP_ER("error del_fil:", err);
		}
	}

	wswi_wnd(wid_butup, NULL);

	/* temporary fix. to fix canceling layout. */
	req_tmg(0, BCHAN_MESSAGE_RETRIEVER_UPDATE);
}

LOCAL VOID bchan_paste(bchan_t *bchan)
{
	W err;
	PNT p;
	postresdata_t *post = NULL;

	err = poptray_gettraydata(&post);
	if (err < 0) {
		datwindow_responsepasterequest(bchan->window, /* NACK */ 1, NULL);
		return;
	}

	p.x = 0x8000;
	p.y = 0x8000;
	datwindow_responsepasterequest(bchan->window, /* ACK */ 0, &p);

	if (post != NULL) {
		if (bchan->resdata != NULL) {
			postresdata_delete(bchan->resdata);
		}
		bchan->resdata = post;
		cfrmwindow_setpostresdata(bchan->confirm, post);
		bchan->request_confirm_open = True;
	}
}

LOCAL VOID bchan_recieveclose(bchan_t *bchan, Bool send)
{
	TC *dmsg = NULL;
	W dmsg_len, err;

	DP(("bchan_recieveclose = %d\n", send));
	if (send == True) {
		bchan_hmistate_updateptrstyle(&bchan->hmistate, PS_BUSY);
		err = ressubmit_respost(bchan->submit, bchan->resdata, &dmsg, &dmsg_len);
		bchan_hmistate_updateptrstyle(&bchan->hmistate, PS_SELECT);
		switch (err) {
		case RESSUBMIT_RESPOST_SUCCEED:
			pdsp_msg(bchan->hmistate.msg_postsucceed);
			break;
		case RESSUBMIT_RESPOST_DENIED:
			bchan_hmistate_display_postdenied(&bchan->hmistate, dmsg, dmsg_len);
			break;
		case RESSUBMIT_RESPOST_ERROR_CLIENT:
		case RESSUBMIT_RESPOST_ERROR_STATUS:
		case RESSUBMIT_RESPOST_ERROR_CONTENT:
		default:
			pdsp_msg(bchan->hmistate.msg_posterror);
			break;
		}
		if (dmsg != NULL) {
			free(dmsg);
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
	err = dget_dtp(TEXT_DATA, BCHAN_DBX_MSGTEXT_NETWORKERROR, (void**)&hmistate->msg_networkerror);
	if (err < 0) {
		DP_ER("dget_dtp: message network error error", err);
		hmistate->msg_networkerror = NULL;
	}
	err = dget_dtp(TEXT_DATA, BCHAN_DBX_MSGTEXT_NOTFOUND, (void**)&hmistate->msg_notfound);
	if (err < 0) {
		DP_ER("dget_dtp: message notfound error", err);
		hmistate->msg_notfound = NULL;
	}
	err = dget_dtp(TEXT_DATA, BCHAN_DBX_MSGTEXT_CANTRETRIEVE, (void**)&hmistate->msg_cantretrieve);
	if (err < 0) {
		DP_ER("dget_dtp: message cantretrieve error", err);
		hmistate->msg_cantretrieve = NULL;
	}
}

LOCAL W bchan_initialize(bchan_t *bchan, VID vid, WID wid/*tmp*/, W exectype, dathmi_t *hmi, datwindow_t *datwindow, cfrmwindow_t *cfrmwindow, ngwordwindow_t *ngwordwindow)
{
	GID gid;
	datcache_t *cache;
	datparser_t *parser;
	datlayoutarray_t *layoutarray;
	datlayout_t *layout;
	datrender_t *render;
	dattraydata_t *traydata;
	datretriever_t *retriever;
	ressubmit_t *submit;
	RECT w_work;
	W err;

	gid = datwindow_getGID(datwindow);

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
	layoutarray = datlayoutarray_new();
	if (layoutarray == NULL) {
		DP_ER("datlayoutarray_new error", 0);
		goto error_layoutarray;
	}
	datlayoutstyle_setdefault(&bchan->style);
	layout = datlayout_new(gid, &bchan->style, layoutarray);
	if (layout == NULL) {
		DP_ER("datlayout_new error", 0);
		goto error_layout;
	}
	render = datrender_new(gid, &bchan->style, layoutarray);
	if (render == NULL) {
		DP_ER("datrender_new error", 0);
		goto error_render;
	}
	traydata = dattraydata_new(layoutarray);
	if (traydata == NULL) {
		DP_ER("dattraydata_new error", 0);
		goto error_traydata;
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
	err = bchan_mainmenu_initialize(&bchan->mainmenu, BCHAN_DBX_MENU_TEST);
	if (err < 0) {
		DP_ER("bchan_mainmenu_initialize error", err);
		goto error_mainmenu_initialize;
	}
	err = bchan_resnumbermenu_initialize(&bchan->resnumbermenu, BCHAN_DBX_GMENU_RESNUMBER);
	if (err < 0) {
		DP_ER("bchan_resnumbermenu_initialize", err);
		goto error_resnumbermenu_initialize;
	}
	err = bchan_residmenu_initialize(&bchan->residmenu, BCHAN_DBX_GMENU_RESID);
	if (err < 0) {
		DP_ER("bchan_residmenu_initialize", err);
		goto error_residmenu_initialize;
	}

	bchan_hmistate_initialize(&bchan->hmistate);

	datwindow_getworkrect(datwindow, &w_work);
	datrender_setviewrect(render, 0, 0, w_work.c.right, w_work.c.bottom);
	datwindow_setworkrect(datwindow, 0, 0, w_work.c.right, w_work.c.bottom);

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
	bchan->layoutarray = layoutarray;
	bchan->layout = layout;
	bchan->render = render;
	bchan->traydata = traydata;
	bchan->hmi = hmi;
	bchan->window = datwindow;
	bchan->retriever = retriever;
	bchan->submit = submit;
	bchan->confirm = cfrmwindow;
	bchan->resdata = NULL;
	bchan->ngword = ngwordwindow;

	return 0;

error_residmenu_initialize:
	bchan_resnumbermenu_finalize(&bchan->resnumbermenu);
error_resnumbermenu_initialize:
	bchan_mainmenu_finalize(&bchan->mainmenu);
error_mainmenu_initialize:
	ressubmit_delete(submit);
error_submit:
	datretriever_delete(retriever);
error_retriever:
	dattraydata_delete(traydata);
error_traydata:
	datrender_delete(render);
error_render:
	datlayout_delete(layout);
error_layout:
	datlayoutarray_delete(layoutarray);
error_layoutarray:
	datparser_delete(parser);
error_parser:
	datcache_delete(cache);
error_cache:
	return -1; /* TODO */
}

#define BCHAN_LAYOUT_MAXBLOCKING 20

LOCAL W bchan_layout_appendres(bchan_t *bchan, datparser_res_t *res)
{
	W err, ret, index, id_len;
	TC *id;
	UW attr;
	COLOR color;
	Bool found;
	datlayout_res_t *layout_res;

	err = datlayout_appendres(bchan->layout, res);
	if (err < 0) {
		return err;
	}

	found = datlayoutarray_getreslast(bchan->layoutarray, &layout_res);
	if (found == False) {
		return 0;
	}
	index = datlayoutarray_length(bchan->layoutarray) - 1;

	datlayout_res_getid(layout_res, &id, &id_len);
	ret = datcache_searchresiddata(bchan->cache, id, id_len, &attr, &color);
	if (ret == DATCACHE_SEARCHRESIDDATA_FOUND) {
		bchan_layout_res_updateattrbyid(layout_res, attr, color);
	}
	ret = datcache_searchresindexdata(bchan->cache, index, &attr, &color);
	if (ret == DATCACHE_SEARCHRESINDEXDATA_FOUND) {
		bchan_layout_res_updateattrbyindex(layout_res, attr, color);
	}

	return 0;
}

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
		bchan_layout_appendres(bchan, res);
	}

	datwindow_getworkrect(bchan->window, &w_work);

	datlayout_getdrawrect(bchan->layout, &l, &t, &r, &b);
	datwindow_setdrawrect(bchan->window, l, t, r, b);

	title = datlayout_gettitle(bchan->layout);
	datwindow_settitle(bchan->window, title);

	datwindow_requestredisp(bchan->window);
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
		bchan_layout_appendres(bchan, res);
	}

	datwindow_getworkrect(bchan->window, &w_work);

	datlayout_getdrawrect(bchan->layout, &l, &t, &r, &b);
	datwindow_setdrawrect(bchan->window, l, t, r, b);

	datwindow_requestredisp(bchan->window);
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
		case DATRETRIEVER_REQUEST_NOT_FOUND:
			req_tmg(0, BCHAN_MESSAGE_RETRIEVER_NOTFOUND);
			break;
		case DATRETRIEVER_REQUEST_UNEXPECTED:
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
	if (datretriever_isenablenetwork(bchan->retriever) == False) {
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

	if (datretriever_isenablenetwork(bchan->retriever) == False) {
		pdsp_msg(bchan->hmistate.msg_cantretrieve);
		return 0;
	}
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

LOCAL VOID keydwn(bchan_t *bchan, UH keycode, TC ch, UW stat)
{
	W l,t,r,b,l1,t1,r1,b1,scr;

	switch (ch) {
	case KC_CC_U:
		datrender_getviewrect(bchan->render, &l, &t, &r, &b);
		if (t < 16) {
			scr = -t;
		} else {
			scr = -16;
		}
		datwindow_scrollbyvalue(bchan->window, 0, scr);
		break;
	case KC_CC_D:
		datrender_getviewrect(bchan->render, &l, &t, &r, &b);
		datlayout_getdrawrect(bchan->layout, &l1, &t1, &r1, &b1);
		if (b + 16 > b1) {
			scr = b1 - b;
		} else {
			scr = 16;
		}
		if (scr > 0) {
			datwindow_scrollbyvalue(bchan->window, 0, scr);
		}
		break;
	case KC_CC_R:
		datrender_getviewrect(bchan->render, &l, &t, &r, &b);
		datlayout_getdrawrect(bchan->layout, &l1, &t1, &r1, &b1);
		if (r + 16 > r1) {
			scr = r1 - r;
		} else {
			scr = 16;
		}
		if (scr > 0) {
			datwindow_scrollbyvalue(bchan->window, scr, 0);
		}
		break;
	case KC_CC_L:
		datrender_getviewrect(bchan->render, &l, &t, &r, &b);
		if (l < 16) {
			scr = -l;
		} else {
			scr = -16;
		}
		datwindow_scrollbyvalue(bchan->window, scr, 0);
		break;
	case KC_PG_U:
		datrender_getviewrect(bchan->render, &l, &t, &r, &b);
		if (t < b - t) {
			scr = -t;
		} else {
			scr = - (b - t);
		}
		datwindow_scrollbyvalue(bchan->window, 0, scr);
		break;
	case KC_PG_D:
		datrender_getviewrect(bchan->render, &l, &t, &r, &b);
		datlayout_getdrawrect(bchan->layout, &l1, &t1, &r1, &b1);
		if (b + (b - t) > b1) {
			scr = b1 - b;
		} else {
			scr = (b - t);
		}
		if (scr > 0) {
			datwindow_scrollbyvalue(bchan->window, 0, scr);
		}
		break;
	case KC_PG_R:
		datrender_getviewrect(bchan->render, &l, &t, &r, &b);
		datlayout_getdrawrect(bchan->layout, &l1, &t1, &r1, &b1);
		if (r + (r - l) > r1) {
			scr = r1 - r;
		} else {
			scr = (r - l);
		}
		if (scr > 0) {
			datwindow_scrollbyvalue(bchan->window, scr, 0);
		}
		break;
	case KC_PG_L:
		datrender_getviewrect(bchan->render, &l, &t, &r, &b);
		if (l < r - l) {
			scr = -l;
		} else {
			scr = - (r - l);
		}
		datwindow_scrollbyvalue(bchan->window, scr, 0);
		break;
	case KC_PF5:
		bchan_networkrequest(bchan);
		break;
	case TK_E: /* temporary */
		if (stat & ES_CMD) {
			killme(bchan);
		}
		break;
	}
}

LOCAL VOID bchan_setupmenu(bchan_t *bchan)
{
	TC *str;
	Bool titleenable, networkenable;

	str = datlayout_gettitle(bchan->layout);
	if (str == NULL) {
		titleenable = False;
	} else {
		titleenable = True;
	}

	networkenable = datretriever_isenablenetwork(bchan->retriever);

	bchan_mainmenu_setup(&bchan->mainmenu, titleenable, networkenable);
}

LOCAL VOID bchan_selectmenu(bchan_t *bchan, W sel)
{
	UB *host, *board, *thread;
	W host_len, board_len, thread_len;

	switch (sel) {
	case BCHAN_MAINMENU_SELECT_CLOSE: /* [終了] */
		killme(bchan);
		break;
	case BCHAN_MAINMENU_SELECT_REDISPLAY: /* [再表示] */
		datwindow_requestredisp(bchan->window);
		break;
	case BCHAN_MAINMENU_SELECT_THREADINFO: /* [スレッド情報を表示] */
		datcache_gethost(bchan->cache, &host, &host_len);
		datcache_getborad(bchan->cache, &board, &board_len);
		datcache_getthread(bchan->cache, &thread, &thread_len);
		bchan_panels_threadinfo(host, host_len, board, board_len, thread, thread_len);
		break;
	case BCHAN_MAINMENU_SELECT_TITLETOTRAY:	/* [スレタイをトレーに複写] */
		bchan_pushthreadtitle(bchan);
		break;
	case BCHAN_MAINMENU_SELECT_URLTOTRAY: /* [スレッドＵＲＬをトレーに複写] */
		bchan_pushthreadurl(bchan);
		break;
	case BCHAN_MAINMENU_SELECT_NGWORD: /* [ＮＧワード設定] */
		ngwordwindow_open(bchan->ngword);
		break;
	case BCHAN_MAINMENU_SELECT_THREADFETCH:	/* [スレッド取得] */
		bchan_networkrequest(bchan);
		break;
	}
	return;
}

LOCAL VOID bchan_popupmenu(bchan_t *bchan, PNT pos)
{
	W sel;

	bchan_setupmenu(bchan);
	gset_ptr(PS_SELECT, NULL, -1, -1);
	sel = bchan_mainmenu_popup(&bchan->mainmenu, pos);
	if (sel > 0) {
		bchan_selectmenu(bchan, sel);
	}
}

LOCAL VOID bchan_handletimeout(bchan_t *bchan, W code)
{
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
	case BCHAN_MESSAGE_RETRIEVER_NOTFOUND:
		bchan_hmistate_updateptrstyle(&bchan->hmistate, PS_SELECT);
		pdsp_msg(bchan->hmistate.msg_notfound);
		break;
	case BCHAN_MESSAGE_RETRIEVER_ERROR:
		bchan_hmistate_updateptrstyle(&bchan->hmistate, PS_SELECT);
		pdsp_msg(bchan->hmistate.msg_networkerror);
		break;
	}
}

LOCAL VOID bchan_eventdispatch(bchan_t *bchan, dathmi_t *hmi)
{
	W sel, err;
	dathmievent_t *evt;

	err = dathmi_getevent(hmi, &evt);
	if (err < 0) {
		/* TODO: error handling or ext_prc ? */
		return;
	}

	switch (evt->type) {
	case DATHMIEVENT_TYPE_COMMON_MOUSEMOVE:
		break;
	case DATHMIEVENT_TYPE_COMMON_KEYDOWN:
		if (evt->data.common_keydown.stat & ES_CMD) {	/*命令キー*/
			bchan_setupmenu(bchan);
			sel = bchan_mainmenu_keyselect(&bchan->mainmenu, evt->data.common_keydown.keycode);
			if (sel > 0) {
				bchan_selectmenu(bchan, sel);
				break;
			}
		}
		keydwn(bchan, evt->data.common_keydown.keytop, evt->data.common_keydown.keycode, evt->data.common_keydown.stat);
		break;
	case DATHMIEVENT_TYPE_COMMON_MENU:
		bchan_popupmenu(bchan, evt->data.common_menu.pos);
		break;
	case DATHMIEVENT_TYPE_COMMON_TIMEOUT:
		bchan_handletimeout(bchan, evt->data.common_timeout.code);
		break;
	case DATHMIEVENT_TYPE_THREAD_DRAW:
		bchan_draw(bchan);
		break;
	case DATHMIEVENT_TYPE_THREAD_RESIZE:
		bchan_resize(bchan, evt->data.main_resize.work_sz);
		break;
	case DATHMIEVENT_TYPE_THREAD_CLOSE:
		bchan_close(bchan, evt->data.main_close.save);
		break;
	case DATHMIEVENT_TYPE_THREAD_BUTDN:
		bchan_butdn(bchan, evt->data.main_butdn.type, evt->data.main_butdn.pos);
		break;
	case DATHMIEVENT_TYPE_THREAD_PASTE:
		bchan_paste(bchan);
		break;
	case DATHMIEVENT_TYPE_THREAD_SWITCH:
		if (bchan->request_confirm_open == True) {
			cfrmwindow_open(bchan->confirm);
			bchan->request_confirm_open = False;
		}
		if (evt->data.main_switch.needdraw == True) {
			bchan_draw(bchan);
		}
		break;
	case DATHMIEVENT_TYPE_THREAD_MOUSEMOVE:
		gset_ptr(bchan->hmistate.ptr, NULL, -1, -1);
		break;
	case DATHMIEVENT_TYPE_CONFIRM_CLOSE:
		bchan_recieveclose(bchan, evt->data.confirm_close.send);
		break;
	case DATHMIEVENT_TYPE_NGWORD_CLOSE:
	case DATHMIEVENT_TYPE_NONE:
	default:
		break;
	}
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
	static	RECT	r1 = {{200, 80, 500+7, 230+30}};
	static	RECT	r2 = {{200, 80, 500+7, 230+30}};
	static	TC	tit0[21];
	static	PAT	pat0 = {{
		0,
		16, 16,
		0x10efefef,
		0x10efefef,
		FILL100
	}};
	W	err, size;
	VID vid;
	LINK lnk, dbx;
	CLI_arg arg;
	bchan_t bchan;
	dathmi_t *hmi;
	datwindow_t *window;
	cfrmwindow_t *cfrmwindow;
	ngwordwindow_t *ngwordwindow;
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

	hmi = dathmi_new();
	if (hmi == NULL) {
		ext_prc(0);
	}
	window = dathmi_newmainwindow(hmi, &r0, tit0, &pat0, bchan_scroll, &bchan/* Ugh! */);
	if (window == NULL) {
		DP_ER("dathmi_newmainwindow error:", 0);
		dathmi_delete(hmi);
		ext_prc(0);
	}
	cfrmwindow = dathmi_newconfirmwindow(hmi, &r1, BCHAN_DBX_TEXT_CONFIRM_TITLE, BCHAN_DBX_MS_CONFIRM_POST, BCHAN_DBX_MS_CONFIRM_CANCEL);
	if (cfrmwindow == NULL) {
		DP_ER("dathmi_newconfirmwindow error:", 0);
		dathmi_deletemainwindow(hmi, window);
		dathmi_delete(hmi);
		ext_prc(0);
	}
	ngwordwindow = dathmi_newngwordwindow(hmi, &r2, BCHAN_DBX_SS_NGWORD_LIST, BCHAN_DBX_MS_NGWORD_DELETE, BCHAN_DBX_TB_NGWORD_APPEND, BCHAN_DBX_MS_NGWORD_APPEND);
	if (ngwordwindow == NULL) {
		DP_ER("dathmi_newngwordwindow error:", 0);
		dathmi_deleteconfirmwindow(hmi, cfrmwindow);
		dathmi_deletemainwindow(hmi, window);
		dathmi_delete(hmi);
		ext_prc(0);
	}
	
	err = bchan_initialize(&bchan, vid, datwindow_getWID(window), msg->msg_type, hmi, window, cfrmwindow, ngwordwindow);
	if (err < 0) {
		DP_ER("bchan_initialize error", err);
		ext_prc(0);
	}

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

	datwindow_requestredisp(window);

	/*イベントループ*/
	for (;;) {
		bchan_eventdispatch(&bchan, hmi);
	}

	return 0;
}
