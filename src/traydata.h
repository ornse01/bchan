/*
 * traydata.h
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

#include    "layoutarray.h"

#ifndef __TRAYDATA_H__
#define __TRAYDATA_H__

typedef struct dattraydata_t_ dattraydata_t;

IMPORT dattraydata_t* dattraydata_new(datlayoutarray_t *layoutarray);
IMPORT VOID dattraydata_delete(dattraydata_t *traydata);
IMPORT W dattraydata_resindextotraytextdata(dattraydata_t *traydata, W n, B *data, W data_len);
IMPORT W dattraydata_idtotraytextdata(dattraydata_t *traydata, TC *id, W id_len, B *data, W data_len);

#endif
