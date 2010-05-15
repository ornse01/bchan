/*
 * test.h
 *
 * Copyright (c) 2009-2010 project bchan
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

#ifndef __BCHAN_TEST_H__
#define __BCHAN_TEST_H__

typedef enum {
	TEST_RESULT_PASS,
	TEST_RESULT_FAIL
} TEST_RESULT;

/* test_cache.c */
IMPORT VOID test_cache_main();

/* test_parser.c */
IMPORT VOID test_parser_main();

/* test_layout.c */
IMPORT VOID test_layout_main();

/* test_parselib.c */
IMPORT VOID test_parselib_main();

/* test_submitutil.c */
IMPORT VOID test_submitutil_main();

/* test_sjisstring.c */
IMPORT VOID test_sjistring_main();

/* test_residhash.c */
IMPORT VOID test_residhash_main();

#endif
