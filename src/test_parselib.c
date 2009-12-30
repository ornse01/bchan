/*
 * test_parselib.c
 *
 * Copyright (c) 2009 project bchan
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
#include    <bstring.h>
#include    <bstdlib.h>

#include    "test.h"

#include    "parselib.h"

LOCAL TEST_RESULT test_character_reference_1()
{
	charreferparser_t parser;
	charreferparser_result_t parseresult;
	W err;

	err = charreferparser_initialize(&parser);
	if (err < 0) {
		return TEST_RESULT_FAIL;
	}
	parseresult = charreferparser_parsechar(&parser, '&');
	if (parseresult != CHARREFERPARSER_RESULT_CONTINUE) {
		charreferparser_finalize(&parser);
		return TEST_RESULT_FAIL;
	}
	parseresult = charreferparser_parsechar(&parser, '#');
	if (parseresult != CHARREFERPARSER_RESULT_CONTINUE) {
		charreferparser_finalize(&parser);
		return TEST_RESULT_FAIL;
	}
	parseresult = charreferparser_parsechar(&parser, '1');
	if (parseresult != CHARREFERPARSER_RESULT_CONTINUE) {
		charreferparser_finalize(&parser);
		return TEST_RESULT_FAIL;
	}
	parseresult = charreferparser_parsechar(&parser, '0');
	if (parseresult != CHARREFERPARSER_RESULT_CONTINUE) {
		charreferparser_finalize(&parser);
		return TEST_RESULT_FAIL;
	}
	parseresult = charreferparser_parsechar(&parser, ';');
		charreferparser_finalize(&parser);
	if (parseresult != CHARREFERPARSER_RESULT_DETERMINE) {
		return TEST_RESULT_FAIL;
	}
	err = charreferparser_getcharnumber(&parser);
	if (err < 0) {
		charreferparser_finalize(&parser);
		return TEST_RESULT_FAIL;
	}
	if (err != 10) {
		charreferparser_finalize(&parser);
		return TEST_RESULT_FAIL;
	}
	charreferparser_finalize(&parser);

	return TEST_RESULT_PASS;
}

LOCAL W test_charrefer_parsestr(charreferparser_t *parser, UB *str)
{
	W i;
	charreferparser_result_t result;

	for (i = 0; str[i] != '\0'; i++) {
		result = charreferparser_parsechar(parser, str[i]);
		if (result == CHARREFERPARSER_RESULT_INVALID) {
			return -1;
		}
		if (result == CHARREFERPARSER_RESULT_DETERMINE) {
			return 0;
		}
	}
	return -1;
}

LOCAL TEST_RESULT test_character_reference_2()
{
	W i;
	struct {
		UB *str;
		W val;
	} tuple[] = {
		{"&#0;", 0},
		{"&#1;", 1},
		{"&#2;", 2},
		{"&#3;", 3},
		{"&#4;", 4},
		{"&#5;", 5},
		{"&#6;", 6},
		{"&#7;", 7},
		{"&#8;", 8},
		{"&#9;", 9},
		{"&#10;", 10},
		{"&#11;", 11},
		{"&#12;", 12},
		{"&#13;", 13},
		{"&#14;", 14},
		{"&#15;", 15},
		{"&#16;", 16},
		{"&#17;", 17},
		{"&#18;", 18},
		{"&#19;", 19},
		{"&#20;", 20},
		{"&#x0;", 0x0},
		{"&#x1;", 0x1},
		{"&#x2;", 0x2},
		{"&#x3;", 0x3},
		{"&#x4;", 0x4},
		{"&#x5;", 0x5},
		{"&#x6;", 0x6},
		{"&#x7;", 0x7},
		{"&#x8;", 0x8},
		{"&#x9;", 0x9},
		{"&#xa;", 0xa},
		{"&#xb;", 0xb},
		{"&#xc;", 0xc},
		{"&#xd;", 0xd},
		{"&#xe;", 0xe},
		{"&#xf;", 0xf},
		{"&#xA;", 0xa},
		{"&#xB;", 0xb},
		{"&#xC;", 0xc},
		{"&#xD;", 0xd},
		{"&#xE;", 0xe},
		{"&#xF;", 0xf},
		{"&#x10;", 0x10},
		{"&#x11;", 0x11},
		{"&#x12;", 0x12},
		{"&#x13;", 0x13},
		{"&#x14;", 0x14},
		{"&#x15;", 0x15},
		{"&#xA0;", 0xA0},
		{"&#xB1;", 0xB1},
		{"&#xC2;", 0xC2},
		{"&#xD3;", 0xD3},
		{"&#xE4;", 0xE4},
		{"&#xF5;", 0xF5},
		{"&#xa0;", 0xA0},
		{"&#xb1;", 0xB1},
		{"&#xc2;", 0xC2},
		{"&#xd3;", 0xD3},
		{"&#xe4;", 0xE4},
		{"&#xf5;", 0xF5},
		{NULL, 0}
	};
	charreferparser_t parser;
	W err;

	for (i = 0; tuple[i].str != NULL; i++) {
		err = charreferparser_initialize(&parser);
		if (err < 0) {
			return TEST_RESULT_FAIL;
		}
		err = test_charrefer_parsestr(&parser, tuple[i].str);
		if (err < 0) {
			printf("failed to parsing\n");
			return TEST_RESULT_FAIL;
		}
		err = charreferparser_getcharnumber(&parser);
		if (err < 0) {
			printf("failed to get character number\n");
			charreferparser_finalize(&parser);
			return TEST_RESULT_FAIL;
		}
		if (err != tuple[i].val) {
			printf("invalid parse result: %d(0x%x)\n", err, err);
			printf("                         <- %s\n", tuple[i].str);
			printf("                         <- %d\n", tuple[i].val);
			printf("                         <- 0x%x\n", tuple[i].val);
			charreferparser_finalize(&parser);
			return TEST_RESULT_FAIL;
		}
		charreferparser_finalize(&parser);
	}

	return TEST_RESULT_PASS;
}

LOCAL TEST_RESULT test_character_reference_3()
{
	W i;
	struct {
		UB *str;
		W val;
	} tuple[] = {
		{"&amp;", '&'},
		{"&gt;", '>'},
		{"&lt;", '<'},
		{"&quot;", '"'},
		{NULL, 0}
	};
	charreferparser_t parser;
	W err;

	for (i = 0; tuple[i].str != NULL; i++) {
		err = charreferparser_initialize(&parser);
		if (err < 0) {
			return TEST_RESULT_FAIL;
		}
		err = test_charrefer_parsestr(&parser, tuple[i].str);
		if (err < 0) {
			printf("failed to parsing\n");
			return TEST_RESULT_FAIL;
		}
		err = charreferparser_getcharnumber(&parser);
		if (err < 0) {
			printf("failed to get character number\n");
			charreferparser_finalize(&parser);
			return TEST_RESULT_FAIL;
		}
		if (err != tuple[i].val) {
			printf("invalid parse result: %d(0x%x)\n", err, err);
			printf("                         <- %s\n", tuple[i].str);
			printf("                         <- %d\n", tuple[i].val);
			printf("                         <- 0x%x\n", tuple[i].val);
			charreferparser_finalize(&parser);
			return TEST_RESULT_FAIL;
		}
		charreferparser_finalize(&parser);
	}

	return TEST_RESULT_PASS;
}

LOCAL VOID test_parselib_printresult(TEST_RESULT (*proc)(), B *test_name)
{
	TEST_RESULT result;

	printf("test_parselib: %s\n", test_name);
	printf("---------------------------------------------\n");
	result = proc();
	if (result == TEST_RESULT_PASS) {
		printf("--pass---------------------------------------\n");
	} else {
		printf("--fail---------------------------------------\n");
	}
	printf("---------------------------------------------\n");
}

EXPORT VOID test_parselib_main()
{
	test_parselib_printresult(test_character_reference_1, "test_character_reference_1");
	test_parselib_printresult(test_character_reference_2, "test_character_reference_2");
	test_parselib_printresult(test_character_reference_3, "test_character_reference_3");
}
