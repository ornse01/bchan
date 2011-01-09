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

struct bchan_mainmenu_t_ {
	MENUITEM *mnitem;
	MNID mnid;
};
typedef struct bchan_mainmenu_t_ bchan_mainmenu_t;

IMPORT W bchan_mainmenu_initialize(bchan_mainmenu_t *mainmenu, W dnum);
IMPORT VOID bchan_mainmenu_finalize(bchan_mainmenu_t *mainmenu);
IMPORT W bchan_mainmenu_setup(bchan_mainmenu_t *mainmenu, Bool titleenable, Bool networkenable);
#define BCHAN_MAINMENU_SELECT_NOSELECT 0
#define BCHAN_MAINMENU_SELECT_CLOSE 1
#define BCHAN_MAINMENU_SELECT_REDISPLAY 2
#define BCHAN_MAINMENU_SELECT_THREADINFO 3
#define BCHAN_MAINMENU_SELECT_TITLETOTRAY 4
#define BCHAN_MAINMENU_SELECT_URLTOTRAY 5
#define BCHAN_MAINMENU_SELECT_THREADFETCH 6
#define BCHAN_MAINMENU_SELECT_NGWORD 7
IMPORT W bchan_mainmenu_popup(bchan_mainmenu_t *mainmenu, PNT pos);
IMPORT W bchan_mainmenu_keyselect(bchan_mainmenu_t *mainmenu, TC keycode);

struct bchan_resnumbermenu_t_ {
	MNID mnid;
};
typedef struct bchan_resnumbermenu_t_ bchan_resnumbermenu_t;

IMPORT W bchan_resnumbermenu_initialize(bchan_resnumbermenu_t *resnumbermenu, W dnum);
IMPORT VOID bchan_resnumbermenu_finalize(bchan_resnumbermenu_t *resnumbermenu);
IMPORT W bchan_resnumbermenu_setngselected(bchan_resnumbermenu_t *resnumbermenu, Bool selected);
#define BCHAN_RESNUMBERMENU_SELECT_NOSELECT 0
#define BCHAN_RESNUMBERMENU_SELECT_NG 1
#define BCHAN_RESNUMBERMENU_SELECT_PUSHTRAY 2
IMPORT W bchan_resnumbermenu_select(bchan_resnumbermenu_t *resnumbermenu, PNT pos);

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
