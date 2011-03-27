/*
 * test_httpdateparser.c
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

#include    "test.h"

#include    "httpdateparser.h"

LOCAL UB test_httpdateparser_testdata_01[] = "Friday, 01-Jan-2016 00:00:00 GMT";
LOCAL UB test_httpdateparser_testdata_02[] = "Sun, 10-Jun-2001 12:00:00 GMT";
LOCAL UB test_httpdateparser_testdata_03[] = "Mon, 31 Dec 2001 23:59:59 GMT";
LOCAL UB test_httpdateparser_testdata_04[] = "10-Jun-2001 12:00:00 GMT";
LOCAL UB test_httpdateparser_testdata_05[] = "31 Dec 2001 23:59:59 GMT";

LOCAL TEST_RESULT test_httpdateparser_check(UB *str, DATE_TIM *expected)
{
	W i,len, ret;
	rfc733dateparser_t parser;
	DATE_TIM dt;
	TEST_RESULT result = TEST_RESULT_PASS;

	len = strlen(str);

	rfc733dateparser_initialize(&parser);
	for (i = 0; i < len; i++) {
		ret = rfc733dateparser_inputchar(&parser, str[i], &dt);
		if (ret != HTTPDATEPARSER_CONTINUE) {
			printf("fail in parse at %d\n", i);
			result = TEST_RESULT_FAIL;
			break;
		}
	}
	ret = rfc733dateparser_endinput(&parser, &dt);
	if (ret != HTTPDATEPARSER_DETERMINE) {
		printf("fail at end\n");
		result = TEST_RESULT_FAIL;
	}
	rfc733dateparser_finalize(&parser);

	if (dt.d_year != expected->d_year) {
		printf("fail in d_year: %d, %d\n", dt.d_year, expected->d_year);
		result = TEST_RESULT_FAIL;
	}
	if (dt.d_month != expected->d_month) {
		printf("fail in d_month: %d, %d\n", dt.d_month, expected->d_month);
		result = TEST_RESULT_FAIL;
	}
	if (dt.d_day != expected->d_day) {
		printf("fail in d_day: %d, %d\n", dt.d_day, expected->d_day);
		result = TEST_RESULT_FAIL;
	}
	if (dt.d_hour != expected->d_hour) {
		printf("fail in d_hour: %d, %d\n", dt.d_hour, expected->d_hour);
		result = TEST_RESULT_FAIL;
	}
	if (dt.d_min != expected->d_min) {
		printf("fail in d_min: %d, %d\n", dt.d_min, expected->d_min);
		result = TEST_RESULT_FAIL;
	}
	if (dt.d_sec != expected->d_sec) {
		printf("fail in d_sec: %d, %d\n", dt.d_sec, expected->d_sec);
		result = TEST_RESULT_FAIL;
	}
	/* TODO: d_week, d_wday, d_days. */

	return result;
}

LOCAL TEST_RESULT test_httpdateparser_1()
{
	DATE_TIM expected;
	expected.d_year = 116;
	expected.d_month = 1;
	expected.d_day = 1;
	expected.d_hour = 0;
	expected.d_min = 0;
	expected.d_sec = 0;
	return test_httpdateparser_check(test_httpdateparser_testdata_01, &expected);
}

LOCAL TEST_RESULT test_httpdateparser_2()
{
	DATE_TIM expected;
	expected.d_year = 101;
	expected.d_month = 6;
	expected.d_day = 10;
	expected.d_hour = 12;
	expected.d_min = 0;
	expected.d_sec = 0;
	return test_httpdateparser_check(test_httpdateparser_testdata_02, &expected);
}

LOCAL TEST_RESULT test_httpdateparser_3()
{
	DATE_TIM expected;
	expected.d_year = 101;
	expected.d_month = 12;
	expected.d_day = 31;
	expected.d_hour = 23;
	expected.d_min = 59;
	expected.d_sec = 59;
	return test_httpdateparser_check(test_httpdateparser_testdata_03, &expected);
}

LOCAL TEST_RESULT test_httpdateparser_4()
{
	DATE_TIM expected;
	expected.d_year = 101;
	expected.d_month = 6;
	expected.d_day = 10;
	expected.d_hour = 12;
	expected.d_min = 0;
	expected.d_sec = 0;
	return test_httpdateparser_check(test_httpdateparser_testdata_04, &expected);
}

LOCAL TEST_RESULT test_httpdateparser_5()
{
	DATE_TIM expected;
	expected.d_year = 101;
	expected.d_month = 12;
	expected.d_day = 31;
	expected.d_hour = 23;
	expected.d_min = 59;
	expected.d_sec = 59;
	return test_httpdateparser_check(test_httpdateparser_testdata_05, &expected);
}

LOCAL VOID test_httpdateparser_printresult(TEST_RESULT (*proc)(), B *test_name)
{
	TEST_RESULT result;

	printf("test_httpdateparser: %s\n", test_name);
	printf("---------------------------------------------\n");
	result = proc();
	if (result == TEST_RESULT_PASS) {
		printf("--pass---------------------------------------\n");
	} else {
		printf("--fail---------------------------------------\n");
	}
	printf("---------------------------------------------\n");
}

EXPORT VOID test_httpdateparser_main()
{
	test_httpdateparser_printresult(test_httpdateparser_1, "test_httpdateparser_1");
	test_httpdateparser_printresult(test_httpdateparser_2, "test_httpdateparser_2");
	test_httpdateparser_printresult(test_httpdateparser_3, "test_httpdateparser_3");
	test_httpdateparser_printresult(test_httpdateparser_4, "test_httpdateparser_4");
	test_httpdateparser_printresult(test_httpdateparser_5, "test_httpdateparser_5");
}
