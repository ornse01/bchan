/*
 * hmi.h
 *
 * Copyright (c) 2011 project bchan
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

#include    <basic.h>
#include	<btron/dp.h>
#include	<btron/hmi.h>

#include	"postres.h"

#ifndef __HMI_H__
#define __HMI_H__

typedef struct datwindow_t_ datwindow_t;
typedef VOID (*datwindow_scrollcalback)(VP arg, W dh, W dv);

IMPORT VOID datwindow_scrollbyvalue(datwindow_t *wscr, W dh, W dv);
IMPORT W datwindow_setdrawrect(datwindow_t *window, W l, W t, W r, W b);
IMPORT W datwindow_setworkrect(datwindow_t *window, W l, W t, W r, W b);
IMPORT VOID datwindow_responsepasterequest(datwindow_t *window, W nak, PNT *pos);
IMPORT W datwindow_startredisp(datwindow_t *window, RECT *r);
IMPORT W datwindow_endredisp(datwindow_t *window);
IMPORT W datwindow_eraseworkarea(datwindow_t *window, RECT *r);
IMPORT W datwindow_scrollworkarea(datwindow_t *window, W dh, W dv);
IMPORT W datwindow_getworkrect(datwindow_t *window, RECT *r);
IMPORT W datwindow_requestredisp(datwindow_t *window);
IMPORT GID datwindow_startdrag(datwindow_t *window);
IMPORT W datwindow_getdrag(datwindow_t *window, PNT *pos, WID *wid, PNT *pos_butup);
IMPORT VOID datwindow_enddrag(datwindow_t *window);
IMPORT GID datwindow_getGID(datwindow_t *window);
IMPORT WID datwindow_getWID(datwindow_t *window);
IMPORT W datwindow_settitle(datwindow_t *window, TC *title);

typedef struct cfrmwindow_t_ cfrmwindow_t;

IMPORT W cfrmwindow_open(cfrmwindow_t* window);
IMPORT VOID cfrmwindow_setpostresdata(cfrmwindow_t *window, postresdata_t *post);

typedef struct ngwordwindow_t_ ngwordwindow_t;

IMPORT W ngwordwindow_open(ngwordwindow_t *window);
IMPORT W ngwordwindow_appendword(ngwordwindow_t *window, TC *str, W len);
IMPORT W ngwordwindow_removeword(ngwordwindow_t *window, TC *str, W len);
IMPORT W ngwordwindow_starttextboxaction(ngwordwindow_t *window);
#define NGWORDWINDOW_GETTEXTBOXACTION_FINISH 0
#define NGWORDWINDOW_GETTEXTBOXACTION_MENU 1
#define NGWORDWINDOW_GETTEXTBOXACTION_KEYMENU 2
#define NGWORDWINDOW_GETTEXTBOXACTION_MOVE 3
#define NGWORDWINDOW_GETTEXTBOXACTION_COPY 4
#define NGWORDWINDOW_GETTEXTBOXACTION_APPEND 5
IMPORT W ngwordwindow_gettextboxaction(ngwordwindow_t *window, TC *key, TC **val, W *len);
IMPORT W ngwordwindow_endtextboxaction(ngwordwindow_t *window);

enum {
	DATHMIEVENT_TYPE_NONE,
	DATHMIEVENT_TYPE_COMMON_MOUSEMOVE,
	DATHMIEVENT_TYPE_COMMON_KEYDOWN,
	DATHMIEVENT_TYPE_COMMON_MENU,
	DATHMIEVENT_TYPE_COMMON_TIMEOUT,
	DATHMIEVENT_TYPE_THREAD_DRAW,
	DATHMIEVENT_TYPE_THREAD_RESIZE,
	DATHMIEVENT_TYPE_THREAD_CLOSE,
	DATHMIEVENT_TYPE_THREAD_BUTDN,
	DATHMIEVENT_TYPE_THREAD_PASTE,
	DATHMIEVENT_TYPE_THREAD_SWITCH,
	DATHMIEVENT_TYPE_THREAD_MOUSEMOVE,
	DATHMIEVENT_TYPE_CONFIRM_CLOSE,
	DATHMIEVENT_TYPE_NGWORD_APPEND,
	DATHMIEVENT_TYPE_NGWORD_REMOVE,
	DATHMIEVENT_TYPE_NGWORD_CLOSE,
	DATHMIEVENT_TYPE_NGWORD_TEXTBOX,
};

struct dathmi_eventdata_mousemove_t_ {
	PNT pos;
};

struct dathmi_eventdata_keydown_t_ {
	TC keycode;
	UH keytop;
	UW stat;
};

struct dathmi_eventdata_menu_t_ {
	PNT pos;
};

struct dathmi_eventdata_timeout_t_ {
	W code;
};

struct datwindow_eventdata_draw_t_ {
};

struct datwindow_eventdata_resize_t_ {
	SIZE work_sz;
	Bool needdraw;
};

struct datwindow_eventdata_close_t_ {
	Bool save;
};

struct datwindow_eventdata_butdn_t_ {
	W type;
	PNT pos;
};

struct datwindow_eventdata_paste_t_ {
};

struct datwindow_eventdata_switch_t_ {
	Bool needdraw;
};

struct datwindow_eventdata_mousemove_t_ {
	PNT pos;
	UW stat;
};

struct cfrmwindow_eventdata_close_t_ {
	Bool send;
};

struct ngwordwindow_eventdata_append_t_ {
	TC *str;
	W len;
};

struct ngwordwindow_eventdata_remove_t_ {
	TC *str;
	W len;
};

typedef struct dathmi_eventdata_mousemove_t_ dathmi_eventdata_mousemove_t;
typedef struct dathmi_eventdata_keydown_t_ dathmi_eventdata_keydown_t;
typedef struct dathmi_eventdata_menu_t_ dathmi_eventdata_menu_t;
typedef struct dathmi_eventdata_timeout_t_ dathmi_eventdata_timeout_t;
typedef struct datwindow_eventdata_draw_t_ datwindow_eventdata_draw_t;
typedef struct datwindow_eventdata_resize_t_ datwindow_eventdata_resize_t;
typedef struct datwindow_eventdata_close_t_ datwindow_eventdata_close_t;
typedef struct datwindow_eventdata_butdn_t_ datwindow_eventdata_butdn_t;
typedef struct datwindow_eventdata_paste_t_ datwindow_eventdata_paste_t;
typedef struct datwindow_eventdata_switch_t_ datwindow_eventdata_switch_t;
typedef struct datwindow_eventdata_mousemove_t_ datwindow_eventdata_mousemove_t;
typedef struct cfrmwindow_eventdata_close_t_ cfrmwindow_eventdata_close_t;
typedef struct ngwordwindow_eventdata_append_t_ ngwordwindow_eventdata_append_t;
typedef struct ngwordwindow_eventdata_remove_t_ ngwordwindow_eventdata_remove_t;

struct dathmievent_t_ {
	W type;
	union  {
		dathmi_eventdata_mousemove_t common_mousemove;
		dathmi_eventdata_keydown_t common_keydown;
		dathmi_eventdata_menu_t common_menu;
		dathmi_eventdata_timeout_t common_timeout;
		datwindow_eventdata_draw_t main_draw;
		datwindow_eventdata_resize_t main_resize;
		datwindow_eventdata_close_t main_close;
		datwindow_eventdata_butdn_t main_butdn;
		datwindow_eventdata_paste_t main_paste;
		datwindow_eventdata_switch_t main_switch;
		datwindow_eventdata_mousemove_t main_mousemove;
		cfrmwindow_eventdata_close_t confirm_close;
		ngwordwindow_eventdata_append_t ngword_append;
		ngwordwindow_eventdata_remove_t ngword_remove;
	} data;
};
typedef struct dathmievent_t_ dathmievent_t;

typedef struct dathmi_t_ dathmi_t;

IMPORT dathmi_t* dathmi_new();
IMPORT VOID dathmi_delete(dathmi_t *hmi);
IMPORT W dathmi_getevent(dathmi_t *hmi, dathmievent_t **evt);
IMPORT datwindow_t* dathmi_newmainwindow(dathmi_t *hmi, RECT *r, TC *title, PAT *bgpat, datwindow_scrollcalback scrollcallback, VP arg);
IMPORT cfrmwindow_t *dathmi_newconfirmwindow(dathmi_t *hmi, RECT *r, W dnum_title, W dnum_post, W dnum_cancel);
IMPORT ngwordwindow_t *dathmi_newngwordwindow(dathmi_t *hmi, PNT *p, W dnum_list, W dnum_delete, W dnum_input, W dnum_append);
IMPORT VOID dathmi_deletemainwindow(dathmi_t *hmi, datwindow_t *window);
IMPORT VOID dathmi_deleteconfirmwindow(dathmi_t *hmi, cfrmwindow_t *window);
IMPORT VOID dathmi_deletengwordwindow(dathmi_t *hmi, ngwordwindow_t *window);

#endif
