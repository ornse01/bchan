/*
 * bchan_vobj.c
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

#include    "bchan_vobj.h"

#include	<basic.h>
#include	<bstdlib.h>
#include	<bstdio.h>
#include	<btron/btron.h>
#include	<btron/vobj.h>
#include	<btron/hmi.h>

#ifdef BCHAN_CONFIG_DEBUG
# define DP(arg) printf arg
# define DP_ER(msg, err) printf("%s (%d/%x)\n", msg, err>>16, err)
#else
# define DP(arg) /**/
# define DP_ER(msg, err) /**/
#endif

EXPORT W bchan_createbbbvobj(UB *fsn_bbb, W fsn_bbb_len, UB *fsn_texedit, W fsn_texedit_len, UB *taddata, W taddata_len, VOBJSEG *seg, LINK *lnk)
{
	W fd, err;
	VID vid;
	RECT newr;
	UB bin[4+24];
	TADSEG *base = (TADSEG*)bin;
	INFOSEG *infoseg = (INFOSEG*)(bin + 4);
	TEXTSEG *textseg = (TEXTSEG*)(bin + 4);

	seg->view = (RECT){{0,0,300,20}};
	seg->height = 100;
	seg->chsz = 16;
	seg->frcol = 0x10000000;
	seg->chcol = 0x10000000;
	err = wget_inf(WI_VOBJBGCOL, &seg->tbcol, sizeof(COLOR));
	if (err < 0) {
		seg->tbcol = 0x10ffffff;
	}
	seg->bgcol = 0x10ffffff;
	seg->dlen = 0;

	fd = cre_fil(lnk, (TC*)taddata, NULL, 0x31, F_FLOAT);
	if (fd < 0) {
		DP_ER("cre_fil error", fd);
		return fd;
	}

	err = apd_rec(fd, fsn_bbb, fsn_bbb_len, 8, 0, 0);
	if (err < 0) {
		DP_ER("apd_rec:fusen rec error", err);
		cls_fil(fd);
		del_fil(NULL, lnk, 0);
		return fd;
	}

	err = apd_rec(fd, fsn_texedit, fsn_texedit_len, 8, 0, 0);
	if (err < 0) {
		DP_ER("apd_rec:fusen rec error", err);
		cls_fil(fd);
		del_fil(NULL, lnk, 0);
		return fd;
	}

	err = apd_rec(fd, NULL, 0, RT_TADDATA, 0, 0);
	if (err < 0) {
		DP_ER("apd_rec:retrieve info error", err);
		cls_fil(fd);
		del_fil(NULL, lnk, 0);
		return fd;
	}
	err = see_rec(fd, -1, -1, NULL);
	if (err < 0) {
		DP_ER("see_rec error", err);
		cls_fil(fd);
		del_fil(NULL, lnk, 0);
		return fd;
	}
	base->id = 0xFFE0;
	base->len = 6;
	infoseg->subid = 0;
	infoseg->sublen = 2;
	infoseg->data[0] = 0x0122;
	err = wri_rec(fd, -1, bin, 4+6, NULL, NULL, 0);
	if (err < 0) {
		DP_ER("wri_rec:infoseg error", err);
		cls_fil(fd);
		del_fil(NULL, lnk, 0);
		return fd;
	}
	base->id = 0xFFE1;
	base->len = 24;
	textseg->view = (RECT){{0, 0, 0, 0}};
	textseg->draw = (RECT){{0, 0, 0, 0}};
	textseg->h_unit = -120;
	textseg->v_unit = -120;
	textseg->lang = 0x21;
	textseg->bgpat = 0;
	err = wri_rec(fd, -1, bin, 4+24, NULL, NULL, 0);
	if (err < 0) {
		DP_ER("wri_rec:textseg error", err);
		cls_fil(fd);
		del_fil(NULL, lnk, 0);
		return fd;
	}
	err = wri_rec(fd, -1, taddata, taddata_len, NULL, NULL, 0);
	if (err < 0) {
		DP_ER("wri_rec:data error", err);
		cls_fil(fd);
		del_fil(NULL, lnk, 0);
		return fd;
	}
	base->id = 0xFFE2;
	base->len = 0;
	err = wri_rec(fd, -1, bin, 4, NULL, NULL, 0);
	if (err < 0) {
		DP_ER("wri_rec:textend error", err);
		cls_fil(fd);
		del_fil(NULL, lnk, 0);
		return fd;
	}

	cls_fil(fd);

	vid = oreg_vob((VLINK*)lnk, seg, -1, V_NODISP);
	if (vid < 0) {
		DP_ER("oreg_vob", vid);
		del_fil(NULL, lnk, 0);
		return vid;
	}
	err = orsz_vob(vid, &newr, V_ADJUST1|V_NODISP);
	if (err < 0) {
		DP_ER("orsz_vob", vid);
		odel_vob(vid, 0);
		del_fil(NULL, lnk, 0);
		return err;
	}
	seg->view = newr;
	err = odel_vob(vid, 0);
	if (err < 0) {
		DP_ER("odel_vob", err);
		del_fil(NULL, lnk, 0);
		return err;
	}

	return 0;
}
