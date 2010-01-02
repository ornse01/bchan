/*
 * submit.h
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
#include	"cache.h"
#include	"postres.h"

#ifndef __SUBMIT_H__
#define __SUBMIT_H__

typedef struct ressubmit_t_ ressubmit_t;

IMPORT ressubmit_t* ressubmit_new(datcache_t *cache);
IMPORT VOID ressubmit_delete(ressubmit_t *submit);
IMPORT W ressubmit_respost(ressubmit_t *submit, postresdata_t *post);

#define RESSUBMIT_RESPOST_ERROR_CLIENT -1
#define RESSUBMIT_RESPOST_ERROR_STATUS -2
#define RESSUBMIT_RESPOST_ERROR_CONTENT -3
#define RESSUBMIT_RESPOST_SUCCEED 0
#define RESSUBMIT_RESPOST_DENIED 1

#endif
