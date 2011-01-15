/*
 * test_wordlist.c
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

#include    <btron/btron.h>
#include    <bstdio.h>
#include    <tcode.h>
#include    <tstring.h>

#include    "test.h"

#include    "wordlist.h"

LOCAL TC test_wordlist_testdata001[] = {TK_A, TK_A, TK_A, TNULL};
LOCAL TC test_wordlist_testdata002[] = {TK_B, TK_B, TK_B, TNULL};
LOCAL TC test_wordlist_testdata003[] = {TK_C, TK_C, TK_C, TNULL};
LOCAL TC test_wordlist_testdata004[] = {TK_D, TK_D, TK_D, TNULL};

LOCAL TEST_RESULT test_wordlist_1()
{
	wordlist_t wlist;
	W err;

	err = wordlist_initialize(&wlist);
	if (err < 0) {
		return TEST_RESULT_FAIL;
	}

	wordlist_finalize(&wlist);

	return TEST_RESULT_PASS;
}

LOCAL TEST_RESULT test_wordlist_2()
{
	wordlist_t wlist;
	W err;
	TEST_RESULT result = TEST_RESULT_PASS;

	err = wordlist_initialize(&wlist);
	if (err < 0) {
		return TEST_RESULT_FAIL;
	}

	err = wordlist_appendword(&wlist, test_wordlist_testdata001, 3);
	if (err < 0) {
		result = TEST_RESULT_FAIL;
	}

	wordlist_finalize(&wlist);

	return result;
}

LOCAL TEST_RESULT test_wordlist_3()
{
	wordlist_t wlist;
	W err;
	TEST_RESULT result = TEST_RESULT_PASS;

	err = wordlist_initialize(&wlist);
	if (err < 0) {
		return TEST_RESULT_FAIL;
	}

	err = wordlist_appendword(&wlist, test_wordlist_testdata001, 3);
	if (err < 0) {
		result = TEST_RESULT_FAIL;
	}
	err = wordlist_appendword(&wlist, test_wordlist_testdata002, 3);
	if (err < 0) {
		result = TEST_RESULT_FAIL;
	}
	err = wordlist_appendword(&wlist, test_wordlist_testdata003, 3);
	if (err < 0) {
		result = TEST_RESULT_FAIL;
	}

	wordlist_finalize(&wlist);

	return result;
}

LOCAL TEST_RESULT test_wordlist_4()
{
	wordlist_t wlist;
	W i, len, err;
	Bool found;
	TC *str;
	TEST_RESULT result = TEST_RESULT_PASS;

	err = wordlist_initialize(&wlist);
	if (err < 0) {
		return TEST_RESULT_FAIL;
	}

	for (i = 0; i < 100; i++) {
		found = wordlist_searchwordbyindex(&wlist, i, &str, &len);
		if (found == True) {
			result = TEST_RESULT_FAIL;
			break;
		}
	}

	wordlist_finalize(&wlist);

	return result;
}

LOCAL TEST_RESULT test_wordlist_5()
{
	wordlist_t wlist;
	W i, len, err;
	Bool found;
	TC *str;
	TEST_RESULT result = TEST_RESULT_PASS;

	err = wordlist_initialize(&wlist);
	if (err < 0) {
		return TEST_RESULT_FAIL;
	}

	err = wordlist_appendword(&wlist, test_wordlist_testdata001, 3);
	if (err < 0) {
		result = TEST_RESULT_FAIL;
	}

	for (i = 0; i < 100; i++) {
		found = wordlist_searchwordbyindex(&wlist, i, &str, &len);
		if (i == 0) {
			if (found == False) {
				result = TEST_RESULT_FAIL;
				break;
			}
		} else {
			if (found == True) {
				result = TEST_RESULT_FAIL;
				break;
			}
		}
	}

	wordlist_finalize(&wlist);

	return result;
}

LOCAL TEST_RESULT test_wordlist_6()
{
	wordlist_t wlist;
	W i, len, err;
	Bool found;
	TC *str;
	TEST_RESULT result = TEST_RESULT_PASS;

	err = wordlist_initialize(&wlist);
	if (err < 0) {
		return TEST_RESULT_FAIL;
	}

	err = wordlist_appendword(&wlist, test_wordlist_testdata001, 3);
	if (err < 0) {
		result = TEST_RESULT_FAIL;
	}
	err = wordlist_appendword(&wlist, test_wordlist_testdata002, 3);
	if (err < 0) {
		result = TEST_RESULT_FAIL;
	}
	err = wordlist_appendword(&wlist, test_wordlist_testdata003, 3);
	if (err < 0) {
		result = TEST_RESULT_FAIL;
	}

	for (i = 0; i < 100; i++) {
		found = wordlist_searchwordbyindex(&wlist, i, &str, &len);
		if (i < 3) {
			if (found == False) {
				result = TEST_RESULT_FAIL;
				break;
			}
		} else {
			if (found == True) {
				result = TEST_RESULT_FAIL;
				break;
			}
		}
	}

	wordlist_finalize(&wlist);

	return result;
}

LOCAL TEST_RESULT test_wordlist_7()
{
	wordlist_t wlist;
	W err;
	Bool exist;
	TEST_RESULT result = TEST_RESULT_PASS;

	err = wordlist_initialize(&wlist);
	if (err < 0) {
		return TEST_RESULT_FAIL;
	}

	exist = wordlist_checkexistbyword(&wlist, test_wordlist_testdata001, 3);
	if (exist != False) {
		result = TEST_RESULT_FAIL;
	}
	exist = wordlist_checkexistbyword(&wlist, test_wordlist_testdata002, 3);
	if (exist != False) {
		result = TEST_RESULT_FAIL;
	}
	exist = wordlist_checkexistbyword(&wlist, test_wordlist_testdata003, 3);
	if (exist != False) {
		result = TEST_RESULT_FAIL;
	}
	exist = wordlist_checkexistbyword(&wlist, test_wordlist_testdata004, 3);
	if (exist != False) {
		result = TEST_RESULT_FAIL;
	}

	wordlist_finalize(&wlist);

	return result;
}

LOCAL TEST_RESULT test_wordlist_8()
{
	wordlist_t wlist;
	W err;
	Bool exist;
	TEST_RESULT result = TEST_RESULT_PASS;

	err = wordlist_initialize(&wlist);
	if (err < 0) {
		return TEST_RESULT_FAIL;
	}

	err = wordlist_appendword(&wlist, test_wordlist_testdata001, 3);
	if (err < 0) {
		result = TEST_RESULT_FAIL;
	}

	exist = wordlist_checkexistbyword(&wlist, test_wordlist_testdata001, 3);
	if (exist != True) {
		result = TEST_RESULT_FAIL;
	}
	exist = wordlist_checkexistbyword(&wlist, test_wordlist_testdata002, 3);
	if (exist != False) {
		result = TEST_RESULT_FAIL;
	}
	exist = wordlist_checkexistbyword(&wlist, test_wordlist_testdata003, 3);
	if (exist != False) {
		result = TEST_RESULT_FAIL;
	}
	exist = wordlist_checkexistbyword(&wlist, test_wordlist_testdata004, 3);
	if (exist != False) {
		result = TEST_RESULT_FAIL;
	}

	wordlist_finalize(&wlist);

	return result;
}

LOCAL TEST_RESULT test_wordlist_9()
{
	wordlist_t wlist;
	W err;
	Bool exist;
	TEST_RESULT result = TEST_RESULT_PASS;

	err = wordlist_initialize(&wlist);
	if (err < 0) {
		return TEST_RESULT_FAIL;
	}

	err = wordlist_appendword(&wlist, test_wordlist_testdata001, 3);
	if (err < 0) {
		result = TEST_RESULT_FAIL;
	}
	err = wordlist_appendword(&wlist, test_wordlist_testdata002, 3);
	if (err < 0) {
		result = TEST_RESULT_FAIL;
	}
	err = wordlist_appendword(&wlist, test_wordlist_testdata003, 3);
	if (err < 0) {
		result = TEST_RESULT_FAIL;
	}

	exist = wordlist_checkexistbyword(&wlist, test_wordlist_testdata001, 3);
	if (exist != True) {
		result = TEST_RESULT_FAIL;
	}
	exist = wordlist_checkexistbyword(&wlist, test_wordlist_testdata002, 3);
	if (exist != True) {
		result = TEST_RESULT_FAIL;
	}
	exist = wordlist_checkexistbyword(&wlist, test_wordlist_testdata003, 3);
	if (exist != True) {
		result = TEST_RESULT_FAIL;
	}
	exist = wordlist_checkexistbyword(&wlist, test_wordlist_testdata004, 3);
	if (exist != False) {
		result = TEST_RESULT_FAIL;
	}

	wordlist_finalize(&wlist);

	return result;
}

LOCAL TEST_RESULT test_wordlist_10()
{
	wordlist_t wlist;
	W err;
	Bool exist;
	TEST_RESULT result = TEST_RESULT_PASS;

	err = wordlist_initialize(&wlist);
	if (err < 0) {
		return TEST_RESULT_FAIL;
	}

	exist = wordlist_removeword(&wlist, test_wordlist_testdata001, 3);
	if (exist != False) {
		result = TEST_RESULT_FAIL;
	}
	exist = wordlist_removeword(&wlist, test_wordlist_testdata002, 3);
	if (exist != False) {
		result = TEST_RESULT_FAIL;
	}
	exist = wordlist_removeword(&wlist, test_wordlist_testdata003, 3);
	if (exist != False) {
		result = TEST_RESULT_FAIL;
	}
	exist = wordlist_removeword(&wlist, test_wordlist_testdata004, 3);
	if (exist != False) {
		result = TEST_RESULT_FAIL;
	}

	wordlist_finalize(&wlist);

	return result;
}

LOCAL TEST_RESULT test_wordlist_11()
{
	wordlist_t wlist;
	W err;
	Bool exist;
	TEST_RESULT result = TEST_RESULT_PASS;

	err = wordlist_initialize(&wlist);
	if (err < 0) {
		return TEST_RESULT_FAIL;
	}

	err = wordlist_appendword(&wlist, test_wordlist_testdata001, 3);
	if (err < 0) {
		result = TEST_RESULT_FAIL;
	}

	exist = wordlist_removeword(&wlist, test_wordlist_testdata001, 3);
	if (exist != True) {
		result = TEST_RESULT_FAIL;
	}
	exist = wordlist_removeword(&wlist, test_wordlist_testdata002, 3);
	if (exist != False) {
		result = TEST_RESULT_FAIL;
	}
	exist = wordlist_removeword(&wlist, test_wordlist_testdata003, 3);
	if (exist != False) {
		result = TEST_RESULT_FAIL;
	}
	exist = wordlist_removeword(&wlist, test_wordlist_testdata004, 3);
	if (exist != False) {
		result = TEST_RESULT_FAIL;
	}

	wordlist_finalize(&wlist);

	return result;
}

LOCAL TEST_RESULT test_wordlist_12()
{
	wordlist_t wlist;
	W err;
	Bool exist;
	TEST_RESULT result = TEST_RESULT_PASS;

	err = wordlist_initialize(&wlist);
	if (err < 0) {
		return TEST_RESULT_FAIL;
	}

	err = wordlist_appendword(&wlist, test_wordlist_testdata001, 3);
	if (err < 0) {
		result = TEST_RESULT_FAIL;
	}
	err = wordlist_appendword(&wlist, test_wordlist_testdata002, 3);
	if (err < 0) {
		result = TEST_RESULT_FAIL;
	}
	err = wordlist_appendword(&wlist, test_wordlist_testdata003, 3);
	if (err < 0) {
		result = TEST_RESULT_FAIL;
	}

	exist = wordlist_removeword(&wlist, test_wordlist_testdata001, 3);
	if (exist != True) {
		result = TEST_RESULT_FAIL;
	}
	exist = wordlist_removeword(&wlist, test_wordlist_testdata002, 3);
	if (exist != True) {
		result = TEST_RESULT_FAIL;
	}
	exist = wordlist_removeword(&wlist, test_wordlist_testdata003, 3);
	if (exist != True) {
		result = TEST_RESULT_FAIL;
	}
	exist = wordlist_removeword(&wlist, test_wordlist_testdata004, 3);
	if (exist != False) {
		result = TEST_RESULT_FAIL;
	}

	wordlist_finalize(&wlist);

	return result;
}

LOCAL TEST_RESULT test_wordlist_13()
{
	wordlist_t wlist;
	W len, err;
	Bool cont;
	TC *str;
	TEST_RESULT result = TEST_RESULT_PASS;
	wordlist_iterator_t iter;

	err = wordlist_initialize(&wlist);
	if (err < 0) {
		return TEST_RESULT_FAIL;
	}

	wordlist_iterator_initialize(&iter, &wlist);

	cont = wordlist_iterator_next(&iter, &str, &len);
	if (cont != False) {
		result = TEST_RESULT_FAIL;
	}

	wordlist_iterator_finalize(&iter);

	wordlist_finalize(&wlist);

	return result;
}

LOCAL TEST_RESULT test_wordlist_14()
{
	wordlist_t wlist;
	W len, err;
	Bool cont;
	TC *str;
	TEST_RESULT result = TEST_RESULT_PASS;
	wordlist_iterator_t iter;

	err = wordlist_initialize(&wlist);
	if (err < 0) {
		return TEST_RESULT_FAIL;
	}

	err = wordlist_appendword(&wlist, test_wordlist_testdata001, 3);
	if (err < 0) {
		result = TEST_RESULT_FAIL;
	}

	wordlist_iterator_initialize(&iter, &wlist);

	cont = wordlist_iterator_next(&iter, &str, &len);
	if (cont != True) {
		result = TEST_RESULT_FAIL;
	}
	if (len != 3) {
		result = TEST_RESULT_FAIL;
	}
	if (tc_strncmp(str, test_wordlist_testdata001, 3) != 0) {
		result = TEST_RESULT_FAIL;
	}
	cont = wordlist_iterator_next(&iter, &str, &len);
	if (cont != False) {
		result = TEST_RESULT_FAIL;
	}

	wordlist_iterator_finalize(&iter);

	wordlist_finalize(&wlist);

	return result;
}

LOCAL TEST_RESULT test_wordlist_15()
{
	wordlist_t wlist;
	W len, err;
	Bool cont;
	TC *str;
	TEST_RESULT result = TEST_RESULT_PASS;
	wordlist_iterator_t iter;

	err = wordlist_initialize(&wlist);
	if (err < 0) {
		return TEST_RESULT_FAIL;
	}

	err = wordlist_appendword(&wlist, test_wordlist_testdata001, 3);
	if (err < 0) {
		result = TEST_RESULT_FAIL;
	}
	err = wordlist_appendword(&wlist, test_wordlist_testdata002, 3);
	if (err < 0) {
		result = TEST_RESULT_FAIL;
	}
	err = wordlist_appendword(&wlist, test_wordlist_testdata003, 3);
	if (err < 0) {
		result = TEST_RESULT_FAIL;
	}

	wordlist_iterator_initialize(&iter, &wlist);

	cont = wordlist_iterator_next(&iter, &str, &len);
	if (cont != True) {
		result = TEST_RESULT_FAIL;
	}
	if (len != 3) {
		result = TEST_RESULT_FAIL;
	}
	if (tc_strncmp(str, test_wordlist_testdata001, 3) != 0) {
		result = TEST_RESULT_FAIL;
	}
	cont = wordlist_iterator_next(&iter, &str, &len);
	if (cont != True) {
		result = TEST_RESULT_FAIL;
	}
	if (len != 3) {
		result = TEST_RESULT_FAIL;
	}
	if (tc_strncmp(str, test_wordlist_testdata002, 3) != 0) {
		result = TEST_RESULT_FAIL;
	}
	cont = wordlist_iterator_next(&iter, &str, &len);
	if (cont != True) {
		result = TEST_RESULT_FAIL;
	}
	if (len != 3) {
		result = TEST_RESULT_FAIL;
	}
	if (tc_strncmp(str, test_wordlist_testdata003, 3) != 0) {
		result = TEST_RESULT_FAIL;
	}
	cont = wordlist_iterator_next(&iter, &str, &len);
	if (cont != False) {
		result = TEST_RESULT_FAIL;
	}

	wordlist_iterator_finalize(&iter);

	wordlist_finalize(&wlist);

	return result;
}

LOCAL TEST_RESULT test_wordlist_16()
{
	wordlist_t wlist;
	W err;
	Bool empty;
	TEST_RESULT result = TEST_RESULT_PASS;

	err = wordlist_initialize(&wlist);
	if (err < 0) {
		return TEST_RESULT_FAIL;
	}

	empty = wordlist_isempty(&wlist);
	if (empty != True) {
		result = TEST_RESULT_FAIL;
	}

	wordlist_finalize(&wlist);

	return result;
}

LOCAL TEST_RESULT test_wordlist_17()
{
	wordlist_t wlist;
	W err;
	Bool empty;
	TEST_RESULT result = TEST_RESULT_PASS;

	err = wordlist_initialize(&wlist);
	if (err < 0) {
		return TEST_RESULT_FAIL;
	}

	err = wordlist_appendword(&wlist, test_wordlist_testdata001, 3);
	if (err < 0) {
		result = TEST_RESULT_FAIL;
	}

	empty = wordlist_isempty(&wlist);
	if (empty != False) {
		result = TEST_RESULT_FAIL;
	}

	wordlist_finalize(&wlist);

	return result;
}

LOCAL TEST_RESULT test_wordlist_18()
{
	wordlist_t wlist;
	W err;
	Bool empty;
	TEST_RESULT result = TEST_RESULT_PASS;

	err = wordlist_initialize(&wlist);
	if (err < 0) {
		return TEST_RESULT_FAIL;
	}

	err = wordlist_appendword(&wlist, test_wordlist_testdata001, 3);
	if (err < 0) {
		result = TEST_RESULT_FAIL;
	}
	wordlist_removeword(&wlist, test_wordlist_testdata001, 3);

	empty = wordlist_isempty(&wlist);
	if (empty != True) {
		result = TEST_RESULT_FAIL;
	}

	wordlist_finalize(&wlist);

	return result;
}

LOCAL VOID test_wordlist_printresult(TEST_RESULT (*proc)(), B *test_name)
{
	TEST_RESULT result;

	printf("test_wordlist: %s\n", test_name);
	printf("---------------------------------------------\n");
	result = proc();
	if (result == TEST_RESULT_PASS) {
		printf("--pass---------------------------------------\n");
	} else {
		printf("--fail---------------------------------------\n");
	}
	printf("---------------------------------------------\n");
}

EXPORT VOID test_wordlist_main()
{
	test_wordlist_printresult(test_wordlist_1, "test_wordlist_1");
	test_wordlist_printresult(test_wordlist_2, "test_wordlist_2");
	test_wordlist_printresult(test_wordlist_3, "test_wordlist_3");
	test_wordlist_printresult(test_wordlist_4, "test_wordlist_4");
	test_wordlist_printresult(test_wordlist_5, "test_wordlist_5");
	test_wordlist_printresult(test_wordlist_6, "test_wordlist_6");
	test_wordlist_printresult(test_wordlist_7, "test_wordlist_7");
	test_wordlist_printresult(test_wordlist_8, "test_wordlist_8");
	test_wordlist_printresult(test_wordlist_9, "test_wordlist_9");
	test_wordlist_printresult(test_wordlist_10, "test_wordlist_10");
	test_wordlist_printresult(test_wordlist_11, "test_wordlist_11");
	test_wordlist_printresult(test_wordlist_12, "test_wordlist_12");
	test_wordlist_printresult(test_wordlist_13, "test_wordlist_13");
	test_wordlist_printresult(test_wordlist_14, "test_wordlist_14");
	test_wordlist_printresult(test_wordlist_15, "test_wordlist_15");
	test_wordlist_printresult(test_wordlist_16, "test_wordlist_16");
	test_wordlist_printresult(test_wordlist_17, "test_wordlist_17");
	test_wordlist_printresult(test_wordlist_18, "test_wordlist_18");
}
