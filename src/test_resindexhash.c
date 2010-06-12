/*
 * test_resindexhash.c
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
#include    <bstring.h>

#include    "test.h"

#include    "resindexhash.h"

LOCAL TEST_RESULT test_resindexhash_1()
{
	resindexhash_t resindexhash;
	W ret;

	ret = resindexhash_initialize(&resindexhash);
	if (ret < 0) {
		printf("resindexhash_initialize fail\n");
		return TEST_RESULT_FAIL;
	}
	resindexhash_finalize(&resindexhash);

	return TEST_RESULT_PASS;
}

LOCAL TEST_RESULT test_resindexhash_2()
{
	resindexhash_t resindexhash;
	W index = 1;
	W ret;
	UW attr;
	COLOR color;
	TEST_RESULT result = TEST_RESULT_PASS;

	ret = resindexhash_initialize(&resindexhash);
	if (ret < 0) {
		printf("resindexhash_initialize fail\n");
		return TEST_RESULT_FAIL;
	}
	ret = resindexhash_searchdata(&resindexhash, index, &attr, &color);
	if (ret != RESINDEXHASH_SEARCHDATA_NOTFOUND) {
		printf("resindexhash_searchdata fail\n");
		result = TEST_RESULT_FAIL;
	}
	resindexhash_finalize(&resindexhash);

	return result;
}

LOCAL TEST_RESULT test_resindexhash_3()
{
	resindexhash_t resindexhash;
	W index = 1;
	W ret;
	UW attr, attr1 = 0x01010101;
	COLOR color, color1 = 0x10FF00FF;
	TEST_RESULT result = TEST_RESULT_PASS;

	ret = resindexhash_initialize(&resindexhash);
	if (ret < 0) {
		printf("resindexhash_initialize fail\n");
		return TEST_RESULT_FAIL;
	}
	ret = resindexhash_adddata(&resindexhash, index, attr1, color1);
	if (ret < 0) {
		printf("resindexhash_adddata fail\n");
		result = TEST_RESULT_FAIL;
	}
	ret = resindexhash_searchdata(&resindexhash, index, &attr, &color);
	if (ret != RESINDEXHASH_SEARCHDATA_FOUND) {
		printf("resindexhash_searchdata fail\n");
		result = TEST_RESULT_FAIL;
	}
	if (attr != attr1) {
		printf("resindexhash_searchdata result fail\n");
		result = TEST_RESULT_FAIL;
	}
	if (color != color1) {
		printf("resindexhash_searchdata result fail\n");
		result = TEST_RESULT_FAIL;
	}
	resindexhash_finalize(&resindexhash);

	return result;
}

LOCAL TEST_RESULT test_resindexhash_4()
{
	resindexhash_t resindexhash;
	W index = 1;
	W ret;
	UW attr, attr1 = 0x01010101, attr2 = 0x10101010;
	COLOR color, color1 = 0x10FF00FF, color2 = 0x1000FF00;
	TEST_RESULT result = TEST_RESULT_PASS;

	ret = resindexhash_initialize(&resindexhash);
	if (ret < 0) {
		printf("resindexhash_initialize fail\n");
		return TEST_RESULT_FAIL;
	}
	ret = resindexhash_adddata(&resindexhash, index, attr1, color1);
	if (ret < 0) {
		printf("resindexhash_adddata fail\n");
		result = TEST_RESULT_FAIL;
	}
	ret = resindexhash_adddata(&resindexhash, index, attr2, color2);
	if (ret < 0) {
		printf("resindexhash_adddata fail\n");
		result = TEST_RESULT_FAIL;
	}
	ret = resindexhash_searchdata(&resindexhash, index, &attr, &color);
	if (ret != RESINDEXHASH_SEARCHDATA_FOUND) {
		printf("resindexhash_searchdata fail\n");
		result = TEST_RESULT_FAIL;
	}
	if (attr != attr2) {
		printf("resindexhash_searchdata result fail\n");
		result = TEST_RESULT_FAIL;
	}
	if (color != color2) {
		printf("resindexhash_searchdata result fail\n");
		result = TEST_RESULT_FAIL;
	}
	resindexhash_finalize(&resindexhash);

	return result;
}

LOCAL TEST_RESULT test_resindexhash_5()
{
	resindexhash_t resindexhash;
	W index = 1;
	W ret;
	UW attr, attr1 = 0x01010101;
	COLOR color, color1 = 0x10FF00FF;
	TEST_RESULT result = TEST_RESULT_PASS;

	ret = resindexhash_initialize(&resindexhash);
	if (ret < 0) {
		printf("resindexhash_initialize fail\n");
		return TEST_RESULT_FAIL;
	}
	ret = resindexhash_adddata(&resindexhash, index, attr1, color1);
	if (ret < 0) {
		printf("resindexhash_adddata fail\n");
		result = TEST_RESULT_FAIL;
	}
	resindexhash_removedata(&resindexhash, index);
	ret = resindexhash_searchdata(&resindexhash, index, &attr, &color);
	if (ret != RESINDEXHASH_SEARCHDATA_NOTFOUND) {
		printf("resindexhash_searchdata fail\n");
		result = TEST_RESULT_FAIL;
	}
	resindexhash_finalize(&resindexhash);

	return result;
}

LOCAL TEST_RESULT test_resindexhash_6()
{
	resindexhash_t resindexhash;
	W index = 1;
	W ret;
	TEST_RESULT result = TEST_RESULT_PASS;

	ret = resindexhash_initialize(&resindexhash);
	if (ret < 0) {
		printf("resindexhash_initialize fail\n");
		return TEST_RESULT_FAIL;
	}
	resindexhash_removedata(&resindexhash, index);
	resindexhash_finalize(&resindexhash);

	return result;
}

LOCAL TEST_RESULT test_resindexhash_7()
{
	resindexhash_t resindexhash;
	W index1 = 1, index2 = 34;
	W ret;
	UW attr, attr1 = 0x01010101, attr2 = 0x10101010;
	COLOR color, color1 = 0x10FF00FF, color2 = 0x1000FF00;
	TEST_RESULT result = TEST_RESULT_PASS;

	ret = resindexhash_initialize(&resindexhash);
	if (ret < 0) {
		printf("resindexhash_initialize fail\n");
		return TEST_RESULT_FAIL;
	}
	ret = resindexhash_adddata(&resindexhash, index1, attr1, color1);
	if (ret < 0) {
		printf("resindexhash_adddata fail\n");
		result = TEST_RESULT_FAIL;
	}
	ret = resindexhash_adddata(&resindexhash, index2, attr2, color2);
	if (ret < 0) {
		printf("resindexhash_adddata fail\n");
		result = TEST_RESULT_FAIL;
	}
	ret = resindexhash_searchdata(&resindexhash, index1, &attr, &color);
	if (ret != RESINDEXHASH_SEARCHDATA_FOUND) {
		printf("resindexhash_searchdata 1 fail\n");
		result = TEST_RESULT_FAIL;
	}
	if (attr != attr1) {
		printf("resindexhash_searchdata 1 result fail\n");
		result = TEST_RESULT_FAIL;
	}
	if (color != color1) {
		printf("resindexhash_searchdata 1 result fail\n");
		result = TEST_RESULT_FAIL;
	}
	ret = resindexhash_searchdata(&resindexhash, index2, &attr, &color);
	if (ret != RESINDEXHASH_SEARCHDATA_FOUND) {
		printf("resindexhash_searchdata fail\n");
		result = TEST_RESULT_FAIL;
	}
	if (attr != attr2) {
		printf("resindexhash_searchdata 2 result fail\n");
		result = TEST_RESULT_FAIL;
	}
	if (color != color2) {
		printf("resindexhash_searchdata 2 result fail\n");
		result = TEST_RESULT_FAIL;
	}
	resindexhash_finalize(&resindexhash);

	return result;
}

LOCAL TEST_RESULT test_resindexhash_8()
{
	resindexhash_t resindexhash;
	resindexhash_iterator_t iterator;
	W index;
	W ret, num;
	UW attr;
	COLOR color;
	Bool next;
	TEST_RESULT result = TEST_RESULT_PASS;

	ret = resindexhash_initialize(&resindexhash);
	if (ret < 0) {
		printf("resindexhash_initialize fail\n");
		return TEST_RESULT_FAIL;
	}


	resindexhash_iterator_initialize(&iterator, &resindexhash);
	num = 0;
	for (;;) {
		next = resindexhash_iterator_next(&iterator, &index, &attr, &color);
		if (next == False) {
			break;
		}
		num++;
	}
	if (num != 0) {
		printf("resindexhash_iterator unexpected length\n");
		result = TEST_RESULT_FAIL;
	}
	resindexhash_iterator_finalize(&iterator);


	resindexhash_finalize(&resindexhash);

	return result;
}

LOCAL TEST_RESULT test_resindexhash_9()
{
	resindexhash_t resindexhash;
	resindexhash_iterator_t iterator;
	W index, index1 = 1;
	W ret, num;
	UW attr, attr1 = 0x01010101;
	COLOR color, color1 = 0x10FF00FF;
	Bool next;
	TEST_RESULT result = TEST_RESULT_PASS;

	ret = resindexhash_initialize(&resindexhash);
	if (ret < 0) {
		printf("resindexhash_initialize fail\n");
		return TEST_RESULT_FAIL;
	}
	ret = resindexhash_adddata(&resindexhash, index1, attr1, color1);
	if (ret < 0) {
		printf("resindexhash_adddata fail\n");
		result = TEST_RESULT_FAIL;
	}


	resindexhash_iterator_initialize(&iterator, &resindexhash);
	num = 0;
	for (;;) {
		next = resindexhash_iterator_next(&iterator, &index, &attr, &color);
		if (next == False) {
			break;
		}
		num++;
		if (index == index1) {
			if (attr != attr1) {
				result = TEST_RESULT_FAIL;
			}
			if (color != color1) {
				result = TEST_RESULT_FAIL;
			}
		} else {
			result = TEST_RESULT_FAIL;
		}
	}
	if (num != 1) {
		printf("resindexhash_iterator unexpected length\n");
		result = TEST_RESULT_FAIL;
	}
	resindexhash_iterator_finalize(&iterator);


	resindexhash_finalize(&resindexhash);

	return result;
}

LOCAL TEST_RESULT test_resindexhash_10()
{
	resindexhash_t resindexhash;
	resindexhash_iterator_t iterator;
	W index1 = 1, index2 = 34;
	W ret, num, index;
	UW attr, attr1 = 0x01010101, attr2 = 0x10101010;
	COLOR color, color1 = 0x10FF00FF, color2 = 0x1000FF00;
	Bool next;
	TEST_RESULT result = TEST_RESULT_PASS;

	ret = resindexhash_initialize(&resindexhash);
	if (ret < 0) {
		printf("resindexhash_initialize fail\n");
		return TEST_RESULT_FAIL;
	}
	ret = resindexhash_adddata(&resindexhash, index1, attr1, color1);
	if (ret < 0) {
		printf("resindexhash_adddata fail\n");
		result = TEST_RESULT_FAIL;
	}
	ret = resindexhash_adddata(&resindexhash, index2, attr2, color2);
	if (ret < 0) {
		printf("resindexhash_adddata fail\n");
		result = TEST_RESULT_FAIL;
	}


	resindexhash_iterator_initialize(&iterator, &resindexhash);
	num = 0;
	for (;;) {
		next = resindexhash_iterator_next(&iterator, &index, &attr, &color);
		if (next == False) {
			break;
		}
		num++;
		if (index == index1) {
			if (attr != attr1) {
				result = TEST_RESULT_FAIL;
			}
			if (color != color1) {
				result = TEST_RESULT_FAIL;
			}
		} else if (index == index2) {
			if (attr != attr2) {
				result = TEST_RESULT_FAIL;
			}
			if (color != color2) {
				result = TEST_RESULT_FAIL;
			}
		} else {
			result = TEST_RESULT_FAIL;
		}
	}
	if (num != 2) {
		printf("resindexhash_iterator unexpected length\n");
		result = TEST_RESULT_FAIL;
	}
	resindexhash_iterator_finalize(&iterator);


	resindexhash_finalize(&resindexhash);

	return result;
}

LOCAL VOID test_resindexhash_printresult(TEST_RESULT (*proc)(), B *test_name)
{
	TEST_RESULT result;

	printf("test_resindexhash: %s\n", test_name);
	printf("---------------------------------------------\n");
	result = proc();
	if (result == TEST_RESULT_PASS) {
		printf("--pass---------------------------------------\n");
	} else {
		printf("--fail---------------------------------------\n");
	}
	printf("---------------------------------------------\n");
}

EXPORT VOID test_resindexhash_main()
{
	test_resindexhash_printresult(test_resindexhash_1, "test_resindexhash_1");
	test_resindexhash_printresult(test_resindexhash_2, "test_resindexhash_2");
	test_resindexhash_printresult(test_resindexhash_3, "test_resindexhash_3");
	test_resindexhash_printresult(test_resindexhash_4, "test_resindexhash_4");
	test_resindexhash_printresult(test_resindexhash_5, "test_resindexhash_5");
	test_resindexhash_printresult(test_resindexhash_6, "test_resindexhash_6");
	test_resindexhash_printresult(test_resindexhash_7, "test_resindexhash_7");
	test_resindexhash_printresult(test_resindexhash_8, "test_resindexhash_8");
	test_resindexhash_printresult(test_resindexhash_9, "test_resindexhash_9");
	test_resindexhash_printresult(test_resindexhash_10, "test_resindexhash_10");
}
