/*
 * bchan_menus.c
 *
 * Copyright (c) 2010-2011 project bchan
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

#include    "bchan_menus.h"

#include	<basic.h>
#include	<bstdio.h>
#include	<bstdlib.h>
#include	<btron/hmi.h>
#include	<btron/vobj.h>

#ifdef BCHAN_CONFIG_DEBUG
# define DP(arg) printf arg
# define DP_ER(msg, err) printf("%s (%d/%x)\n", msg, err>>16, err)
#else
# define DP(arg) /**/
# define DP_ER(msg, err) /**/
#endif

#define BCHAN_MAINMENU_ITEMNUM_WINDOW 4
#define BCHAN_MAINMENU_ITEMNUM_GADGET (BCHAN_MAINMENU_ITEMNUM_WINDOW + 1)

EXPORT W bchan_mainmenu_setup(bchan_mainmenu_t *mainmenu, Bool titleenable, Bool networkenable, Bool ngwindowenable)
{
	/* [操作] -> [スレタイをトレーに複写] */
	if (titleenable == False) {
		mchg_atr(mainmenu->mnid, (2 << 8)|1, M_INACT);
	} else {
		mchg_atr(mainmenu->mnid, (2 << 8)|1, M_ACT);
	}

	/* [表示] -> [スレッド情報を表示] */
	/* [編集] -> [スレッドＵＲＬをトレーに複写] */
	/* [操作] -> [スレッド取得] */
	if (networkenable == False) {
		mchg_atr(mainmenu->mnid, (1 << 8)|2, M_INACT);
		mchg_atr(mainmenu->mnid, (2 << 8)|2, M_INACT);
		mchg_atr(mainmenu->mnid, (3 << 8)|1, M_INACT);
	} else {
		mchg_atr(mainmenu->mnid, (1 << 8)|2, M_ACT);
		mchg_atr(mainmenu->mnid, (2 << 8)|2, M_ACT);
		mchg_atr(mainmenu->mnid, (3 << 8)|1, M_ACT);
	}

	/* [編集] -> [ＮＧワード設定] */
	if (ngwindowenable == False) {
		mchg_atr(mainmenu->mnid, (2 << 8)|3, M_NOSEL);
	} else {
		mchg_atr(mainmenu->mnid, (2 << 8)|3, M_SEL);
	}

	wget_dmn(&(mainmenu->mnitem[BCHAN_MAINMENU_ITEMNUM_WINDOW].ptr));
	mset_itm(mainmenu->mnid, BCHAN_MAINMENU_ITEMNUM_WINDOW, mainmenu->mnitem+BCHAN_MAINMENU_ITEMNUM_WINDOW);
	oget_men(0, NULL, &(mainmenu->mnitem[BCHAN_MAINMENU_ITEMNUM_GADGET].ptr), NULL, NULL);
	mset_itm(mainmenu->mnid, BCHAN_MAINMENU_ITEMNUM_GADGET, mainmenu->mnitem+BCHAN_MAINMENU_ITEMNUM_GADGET);

	return 0; /* tmp */
}

LOCAL W bchan_mainmenu_select(bchan_mainmenu_t *mainmenu, W i)
{
	W ret;

	switch(i >> 8) {
	case 0: /* [終了] */
		ret = BCHAN_MAINMENU_SELECT_CLOSE;
		break;
	case 1: /* [表示] */
		switch(i & 0xff) {
		case 1: /* [再表示] */
			ret = BCHAN_MAINMENU_SELECT_REDISPLAY;
			break;
		case 2: /* [スレッド情報を表示] */
			ret = BCHAN_MAINMENU_SELECT_THREADINFO;
			break;
		default:
			ret = BCHAN_MAINMENU_SELECT_NOSELECT;
			break;
		}
		break;
	case 2:	/* [編集] */
		switch(i & 0xff) {
		case 1: /* [スレタイをトレーに複写] */
			ret = BCHAN_MAINMENU_SELECT_TITLETOTRAY;
			break;
		case 2: /* [スレッドＵＲＬをトレーに複写] */
			ret = BCHAN_MAINMENU_SELECT_URLTOTRAY;
			break;
		case 3: /* [ＮＧワード設定] */
			ret = BCHAN_MAINMENU_SELECT_NGWORD;
			break;
		default:
			ret = BCHAN_MAINMENU_SELECT_NOSELECT;
			break;
		}
		break;
	case 3:	/* [操作] */
		switch(i & 0xff) {
		case 1: /* [スレッド取得] */
			ret = BCHAN_MAINMENU_SELECT_THREADFETCH;
			break;
		default:
			ret = BCHAN_MAINMENU_SELECT_NOSELECT;
			break;
		}
		break;
	case BCHAN_MAINMENU_ITEMNUM_WINDOW: /* [ウィンドウ] */
		wexe_dmn(i);
		ret = BCHAN_MAINMENU_SELECT_NOSELECT;
		break;
	case BCHAN_MAINMENU_ITEMNUM_GADGET: /* [小物] */
	    oexe_apg(0, i);
		ret = BCHAN_MAINMENU_SELECT_NOSELECT;
		break;
	default:
		ret = BCHAN_MAINMENU_SELECT_NOSELECT;
		break;
	}

	return ret;
}

EXPORT W bchan_mainmenu_popup(bchan_mainmenu_t *mainmenu, PNT pos)
{
	W i;
	gset_ptr(PS_SELECT, NULL, -1, -1);
	i = msel_men(mainmenu->mnid, pos);
	if (i < 0) {
		DP_ER("msel_men error:", i);
		return i;
	}
	if (i == 0) {
		return BCHAN_MAINMENU_SELECT_NOSELECT;
	}
	return bchan_mainmenu_select(mainmenu, i);
}

