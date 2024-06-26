/*
 * bchan_vobj.h
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
#include	<btron/vobj.h>

#ifndef __BCHAN_VOBJ_H__
#define __BCHAN_VOBJ_H__

IMPORT W bchan_createbbbvobj(UB *fsn_bbb, W fsn_bbb_len, UB *fsn_texedit, W fsn_texedit_len, UB *taddata, W taddata_len, VOBJSEG *vseg, LINK *lnk);
IMPORT W bchan_createviewervobj(TC *title, UB *fsn, W fsn_len, UB *host, W host_len, UB *board, W board_len, UB *thread, W thread_len, VOBJSEG *seg, LINK *lnk);

#endif
