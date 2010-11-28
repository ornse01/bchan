/*
 * test_array.c
 *
 * Copyright (c) 2010 project bchan
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

#include    "array.h"

LOCAL TEST_RESULT test_array_1()
{
	arraybase_t array;
	W i, err, *val;
	TEST_RESULT result = TEST_RESULT_PASS;
	Bool found;

	err = arraybase_initialize(&array, sizeof(W), 100);
	if (err < 0) {
		return TEST_RESULT_FAIL;
	}

	for (i = 0; i < 100; i++) {
		err = arraybase_appendunit(&array, (VP)&i);
		if (err < 0) {
			printf("arraybase_appendunit failure\n");
			result = TEST_RESULT_FAIL;
		}
	}
	for (i = 0; i < 100; i++) {
		found = arraybase_getunitbyindex(&array, i, (VP)&val);
		if (found != True) {
			printf("arraybase_getunitbyindex not found failure\n");
			result = TEST_RESULT_FAIL;
		}
		if (*val != i) {
			printf("arraybase_getunitbyindex unexpected value failure: %d:%d\n", i, *val);
			result = TEST_RESULT_FAIL;
		}
	}

	arraybase_finalize(&array);

	return result;
}

LOCAL TEST_RESULT test_array_2()
{
	arraybase_t array;
	W i, err, *val;
	TEST_RESULT result = TEST_RESULT_PASS;
	Bool found;

	err = arraybase_initialize(&array, sizeof(W), 100);
	if (err < 0) {
		return TEST_RESULT_FAIL;
	}

	for (i = 0; i < 200; i++) {
		err = arraybase_appendunit(&array, (VP)&i);
		if (err < 0) {
			printf("arraybase_appendunit failure\n");
			result = TEST_RESULT_FAIL;
		}
	}
	for (i = 0; i < 200; i++) {
		found = arraybase_getunitbyindex(&array, i, (VP)&val);
		if (found != True) {
			printf("arraybase_getunitbyindex not found failure\n");
			result = TEST_RESULT_FAIL;
		}
		if (*val != i) {
			printf("arraybase_getunitbyindex unexpected value failure: %d:%d\n", i, *val);
			result = TEST_RESULT_FAIL;
		}
	}

	arraybase_finalize(&array);

	return result;
}

LOCAL TEST_RESULT test_array_3()
{
	arraybase_t array;
	W i, err, *val;
	TEST_RESULT result = TEST_RESULT_PASS;
	Bool found;

	err = arraybase_initialize(&array, sizeof(W), 100);
	if (err < 0) {
		return TEST_RESULT_FAIL;
	}

	for (i = 0; i < 2000; i++) {
		err = arraybase_appendunit(&array, (VP)&i);
		if (err < 0) {
			printf("arraybase_appendunit failure\n");
			result = TEST_RESULT_FAIL;
		}
	}
	for (i = 0; i < 2000; i++) {
		found = arraybase_getunitbyindex(&array, i, (VP)&val);
		if (found != True) {
			printf("arraybase_getunitbyindex not found failure\n");
			result = TEST_RESULT_FAIL;
		}
		if (*val != i) {
			printf("arraybase_getunitbyindex unexpected value failure: %d:%d\n", i, *val);
			result = TEST_RESULT_FAIL;
		}
	}

	arraybase_finalize(&array);

	return result;
}

LOCAL TEST_RESULT test_array_4()
{
	arraybase_t array;
	W i, err, *val;
	TEST_RESULT result = TEST_RESULT_PASS;
	Bool found;

	err = arraybase_initialize(&array, sizeof(W), 100);
	if (err < 0) {
		return TEST_RESULT_FAIL;
	}

	for (i = 0; i < 100; i++) {
		err = arraybase_appendunit(&array, (VP)&i);
		if (err < 0) {
			printf("arraybase_appendunit failure\n");
			result = TEST_RESULT_FAIL;
		}
	}
	for (i = 0; i < 200; i++) {
		found = arraybase_getunitbyindex(&array, i, (VP)&val);
		if (i < 100) {
			if (found != True) {
				printf("arraybase_getunitbyindex not found failure\n");
				result = TEST_RESULT_FAIL;
			}
			if (*val != i) {
				printf("arraybase_getunitbyindex unexpected value failure: %d:%d\n", i, *val);
				result = TEST_RESULT_FAIL;
			}
		} else {
			if (found != False) {
				printf("arraybase_getunitbyindex found failure\n");
				result = TEST_RESULT_FAIL;
			}
		}
	}

	arraybase_finalize(&array);

	return result;
}

LOCAL TEST_RESULT test_array_5()
{
	arraybase_t array;
	W i, err, *val;
	TEST_RESULT result = TEST_RESULT_PASS;
	Bool found;

	err = arraybase_initialize(&array, sizeof(W), 100);
	if (err < 0) {
		return TEST_RESULT_FAIL;
	}

	for (i = 0; i < 50; i++) {
		err = arraybase_appendunit(&array, (VP)&i);
		if (err < 0) {
			printf("arraybase_appendunit failure\n");
			result = TEST_RESULT_FAIL;
		}
	}
	for (i = 0; i < 200; i++) {
		found = arraybase_getunitbyindex(&array, i, (VP)&val);
		if (i < 50) {
			if (found != True) {
				printf("arraybase_getunitbyindex not found failure\n");
				result = TEST_RESULT_FAIL;
			}
			if (*val != i) {
				printf("arraybase_getunitbyindex unexpected value failure: %d:%d\n", i, *val);
				result = TEST_RESULT_FAIL;
			}
		} else {
			if (found != False) {
				printf("arraybase_getunitbyindex found failure\n");
				result = TEST_RESULT_FAIL;
			}
		}
	}

	arraybase_finalize(&array);

	return result;
}

LOCAL TEST_RESULT test_array_6()
{
	arraybase_t array;
	W i, err, *val;
	TEST_RESULT result = TEST_RESULT_PASS;
	Bool found;

	err = arraybase_initialize(&array, sizeof(W), 100);
	if (err < 0) {
		return TEST_RESULT_FAIL;
	}

	for (i = 0; i < 150; i++) {
		err = arraybase_appendunit(&array, (VP)&i);
		if (err < 0) {
			printf("arraybase_appendunit failure\n");
			result = TEST_RESULT_FAIL;
		}
	}
	for (i = 0; i < 200; i++) {
		found = arraybase_getunitbyindex(&array, i, (VP)&val);
		if (i < 150) {
			if (found != True) {
				printf("arraybase_getunitbyindex not found failure\n");
				result = TEST_RESULT_FAIL;
			}
			if (*val != i) {
				printf("arraybase_getunitbyindex unexpected value failure: %d:%d\n", i, *val);
				result = TEST_RESULT_FAIL;
			}
		} else {
			if (found != False) {
				printf("arraybase_getunitbyindex found failure\n");
				result = TEST_RESULT_FAIL;
			}
		}
	}

	arraybase_finalize(&array);

	return result;
}

typedef struct {
	W a;
	W b;
	W c;
} test_array_dummydata;

LOCAL TEST_RESULT test_array_7()
{
	arraybase_t array;
	W i, err;
	test_array_dummydata val, *val2;
	TEST_RESULT result = TEST_RESULT_PASS;
	Bool found;

	err = arraybase_initialize(&array, sizeof(test_array_dummydata), 100);
	if (err < 0) {
		return TEST_RESULT_FAIL;
	}

	for (i = 0; i < 100; i++) {
		val.a = 100;
		val.b = i + 1234;
		val.c = i % 23 ;
		err = arraybase_appendunit(&array, (VP)&val);
		if (err < 0) {
			printf("arraybase_appendunit failure\n");
			result = TEST_RESULT_FAIL;
		}
	}
	for (i = 0; i < 100; i++) {
		found = arraybase_getunitbyindex(&array, i, (VP)&val2);
		if (found != True) {
			printf("arraybase_getunitbyindex not found failure\n");
			result = TEST_RESULT_FAIL;
		}
		if ((val2->a != 100)||(val2->b != i + 1234)||(val2->c != i % 23)) {
			printf("arraybase_getunitbyindex unexpected value failure: %d\n", i);
			result = TEST_RESULT_FAIL;
		}
	}

	arraybase_finalize(&array);

	return result;
}

LOCAL TEST_RESULT test_array_8()
{
	arraybase_t array;
	W i, err;
	test_array_dummydata val, *val2;
	TEST_RESULT result = TEST_RESULT_PASS;
	Bool found;

	err = arraybase_initialize(&array, sizeof(test_array_dummydata), 100);
	if (err < 0) {
		return TEST_RESULT_FAIL;
	}

	for (i = 0; i < 200; i++) {
		val.a = 100;
		val.b = i + 1234;
		val.c = i % 23 ;
		err = arraybase_appendunit(&array, (VP)&val);
		if (err < 0) {
			printf("arraybase_appendunit failure\n");
			result = TEST_RESULT_FAIL;
		}
	}
	for (i = 0; i < 200; i++) {
		found = arraybase_getunitbyindex(&array, i, (VP)&val2);
		if (found != True) {
			printf("arraybase_getunitbyindex not found failure\n");
			result = TEST_RESULT_FAIL;
		}
		if ((val2->a != 100)||(val2->b != i + 1234)||(val2->c != i % 23)) {
			printf("arraybase_getunitbyindex unexpected value failure: %d\n", i);
			result = TEST_RESULT_FAIL;
		}
	}

	arraybase_finalize(&array);

	return result;
}

LOCAL TEST_RESULT test_array_9()
{
	arraybase_t array;
	W i, err;
	test_array_dummydata val, *val2;
	TEST_RESULT result = TEST_RESULT_PASS;
	Bool found;

	err = arraybase_initialize(&array, sizeof(test_array_dummydata), 100);
	if (err < 0) {
		return TEST_RESULT_FAIL;
	}

	for (i = 0; i < 2000; i++) {
		val.a = 100;
		val.b = i + 1234;
		val.c = i % 23 ;
		err = arraybase_appendunit(&array, (VP)&val);
		if (err < 0) {
			printf("arraybase_appendunit failure\n");
			result = TEST_RESULT_FAIL;
		}
	}
	for (i = 0; i < 2000; i++) {
		found = arraybase_getunitbyindex(&array, i, (VP)&val2);
		if (found != True) {
			printf("arraybase_getunitbyindex not found failure\n");
			result = TEST_RESULT_FAIL;
		}
		if ((val2->a != 100)||(val2->b != i + 1234)||(val2->c != i % 23)) {
			printf("arraybase_getunitbyindex unexpected value failure: %d\n", i);
			result = TEST_RESULT_FAIL;
		}
	}

	arraybase_finalize(&array);

	return result;
}

LOCAL TEST_RESULT test_array_10()
{
	arraybase_t array;
	W i, err;
	test_array_dummydata val, *val2;
	TEST_RESULT result = TEST_RESULT_PASS;
	Bool found;

	err = arraybase_initialize(&array, sizeof(test_array_dummydata), 100);
	if (err < 0) {
		return TEST_RESULT_FAIL;
	}

	for (i = 0; i < 100; i++) {
		val.a = 100;
		val.b = i + 1234;
		val.c = i % 23 ;
		err = arraybase_appendunit(&array, (VP)&val);
		if (err < 0) {
			printf("arraybase_appendunit failure\n");
			result = TEST_RESULT_FAIL;
		}
	}
	for (i = 0; i < 200; i++) {
		found = arraybase_getunitbyindex(&array, i, (VP)&val2);
		if (i < 100) {
			if (found != True) {
				printf("arraybase_getunitbyindex not found failure\n");
				result = TEST_RESULT_FAIL;
			}
			if ((val2->a != 100)||(val2->b != i + 1234)||(val2->c != i % 23)) {
				printf("arraybase_getunitbyindex unexpected value failure: %d\n", i);
				result = TEST_RESULT_FAIL;
			}
		} else {
			if (found != False) {
				printf("arraybase_getunitbyindex found failure\n");
				result = TEST_RESULT_FAIL;
			}
		}
	}

	arraybase_finalize(&array);

	return result;
}

LOCAL TEST_RESULT test_array_11()
{
	arraybase_t array;
	W i, err;
	test_array_dummydata val, *val2;
	TEST_RESULT result = TEST_RESULT_PASS;
	Bool found;

	err = arraybase_initialize(&array, sizeof(test_array_dummydata), 100);
	if (err < 0) {
		return TEST_RESULT_FAIL;
	}

	for (i = 0; i < 50; i++) {
		val.a = 100;
		val.b = i + 1234;
		val.c = i % 23 ;
		err = arraybase_appendunit(&array, (VP)&val);
		if (err < 0) {
			printf("arraybase_appendunit failure\n");
			result = TEST_RESULT_FAIL;
		}
	}
	for (i = 0; i < 200; i++) {
		found = arraybase_getunitbyindex(&array, i, (VP)&val2);
		if (i < 50) {
			if (found != True) {
				printf("arraybase_getunitbyindex not found failure\n");
				result = TEST_RESULT_FAIL;
			}
			if ((val2->a != 100)||(val2->b != i + 1234)||(val2->c != i % 23)) {
				printf("arraybase_getunitbyindex unexpected value failure: %d\n", i);
				result = TEST_RESULT_FAIL;
			}
		} else {
			if (found != False) {
				printf("arraybase_getunitbyindex found failure\n");
				result = TEST_RESULT_FAIL;
			}
		}
	}

	arraybase_finalize(&array);

	return result;
}

LOCAL TEST_RESULT test_array_12()
{
	arraybase_t array;
	W i, err;
	test_array_dummydata val, *val2;
	TEST_RESULT result = TEST_RESULT_PASS;
	Bool found;

	err = arraybase_initialize(&array, sizeof(test_array_dummydata), 100);
	if (err < 0) {
		return TEST_RESULT_FAIL;
	}

	for (i = 0; i < 150; i++) {
		val.a = 100;
		val.b = i + 1234;
		val.c = i % 23 ;
		err = arraybase_appendunit(&array, (VP)&val);
		if (err < 0) {
			printf("arraybase_appendunit failure\n");
			result = TEST_RESULT_FAIL;
		}
	}
	for (i = 0; i < 200; i++) {
		found = arraybase_getunitbyindex(&array, i, (VP)&val2);
		if (i < 150) {
			if (found != True) {
				printf("arraybase_getunitbyindex not found failure\n");
				result = TEST_RESULT_FAIL;
			}
			if ((val2->a != 100)||(val2->b != i + 1234)||(val2->c != i % 23)) {
				printf("arraybase_getunitbyindex unexpected value failure: %d\n", i);
				result = TEST_RESULT_FAIL;
			}
		} else {
			if (found != False) {
				printf("arraybase_getunitbyindex found failure\n");
				result = TEST_RESULT_FAIL;
			}
		}
	}

	arraybase_finalize(&array);

	return result;
}

LOCAL VOID test_array_printresult(TEST_RESULT (*proc)(), B *test_name)
{
	TEST_RESULT result;

	printf("test_array: %s\n", test_name);
	printf("---------------------------------------------\n");
	result = proc();
	if (result == TEST_RESULT_PASS) {
		printf("--pass---------------------------------------\n");
	} else {
		printf("--fail---------------------------------------\n");
	}
	printf("---------------------------------------------\n");
}

EXPORT VOID test_array_main()
{
	test_array_printresult(test_array_1, "test_array_1");
	test_array_printresult(test_array_2, "test_array_2");
	test_array_printresult(test_array_3, "test_array_3");
	test_array_printresult(test_array_4, "test_array_4");
	test_array_printresult(test_array_5, "test_array_5");
	test_array_printresult(test_array_6, "test_array_6");
	test_array_printresult(test_array_7, "test_array_7");
	test_array_printresult(test_array_8, "test_array_8");
	test_array_printresult(test_array_9, "test_array_9");
	test_array_printresult(test_array_10, "test_array_10");
	test_array_printresult(test_array_11, "test_array_11");
	test_array_printresult(test_array_12, "test_array_12");
}
