/*
 * layoutstyle.h
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

#include    <basic.h>
#include	<btron/dp.h>

#ifndef __LAYOUTSTYLE_H__
#define __LAYOUTSTYLE_H__

typedef struct datlayout_style_t_ datlayout_style_t;
struct datlayout_style_t_ {
	UH margin_width_left;
	UH margin_width_top;
	UH margin_width_right;
	UH margin_width_bottom;
	UH border_width_left;
	UH border_width_top;
	UH border_width_right;
	UH border_width_bottom;
	UH padding_width_left;
	UH padding_width_top;
	UH padding_width_right;
	UH padding_width_bottom;
	COLOR border_color_left;
	COLOR border_color_top;
	COLOR border_color_right;
	COLOR border_color_bottom;
};

typedef struct datlayoutstyle_t_ datlayoutstyle_t;
struct datlayoutstyle_t_ {
	datlayout_style_t body;
	datlayout_style_t res;
	datlayout_style_t resheader;
	datlayout_style_t resmessage;
};

IMPORT W layoutstyle_resetgenvfont(GID target);
IMPORT W layoutstyle_sethankakufont(GID target);

IMPORT VOID datlayoutstyle_setdefault(datlayoutstyle_t *style);

#endif