EXPORT W bchan_mainmenu_keyselect(bchan_mainmenu_t *mainmenu, TC keycode)
{
	W i;
	i = mfnd_key(mainmenu->mnid, keycode);
	if (i < 0) {
		DP_ER("mfnd_key error:", i);
		return i;
	}
	if (i == 0) {
		return BCHAN_MAINMENU_SELECT_NOSELECT;
	}
	return bchan_mainmenu_select(mainmenu, i);
}

EXPORT W bchan_mainmenu_initialize(bchan_mainmenu_t *mainmenu, W dnum)
{
	MENUITEM *mnitem_dbx, *mnitem;
	MNID mnid;
	W len, err;

	err = dget_dtp(8, dnum, (void**)&mnitem_dbx);
	if (err < 0) {
		DP_ER("dget_dtp error:", err);
		return err;
	}
	len = dget_siz((B*)mnitem_dbx);
	mnitem = malloc(len);
	if (mnitem == NULL) {
		DP_ER("mallod error", err);
		return -1;
	}
	memcpy(mnitem, mnitem_dbx, len);
	mnid = mcre_men(BCHAN_MAINMENU_ITEMNUM_WINDOW+2, mnitem, NULL);
	if (mnid < 0) {
		DP_ER("mcre_men error", mnid);
		free(mnitem);
		return -1;
	}

	mainmenu->mnid = mnid;
	mainmenu->mnitem = mnitem;

	return 0;
}

EXPORT VOID bchan_mainmenu_finalize(bchan_mainmenu_t *mainmenu)
{
	mdel_men(mainmenu->mnid);
	free(mainmenu->mnitem);
}

EXPORT W bchan_resnumbermenu_setngselected(bchan_resnumbermenu_t *resnumbermenu, Bool selected)
{
	W err;

	if (selected == True) {
		err = mchg_gat(resnumbermenu->mnid, 1, M_SEL);
		if (err < 0) {
			return err;
		}
		err = mchg_gat(resnumbermenu->mnid, 2, M_INACT);
	} else {
		err = mchg_gat(resnumbermenu->mnid, 1, M_NOSEL);
		if (err < 0) {
			return err;
		}
		err = mchg_gat(resnumbermenu->mnid, 2, M_ACT);
	}

	return err;
}

EXPORT W bchan_resnumbermenu_select(bchan_resnumbermenu_t *resnumbermenu, PNT pos)
{
	W err, ret;

	err = msel_gmn(resnumbermenu->mnid, pos);
	if (err < 0) {
		DP_ER("msel_gmn", err);
		return err;
	}

	switch (err) {
	case 1: /* [レスのＮＧ指定] */
		ret = BCHAN_RESNUMBERMENU_SELECT_NG;
		break;
	case 2: /* [このレスをトレーに複写] */
		ret = BCHAN_RESNUMBERMENU_SELECT_PUSHTRAY;
		break;
	default:
		ret = BCHAN_RESNUMBERMENU_SELECT_NOSELECT;
		break;
	}

	return ret;
}

EXPORT W bchan_resnumbermenu_initialize(bchan_resnumbermenu_t *resnumbermenu, W dnum)
{
	W err;

	err = mopn_gmn(dnum);
	if (err < 0) {
		DP_ER("mopn_gmn", err);
		return err;
	}

	resnumbermenu->mnid = err;

	return 0;
}

EXPORT VOID bchan_resnumbermenu_finalize(bchan_resnumbermenu_t *resnumbermenu)
{
	mdel_gmn(resnumbermenu->mnid);
}

EXPORT W bchan_residmenu_setngselected(bchan_residmenu_t *residmenu, Bool selected)
{
	W err;

	if (selected == True) {
		err = mchg_gat(residmenu->mnid, 1, M_SEL);
		if (err < 0) {
			return err;
		}
		err = mchg_gat(residmenu->mnid, 2, M_INACT);
	} else {
		err = mchg_gat(residmenu->mnid, 1, M_NOSEL);
		if (err < 0) {
			return err;
		}
		err = mchg_gat(residmenu->mnid, 2, M_ACT);
	}

	return err;
}

EXPORT W bchan_residmenu_select(bchan_residmenu_t *residmenu, PNT pos)
{
	W err, ret;

	err = msel_gmn(residmenu->mnid, pos);
	if (err < 0) {
		DP_ER("msel_gmn", err);
		return err;
	}

	switch (err) {
	case 1: /* [このＩＤのＮＧ指定] */
		ret = BCHAN_RESIDMENU_SELECT_NG;
		break;
	case 2: /* [このＩＤのレスをトレーに複写] */
		ret = BCHAN_RESIDMENU_SELECT_PUSHTRAY;
		break;
	default:
		ret = BCHAN_RESIDMENU_SELECT_NOSELECT;
		break;
	}

	return ret;
}

EXPORT W bchan_residmenu_initialize(bchan_residmenu_t *residmenu, W dnum)
{
	W err;

	err = mopn_gmn(dnum);
	if (err < 0) {
		DP_ER("mopn_gmn", err);
		return err;
	}

	residmenu->mnid = err;

	return 0;
}

EXPORT VOID bchan_residmenu_finalize(bchan_residmenu_t *residmenu)
{
	mdel_gmn(residmenu->mnid);
}
