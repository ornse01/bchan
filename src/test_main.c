/*
 * test_main.c
 *
 * Copyright (c) 2009-2011 project bchan
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
#include	<bstring.h>
#include	<errcode.h>
#include	<tstring.h>
#include	<keycode.h>
#include	<tcode.h>
#include	<btron/btron.h>
#include	<btron/dp.h>
#include	<btron/hmi.h>
#include	<btron/vobj.h>
#include	<btron/libapp.h>
#include	<btron/bsocket.h>

#include    "test.h"

EXPORT	W	MAIN(MESSAGE *msg)
{
	test_cache_main();
	test_parser_main();
	test_layout_main();
	test_parselib_main();
	test_submitutil_main();
	test_sjistring_main();
	test_residhash_main();
	test_resindexhash_main();
	test_postres_main();
	test_tadimf_main();
	test_array_main();
	test_wordlist_main();
	test_tadsearch_main();
	test_httpheaderlexer_main();
	test_setcookieheader_main();
	test_httpdateparser_main();
	test_cookiedb_main();

	return 0;
}
