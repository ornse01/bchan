/*
 * test_psvlexer.c
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

#include    "psvlexer.h"

#include    <btron/btron.h>
#include    <bstdio.h>
#include    <bstdlib.h>
#include    <bstring.h>

#include    <unittest_driver.h>

#if 0
# define DUMP_RESULT(arg) printf arg
#else
# define DUMP_RESULT(arg) /**/
#endif

struct testpsvlexer_result_t_ {
	UB *str;
	W len;
};
typedef struct testpsvlexer_result_t_ testpsvlexer_result_t;

LOCAL W testpsvlexer_result_append(testpsvlexer_result_t *testresult, UB *apd, W apd_len)
{
	UB *str;

	str = realloc(testresult->str, testresult->len + apd_len + 1);
	if (str == NULL) {
		return -1;
	}
	testresult->str = str;

	memcpy(testresult->str + testresult->len, apd, apd_len);
	testresult->len += apd_len;
	testresult->str[testresult->len] = '\0';

	return 0;
}

LOCAL W testpsvlexer_result_inputresult(testpsvlexer_result_t *testresult, psvlexer_result_t *result)
{
	W i;

	switch (result->type) {
	case PSVLEXER_RESULT_VALUE:
		DUMP_RESULT(("type = VALUE: "));
		for (i = 0; i < result->len; i++) {
			DUMP_RESULT(("%c", result->str[i]));
		}
		DUMP_RESULT(("\n"));
		return testpsvlexer_result_append(testresult, result->str, result->len);
	case PSVLEXER_RESULT_SEPARATION:
		DUMP_RESULT(("type = SEPARATION\n"));
		return testpsvlexer_result_append(testresult, "<>", 2);
	case PSVLEXER_RESULT_FIELDEND:
		DUMP_RESULT(("type = FIELDEND\n"));
		return testpsvlexer_result_append(testresult, "\n", 1);
	case PSVLEXER_RESULT_ERROR:
		DUMP_RESULT(("type = ERROR\n"));
		return 0;
	default:
		DUMP_RESULT(("error type value\n"));
		return 0;
	}

	return 0;
}

LOCAL W testpsvlexer_result_inputresultarray(testpsvlexer_result_t *testresult, psvlexer_result_t *result, W result_len)
{
	W i, err;
	for (i = 0; i < result_len; i++) {
		err = testpsvlexer_result_inputresult(testresult, result + i);
		if (err < 0) {
			return err;
		}
	}
	return 0;
}

LOCAL Bool testpsvlexer_result_compaire(testpsvlexer_result_t *testresult, UB *expected, W expected_len)
{
	if (testresult->len != expected_len) {
		return False;
	}
	if (strncmp(testresult->str, expected, expected_len) != 0) {
		return False;
	}
	return True;
}

LOCAL W testpsvlexer_result_initialize(testpsvlexer_result_t *testresult)
{
	testresult->str = malloc(sizeof(UB));
	if (testresult->str == NULL) {
		return -1;
	}
	testresult->str[0] = '\0';
	testresult->len = 0;
	return 0;
}

LOCAL VOID testpsvlexer_result_finalize(testpsvlexer_result_t *testresult)
{
	free(testresult->str);
}

LOCAL UB test_psvlexer_testdata01[] = "aaa<>bbb<>ccc<>\nddd<>eee<>fff\n";
LOCAL UB test_psvlexer_testdata02[] = "<aa<>b<b<>cc<<>dd<\n";

LOCAL UNITTEST_RESULT test_psvlexer_common(UB *test, W test_len)
{
	psvlexer_t lexer;
	psvlexer_result_t *result;
	testpsvlexer_result_t testresult;
	W i, err, result_len;
	UNITTEST_RESULT ret = UNITTEST_RESULT_PASS;
	Bool ok;

	err = psvlexer_initialize(&lexer);
	if (err < 0) {
		return UNITTEST_RESULT_FAIL;
	}

	err = testpsvlexer_result_initialize(&testresult);
	if (err < 0) {
		psvlexer_finalize(&lexer);
		return UNITTEST_RESULT_FAIL;
	}

	for (i = 0; i < test_len; i++) {
		psvlexer_inputchar(&lexer, test + i, 1, &result, &result_len);
		err = testpsvlexer_result_inputresultarray(&testresult, result, result_len);
		if (err < 0) {
			ret = UNITTEST_RESULT_FAIL;
		}
	}
	ok = testpsvlexer_result_compaire(&testresult, test, test_len);
	if (ok == False) {
		ret = UNITTEST_RESULT_FAIL;
	}

	testpsvlexer_result_finalize(&testresult);
	psvlexer_finalize(&lexer);

	return ret;
}

LOCAL UNITTEST_RESULT test_psvlexer_1()
{
	return test_psvlexer_common(test_psvlexer_testdata01, strlen(test_psvlexer_testdata01));
}

LOCAL UNITTEST_RESULT test_psvlexer_2()
{
	return test_psvlexer_common(test_psvlexer_testdata02, strlen(test_psvlexer_testdata02));
}

EXPORT VOID test_psvlexer_main(unittest_driver_t *driver)
{
	UNITTEST_DRIVER_REGIST(driver, test_psvlexer_1);
	UNITTEST_DRIVER_REGIST(driver, test_psvlexer_2);
}
