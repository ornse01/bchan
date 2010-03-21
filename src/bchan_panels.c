/*
 * bchan_panels.c
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

#include    "bchan_panels.h"

#include	<basic.h>
#include	<bstdio.h>
#include	<bstdlib.h>
#include	<btron/hmi.h>
#include	<btron/vobj.h>
#include	"sjisstring.h"

#ifdef BCHAN_CONFIG_DEBUG
# define DP(arg) printf arg
# define DP_ER(msg, err) printf("%s (%d/%x)\n", msg, err>>16, err)
#else
# define DP(arg) /**/
# define DP_ER(msg, err) /**/
#endif

#define BCHAN_DBX_MS_PANEL_OK 39
#define BCHAN_DBX_TEXT_SERVERNAME 40
#define BCHAN_DBX_TEXT_BOARDNAME 41
#define BCHAN_DBX_TEXT_THREADNUMBER 42
#define BCHAN_DBX_TEXT_THREADINFO 43

EXPORT W bchan_panels_threadinfo_tc(TC *host, W host_len, TC *board, W board_len, TC *thread, W thread_len)
{
	PNL_ITEM pnl_item[8];
	PNID pnid0;
	PNT p0 = {0x8000,0x8000};
	WEVENT wev0;
	W stat,itemno;
	PANEL pnl = {
		2,0x48,0,
		{{0, 0, 436, 188}},
		1,
		8,
		pnl_item
	};

	pnl_item[0].itype = PARTS_ITEM;
	pnl_item[0].info = 0;
	pnl_item[0].ir = (RECT){{178,140,0,0}};
	pnl_item[0].desc = 0;
	pnl_item[0].dnum = BCHAN_DBX_MS_PANEL_OK;
	pnl_item[0].ptr = NULL;
	pnl_item[1].itype = TEXT_ITEM;
	pnl_item[1].info = 0;
	pnl_item[1].ir = (RECT){{24,50,0,0}};
	pnl_item[1].desc = 0;
	pnl_item[1].dnum = BCHAN_DBX_TEXT_SERVERNAME;
	pnl_item[1].ptr = NULL;
	pnl_item[2].itype = TEXT_ITEM;
	pnl_item[2].info = 0;
	pnl_item[2].ir = (RECT){{24,80,0,0}};
	pnl_item[2].desc = 0;
	pnl_item[2].dnum = BCHAN_DBX_TEXT_BOARDNAME;
	pnl_item[2].ptr = NULL;
	pnl_item[3].itype = TEXT_ITEM;
	pnl_item[3].info = 0;
	pnl_item[3].ir = (RECT){{24,110,0,0}};
	pnl_item[3].desc = 0;
	pnl_item[3].dnum = BCHAN_DBX_TEXT_THREADNUMBER;
	pnl_item[3].ptr = NULL;
	pnl_item[4].itype = TEXT_ITEM;
	pnl_item[4].info = 0;
	pnl_item[4].ir = (RECT){{170,24,0,0}};
	pnl_item[4].desc = 0;
	pnl_item[4].dnum = BCHAN_DBX_TEXT_THREADINFO;
	pnl_item[4].ptr = NULL;
	pnl_item[5].itype = TEXT_ITEM|ATR_TEXT;
	pnl_item[5].info = 0;
	pnl_item[5].ir = (RECT){{120,50,0,0}};
	pnl_item[5].desc = 0;
	pnl_item[5].dnum = 0;
	pnl_item[5].ptr = (H*)host;
	pnl_item[6].itype = TEXT_ITEM|ATR_TEXT;
	pnl_item[6].info = 0;
	pnl_item[6].ir = (RECT){{68,80,0,0}};
	pnl_item[6].desc = 0;
	pnl_item[6].dnum = 0;
	pnl_item[6].ptr = (H*)board;
	pnl_item[7].itype = TEXT_ITEM|ATR_TEXT;
	pnl_item[7].info = 0;
	pnl_item[7].ir = (RECT){{136,110,0,0}};
	pnl_item[7].desc = 0;
	pnl_item[7].dnum = 0;
	pnl_item[7].ptr = (H*)thread;

	pnid0 = pcre_pnl(&pnl, &p0);
	if (pnid0 < 0) {
		DP_ER("pcre_pnl error", pnid0);
		return pnid0;
	}

	for (;;) {
		stat = pact_pnl(pnid0, &wev0.e, &itemno);
		switch (stat) {
		case	P_EVENT:
			if (wev0.s.type == EV_DEVICE) {
				oprc_dev(&wev0.e, NULL, 0);
			}
			continue;
		default:
			if (itemno == 1) { /* default switch */
				break;
			}
			if (itemno >= 0) {
				continue;
			}
		case	0x5001:
			if (itemno == 1) { /* default switch */
				break;
			}
			if (itemno >= 0) {
				continue;
			}
			break;
		}
		if (itemno == 1) { /* default switch */
			break;
		}
	}

	pdel_pnl(pnid0);

	return 0;
}

EXPORT W bchan_panels_threadinfo(UB *host, W host_len, UB *board, W board_len, UB *thread, W thread_len)
{
	TC *host_tc, *board_tc, *thread_tc;
	W host_tc_len, board_tc_len, thread_tc_len;
	W err;

	host_tc_len = sjstring_totcs(host, host_len, NULL);
	if (host_tc_len < 0) {
		return -1; /* TODO */
	}
	host_tc = malloc(sizeof(TC)*(host_tc_len + 1));
	if (host_tc == NULL) {
		return -1; /* TODO */
	}
	sjstring_totcs(host, host_len, host_tc);
	host_tc[host_tc_len] = TNULL;
	board_tc_len = sjstring_totcs(board, board_len, NULL);
	if (board_tc_len < 0) {
		free(host_tc);
		return -1; /* TODO */
	}
	board_tc = malloc(sizeof(TC)*(board_tc_len + 1));
	if (board_tc == NULL) {
		free(host_tc);
		return -1; /* TODO */
	}
	sjstring_totcs(board, board_len, board_tc);
	board_tc[board_tc_len] = TNULL;
	thread_tc_len = sjstring_totcs(thread, thread_len, NULL);
	if (thread_tc_len < 0) {
		free(board_tc);
		free(host_tc);
		return -1; /* TODO */
	}
	thread_tc = malloc(sizeof(TC)*(thread_tc_len + 1));
	if (thread_tc == NULL) {
		free(board_tc);
		free(host_tc);
		return -1; /* TODO */
	}
	sjstring_totcs(thread, thread_len, thread_tc);
	thread_tc[thread_tc_len] = TNULL;

	err = bchan_panels_threadinfo_tc(host_tc, host_tc_len, board_tc, board_tc_len, thread_tc, thread_tc_len);

	free(thread_tc);
	free(board_tc);
	free(host_tc);

	return err;
}
