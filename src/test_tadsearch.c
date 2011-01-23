/*
 * test_tadsearch.c
 *
 * Copyright (c) 2011 project bchan
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

#include    <bstdio.h>
#include    <tcode.h>

#include    "test.h"

#include    "tadsearch.h"

LOCAL TC test_tadsearch_testdata01[] = {TK_A, TK_B, TK_C, TNULL};
LOCAL TC test_tadsearch_testdata02[] = {TK_B, TK_C, TK_D, TNULL};
LOCAL TC test_tadsearch_testdata03[] = {TK_E, TK_F, TK_G, TK_E, TNULL};
LOCAL TC test_tadsearch_testdata04[] = {TK_a, TK_b, TK_c, TK_d, TK_e, TNULL};
LOCAL TC test_tadsearch_testdata05[] = {TK_a, TK_b, TK_A, TK_B, TK_C, TNULL};

LOCAL TEST_RESULT test_tadsearch_1()
{
	tcstrbuffer_t set[3];
	tadlib_rk_result_t result;
	W err;

	set[0].buffer = test_tadsearch_testdata01;
	set[0].strlen = 3;
	set[1].buffer = test_tadsearch_testdata02;
	set[1].strlen = 3;
	set[2].buffer = test_tadsearch_testdata03;
	set[2].strlen = 4;

	err = tadlib_rabinkarpsearch(test_tadsearch_testdata05, 5, set, 3, &result);
	if (err != 1) {
		printf("return value failure: %d\n", err);
		return TEST_RESULT_FAIL;
	}
	if (result.i != 0) {
		printf("result failure: %d\n", result.i);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_PASS;
}

LOCAL TEST_RESULT test_tadsearch_2()
{
	tcstrbuffer_t set[3];
	tadlib_rk_result_t result;
	W err;

	set[0].buffer = test_tadsearch_testdata01;
	set[0].strlen = 3;
	set[1].buffer = test_tadsearch_testdata02;
	set[1].strlen = 3;
	set[2].buffer = test_tadsearch_testdata03;
	set[2].strlen = 4;

	err = tadlib_rabinkarpsearch(test_tadsearch_testdata04, 5, set, 3, &result);
	if (err != 0) {
		printf("return value failure: %d\n", err);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_PASS;
}

LOCAL TEST_RESULT test_tadsearch_3()
{
	tcstrbuffer_t set[1];
	tadlib_rk_result_t result;
	W err;

	set[0].buffer = test_tadsearch_testdata01;
	set[0].strlen = 3;

	err = tadlib_rabinkarpsearch(test_tadsearch_testdata05, 5, set, 1, &result);
	if (err != 1) {
		printf("return value failure: %d\n", err);
		return TEST_RESULT_FAIL;
	}
	if (result.i != 0) {
		printf("result failure: %d\n", result.i);
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_PASS;
}

LOCAL VOID test_tadsearch_printresult(TEST_RESULT (*proc)(), B *test_name)
{
	TEST_RESULT result;

	printf("test_tadsearch: %s\n", test_name);
	printf("---------------------------------------------\n");
	result = proc();
	if (result == TEST_RESULT_PASS) {
		printf("--pass---------------------------------------\n");
	} else {
		printf("--fail---------------------------------------\n");
	}
	printf("---------------------------------------------\n");
}

EXPORT VOID test_tadsearch_main()
{
	test_tadsearch_printresult(test_tadsearch_1, "test_tadsearch_1");
	test_tadsearch_printresult(test_tadsearch_2, "test_tadsearch_2");
	test_tadsearch_printresult(test_tadsearch_3, "test_tadsearch_3");
}
