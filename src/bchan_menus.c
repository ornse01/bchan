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

EXPORT W bchan_resmenu_setngselected(bchan_resmenu_t *resmenu, Bool selected)
{
	W err;

	if (selected == True) {
		err = mchg_gat(resmenu->mnid, 1, M_SEL);
	} else {
		err = mchg_gat(resmenu->mnid, 1, M_NOSEL);
	}

	return err;
}

EXPORT W bchan_resmenu_select(bchan_resmenu_t *resmenu, PNT pos)
{
	W err, ret;

	err = msel_gmn(resmenu->mnid, pos);
	if (err < 0) {
		DP_ER("msel_gmn", err);
		return err;
	}

	switch (err) {
	case 1: /* [レスのＮＧ指定] */
		ret = BCHAN_RESMENU_SELECT_NG;
		break;
	case 2: /* [このレスをトレーに複写] */
		ret = BCHAN_RESMENU_SELECT_PUSHTRAY;
		break;
	default:
		ret = BCHAN_RESMENU_SELECT_NOSELECT;
		break;
	}

	return ret;
}

EXPORT W bchan_resmenu_initialize(bchan_resmenu_t *resmenu, W dnum)
{
	W err;

	err = mopn_gmn(dnum);
	if (err < 0) {
		DP_ER("mopn_gmn", err);
		return err;
	}

	resmenu->mnid = err;

	return 0;
}

EXPORT VOID bchan_resmenu_finalize(bchan_resmenu_t *resmenu)
{
	mdel_gmn(resmenu->mnid);
}
