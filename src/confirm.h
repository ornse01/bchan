/*
 * confirm.c
 *
 * Copyright (c) 2009-2010 project bchan
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

#include	"postres.h"

#ifndef __CONFIRM_H__
#define __CONFIRM_H__

typedef struct cfrmwindow_t_ cfrmwindow_t;
typedef VOID (*cfrmwindow_notifyclosecallback)(VP arg, W send);

IMPORT cfrmwindow_t* cfrmwindow_new(RECT *r, WID parent, cfrmwindow_notifyclosecallback proc, VP arg, W dnum_title, W dnum_post, W dnum_cancel);
IMPORT VOID cfrmwindow_delete(cfrmwindow_t *window);
IMPORT W cfrmwindow_open(cfrmwindow_t* window);
IMPORT VOID cfrmwindow_weventbutdn(cfrmwindow_t *window, WEVENT *wev);
IMPORT VOID cfrmwindow_weventrequest(cfrmwindow_t *window, WEVENT *wev);
IMPORT VOID cfrmwindow_weventswitch(cfrmwindow_t *window, WEVENT *wev);
IMPORT VOID cfrmwindow_weventreswitch(cfrmwindow_t *window, WEVENT *wev);
IMPORT Bool cfrmwindow_compairwid(cfrmwindow_t *window, WID wid);
IMPORT VOID cfrmwindow_setpostresdata(cfrmwindow_t *window, postresdata_t *post);

#endif
