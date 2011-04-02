/*
 * test_parselib.c
 *
 * Copyright (c) 2009-2011 project bchan
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

LOCAL TEST_RESULT test_character_reference_4()
{
	W i, len, err;
	UB *str;
	UB data[] = "&aaa;";
	charreferparser_t parser;

	err = charreferparser_initialize(&parser);
	if (err < 0) {
		return TEST_RESULT_FAIL;
	}
	for (i = 0; data[i] != '\0'; i++) {
		err = charreferparser_parsechar(&parser, data[i]);
		if (err == CHARREFERPARSER_RESULT_INVALID) {
			break;
		}
	}
	if (i != 2) {
		printf("failed to invalid position\n");
		charreferparser_finalize(&parser);
		return TEST_RESULT_FAIL;
	}
	charreferparser_getlastmatchedstring(&parser, &str, &len);
	if (len != 1) {
		printf("failed to get last matched string length\n");
		charreferparser_finalize(&parser);
		return TEST_RESULT_FAIL;
	}
	err = strncmp(data + 1, str, len);
	if (err != 0) {
		printf("failed to get last matched string\n");
		charreferparser_finalize(&parser);
		return TEST_RESULT_FAIL;
	}
	charreferparser_finalize(&parser);

	return TEST_RESULT_PASS;
}

LOCAL TEST_RESULT test_character_reference_5()
{
	W i, len, err;
	UB *str;
	UB data[] = "&;";
	charreferparser_t parser;

	err = charreferparser_initialize(&parser);
	if (err < 0) {
		return TEST_RESULT_FAIL;
	}
	for (i = 0; data[i] != '\0'; i++) {
		err = charreferparser_parsechar(&parser, data[i]);
		if (err == CHARREFERPARSER_RESULT_INVALID) {
			break;
		}
	}
	if (i != 1) {
		printf("failed to invalid position\n");
		charreferparser_finalize(&parser);
		return TEST_RESULT_FAIL;
	}
	charreferparser_getlastmatchedstring(&parser, &str, &len);
	if (len != 0) {
		printf("failed to get last matched string length\n");
		charreferparser_finalize(&parser);
		return TEST_RESULT_FAIL;
	}
	err = strncmp(data + 1, str, len);
	if (err != 0) {
		printf("failed to get last matched string\n");
		charreferparser_finalize(&parser);
		return TEST_RESULT_FAIL;
	}
	charreferparser_finalize(&parser);

	return TEST_RESULT_PASS;
}

LOCAL TEST_RESULT test_character_reference_6()
{
	W i, len, err;
	UB *str;
	UB data[] = "&amm;";
	charreferparser_t parser;

	err = charreferparser_initialize(&parser);
	if (err < 0) {
		return TEST_RESULT_FAIL;
	}
	for (i = 0; data[i] != '\0'; i++) {
		err = charreferparser_parsechar(&parser, data[i]);
		if (err == CHARREFERPARSER_RESULT_INVALID) {
			break;
		}
	}
	if (i != 3) {
		printf("failed to invalid position\n");
		charreferparser_finalize(&parser);
		return TEST_RESULT_FAIL;
	}
	charreferparser_getlastmatchedstring(&parser, &str, &len);
	if (len != 2) {
		printf("failed to get last matched string length\n");
		charreferparser_finalize(&parser);
		return TEST_RESULT_FAIL;
	}
	err = strncmp(data + 1, str, len);
	if (err != 0) {
		printf("failed to get last matched string\n");
		charreferparser_finalize(&parser);
		return TEST_RESULT_FAIL;
	}
	charreferparser_finalize(&parser);

	return TEST_RESULT_PASS;
}

LOCAL TEST_RESULT test_character_reference_7()
{
	W i, len, err;
	UB *str;
	UB data[] = "&amps;";
	charreferparser_t parser;

	err = charreferparser_initialize(&parser);
	if (err < 0) {
		return TEST_RESULT_FAIL;
	}
	for (i = 0; data[i] != '\0'; i++) {
		err = charreferparser_parsechar(&parser, data[i]);
		if (err == CHARREFERPARSER_RESULT_INVALID) {
			break;
		}
	}
	if (i != 4) {
		printf("failed to invalid position\n");
		charreferparser_finalize(&parser);
		return TEST_RESULT_FAIL;
	}
	charreferparser_getlastmatchedstring(&parser, &str, &len);
	if (len != 3) {
		printf("failed to get last matched string length\n");
		charreferparser_finalize(&parser);
		return TEST_RESULT_FAIL;
	}
	err = strncmp(data + 1, str, len);
	if (err != 0) {
		printf("failed to get last matched string\n");
		charreferparser_finalize(&parser);
		return TEST_RESULT_FAIL;
	}
	charreferparser_finalize(&parser);

	return TEST_RESULT_PASS;
}

LOCAL TEST_RESULT test_character_reference_8()
{
	W i, len, err;
	UB *str;
	UB data[] = "&qqqq;";
	charreferparser_t parser;

	err = charreferparser_initialize(&parser);
	if (err < 0) {
		return TEST_RESULT_FAIL;
	}
	for (i = 0; data[i] != '\0'; i++) {
		err = charreferparser_parsechar(&parser, data[i]);
		if (err == CHARREFERPARSER_RESULT_INVALID) {
			break;
		}
	}
	if (i != 2) {
		printf("failed to invalid position\n");
		charreferparser_finalize(&parser);
		return TEST_RESULT_FAIL;
	}
	charreferparser_getlastmatchedstring(&parser, &str, &len);
	if (len != 1) {
		printf("failed to get last matched string length\n");
		charreferparser_finalize(&parser);
		return TEST_RESULT_FAIL;
	}
	err = strncmp(data + 1, str, len);
	if (err != 0) {
		printf("failed to get last matched string\n");
		charreferparser_finalize(&parser);
		return TEST_RESULT_FAIL;
	}
	charreferparser_finalize(&parser);

	return TEST_RESULT_PASS;
}

LOCAL TEST_RESULT test_character_reference_9()
{
	W i, len, err;
	UB *str;
	UB data[] = "&quuu;";
	charreferparser_t parser;

	err = charreferparser_initialize(&parser);
	if (err < 0) {
		return TEST_RESULT_FAIL;
	}
	for (i = 0; data[i] != '\0'; i++) {
		err = charreferparser_parsechar(&parser, data[i]);
		if (err == CHARREFERPARSER_RESULT_INVALID) {
			break;
		}
	}
	if (i != 3) {
		printf("failed to invalid position\n");
		charreferparser_finalize(&parser);
		return TEST_RESULT_FAIL;
	}
	charreferparser_getlastmatchedstring(&parser, &str, &len);
	if (len != 2) {
		printf("failed to get last matched string length\n");
		charreferparser_finalize(&parser);
		return TEST_RESULT_FAIL;
	}
	err = strncmp(data + 1, str, len);
	if (err != 0) {
		printf("failed to get last matched string\n");
		charreferparser_finalize(&parser);
		return TEST_RESULT_FAIL;
	}
	charreferparser_finalize(&parser);

	return TEST_RESULT_PASS;
}

LOCAL TEST_RESULT test_character_reference_10()
{
	W i, len, err;
	UB *str;
	UB data[] = "&quoo;";
	charreferparser_t parser;

	err = charreferparser_initialize(&parser);
	if (err < 0) {
		return TEST_RESULT_FAIL;
	}
	for (i = 0; data[i] != '\0'; i++) {
		err = charreferparser_parsechar(&parser, data[i]);
		if (err == CHARREFERPARSER_RESULT_INVALID) {
			break;
		}
	}
	if (i != 4) {
		printf("failed to invalid position\n");
		charreferparser_finalize(&parser);
		return TEST_RESULT_FAIL;
	}
	charreferparser_getlastmatchedstring(&parser, &str, &len);
	if (len != 3) {
		printf("failed to get last matched string length\n");
		charreferparser_finalize(&parser);
		return TEST_RESULT_FAIL;
	}
	err = strncmp(data + 1, str, len);
	if (err != 0) {
		printf("failed to get last matched string\n");
		charreferparser_finalize(&parser);
		return TEST_RESULT_FAIL;
	}
	charreferparser_finalize(&parser);

	return TEST_RESULT_PASS;
}

LOCAL TEST_RESULT test_character_reference_11()
{
	W i, len, err;
	UB *str;
	UB data[] = "&quott;";
	charreferparser_t parser;

	err = charreferparser_initialize(&parser);
	if (err < 0) {
		return TEST_RESULT_FAIL;
	}
	for (i = 0; data[i] != '\0'; i++) {
		err = charreferparser_parsechar(&parser, data[i]);
		if (err == CHARREFERPARSER_RESULT_INVALID) {
			break;
		}
	}
	if (i != 5) {
		printf("failed to invalid position\n");
		charreferparser_finalize(&parser);
		return TEST_RESULT_FAIL;
	}
	charreferparser_getlastmatchedstring(&parser, &str, &len);
	if (len != 4) {
		printf("failed to get last matched string length\n");
		charreferparser_finalize(&parser);
		return TEST_RESULT_FAIL;
	}
	err = strncmp(data + 1, str, len);
	if (err != 0) {
		printf("failed to get last matched string\n");
		charreferparser_finalize(&parser);
		return TEST_RESULT_FAIL;
	}
	charreferparser_finalize(&parser);

	return TEST_RESULT_PASS;
}

LOCAL tokenchecker_valuetuple_t test_tokenchecker_testdata_tuple_0001[] = {
	{"aaa", 1},
	{"ab", 2},
	{"abcde", 3},
	{"bbb", 4},
	{"bcdef", 5},
	{"cdefghi", 6},
	{"d", 7}
};
LOCAL UB test_tokenchecker_testdata_etoken_0001[] = ":>";

LOCAL TEST_RESULT test_tokenchecker_1()
{
	tokenchecker_t checker;
	W i, j, len, len2, val, ret;
	UB *str, *str2;

	str = strdup(test_tokenchecker_testdata_tuple_0001[0].name);
	len = strlen(str);

	tokenchecker_initialize(&checker,
							 test_tokenchecker_testdata_tuple_0001,
							 7,
							 test_tokenchecker_testdata_etoken_0001);

	for (i = 0; i < len; i++) {
		ret = tokenchecker_inputchar(&checker, str[i], &val);
		if (ret != TOKENCHECKER_CONTINUE) {
			printf("tokenchecker_inputchar fail: right string\n");
			free(str);
			return TEST_RESULT_FAIL;
		}
		tokenchecker_getlastmatchedstring(&checker, &str2, &len2);
		if (strncmp(str, str2, len2) != 0) {
			printf("tokenchecker_getlastmatchedstring fail: right string\n");
			printf(" ");
			for (j = 0; j < len; j++) {
				printf("%c", str[j]);
			}
			printf(" : ");
			for (j = 0; j < len; j++) {
				printf("%c", str2[j]);
			}
			printf("\n");
			free(str);
			return TEST_RESULT_FAIL;
		}
	}
	ret = tokenchecker_inputchar(&checker, ':', &val);
	if (ret != TOKENCHECKER_DETERMINE) {
		printf("tokenchecker_inputchar fail: end character\n");
		free(str);
		return TEST_RESULT_FAIL;
	}
	if (val != 1) {
		printf("tokenchecker_inputchar fail: select value\n");
		free(str);
		return TEST_RESULT_FAIL;
	}
	tokenchecker_getlastmatchedstring(&checker, &str2, &len2);
	if (strncmp(str, str2, len2) != 0) {
		printf("tokenchecker_getlastmatchedstring fail: end token\n");
		free(str);
		return TEST_RESULT_FAIL;
	}
	ret = tokenchecker_inputchar(&checker, ':', &val);
	if (ret != TOKENCHECKER_AFTER_END) {
		printf("tokenchecker_inputchar fail: after end\n");
		free(str);
		return TEST_RESULT_FAIL;
	}

	free(str);
	return TEST_RESULT_PASS;
}

LOCAL TEST_RESULT test_tokenchecker_2()
{
	tokenchecker_t checker;
	W i, j, len, len2, val, ret;
	UB *str, *str2;

	str = strdup(test_tokenchecker_testdata_tuple_0001[1].name);
	len = strlen(str);

	tokenchecker_initialize(&checker,
							 test_tokenchecker_testdata_tuple_0001,
							 7,
							 test_tokenchecker_testdata_etoken_0001);

	for (i = 0; i < len; i++) {
		ret = tokenchecker_inputchar(&checker, str[i], &val);
		if (ret != TOKENCHECKER_CONTINUE) {
			printf("tokenchecker_inputchar fail: right string\n");
			free(str);
			return TEST_RESULT_FAIL;
		}
		tokenchecker_getlastmatchedstring(&checker, &str2, &len2);
		if (strncmp(str, str2, len2) != 0) {
			printf("tokenchecker_getlastmatchedstring fail: right string\n");
			printf(" ");
			for (j = 0; j < len; j++) {
				printf("%c", str[j]);
			}
			printf(" : ");
			for (j = 0; j < len; j++) {
				printf("%c", str2[j]);
			}
			printf("\n");
			free(str);
			return TEST_RESULT_FAIL;
		}
	}
	ret = tokenchecker_inputchar(&checker, ':', &val);
	if (ret != TOKENCHECKER_DETERMINE) {
		printf("tokenchecker_inputchar fail: end character\n");
		free(str);
		return TEST_RESULT_FAIL;
	}
	if (val != 2) {
		printf("tokenchecker_inputchar fail: select value\n");
		free(str);
		return TEST_RESULT_FAIL;
	}
	tokenchecker_getlastmatchedstring(&checker, &str2, &len2);
	if (strncmp(str, str2, len2) != 0) {
		printf("tokenchecker_getlastmatchedstring fail: end token\n");
		free(str);
		return TEST_RESULT_FAIL;
	}
	ret = tokenchecker_inputchar(&checker, ':', &val);
	if (ret != TOKENCHECKER_AFTER_END) {
		printf("tokenchecker_inputchar fail: after end\n");
		free(str);
		return TEST_RESULT_FAIL;
	}

	free(str);
	return TEST_RESULT_PASS;
}

LOCAL TEST_RESULT test_tokenchecker_3()
{
	tokenchecker_t checker;
	W i, j, len, len2, val, ret;
	UB *str, *str2;

	str = strdup(test_tokenchecker_testdata_tuple_0001[2].name);
	len = strlen(str);

	tokenchecker_initialize(&checker,
							 test_tokenchecker_testdata_tuple_0001,
							 7,
							 test_tokenchecker_testdata_etoken_0001);

	for (i = 0; i < len; i++) {
		ret = tokenchecker_inputchar(&checker, str[i], &val);
		if (ret != TOKENCHECKER_CONTINUE) {
			printf("tokenchecker_inputchar fail: right string\n");
			free(str);
			return TEST_RESULT_FAIL;
		}
		tokenchecker_getlastmatchedstring(&checker, &str2, &len2);
		if (strncmp(str, str2, len2) != 0) {
			printf("tokenchecker_getlastmatchedstring fail: right string\n");
			printf(" ");
			for (j = 0; j < len; j++) {
				printf("%c", str[j]);
			}
			printf(" : ");
			for (j = 0; j < len; j++) {
				printf("%c", str2[j]);
			}
			printf("\n");
			free(str);
			return TEST_RESULT_FAIL;
		}
	}
	ret = tokenchecker_inputchar(&checker, ':', &val);
	if (ret != TOKENCHECKER_DETERMINE) {
		printf("tokenchecker_inputchar fail: end character\n");
		free(str);
		return TEST_RESULT_FAIL;
	}
	if (val != 3) {
		printf("tokenchecker_inputchar fail: select value\n");
		free(str);
		return TEST_RESULT_FAIL;
	}
	tokenchecker_getlastmatchedstring(&checker, &str2, &len2);
	if (strncmp(str, str2, len2) != 0) {
		printf("tokenchecker_getlastmatchedstring fail: end token\n");
		free(str);
		return TEST_RESULT_FAIL;
	}
	ret = tokenchecker_inputchar(&checker, ':', &val);
	if (ret != TOKENCHECKER_AFTER_END) {
		printf("tokenchecker_inputchar fail: after end\n");
		free(str);
		return TEST_RESULT_FAIL;
	}

	free(str);
	return TEST_RESULT_PASS;
}

LOCAL TEST_RESULT test_tokenchecker_4()
{
	tokenchecker_t checker;
	W i, j, len, len2, val, ret;
	UB *str, *str2;

	str = strdup(test_tokenchecker_testdata_tuple_0001[3].name);
	len = strlen(str);

	tokenchecker_initialize(&checker,
							 test_tokenchecker_testdata_tuple_0001,
							 7,
							 test_tokenchecker_testdata_etoken_0001);

	for (i = 0; i < len; i++) {
		ret = tokenchecker_inputchar(&checker, str[i], &val);
		if (ret != TOKENCHECKER_CONTINUE) {
			printf("tokenchecker_inputchar fail: right string\n");
			free(str);
			return TEST_RESULT_FAIL;
		}
		tokenchecker_getlastmatchedstring(&checker, &str2, &len2);
		if (strncmp(str, str2, len2) != 0) {
			printf("tokenchecker_getlastmatchedstring fail: right string\n");
			printf(" ");
			for (j = 0; j < len; j++) {
				printf("%c", str[j]);
			}
			printf(" : ");
			for (j = 0; j < len; j++) {
				printf("%c", str2[j]);
			}
			printf("\n");
			free(str);
			return TEST_RESULT_FAIL;
		}
	}
	ret = tokenchecker_inputchar(&checker, ':', &val);
	if (ret != TOKENCHECKER_DETERMINE) {
		printf("tokenchecker_inputchar fail: end character\n");
		free(str);
		return TEST_RESULT_FAIL;
	}
	if (val != 4) {
		printf("tokenchecker_inputchar fail: select value\n");
		free(str);
		return TEST_RESULT_FAIL;
	}
	tokenchecker_getlastmatchedstring(&checker, &str2, &len2);
	if (strncmp(str, str2, len2) != 0) {
		printf("tokenchecker_getlastmatchedstring fail: end token\n");
		free(str);
		return TEST_RESULT_FAIL;
	}
	ret = tokenchecker_inputchar(&checker, ':', &val);
	if (ret != TOKENCHECKER_AFTER_END) {
		printf("tokenchecker_inputchar fail: after end\n");
		free(str);
		return TEST_RESULT_FAIL;
	}

	free(str);
	return TEST_RESULT_PASS;
}

LOCAL TEST_RESULT test_tokenchecker_5()
{
	tokenchecker_t checker;
	W i, j, len, len2, val, ret;
	UB *str, *str2;

	str = strdup(test_tokenchecker_testdata_tuple_0001[4].name);
	len = strlen(str);

	tokenchecker_initialize(&checker,
							 test_tokenchecker_testdata_tuple_0001,
							 7,
							 test_tokenchecker_testdata_etoken_0001);

	for (i = 0; i < len; i++) {
		ret = tokenchecker_inputchar(&checker, str[i], &val);
		if (ret != TOKENCHECKER_CONTINUE) {
			printf("tokenchecker_inputchar fail: right string\n");
			free(str);
			return TEST_RESULT_FAIL;
		}
		tokenchecker_getlastmatchedstring(&checker, &str2, &len2);
		if (strncmp(str, str2, len2) != 0) {
			printf("tokenchecker_getlastmatchedstring fail: right string\n");
			printf(" ");
			for (j = 0; j < len; j++) {
				printf("%c", str[j]);
			}
			printf(" : ");
			for (j = 0; j < len; j++) {
				printf("%c", str2[j]);
			}
			printf("\n");
			free(str);
			return TEST_RESULT_FAIL;
		}
	}
	ret = tokenchecker_inputchar(&checker, ':', &val);
	if (ret != TOKENCHECKER_DETERMINE) {
		printf("tokenchecker_inputchar fail: end character\n");
		free(str);
		return TEST_RESULT_FAIL;
	}
	if (val != 5) {
		printf("tokenchecker_inputchar fail: select value\n");
		free(str);
		return TEST_RESULT_FAIL;
	}
	tokenchecker_getlastmatchedstring(&checker, &str2, &len2);
	if (strncmp(str, str2, len2) != 0) {
		printf("tokenchecker_getlastmatchedstring fail: end token\n");
		free(str);
		return TEST_RESULT_FAIL;
	}
	ret = tokenchecker_inputchar(&checker, ':', &val);
	if (ret != TOKENCHECKER_AFTER_END) {
		printf("tokenchecker_inputchar fail: after end\n");
		free(str);
		return TEST_RESULT_FAIL;
	}

	free(str);
	return TEST_RESULT_PASS;
}

LOCAL TEST_RESULT test_tokenchecker_6()
{
	tokenchecker_t checker;
	W i, j, len, len2, val, ret;
	UB *str, *str2;

	str = strdup(test_tokenchecker_testdata_tuple_0001[5].name);
	len = strlen(str);

	tokenchecker_initialize(&checker,
							 test_tokenchecker_testdata_tuple_0001,
							 7,
							 test_tokenchecker_testdata_etoken_0001);

	for (i = 0; i < len; i++) {
		ret = tokenchecker_inputchar(&checker, str[i], &val);
		if (ret != TOKENCHECKER_CONTINUE) {
			printf("tokenchecker_inputchar fail: right string\n");
			free(str);
			return TEST_RESULT_FAIL;
		}
		tokenchecker_getlastmatchedstring(&checker, &str2, &len2);
		if (strncmp(str, str2, len2) != 0) {
			printf("tokenchecker_getlastmatchedstring fail: right string\n");
			printf(" ");
			for (j = 0; j < len; j++) {
				printf("%c", str[j]);
			}
			printf(" : ");
			for (j = 0; j < len; j++) {
				printf("%c", str2[j]);
			}
			printf("\n");
			free(str);
			return TEST_RESULT_FAIL;
		}
	}
	ret = tokenchecker_inputchar(&checker, ':', &val);
	if (ret != TOKENCHECKER_DETERMINE) {
		printf("tokenchecker_inputchar fail: end character\n");
		free(str);
		return TEST_RESULT_FAIL;
	}
	if (val != 6) {
		printf("tokenchecker_inputchar fail: select value\n");
		free(str);
		return TEST_RESULT_FAIL;
	}
	tokenchecker_getlastmatchedstring(&checker, &str2, &len2);
	if (strncmp(str, str2, len2) != 0) {
		printf("tokenchecker_getlastmatchedstring fail: end token\n");
		free(str);
		return TEST_RESULT_FAIL;
	}
	ret = tokenchecker_inputchar(&checker, ':', &val);
	if (ret != TOKENCHECKER_AFTER_END) {
		printf("tokenchecker_inputchar fail: after end\n");
		free(str);
		return TEST_RESULT_FAIL;
	}

	free(str);
	return TEST_RESULT_PASS;
}

LOCAL TEST_RESULT test_tokenchecker_7()
{
	tokenchecker_t checker;
	W i, j, len, len2, val, ret;
	UB *str, *str2;

	str = strdup(test_tokenchecker_testdata_tuple_0001[6].name);
	len = strlen(str);

	tokenchecker_initialize(&checker,
							 test_tokenchecker_testdata_tuple_0001,
							 7,
							 test_tokenchecker_testdata_etoken_0001);

	for (i = 0; i < len; i++) {
		ret = tokenchecker_inputchar(&checker, str[i], &val);
		if (ret != TOKENCHECKER_CONTINUE) {
			printf("tokenchecker_inputchar fail: right string\n");
			free(str);
			return TEST_RESULT_FAIL;
		}
		tokenchecker_getlastmatchedstring(&checker, &str2, &len2);
		if (strncmp(str, str2, len2) != 0) {
			printf("tokenchecker_getlastmatchedstring fail: right string\n");
			printf(" ");
			for (j = 0; j < len; j++) {
				printf("%c", str[j]);
			}
			printf(" : ");
			for (j = 0; j < len; j++) {
				printf("%c", str2[j]);
			}
			printf("\n");
			free(str);
			return TEST_RESULT_FAIL;
		}
	}
	ret = tokenchecker_inputchar(&checker, ':', &val);
	if (ret != TOKENCHECKER_DETERMINE) {
		printf("tokenchecker_inputchar fail: end character\n");
		free(str);
		return TEST_RESULT_FAIL;
	}
	if (val != 7) {
		printf("tokenchecker_inputchar fail: select value\n");
		free(str);
		return TEST_RESULT_FAIL;
	}
	tokenchecker_getlastmatchedstring(&checker, &str2, &len2);
	if (strncmp(str, str2, len2) != 0) {
		printf("tokenchecker_getlastmatchedstring fail: end token\n");
		free(str);
		return TEST_RESULT_FAIL;
	}
	ret = tokenchecker_inputchar(&checker, ':', &val);
	if (ret != TOKENCHECKER_AFTER_END) {
		printf("tokenchecker_inputchar fail: after end\n");
		free(str);
		return TEST_RESULT_FAIL;
	}

	free(str);
	return TEST_RESULT_PASS;
}

LOCAL TEST_RESULT test_tokenchecker_nomatch_getlastmatchedstring_sequence(tokenchecker_t *checker, W num, UB *teststr, W teststr_len)
{
	W i, j, len2, ret, val;
	UB *str2;

	for (i = 0; i < num; i++) {
		ret = tokenchecker_inputchar(checker, teststr[i], &val);
		if (ret != TOKENCHECKER_CONTINUE) {
			printf("tokenchecker_inputchar fail: right string\n");
			return TEST_RESULT_FAIL;
		}
	}
	ret = tokenchecker_inputchar(checker, 'z', &val);
	if (ret != TOKENCHECKER_CONTINUE_NOMATCH) {
		printf("tokenchecker_inputchar fail: nomatch char\n");
		return TEST_RESULT_FAIL;
	}
	tokenchecker_getlastmatchedstring(checker, &str2, &len2);
	if (len2 != num) {
		printf("tokenchecker_getlastmatchedstring fail: length\n");
		return TEST_RESULT_FAIL;
	}
	if (strncmp(teststr, str2, len2) != 0) {
		printf("tokenchecker_getlastmatchedstring fail: nomatch string\n");
		printf(" ");
		for (j = 0; j < len2; j++) {
			printf("%c", teststr[j]);
		}
		printf(" : ");
		for (j = 0; j < len2; j++) {
			printf("%c", str2[j]);
		}
		printf("\n");
		return TEST_RESULT_FAIL;
	}

	return TEST_RESULT_PASS;
}

LOCAL TEST_RESULT test_tokenchecker_8()
{
	tokenchecker_t checker;
	W i,len;
	UB *str;
	TEST_RESULT result;

	str = strdup(test_tokenchecker_testdata_tuple_0001[0].name);
	len = strlen(str);

	tokenchecker_initialize(&checker,
							 test_tokenchecker_testdata_tuple_0001,
							 7,
							 test_tokenchecker_testdata_etoken_0001);

	for (i = 1; i < len; i++) {
		result = test_tokenchecker_nomatch_getlastmatchedstring_sequence(&checker, i, str, len);
		if (result != TEST_RESULT_PASS) {
			return result;
		}
		tokenchecker_clear(&checker);
	}

	free(str);

	return TEST_RESULT_PASS;
}

LOCAL TEST_RESULT test_tokenchecker_9()
{
	tokenchecker_t checker;
	W i,len;
	UB *str;
	TEST_RESULT result;

	str = strdup(test_tokenchecker_testdata_tuple_0001[1].name);
	len = strlen(str);

	tokenchecker_initialize(&checker,
							 test_tokenchecker_testdata_tuple_0001,
							 7,
							 test_tokenchecker_testdata_etoken_0001);

	for (i = 1; i < len; i++) {
		result = test_tokenchecker_nomatch_getlastmatchedstring_sequence(&checker, i, str, len);
		if (result != TEST_RESULT_PASS) {
			return result;
		}
		tokenchecker_clear(&checker);
	}

	free(str);

	return TEST_RESULT_PASS;
}

LOCAL TEST_RESULT test_tokenchecker_10()
{
	tokenchecker_t checker;
	W i,len;
	UB *str;
	TEST_RESULT result;

	str = strdup(test_tokenchecker_testdata_tuple_0001[2].name);
	len = strlen(str);

	tokenchecker_initialize(&checker,
							 test_tokenchecker_testdata_tuple_0001,
							 7,
							 test_tokenchecker_testdata_etoken_0001);

	for (i = 1; i < len; i++) {
		result = test_tokenchecker_nomatch_getlastmatchedstring_sequence(&checker, i, str, len);
		if (result != TEST_RESULT_PASS) {
			return result;
		}
		tokenchecker_clear(&checker);
	}

	free(str);

	return TEST_RESULT_PASS;
}

LOCAL TEST_RESULT test_tokenchecker_11()
{
	tokenchecker_t checker;
	W i,len;
	UB *str;
	TEST_RESULT result;

	str = strdup(test_tokenchecker_testdata_tuple_0001[3].name);
	len = strlen(str);

	tokenchecker_initialize(&checker,
							 test_tokenchecker_testdata_tuple_0001,
							 7,
							 test_tokenchecker_testdata_etoken_0001);

	for (i = 1; i < len; i++) {
		result = test_tokenchecker_nomatch_getlastmatchedstring_sequence(&checker, i, str, len);
		if (result != TEST_RESULT_PASS) {
			return result;
		}
		tokenchecker_clear(&checker);
	}

	free(str);

	return TEST_RESULT_PASS;
}

LOCAL TEST_RESULT test_tokenchecker_12()
{
	tokenchecker_t checker;
	W i,len;
	UB *str;
	TEST_RESULT result;

	str = strdup(test_tokenchecker_testdata_tuple_0001[4].name);
	len = strlen(str);

	tokenchecker_initialize(&checker,
							 test_tokenchecker_testdata_tuple_0001,
							 7,
							 test_tokenchecker_testdata_etoken_0001);

	for (i = 1; i < len; i++) {
		result = test_tokenchecker_nomatch_getlastmatchedstring_sequence(&checker, i, str, len);
		if (result != TEST_RESULT_PASS) {
			return result;
		}
		tokenchecker_clear(&checker);
	}

	free(str);

	return TEST_RESULT_PASS;
}

LOCAL TEST_RESULT test_tokenchecker_13()
{
	tokenchecker_t checker;
	W i,len;
	UB *str;
	TEST_RESULT result;

	str = strdup(test_tokenchecker_testdata_tuple_0001[5].name);
	len = strlen(str);

	tokenchecker_initialize(&checker,
							 test_tokenchecker_testdata_tuple_0001,
							 7,
							 test_tokenchecker_testdata_etoken_0001);

	for (i = 1; i < len; i++) {
		result = test_tokenchecker_nomatch_getlastmatchedstring_sequence(&checker, i, str, len);
		if (result != TEST_RESULT_PASS) {
			return result;
		}
		tokenchecker_clear(&checker);
	}

	free(str);

	return TEST_RESULT_PASS;
}

LOCAL TEST_RESULT test_tokenchecker_14()
{
	tokenchecker_t checker;
	W i,len;
	UB *str;
	TEST_RESULT result;

	str = strdup(test_tokenchecker_testdata_tuple_0001[6].name);
	len = strlen(str);

	tokenchecker_initialize(&checker,
							 test_tokenchecker_testdata_tuple_0001,
							 7,
							 test_tokenchecker_testdata_etoken_0001);

	for (i = 1; i < len; i++) {
		result = test_tokenchecker_nomatch_getlastmatchedstring_sequence(&checker, i, str, len);
		if (result != TEST_RESULT_PASS) {
			return result;
		}
		tokenchecker_clear(&checker);
	}

	free(str);

	return TEST_RESULT_PASS;
}

/* testing endinput */

