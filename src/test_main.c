/*
 * test_main.c
 *
 * Copyright (c) 2009-2012 project bchan
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

#include    <unittest_driver.h>

#include    "test.h"

EXPORT	W	MAIN(MESSAGE *msg)
{
	unittest_driver_t *driver;

	driver = unittest_driver_new();
	if (driver == NULL) {
		return 0;
	}

	test_cache_main(driver);
	test_parser_main(driver);
	test_layout_main(driver);
	test_parselib_main(driver);
	test_submitutil_main(driver);
	test_sjistring_main(driver);
	test_residhash_main(driver);
	test_resindexhash_main(driver);
	test_postres_main(driver);
	test_tadimf_main(driver);
	test_wordlist_main(driver);
	test_tadsearch_main(driver);
	test_httpheaderlexer_main(driver);
	test_setcookieheader_main(driver);
	test_httpdateparser_main(driver);
	test_cookiedb_main(driver);
	test_psvlexer_main(driver);

	unittest_driver_runnning(driver);
	unittest_driver_delete(driver);

	return 0;
}
