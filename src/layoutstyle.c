/*
 * layoutstyle.c
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

#include    "layoutstyle.h"

#include	<btron/btron.h>
#include	<btron/dp.h>

//#define datlayout_fontconfig_class FTC_MINCHO
#define datlayout_fontconfig_class 0x000000c0 /* gothic */
//#define datlayout_fontconfig_class FTC_DEFAULT

EXPORT W layoutstyle_resetgenvfont(GID target)
{
	FSSPEC spec;
	W err;

	err = gget_fon(target, &spec, NULL);
	if (err < 0) {
		return err;
	}

	spec.attr |= FT_PROP;
	spec.attr |= FT_GRAYSCALE;
	spec.fclass = datlayout_fontconfig_class;
	spec.size.h = 16;
	spec.size.v = 16;

	err = gset_fon(target, &spec);

	return err;
}

EXPORT W layoutstyle_sethankakufont(GID target)
{
	FSSPEC spec;
	W err;

	err = gget_fon(target, &spec, NULL);
	if (err < 0) {
		return err;
	}

	spec.attr |= FT_PROP;
	spec.attr |= FT_GRAYSCALE;
	spec.fclass = datlayout_fontconfig_class;
	spec.size.h = 8;
	spec.size.v = 16;

	err = gset_fon(target, &spec);

	return err;
}

EXPORT VOID datlayoutstyle_setdefault(datlayoutstyle_t *style)
{
	style->body.margin_width_left = 0;
	style->body.margin_width_top = 0;
	style->body.margin_width_right = 0;
	style->body.margin_width_bottom = 0;
	style->body.border_width_left = 0;
	style->body.border_width_top = 0;
	style->body.border_width_right = 0;
	style->body.border_width_bottom = 0;
	style->body.padding_width_left = 0;
	style->body.padding_width_top = 0;
	style->body.padding_width_right = 0;
	style->body.padding_width_bottom = 0;
	style->body.border_color_left = 0x10ffffff;
	style->body.border_color_top = 0x10ffffff;
	style->body.border_color_right = 0x10ffffff;
	style->body.border_color_bottom = 0x10ffffff;
	style->res.margin_width_left = 0;
	style->res.margin_width_top = 3;
	style->res.margin_width_right = 0;
	style->res.margin_width_bottom = 3;
	style->res.border_width_left = 0;
	style->res.border_width_top = 0;
	style->res.border_width_right = 0;
	style->res.border_width_bottom = 0;
	style->res.padding_width_left = 2;
	style->res.padding_width_top = 2;
	style->res.padding_width_right = 2;
	style->res.padding_width_bottom = 5;
	style->res.border_color_left = 0x10ffffff;
	style->res.border_color_top = 0x10fffff;
	style->res.border_color_right = 0x10ffffff;
	style->res.border_color_bottom = 0x10ffffff;
	style->resheader.margin_width_left = 3;
	style->resheader.margin_width_top = 3;
	style->resheader.margin_width_right = 3;
	style->resheader.margin_width_bottom = 0;
	style->resheader.border_width_left = 0;
	style->resheader.border_width_top = 0;
	style->resheader.border_width_right = 0;
	style->resheader.border_width_bottom = 0;
	style->resheader.padding_width_left = 2;
	style->resheader.padding_width_top = 2;
	style->resheader.padding_width_right = 2;
	style->resheader.padding_width_bottom = 0;
	style->resheader.border_color_left = 0x10ffffff;
	style->resheader.border_color_top = 0x10ffffff;
	style->resheader.border_color_right = 0x10ffffff;
	style->resheader.border_color_bottom = 0x10ffffff;
	style->resmessage.margin_width_left = 25;
	style->resmessage.margin_width_top = 3;
	style->resmessage.margin_width_right = 3;
	style->resmessage.margin_width_bottom = 3;
	style->resmessage.border_width_left = 0;
	style->resmessage.border_width_top = 0;
	style->resmessage.border_width_right = 0;
	style->resmessage.border_width_bottom = 0;
	style->resmessage.padding_width_left = 0;
	style->resmessage.padding_width_top = 0;
	style->resmessage.padding_width_right = 0;
	style->resmessage.padding_width_bottom = 0;
	style->resmessage.border_color_left = 0x10ffffff;
	style->resmessage.border_color_top = 0x10ffffff;
	style->resmessage.border_color_right = 0x10ffffff;
	style->resmessage.border_color_bottom = 0x10ffffff;
}