LOCAL TEST_RESULT test_tokenchecker_15()
{
	tokenchecker_t checker;
	W i, j, len, len2, val, ret;
	UB *str, *str2;

	str = strdup(test_tokenchecker_testdata_tuple_0001[0].name);
	len = strlen(str);

	tokenchecker_initialize(&checker,
							 test_tokenchecker_testdata_tuple_0001,
							 7,
							 test_tokenchecker_testdata_etoken_0001);

	for (i = 0; i < len; i++) {
		ret = tokenchecker_inputchar(&checker, str[i], &val);
		if (ret != TOKENCHECKER_CONTINUE) {
			printf("tokenchecker_inputchar fail: right string\n");
			free(str);
			return TEST_RESULT_FAIL;
		}
		tokenchecker_getlastmatchedstring(&checker, &str2, &len2);
		if (strncmp(str, str2, len2) != 0) {
			printf("tokenchecker_getlastmatchedstring fail: right string\n");
			printf(" ");
			for (j = 0; j < len; j++) {
				printf("%c", str[j]);
			}
			printf(" : ");
			for (j = 0; j < len; j++) {
				printf("%c", str2[j]);
			}
			printf("\n");
			free(str);
			return TEST_RESULT_FAIL;
		}
	}
	ret = tokenchecker_endinput(&checker, &val);
	if (ret != TOKENCHECKER_DETERMINE) {
		printf("tokenchecker_endinput fail\n");
		free(str);
		return TEST_RESULT_FAIL;
	}
	if (val != 1) {
		printf("tokenchecker_endinput fail: select value\n");
		free(str);
		return TEST_RESULT_FAIL;
	}
	tokenchecker_getlastmatchedstring(&checker, &str2, &len2);
	if (strncmp(str, str2, len2) != 0) {
		printf("tokenchecker_getlastmatchedstring fail: end token\n");
		free(str);
		return TEST_RESULT_FAIL;
	}
	ret = tokenchecker_inputchar(&checker, ':', &val);
	if (ret != TOKENCHECKER_AFTER_END) {
		printf("tokenchecker_inputchar fail: after end\n");
		free(str);
		return TEST_RESULT_FAIL;
	}

	free(str);
	return TEST_RESULT_PASS;
}

