/*
 * bchan_menus.c
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

#include    "bchan_menus.h"

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
