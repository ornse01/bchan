/*
 * poptray.c
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
#include	<bstdlib.h>
#include	<bstdio.h>
#include	<errcode.h>
#include	<tstring.h>
#include	<tcode.h>
#include	<btron/btron.h>
#include	<btron/dp.h>
#include	<btron/hmi.h>

#include    "poptray.h"
#include    "postres.h"

#ifdef BCHAN_CONFIG_DEBUG
# define DP(arg) printf arg
# define DP_ER(msg, err) printf("%s (%d/%x)\n", msg, err>>16, err)
#else
# define DP(arg) /**/
# define DP_ER(msg, err) /**/
#endif

EXPORT W poptray_gettraydata(postresdata_t **post)
{
	TRAYREC *data;
	TR_VOBJREC *vobjrec = NULL;
	postresdata_t *ret;
	W i, size, recs, err;

	err = tget_dat(NULL, 0, &size, -1);
	if (err < 0) {
		DP_ER("tget_dat: get size error", err);
		return err;
	}
	if (err == 0) {
		/* tray is empty */
		return 0;
	}
	data = (TRAYREC*)malloc(size);
	if (data == NULL) {
		return 0;
	}
	recs = tget_dat(data, size, NULL, -1);
	if (recs < 0) {
		DP_ER("tget_dat: get data error", recs);
		return err;
	}

	for (i = 0; i < recs; i++) {
		if (data[i].id != TR_VOBJ) {
			continue;
		}
		vobjrec = (TR_VOBJREC*)data[i].dt;
	}

	if (vobjrec != NULL) {
		ret = postresdata_new();
		if (ret == NULL) {
			free(data);
			return 0;
		}
		postresdata_readfile(ret, &(vobjrec->vlnk));
		*post = ret;
	}

	tset_dat(NULL, 0);
	free(data);

	return 0;
}