LOCAL TEST_RESULT test_tokenchecker_16()
{
	tokenchecker_t checker;
	W i, j, len, len2, val, ret;
	UB *str, *str2;

	str = strdup(test_tokenchecker_testdata_tuple_0001[1].name);
	len = strlen(str);

	tokenchecker_initialize(&checker,
							 test_tokenchecker_testdata_tuple_0001,
							 7,
							 test_tokenchecker_testdata_etoken_0001);

	for (i = 0; i < len; i++) {
		ret = tokenchecker_inputchar(&checker, str[i], &val);
		if (ret != TOKENCHECKER_CONTINUE) {
			printf("tokenchecker_inputchar fail: right string\n");
			free(str);
			return TEST_RESULT_FAIL;
		}
		tokenchecker_getlastmatchedstring(&checker, &str2, &len2);
		if (strncmp(str, str2, len2) != 0) {
			printf("tokenchecker_getlastmatchedstring fail: right string\n");
			printf(" ");
			for (j = 0; j < len; j++) {
				printf("%c", str[j]);
			}
			printf(" : ");
			for (j = 0; j < len; j++) {
				printf("%c", str2[j]);
			}
			printf("\n");
			free(str);
			return TEST_RESULT_FAIL;
		}
	}
	ret = tokenchecker_endinput(&checker, &val);
	if (ret != TOKENCHECKER_DETERMINE) {
		printf("tokenchecker_endinput fail\n");
		free(str);
		return TEST_RESULT_FAIL;
	}
	if (val != 2) {
		printf("tokenchecker_endinput fail: select value\n");
		free(str);
		return TEST_RESULT_FAIL;
	}
	tokenchecker_getlastmatchedstring(&checker, &str2, &len2);
	if (strncmp(str, str2, len2) != 0) {
		printf("tokenchecker_getlastmatchedstring fail: end token\n");
		free(str);
		return TEST_RESULT_FAIL;
	}
	ret = tokenchecker_inputchar(&checker, ':', &val);
	if (ret != TOKENCHECKER_AFTER_END) {
		printf("tokenchecker_inputchar fail: after end\n");
		free(str);
		return TEST_RESULT_FAIL;
	}

	free(str);
	return TEST_RESULT_PASS;
}

