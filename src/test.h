/*
 * test.h
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

#include    <basic.h>
#include    <unittest_driver.h>

#ifndef __BCHAN_TEST_H__
#define __BCHAN_TEST_H__

/* test_cache.c */
IMPORT VOID test_cache_main(unittest_driver_t *driver);

/* test_parser.c */
IMPORT VOID test_parser_main(unittest_driver_t *driver);

/* test_layout.c */
IMPORT VOID test_layout_main(unittest_driver_t *driver);

/* test_parselib.c */
IMPORT VOID test_parselib_main(unittest_driver_t *driver);

/* test_submitutil.c */
IMPORT VOID test_submitutil_main(unittest_driver_t *driver);

/* test_sjisstring.c */
IMPORT VOID test_sjistring_main(unittest_driver_t *driver);

/* test_residhash.c */
IMPORT VOID test_residhash_main(unittest_driver_t *driver);

/* test_resindexhash.c */
IMPORT VOID test_resindexhash_main(unittest_driver_t *driver);

/* test_postres.c */
IMPORT VOID test_postres_main(unittest_driver_t *driver);

/* test_tadimf.c */
IMPORT VOID test_tadimf_main(unittest_driver_t *driver);

/* test_wordlist.c */
IMPORT VOID test_wordlist_main(unittest_driver_t *driver);

/* test_tadsearch.c */
IMPORT VOID test_tadsearch_main(unittest_driver_t *driver);

/* test_httpheaderlexer.c */
IMPORT VOID test_httpheaderlexer_main(unittest_driver_t *driver);

/* test_setcookieheader.c */
IMPORT VOID test_setcookieheader_main(unittest_driver_t *driver);

/* test_httpdateparser.c */
IMPORT VOID test_httpdateparser_main(unittest_driver_t *driver);

/* test_cookiedb.c */
IMPORT VOID test_cookiedb_main(unittest_driver_t *driver);

/* test_psvlexer.c */
IMPORT VOID test_psvlexer_main(unittest_driver_t *driver);

#endif
