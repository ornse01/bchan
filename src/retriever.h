/*
 * retriever.h
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
#include    "cache.h"

#ifndef __RETREIEVER_H__
#define __RETREIEVER_H__

typedef struct datretriever_t_ datretriever_t;

IMPORT datretriever_t* datretriever_new(datcache_t *cache);
IMPORT VOID datretriever_delete(datretriever_t *retriever);
IMPORT W datretriever_request(datretriever_t *retriever);

#ifdef BCHAN_CONFIG_DEBUG
IMPORT VOID DATRETRIEVER_DP(datretriever_t *retriever);
#else
#define DATRETRIEVER_DP(retriever) /**/
#endif

#define DATRETRIEVER_REQUEST_NOT_MODIFIED    0
#define DATRETRIEVER_REQUEST_ALLRELOAD       1
#define DATRETRIEVER_REQUEST_PARTIAL_CONTENT 2
#define DATRETRIEVER_REQUEST_NON_AUTHORITATIVE 3

#endif