LOCAL TEST_RESULT test_tokenchecker_17()
{
	tokenchecker_t checker;
	W i, j, len, len2, val, ret;
	UB *str, *str2;

	str = strdup(test_tokenchecker_testdata_tuple_0001[2].name);
	len = strlen(str);

	tokenchecker_initialize(&checker,
							 test_tokenchecker_testdata_tuple_0001,
							 7,
							 test_tokenchecker_testdata_etoken_0001);

	for (i = 0; i < len; i++) {
		ret = tokenchecker_inputchar(&checker, str[i], &val);
		if (ret != TOKENCHECKER_CONTINUE) {
			printf("tokenchecker_inputchar fail: right string\n");
			free(str);
			return TEST_RESULT_FAIL;
		}
		tokenchecker_getlastmatchedstring(&checker, &str2, &len2);
		if (strncmp(str, str2, len2) != 0) {
			printf("tokenchecker_getlastmatchedstring fail: right string\n");
			printf(" ");
			for (j = 0; j < len; j++) {
				printf("%c", str[j]);
			}
			printf(" : ");
			for (j = 0; j < len; j++) {
				printf("%c", str2[j]);
			}
			printf("\n");
			free(str);
			return TEST_RESULT_FAIL;
		}
	}
	ret = tokenchecker_endinput(&checker, &val);
	if (ret != TOKENCHECKER_DETERMINE) {
		printf("tokenchecker_endinput fail\n");
		free(str);
		return TEST_RESULT_FAIL;
	}
	if (val != 3) {
		printf("tokenchecker_endinput fail: select value\n");
		free(str);
		return TEST_RESULT_FAIL;
	}
	tokenchecker_getlastmatchedstring(&checker, &str2, &len2);
	if (strncmp(str, str2, len2) != 0) {
		printf("tokenchecker_getlastmatchedstring fail: end token\n");
		free(str);
		return TEST_RESULT_FAIL;
	}
	ret = tokenchecker_inputchar(&checker, ':', &val);
	if (ret != TOKENCHECKER_AFTER_END) {
		printf("tokenchecker_inputchar fail: after end\n");
		free(str);
		return TEST_RESULT_FAIL;
	}

	free(str);
	return TEST_RESULT_PASS;
}

