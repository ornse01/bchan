/*
 * tadlib.h
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

#include    <basic.h>
#include	<btron/dp.h>

#ifndef __TADLIB_H__
#define __TADLIB_H__

IMPORT W tadlib_calcdrawsize(TC *str, W len, GID gid, SIZE *sz);
IMPORT W tadlib_drawtext(TC *str, W len, GID gid, W dh, W dv);

typedef struct {
#if BIGENDIAN
	UB subid;
	UB attr;
#else
	UB attr;
	UB subid;
#endif
	UH appl[3];
} TT_BCHAN;

#define TT_BCHAN_SUBID_ANCHOR_START 1
#define TT_BCHAN_SUBID_ANCHOR_END   2
#define TT_BCHAN_SUBID_URL_START    3
#define TT_BCHAN_SUBID_URL_END      4

#endif
