/*
 * bchan_menus.h
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

#include	<basic.h>
#include	<btron/hmi.h>

#ifndef __BCHAN_MENUS_H__
#define __BCHAN_MENUS_H__

struct bchan_resmenu_t_ {
	MNID mnid;
};
typedef struct bchan_resmenu_t_ bchan_resmenu_t;

IMPORT W bchan_resmenu_initialize(bchan_resmenu_t *resmenu, W dnum);
IMPORT VOID bchan_resmenu_finalize(bchan_resmenu_t *resmenu);
IMPORT W bchan_resmenu_setngselected(bchan_resmenu_t *resmenu, Bool selected);
#define BCHAN_RESMENU_SELECT_NOSELECT 0
#define BCHAN_RESMENU_SELECT_NG 1
#define BCHAN_RESMENU_SELECT_PUSHTRAY 2
IMPORT W bchan_resmenu_select(bchan_resmenu_t *resmenu, PNT pos);

struct bchan_residmenu_t_ {
	MNID mnid;
};
typedef struct bchan_residmenu_t_ bchan_residmenu_t;

IMPORT W bchan_residmenu_initialize(bchan_residmenu_t *residmenu, W dnum);
IMPORT VOID bchan_residmenu_finalize(bchan_residmenu_t *residmenu);
IMPORT W bchan_residmenu_setngselected(bchan_residmenu_t *residmenu, Bool selected);
#define BCHAN_RESIDMENU_SELECT_NOSELECT 0
#define BCHAN_RESIDMENU_SELECT_NG 1
#define BCHAN_RESIDMENU_SELECT_PUSHTRAY 2
IMPORT W bchan_residmenu_select(bchan_residmenu_t *residmenu, PNT pos);

#endif