LOCAL TEST_RESULT test_tokenchecker_18()
{
	tokenchecker_t checker;
	W i, j, len, len2, val, ret;
	UB *str, *str2;

	str = strdup(test_tokenchecker_testdata_tuple_0001[3].name);
	len = strlen(str);

	tokenchecker_initialize(&checker,
							 test_tokenchecker_testdata_tuple_0001,
							 7,
							 test_tokenchecker_testdata_etoken_0001);

	for (i = 0; i < len; i++) {
		ret = tokenchecker_inputchar(&checker, str[i], &val);
		if (ret != TOKENCHECKER_CONTINUE) {
			printf("tokenchecker_inputchar fail: right string\n");
			free(str);
			return TEST_RESULT_FAIL;
		}
		tokenchecker_getlastmatchedstring(&checker, &str2, &len2);
		if (strncmp(str, str2, len2) != 0) {
			printf("tokenchecker_getlastmatchedstring fail: right string\n");
			printf(" ");
			for (j = 0; j < len; j++) {
				printf("%c", str[j]);
			}
			printf(" : ");
			for (j = 0; j < len; j++) {
				printf("%c", str2[j]);
			}
			printf("\n");
			free(str);
			return TEST_RESULT_FAIL;
		}
	}
	ret = tokenchecker_endinput(&checker, &val);
	if (ret != TOKENCHECKER_DETERMINE) {
		printf("tokenchecker_endinput fail\n");
		free(str);
		return TEST_RESULT_FAIL;
	}
	if (val != 4) {
		printf("tokenchecker_endinput fail: select value\n");
		free(str);
		return TEST_RESULT_FAIL;
	}
	tokenchecker_getlastmatchedstring(&checker, &str2, &len2);
	if (strncmp(str, str2, len2) != 0) {
		printf("tokenchecker_getlastmatchedstring fail: end token\n");
		free(str);
		return TEST_RESULT_FAIL;
	}
	ret = tokenchecker_inputchar(&checker, ':', &val);
	if (ret != TOKENCHECKER_AFTER_END) {
		printf("tokenchecker_inputchar fail: after end\n");
		free(str);
		return TEST_RESULT_FAIL;
	}

	free(str);
	return TEST_RESULT_PASS;
}

