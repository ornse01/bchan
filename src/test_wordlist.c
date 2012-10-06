/*
 * test_wordlist.c
 *
 * Copyright (c) 2011-2012 project bchan
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

#include    "test.h"

#include    "wordlist.h"

#include    <btron/btron.h>
#include    <bstdio.h>
#include    <tcode.h>
#include    <tstring.h>

#include    <unittest_driver.h>

LOCAL TC test_wordlist_testdata001[] = {TK_A, TK_A, TK_A, TNULL};
LOCAL TC test_wordlist_testdata002[] = {TK_B, TK_B, TK_B, TNULL};
LOCAL TC test_wordlist_testdata003[] = {TK_C, TK_C, TK_C, TNULL};
LOCAL TC test_wordlist_testdata004[] = {TK_D, TK_D, TK_D, TNULL};

LOCAL UNITTEST_RESULT test_wordlist_1()
{
	wordlist_t wlist;
	W err;

	err = wordlist_initialize(&wlist);
	if (err < 0) {
		return UNITTEST_RESULT_FAIL;
	}

	wordlist_finalize(&wlist);

	return UNITTEST_RESULT_PASS;
}

LOCAL UNITTEST_RESULT test_wordlist_2()
{
	wordlist_t wlist;
	W err;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	err = wordlist_initialize(&wlist);
	if (err < 0) {
		return UNITTEST_RESULT_FAIL;
	}

	err = wordlist_appendword(&wlist, test_wordlist_testdata001, 3);
	if (err < 0) {
		result = UNITTEST_RESULT_FAIL;
	}

	wordlist_finalize(&wlist);

	return result;
}

LOCAL UNITTEST_RESULT test_wordlist_3()
{
	wordlist_t wlist;
	W err;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	err = wordlist_initialize(&wlist);
	if (err < 0) {
		return UNITTEST_RESULT_FAIL;
	}

	err = wordlist_appendword(&wlist, test_wordlist_testdata001, 3);
	if (err < 0) {
		result = UNITTEST_RESULT_FAIL;
	}
	err = wordlist_appendword(&wlist, test_wordlist_testdata002, 3);
	if (err < 0) {
		result = UNITTEST_RESULT_FAIL;
	}
	err = wordlist_appendword(&wlist, test_wordlist_testdata003, 3);
	if (err < 0) {
		result = UNITTEST_RESULT_FAIL;
	}

	wordlist_finalize(&wlist);

	return result;
}

LOCAL UNITTEST_RESULT test_wordlist_4()
{
	wordlist_t wlist;
	W i, len, err;
	Bool found;
	TC *str;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	err = wordlist_initialize(&wlist);
	if (err < 0) {
		return UNITTEST_RESULT_FAIL;
	}

	for (i = 0; i < 100; i++) {
		found = wordlist_searchwordbyindex(&wlist, i, &str, &len);
		if (found == True) {
			result = UNITTEST_RESULT_FAIL;
			break;
		}
	}

	wordlist_finalize(&wlist);

	return result;
}

LOCAL UNITTEST_RESULT test_wordlist_5()
{
	wordlist_t wlist;
	W i, len, err;
	Bool found;
	TC *str;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	err = wordlist_initialize(&wlist);
	if (err < 0) {
		return UNITTEST_RESULT_FAIL;
	}

	err = wordlist_appendword(&wlist, test_wordlist_testdata001, 3);
	if (err < 0) {
		result = UNITTEST_RESULT_FAIL;
	}

	for (i = 0; i < 100; i++) {
		found = wordlist_searchwordbyindex(&wlist, i, &str, &len);
		if (i == 0) {
			if (found == False) {
				result = UNITTEST_RESULT_FAIL;
				break;
			}
		} else {
			if (found == True) {
				result = UNITTEST_RESULT_FAIL;
				break;
			}
		}
	}

	wordlist_finalize(&wlist);

	return result;
}

LOCAL UNITTEST_RESULT test_wordlist_6()
{
	wordlist_t wlist;
	W i, len, err;
	Bool found;
	TC *str;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	err = wordlist_initialize(&wlist);
	if (err < 0) {
		return UNITTEST_RESULT_FAIL;
	}

	err = wordlist_appendword(&wlist, test_wordlist_testdata001, 3);
	if (err < 0) {
		result = UNITTEST_RESULT_FAIL;
	}
	err = wordlist_appendword(&wlist, test_wordlist_testdata002, 3);
	if (err < 0) {
		result = UNITTEST_RESULT_FAIL;
	}
	err = wordlist_appendword(&wlist, test_wordlist_testdata003, 3);
	if (err < 0) {
		result = UNITTEST_RESULT_FAIL;
	}

	for (i = 0; i < 100; i++) {
		found = wordlist_searchwordbyindex(&wlist, i, &str, &len);
		if (i < 3) {
			if (found == False) {
				result = UNITTEST_RESULT_FAIL;
				break;
			}
		} else {
			if (found == True) {
				result = UNITTEST_RESULT_FAIL;
				break;
			}
		}
	}

	wordlist_finalize(&wlist);

	return result;
}

LOCAL UNITTEST_RESULT test_wordlist_7()
{
	wordlist_t wlist;
	W err;
	Bool exist;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	err = wordlist_initialize(&wlist);
	if (err < 0) {
		return UNITTEST_RESULT_FAIL;
	}

	exist = wordlist_checkexistbyword(&wlist, test_wordlist_testdata001, 3);
	if (exist != False) {
		result = UNITTEST_RESULT_FAIL;
	}
	exist = wordlist_checkexistbyword(&wlist, test_wordlist_testdata002, 3);
	if (exist != False) {
		result = UNITTEST_RESULT_FAIL;
	}
	exist = wordlist_checkexistbyword(&wlist, test_wordlist_testdata003, 3);
	if (exist != False) {
		result = UNITTEST_RESULT_FAIL;
	}
	exist = wordlist_checkexistbyword(&wlist, test_wordlist_testdata004, 3);
	if (exist != False) {
		result = UNITTEST_RESULT_FAIL;
	}

	wordlist_finalize(&wlist);

	return result;
}

LOCAL UNITTEST_RESULT test_wordlist_8()
{
	wordlist_t wlist;
	W err;
	Bool exist;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	err = wordlist_initialize(&wlist);
	if (err < 0) {
		return UNITTEST_RESULT_FAIL;
	}

	err = wordlist_appendword(&wlist, test_wordlist_testdata001, 3);
	if (err < 0) {
		result = UNITTEST_RESULT_FAIL;
	}

	exist = wordlist_checkexistbyword(&wlist, test_wordlist_testdata001, 3);
	if (exist != True) {
		result = UNITTEST_RESULT_FAIL;
	}
	exist = wordlist_checkexistbyword(&wlist, test_wordlist_testdata002, 3);
	if (exist != False) {
		result = UNITTEST_RESULT_FAIL;
	}
	exist = wordlist_checkexistbyword(&wlist, test_wordlist_testdata003, 3);
	if (exist != False) {
		result = UNITTEST_RESULT_FAIL;
	}
	exist = wordlist_checkexistbyword(&wlist, test_wordlist_testdata004, 3);
	if (exist != False) {
		result = UNITTEST_RESULT_FAIL;
	}

	wordlist_finalize(&wlist);

	return result;
}

LOCAL UNITTEST_RESULT test_wordlist_9()
{
	wordlist_t wlist;
	W err;
	Bool exist;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	err = wordlist_initialize(&wlist);
	if (err < 0) {
		return UNITTEST_RESULT_FAIL;
	}

	err = wordlist_appendword(&wlist, test_wordlist_testdata001, 3);
	if (err < 0) {
		result = UNITTEST_RESULT_FAIL;
	}
	err = wordlist_appendword(&wlist, test_wordlist_testdata002, 3);
	if (err < 0) {
		result = UNITTEST_RESULT_FAIL;
	}
	err = wordlist_appendword(&wlist, test_wordlist_testdata003, 3);
	if (err < 0) {
		result = UNITTEST_RESULT_FAIL;
	}

	exist = wordlist_checkexistbyword(&wlist, test_wordlist_testdata001, 3);
	if (exist != True) {
		result = UNITTEST_RESULT_FAIL;
	}
	exist = wordlist_checkexistbyword(&wlist, test_wordlist_testdata002, 3);
	if (exist != True) {
		result = UNITTEST_RESULT_FAIL;
	}
	exist = wordlist_checkexistbyword(&wlist, test_wordlist_testdata003, 3);
	if (exist != True) {
		result = UNITTEST_RESULT_FAIL;
	}
	exist = wordlist_checkexistbyword(&wlist, test_wordlist_testdata004, 3);
	if (exist != False) {
		result = UNITTEST_RESULT_FAIL;
	}

	wordlist_finalize(&wlist);

	return result;
}

LOCAL UNITTEST_RESULT test_wordlist_10()
{
	wordlist_t wlist;
	W err;
	Bool exist;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	err = wordlist_initialize(&wlist);
	if (err < 0) {
		return UNITTEST_RESULT_FAIL;
	}

	exist = wordlist_removeword(&wlist, test_wordlist_testdata001, 3);
	if (exist != False) {
		result = UNITTEST_RESULT_FAIL;
	}
	exist = wordlist_removeword(&wlist, test_wordlist_testdata002, 3);
	if (exist != False) {
		result = UNITTEST_RESULT_FAIL;
	}
	exist = wordlist_removeword(&wlist, test_wordlist_testdata003, 3);
	if (exist != False) {
		result = UNITTEST_RESULT_FAIL;
	}
	exist = wordlist_removeword(&wlist, test_wordlist_testdata004, 3);
	if (exist != False) {
		result = UNITTEST_RESULT_FAIL;
	}

	wordlist_finalize(&wlist);

	return result;
}

LOCAL UNITTEST_RESULT test_wordlist_11()
{
	wordlist_t wlist;
	W err;
	Bool exist;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	err = wordlist_initialize(&wlist);
	if (err < 0) {
		return UNITTEST_RESULT_FAIL;
	}

	err = wordlist_appendword(&wlist, test_wordlist_testdata001, 3);
	if (err < 0) {
		result = UNITTEST_RESULT_FAIL;
	}

	exist = wordlist_removeword(&wlist, test_wordlist_testdata001, 3);
	if (exist != True) {
		result = UNITTEST_RESULT_FAIL;
	}
	exist = wordlist_removeword(&wlist, test_wordlist_testdata002, 3);
	if (exist != False) {
		result = UNITTEST_RESULT_FAIL;
	}
	exist = wordlist_removeword(&wlist, test_wordlist_testdata003, 3);
	if (exist != False) {
		result = UNITTEST_RESULT_FAIL;
	}
	exist = wordlist_removeword(&wlist, test_wordlist_testdata004, 3);
	if (exist != False) {
		result = UNITTEST_RESULT_FAIL;
	}

	wordlist_finalize(&wlist);

	return result;
}

LOCAL UNITTEST_RESULT test_wordlist_12()
{
	wordlist_t wlist;
	W err;
	Bool exist;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	err = wordlist_initialize(&wlist);
	if (err < 0) {
		return UNITTEST_RESULT_FAIL;
	}

	err = wordlist_appendword(&wlist, test_wordlist_testdata001, 3);
	if (err < 0) {
		result = UNITTEST_RESULT_FAIL;
	}
	err = wordlist_appendword(&wlist, test_wordlist_testdata002, 3);
	if (err < 0) {
		result = UNITTEST_RESULT_FAIL;
	}
	err = wordlist_appendword(&wlist, test_wordlist_testdata003, 3);
	if (err < 0) {
		result = UNITTEST_RESULT_FAIL;
	}

	exist = wordlist_removeword(&wlist, test_wordlist_testdata001, 3);
	if (exist != True) {
		result = UNITTEST_RESULT_FAIL;
	}
	exist = wordlist_removeword(&wlist, test_wordlist_testdata002, 3);
	if (exist != True) {
		result = UNITTEST_RESULT_FAIL;
	}
	exist = wordlist_removeword(&wlist, test_wordlist_testdata003, 3);
	if (exist != True) {
		result = UNITTEST_RESULT_FAIL;
	}
	exist = wordlist_removeword(&wlist, test_wordlist_testdata004, 3);
	if (exist != False) {
		result = UNITTEST_RESULT_FAIL;
	}

	wordlist_finalize(&wlist);

	return result;
}

LOCAL UNITTEST_RESULT test_wordlist_13()
{
	wordlist_t wlist;
	W len, err;
	Bool cont;
	TC *str;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;
	wordlist_iterator_t iter;

	err = wordlist_initialize(&wlist);
	if (err < 0) {
		return UNITTEST_RESULT_FAIL;
	}

	wordlist_iterator_initialize(&iter, &wlist);

	cont = wordlist_iterator_next(&iter, &str, &len);
	if (cont != False) {
		result = UNITTEST_RESULT_FAIL;
	}

	wordlist_iterator_finalize(&iter);

	wordlist_finalize(&wlist);

	return result;
}

LOCAL UNITTEST_RESULT test_wordlist_14()
{
	wordlist_t wlist;
	W len, err;
	Bool cont;
	TC *str;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;
	wordlist_iterator_t iter;

	err = wordlist_initialize(&wlist);
	if (err < 0) {
		return UNITTEST_RESULT_FAIL;
	}

	err = wordlist_appendword(&wlist, test_wordlist_testdata001, 3);
	if (err < 0) {
		result = UNITTEST_RESULT_FAIL;
	}

	wordlist_iterator_initialize(&iter, &wlist);

	cont = wordlist_iterator_next(&iter, &str, &len);
	if (cont != True) {
		result = UNITTEST_RESULT_FAIL;
	}
	if (len != 3) {
		result = UNITTEST_RESULT_FAIL;
	}
	if (tc_strncmp(str, test_wordlist_testdata001, 3) != 0) {
		result = UNITTEST_RESULT_FAIL;
	}
	cont = wordlist_iterator_next(&iter, &str, &len);
	if (cont != False) {
		result = UNITTEST_RESULT_FAIL;
	}

	wordlist_iterator_finalize(&iter);

	wordlist_finalize(&wlist);

	return result;
}

LOCAL UNITTEST_RESULT test_wordlist_15()
{
	wordlist_t wlist;
	W len, err;
	Bool cont;
	TC *str;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;
	wordlist_iterator_t iter;

	err = wordlist_initialize(&wlist);
	if (err < 0) {
		return UNITTEST_RESULT_FAIL;
	}

	err = wordlist_appendword(&wlist, test_wordlist_testdata001, 3);
	if (err < 0) {
		result = UNITTEST_RESULT_FAIL;
	}
	err = wordlist_appendword(&wlist, test_wordlist_testdata002, 3);
	if (err < 0) {
		result = UNITTEST_RESULT_FAIL;
	}
	err = wordlist_appendword(&wlist, test_wordlist_testdata003, 3);
	if (err < 0) {
		result = UNITTEST_RESULT_FAIL;
	}

	wordlist_iterator_initialize(&iter, &wlist);

	cont = wordlist_iterator_next(&iter, &str, &len);
	if (cont != True) {
		result = UNITTEST_RESULT_FAIL;
	}
	if (len != 3) {
		result = UNITTEST_RESULT_FAIL;
	}
	if (tc_strncmp(str, test_wordlist_testdata001, 3) != 0) {
		result = UNITTEST_RESULT_FAIL;
	}
	cont = wordlist_iterator_next(&iter, &str, &len);
	if (cont != True) {
		result = UNITTEST_RESULT_FAIL;
	}
	if (len != 3) {
		result = UNITTEST_RESULT_FAIL;
	}
	if (tc_strncmp(str, test_wordlist_testdata002, 3) != 0) {
		result = UNITTEST_RESULT_FAIL;
	}
	cont = wordlist_iterator_next(&iter, &str, &len);
	if (cont != True) {
		result = UNITTEST_RESULT_FAIL;
	}
	if (len != 3) {
		result = UNITTEST_RESULT_FAIL;
	}
	if (tc_strncmp(str, test_wordlist_testdata003, 3) != 0) {
		result = UNITTEST_RESULT_FAIL;
	}
	cont = wordlist_iterator_next(&iter, &str, &len);
	if (cont != False) {
		result = UNITTEST_RESULT_FAIL;
	}

	wordlist_iterator_finalize(&iter);

	wordlist_finalize(&wlist);

	return result;
}

LOCAL UNITTEST_RESULT test_wordlist_16()
{
	wordlist_t wlist;
	W err;
	Bool empty;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	err = wordlist_initialize(&wlist);
	if (err < 0) {
		return UNITTEST_RESULT_FAIL;
	}

	empty = wordlist_isempty(&wlist);
	if (empty != True) {
		result = UNITTEST_RESULT_FAIL;
	}

	wordlist_finalize(&wlist);

	return result;
}

LOCAL UNITTEST_RESULT test_wordlist_17()
{
	wordlist_t wlist;
	W err;
	Bool empty;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	err = wordlist_initialize(&wlist);
	if (err < 0) {
		return UNITTEST_RESULT_FAIL;
	}

	err = wordlist_appendword(&wlist, test_wordlist_testdata001, 3);
	if (err < 0) {
		result = UNITTEST_RESULT_FAIL;
	}

	empty = wordlist_isempty(&wlist);
	if (empty != False) {
		result = UNITTEST_RESULT_FAIL;
	}

	wordlist_finalize(&wlist);

	return result;
}

LOCAL UNITTEST_RESULT test_wordlist_18()
{
	wordlist_t wlist;
	W err;
	Bool empty;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	err = wordlist_initialize(&wlist);
	if (err < 0) {
		return UNITTEST_RESULT_FAIL;
	}

	err = wordlist_appendword(&wlist, test_wordlist_testdata001, 3);
	if (err < 0) {
		result = UNITTEST_RESULT_FAIL;
	}
	wordlist_removeword(&wlist, test_wordlist_testdata001, 3);

	empty = wordlist_isempty(&wlist);
	if (empty != True) {
		result = UNITTEST_RESULT_FAIL;
	}

	wordlist_finalize(&wlist);

	return result;
}

EXPORT VOID test_wordlist_main(unittest_driver_t *driver)
{
	UNITTEST_DRIVER_REGIST(driver, test_wordlist_1);
	UNITTEST_DRIVER_REGIST(driver, test_wordlist_2);
	UNITTEST_DRIVER_REGIST(driver, test_wordlist_3);
	UNITTEST_DRIVER_REGIST(driver, test_wordlist_4);
	UNITTEST_DRIVER_REGIST(driver, test_wordlist_5);
	UNITTEST_DRIVER_REGIST(driver, test_wordlist_6);
	UNITTEST_DRIVER_REGIST(driver, test_wordlist_7);
	UNITTEST_DRIVER_REGIST(driver, test_wordlist_8);
	UNITTEST_DRIVER_REGIST(driver, test_wordlist_9);
	UNITTEST_DRIVER_REGIST(driver, test_wordlist_10);
	UNITTEST_DRIVER_REGIST(driver, test_wordlist_11);
	UNITTEST_DRIVER_REGIST(driver, test_wordlist_12);
	UNITTEST_DRIVER_REGIST(driver, test_wordlist_13);
	UNITTEST_DRIVER_REGIST(driver, test_wordlist_14);
	UNITTEST_DRIVER_REGIST(driver, test_wordlist_15);
	UNITTEST_DRIVER_REGIST(driver, test_wordlist_16);
	UNITTEST_DRIVER_REGIST(driver, test_wordlist_17);
	UNITTEST_DRIVER_REGIST(driver, test_wordlist_18);
}
