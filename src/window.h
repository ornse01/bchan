/*
 * window.h
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
#include	<btron/hmi.h>

#ifndef __WINDOW_H__
#define __WINDOW_H__

typedef struct datwindow_t_ datwindow_t;

typedef VOID (*datwindow_scrollcalback)(VP arg, W dh, W dv);
typedef VOID (*datwindow_drawcallback)(VP arg, RECT *r);
typedef VOID (*datwindow_resizecallback)(VP arg);
typedef VOID (*datwindow_closecallback)(VP arg);
typedef VOID (*datwindow_butdncallback)(VP arg, WEVENT *wev);
typedef W (*datwindow_pastecallback)(VP arg, WEVENT *wev);

IMPORT datwindow_t* datwindow_new(WID target, datwindow_scrollcalback scrollcallback, datwindow_drawcallback drawcallback, datwindow_resizecallback resizecallback, datwindow_closecallback closecallback, datwindow_butdncallback butdncallback, datwindow_pastecallback pastecallback, VP arg);
IMPORT VOID datwindow_delete(datwindow_t *wscr);
IMPORT VOID datwindow_setwid(datwindow_t *window, WID target);
IMPORT W datwindow_updatebar(datwindow_t *wscr);
IMPORT VOID datwindow_weventbutdn(datwindow_t *wscr, WEVENT *wev);
IMPORT VOID datwindow_weventrequest(datwindow_t *wscr, WEVENT *wev);
IMPORT VOID datwindow_weventswitch(datwindow_t *wscr, WEVENT *wev);
IMPORT VOID datwindow_weventreswitch(datwindow_t *wscr, WEVENT *wev);
IMPORT VOID datwindow_scrollbyvalue(datwindow_t *wscr, W dh, W dv);
IMPORT W datwindow_setdrawrect(datwindow_t *wscr, W l, W t, W r, W b);
IMPORT W datwindow_setworkrect(datwindow_t *wscr, W l, W t, W r, W b);

#endif