LOCAL TEST_RESULT test_tokenchecker_19()
{
	tokenchecker_t checker;
	W i, j, len, len2, val, ret;
	UB *str, *str2;

	str = strdup(test_tokenchecker_testdata_tuple_0001[4].name);
	len = strlen(str);

	tokenchecker_initialize(&checker,
							 test_tokenchecker_testdata_tuple_0001,
							 7,
							 test_tokenchecker_testdata_etoken_0001);

	for (i = 0; i < len; i++) {
		ret = tokenchecker_inputchar(&checker, str[i], &val);
		if (ret != TOKENCHECKER_CONTINUE) {
			printf("tokenchecker_inputchar fail: right string\n");
			free(str);
			return TEST_RESULT_FAIL;
		}
		tokenchecker_getlastmatchedstring(&checker, &str2, &len2);
		if (strncmp(str, str2, len2) != 0) {
			printf("tokenchecker_getlastmatchedstring fail: right string\n");
			printf(" ");
			for (j = 0; j < len; j++) {
				printf("%c", str[j]);
			}
			printf(" : ");
			for (j = 0; j < len; j++) {
				printf("%c", str2[j]);
			}
			printf("\n");
			free(str);
			return TEST_RESULT_FAIL;
		}
	}
	ret = tokenchecker_endinput(&checker, &val);
	if (ret != TOKENCHECKER_DETERMINE) {
		printf("tokenchecker_endinput fail\n");
		free(str);
		return TEST_RESULT_FAIL;
	}
	if (val != 5) {
		printf("tokenchecker_endinput fail: select value\n");
		free(str);
		return TEST_RESULT_FAIL;
	}
	tokenchecker_getlastmatchedstring(&checker, &str2, &len2);
	if (strncmp(str, str2, len2) != 0) {
		printf("tokenchecker_getlastmatchedstring fail: end token\n");
		free(str);
		return TEST_RESULT_FAIL;
	}
	ret = tokenchecker_inputchar(&checker, ':', &val);
	if (ret != TOKENCHECKER_AFTER_END) {
		printf("tokenchecker_inputchar fail: after end\n");
		free(str);
		return TEST_RESULT_FAIL;
	}

	free(str);
	return TEST_RESULT_PASS;
}

