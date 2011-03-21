/*
 * test_httpheaderlexer.c
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

#include    "test.h"

#include    "httpheaderlexer.h"

LOCAL UB test_httpheaderlexer_testdata_01[] = {
"ABCDEF: afgce\r
BCDERF:   aaaa bbb \r
XYZ:  qwer\r
   AAA\r
LMNOPQ: aaa\r
\r
AAABBBCCC
DDDEEEFFF"
};

LOCAL TEST_RESULT test_httpheaderlexer_1()
{
	httpheaderlexer_t lexer;
	W i,err,len;
	HTTPHEADERLEXER_RESULT_T hr;

	len = strlen(test_httpheaderlexer_testdata_01);

	err = httpheaderlexer_initialize(&lexer);
	if (err < 0) {
		return TEST_RESULT_FAIL;
	}

	for (i = 0; i < len; i++) {
		httpheaderlexer_inputchar(&lexer, test_httpheaderlexer_testdata_01[i], &hr);
		switch (hr) {
		case HTTPHEADERLEXER_RESULT_NONE:
			printf("NONE\n");
			break;
		case HTTPHEADERLEXER_RESULT_FIELDNAME:
			printf("FIELDNAME\n");
			break;
		case HTTPHEADERLEXER_RESULT_FIELDNAME_END:
			printf("FIELDNAME_END\n");
			break;
		case HTTPHEADERLEXER_RESULT_FIELDCONTENT:
			printf("FIELDCONTENT\n");
			break;
		case HTTPHEADERLEXER_RESULT_LWS:
			printf("LWS\n");
			break;
		case HTTPHEADERLEXER_RESULT_MESSAGEHEADER_END:
			printf("MESSAGEHEADER_END\n");
			break;
		}
	}

	httpheaderlexer_finalize(&lexer);

	return TEST_RESULT_PASS;
}

LOCAL TEST_RESULT test_httpheaderlexer_2()
{
	httpheaderlexer_t lexer;
	W err;
	HTTPHEADERLEXER_RESULT_T hr;
	TEST_RESULT result = TEST_RESULT_PASS;

#define check_result_2(info, e, r) \
	if (e != r) { \
		result = TEST_RESULT_FAIL; \
		printf("test expected = %d, result = %d, fail = %s", e, r, info); \
	}

	err = httpheaderlexer_initialize(&lexer);
	if (err < 0) {
		return TEST_RESULT_FAIL;
	}

	httpheaderlexer_inputchar(&lexer, 'A', &hr);
	check_result_2("A", HTTPHEADERLEXER_RESULT_FIELDNAME, hr);
	httpheaderlexer_inputchar(&lexer, 'B', &hr);
	check_result_2("B", HTTPHEADERLEXER_RESULT_FIELDNAME, hr);
	httpheaderlexer_inputchar(&lexer, 'C', &hr);
	check_result_2("C", HTTPHEADERLEXER_RESULT_FIELDNAME, hr);
	httpheaderlexer_inputchar(&lexer, ':', &hr);
	check_result_2(":", HTTPHEADERLEXER_RESULT_FIELDNAME_END, hr);
	httpheaderlexer_inputchar(&lexer, ' ', &hr);
	check_result_2("[SP]", HTTPHEADERLEXER_RESULT_LWS, hr);
	httpheaderlexer_inputchar(&lexer, 'a', &hr);
	check_result_2("a", HTTPHEADERLEXER_RESULT_FIELDCONTENT, hr);
	httpheaderlexer_inputchar(&lexer, 'b', &hr);
	check_result_2("b", HTTPHEADERLEXER_RESULT_FIELDCONTENT, hr);
	httpheaderlexer_inputchar(&lexer, 'c', &hr);
	check_result_2("c", HTTPHEADERLEXER_RESULT_FIELDCONTENT, hr);
	httpheaderlexer_inputchar(&lexer, '\r', &hr);
	check_result_2("[CR]", HTTPHEADERLEXER_RESULT_NONE, hr);
	httpheaderlexer_inputchar(&lexer, '\n', &hr);
	check_result_2("[LF]", HTTPHEADERLEXER_RESULT_NONE, hr);
	httpheaderlexer_inputchar(&lexer, 'D', &hr);
	check_result_2("D", HTTPHEADERLEXER_RESULT_FIELDNAME, hr);
	httpheaderlexer_inputchar(&lexer, 'E', &hr);
	check_result_2("E", HTTPHEADERLEXER_RESULT_FIELDNAME, hr);
	httpheaderlexer_inputchar(&lexer, 'F', &hr);
	check_result_2("F", HTTPHEADERLEXER_RESULT_FIELDNAME, hr);
	httpheaderlexer_inputchar(&lexer, ':', &hr);
	check_result_2(":", HTTPHEADERLEXER_RESULT_FIELDNAME_END, hr);
	httpheaderlexer_inputchar(&lexer, ' ', &hr);
	check_result_2("[SP]", HTTPHEADERLEXER_RESULT_LWS, hr);
	httpheaderlexer_inputchar(&lexer, ' ', &hr);
	check_result_2("[SP]", HTTPHEADERLEXER_RESULT_NONE, hr);
	httpheaderlexer_inputchar(&lexer, ' ', &hr);
	check_result_2("[SP]", HTTPHEADERLEXER_RESULT_NONE, hr);
	httpheaderlexer_inputchar(&lexer, 'a', &hr);
	check_result_2("a", HTTPHEADERLEXER_RESULT_FIELDCONTENT, hr);
	httpheaderlexer_inputchar(&lexer, ' ', &hr);
	check_result_2("[SP]", HTTPHEADERLEXER_RESULT_LWS, hr);
	httpheaderlexer_inputchar(&lexer, 'b', &hr);
	check_result_2("b", HTTPHEADERLEXER_RESULT_FIELDCONTENT, hr);
	httpheaderlexer_inputchar(&lexer, 'b', &hr);
	check_result_2("b", HTTPHEADERLEXER_RESULT_FIELDCONTENT, hr);
	httpheaderlexer_inputchar(&lexer, ' ', &hr);
	check_result_2("[SP]", HTTPHEADERLEXER_RESULT_LWS, hr);
	httpheaderlexer_inputchar(&lexer, '\r', &hr);
	check_result_2("[CR]", HTTPHEADERLEXER_RESULT_NONE, hr);
	httpheaderlexer_inputchar(&lexer, '\n', &hr);
	check_result_2("[LF]", HTTPHEADERLEXER_RESULT_NONE, hr);
	httpheaderlexer_inputchar(&lexer, 'X', &hr);
	check_result_2("X", HTTPHEADERLEXER_RESULT_FIELDNAME, hr);
	httpheaderlexer_inputchar(&lexer, 'Y', &hr);
	check_result_2("Y", HTTPHEADERLEXER_RESULT_FIELDNAME, hr);
	httpheaderlexer_inputchar(&lexer, 'Z', &hr);
	check_result_2("Z", HTTPHEADERLEXER_RESULT_FIELDNAME, hr);
	httpheaderlexer_inputchar(&lexer, ':', &hr);
	check_result_2(":", HTTPHEADERLEXER_RESULT_FIELDNAME_END, hr);
	httpheaderlexer_inputchar(&lexer, 'x', &hr);
	check_result_2("x", HTTPHEADERLEXER_RESULT_FIELDCONTENT, hr);
	httpheaderlexer_inputchar(&lexer, 'y', &hr);
	check_result_2("y", HTTPHEADERLEXER_RESULT_FIELDCONTENT, hr);
	httpheaderlexer_inputchar(&lexer, 'z', &hr);
	check_result_2("z", HTTPHEADERLEXER_RESULT_FIELDCONTENT, hr);
	httpheaderlexer_inputchar(&lexer, '\r', &hr);
	check_result_2("[CR]", HTTPHEADERLEXER_RESULT_NONE, hr);
	httpheaderlexer_inputchar(&lexer, '\n', &hr);
	check_result_2("[LF]", HTTPHEADERLEXER_RESULT_NONE, hr);
	httpheaderlexer_inputchar(&lexer, ' ', &hr);
	check_result_2("[SP]", HTTPHEADERLEXER_RESULT_LWS, hr);
	httpheaderlexer_inputchar(&lexer, ' ', &hr);
	check_result_2("[SP]", HTTPHEADERLEXER_RESULT_NONE, hr);
	httpheaderlexer_inputchar(&lexer, 'A', &hr);
	check_result_2("A", HTTPHEADERLEXER_RESULT_FIELDCONTENT, hr);
	httpheaderlexer_inputchar(&lexer, 'A', &hr);
	check_result_2("A", HTTPHEADERLEXER_RESULT_FIELDCONTENT, hr);
	httpheaderlexer_inputchar(&lexer, 'A', &hr);
	check_result_2("A", HTTPHEADERLEXER_RESULT_FIELDCONTENT, hr);
	httpheaderlexer_inputchar(&lexer, '\r', &hr);
	check_result_2("[CR]", HTTPHEADERLEXER_RESULT_NONE, hr);
	httpheaderlexer_inputchar(&lexer, '\n', &hr);
	check_result_2("[LF]", HTTPHEADERLEXER_RESULT_NONE, hr);
	httpheaderlexer_inputchar(&lexer, 'L', &hr);
	check_result_2("L", HTTPHEADERLEXER_RESULT_FIELDNAME, hr);
	httpheaderlexer_inputchar(&lexer, 'M', &hr);
	check_result_2("M", HTTPHEADERLEXER_RESULT_FIELDNAME, hr);
	httpheaderlexer_inputchar(&lexer, 'N', &hr);
	check_result_2("N", HTTPHEADERLEXER_RESULT_FIELDNAME, hr);
	httpheaderlexer_inputchar(&lexer, ':', &hr);
	check_result_2(":", HTTPHEADERLEXER_RESULT_FIELDNAME_END, hr);
	httpheaderlexer_inputchar(&lexer, ' ', &hr);
	check_result_2("[SP]", HTTPHEADERLEXER_RESULT_LWS, hr);
	httpheaderlexer_inputchar(&lexer, 'l', &hr);
	check_result_2("l", HTTPHEADERLEXER_RESULT_FIELDCONTENT, hr);
	httpheaderlexer_inputchar(&lexer, 'm', &hr);
	check_result_2("m", HTTPHEADERLEXER_RESULT_FIELDCONTENT, hr);
	httpheaderlexer_inputchar(&lexer, 'n', &hr);
	check_result_2("n", HTTPHEADERLEXER_RESULT_FIELDCONTENT, hr);
	httpheaderlexer_inputchar(&lexer, '\r', &hr);
	check_result_2("[CR]", HTTPHEADERLEXER_RESULT_NONE, hr);
	httpheaderlexer_inputchar(&lexer, '\n', &hr);
	check_result_2("[LF]", HTTPHEADERLEXER_RESULT_NONE, hr);
	httpheaderlexer_inputchar(&lexer, '\r', &hr);
	check_result_2("[CR]", HTTPHEADERLEXER_RESULT_NONE, hr);
	httpheaderlexer_inputchar(&lexer, '\n', &hr);
	check_result_2("[LF]", HTTPHEADERLEXER_RESULT_MESSAGEHEADER_END, hr);

	httpheaderlexer_finalize(&lexer);

	return result;
}

LOCAL VOID test_httpheaderlexer_printresult(TEST_RESULT (*proc)(), B *test_name)
{
	TEST_RESULT result;

	printf("test_httpheaderlexer: %s\n", test_name);
	printf("---------------------------------------------\n");
	result = proc();
	if (result == TEST_RESULT_PASS) {
		printf("--pass---------------------------------------\n");
	} else {
		printf("--fail---------------------------------------\n");
	}
	printf("---------------------------------------------\n");
}

EXPORT VOID test_httpheaderlexer_main()
{
	test_httpheaderlexer_printresult(test_httpheaderlexer_1, "test_httpheaderlexer_1");
	test_httpheaderlexer_printresult(test_httpheaderlexer_2, "test_httpheaderlexer_2");
}
