/*
 * tadurl.c
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

#include    "tadurl.h"

#include	<basic.h>
#include	<bstdio.h>
#include	<tstring.h>
#include	<tcode.h>

#ifdef BCHAN_CONFIG_DEBUG
# define DP(arg) printf arg
# define DP_ER(msg, err) printf("%s (%d/%x)\n", msg, err>>16, err)
#else
# define DP(arg) /**/
# define DP_ER(msg, err) /**/
#endif

/* these functions assume argument data is only TC string. */
/*  should add code skipping segments? */

EXPORT W tadurl_cmpscheme(UB *tadurl, W url_len, UB *scheme, W scheme_len)
{
	TC *str;
	W str_len, i, ret;
	UB conv[2];

	str = (TC*)tadurl;
	str_len = url_len/2;

	for (i = 0; i < str_len; i++) {
		if (i >= scheme_len) {
			break;
		}
		ret = tctoeuc(conv, str[i]);
		if (ret != 1) {
			return 1;
		}
		if (conv[0] != scheme[i]) {
			return 1;
		}
	}
	if (str[i] != TK_COLN) {
		return 1;
	}

	return 0;
}

EXPORT W tadurl_cmphost(UB *tadurl, W url_len, UB *host, W host_len)
{
}

EXPORT UB* tadurl_searchstringinhost(UB *tadurl, W url_len, UB *str, W str_len)
{
}

EXPORT W tadurl_cmppath(UB *tadurl, W url_len, UB *path, W path_len)
{
	TC *str;
	W str_len, i, j, ret, n_slsh = 0;
	UB conv[2];

	str = (TC*)tadurl;
	str_len = url_len/2;

	for (i = 0; i < str_len; i++) {
		if (n_slsh == 3) {
			break;
		}
		if (str[i] == TK_SLSH) {
			n_slsh++;
		}
	}

	if (n_slsh != 3) {
		return 1;
	}

	j = 0;
	for (; i < str_len; i++) {
		if (j >= path_len) {
			break;
		}
		ret = tctoeuc(conv, str[i]);
		if (ret != 1) {
			return 1;
		}
		if (conv[0] != path[j++]) {
			return 1;
		}
	}

	return 0;
}