LOCAL TEST_RESULT test_tokenchecker_20()
{
	tokenchecker_t checker;
	W i, j, len, len2, val, ret;
	UB *str, *str2;

	str = strdup(test_tokenchecker_testdata_tuple_0001[5].name);
	len = strlen(str);

	tokenchecker_initialize(&checker,
							 test_tokenchecker_testdata_tuple_0001,
							 7,
							 test_tokenchecker_testdata_etoken_0001);

	for (i = 0; i < len; i++) {
		ret = tokenchecker_inputchar(&checker, str[i], &val);
		if (ret != TOKENCHECKER_CONTINUE) {
			printf("tokenchecker_inputchar fail: right string\n");
			free(str);
			return TEST_RESULT_FAIL;
		}
		tokenchecker_getlastmatchedstring(&checker, &str2, &len2);
		if (strncmp(str, str2, len2) != 0) {
			printf("tokenchecker_getlastmatchedstring fail: right string\n");
			printf(" ");
			for (j = 0; j < len; j++) {
				printf("%c", str[j]);
			}
			printf(" : ");
			for (j = 0; j < len; j++) {
				printf("%c", str2[j]);
			}
			printf("\n");
			free(str);
			return TEST_RESULT_FAIL;
		}
	}
	ret = tokenchecker_endinput(&checker, &val);
	if (ret != TOKENCHECKER_DETERMINE) {
		printf("tokenchecker_endinput fail\n");
		free(str);
		return TEST_RESULT_FAIL;
	}
	if (val != 6) {
		printf("tokenchecker_endinput fail: select value\n");
		free(str);
		return TEST_RESULT_FAIL;
	}
	tokenchecker_getlastmatchedstring(&checker, &str2, &len2);
	if (strncmp(str, str2, len2) != 0) {
		printf("tokenchecker_getlastmatchedstring fail: end token\n");
		free(str);
		return TEST_RESULT_FAIL;
	}
	ret = tokenchecker_inputchar(&checker, ':', &val);
	if (ret != TOKENCHECKER_AFTER_END) {
		printf("tokenchecker_inputchar fail: after end\n");
		free(str);
		return TEST_RESULT_FAIL;
	}

	free(str);
	return TEST_RESULT_PASS;
}

LOCAL TEST_RESULT test_tokenchecker_21()
{
	tokenchecker_t checker;
	W i, j, len, len2, val, ret;
	UB *str, *str2;

	str = strdup(test_tokenchecker_testdata_tuple_0001[6].name);
	len = strlen(str);

	tokenchecker_initialize(&checker,
							 test_tokenchecker_testdata_tuple_0001,
							 7,
							 test_tokenchecker_testdata_etoken_0001);

	for (i = 0; i < len; i++) {
		ret = tokenchecker_inputchar(&checker, str[i], &val);
		if (ret != TOKENCHECKER_CONTINUE) {
			printf("tokenchecker_inputchar fail: right string\n");
			free(str);
			return TEST_RESULT_FAIL;
		}
		tokenchecker_getlastmatchedstring(&checker, &str2, &len2);
		if (strncmp(str, str2, len2) != 0) {
			printf("tokenchecker_getlastmatchedstring fail: right string\n");
			printf(" ");
			for (j = 0; j < len; j++) {
				printf("%c", str[j]);
			}
			printf(" : ");
			for (j = 0; j < len; j++) {
				printf("%c", str2[j]);
			}
			printf("\n");
			free(str);
			return TEST_RESULT_FAIL;
		}
	}
	ret = tokenchecker_endinput(&checker, &val);
	if (ret != TOKENCHECKER_DETERMINE) {
		printf("tokenchecker_endinput fail\n");
		free(str);
		return TEST_RESULT_FAIL;
	}
	if (val != 7) {
		printf("tokenchecker_endinput fail: select value\n");
		free(str);
		return TEST_RESULT_FAIL;
	}
	tokenchecker_getlastmatchedstring(&checker, &str2, &len2);
	if (strncmp(str, str2, len2) != 0) {
		printf("tokenchecker_getlastmatchedstring fail: end token\n");
		free(str);
		return TEST_RESULT_FAIL;
	}
	ret = tokenchecker_inputchar(&checker, ':', &val);
	if (ret != TOKENCHECKER_AFTER_END) {
		printf("tokenchecker_inputchar fail: after end\n");
		free(str);
		return TEST_RESULT_FAIL;
	}

	free(str);
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
	test_parselib_printresult(test_character_reference_4, "test_character_reference_4");
	test_parselib_printresult(test_character_reference_5, "test_character_reference_5");
	test_parselib_printresult(test_character_reference_6, "test_character_reference_6");
	test_parselib_printresult(test_character_reference_7, "test_character_reference_7");
	test_parselib_printresult(test_character_reference_8, "test_character_reference_8");
	test_parselib_printresult(test_character_reference_9, "test_character_reference_9");
	test_parselib_printresult(test_character_reference_10, "test_character_reference_10");
	test_parselib_printresult(test_character_reference_11, "test_character_reference_11");
	test_parselib_printresult(test_tokenchecker_1, "test_tokenchecker_1");
	test_parselib_printresult(test_tokenchecker_2, "test_tokenchecker_2");
	test_parselib_printresult(test_tokenchecker_3, "test_tokenchecker_3");
	test_parselib_printresult(test_tokenchecker_4, "test_tokenchecker_4");
	test_parselib_printresult(test_tokenchecker_5, "test_tokenchecker_5");
	test_parselib_printresult(test_tokenchecker_6, "test_tokenchecker_6");
	test_parselib_printresult(test_tokenchecker_7, "test_tokenchecker_7");
	test_parselib_printresult(test_tokenchecker_8, "test_tokenchecker_8");
	test_parselib_printresult(test_tokenchecker_9, "test_tokenchecker_9");
	test_parselib_printresult(test_tokenchecker_10, "test_tokenchecker_10");
	test_parselib_printresult(test_tokenchecker_11, "test_tokenchecker_11");
	test_parselib_printresult(test_tokenchecker_12, "test_tokenchecker_12");
	test_parselib_printresult(test_tokenchecker_13, "test_tokenchecker_13");
	test_parselib_printresult(test_tokenchecker_14, "test_tokenchecker_14");
	test_parselib_printresult(test_tokenchecker_15, "test_tokenchecker_15");
	test_parselib_printresult(test_tokenchecker_16, "test_tokenchecker_16");
	test_parselib_printresult(test_tokenchecker_17, "test_tokenchecker_17");
	test_parselib_printresult(test_tokenchecker_18, "test_tokenchecker_18");
	test_parselib_printresult(test_tokenchecker_19, "test_tokenchecker_19");
	test_parselib_printresult(test_tokenchecker_20, "test_tokenchecker_20");
	test_parselib_printresult(test_tokenchecker_21, "test_tokenchecker_21");
}
