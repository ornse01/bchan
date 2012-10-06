/*
 * test_resindexhash.c
 *
 * Copyright (c) 2010-2012 project bchan
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

#include    "resindexhash.h"

#include    <btron/btron.h>
#include    <bstdio.h>
#include    <bstring.h>

#include    <unittest_driver.h>

LOCAL UNITTEST_RESULT test_resindexhash_1()
{
	resindexhash_t resindexhash;
	W ret;

	ret = resindexhash_initialize(&resindexhash);
	if (ret < 0) {
		printf("resindexhash_initialize fail\n");
		return UNITTEST_RESULT_FAIL;
	}
	resindexhash_finalize(&resindexhash);

	return UNITTEST_RESULT_PASS;
}

LOCAL UNITTEST_RESULT test_resindexhash_2()
{
	resindexhash_t resindexhash;
	W index = 1;
	W ret;
	UW attr;
	COLOR color;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	ret = resindexhash_initialize(&resindexhash);
	if (ret < 0) {
		printf("resindexhash_initialize fail\n");
		return UNITTEST_RESULT_FAIL;
	}
	ret = resindexhash_searchdata(&resindexhash, index, &attr, &color);
	if (ret != RESINDEXHASH_SEARCHDATA_NOTFOUND) {
		printf("resindexhash_searchdata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	resindexhash_finalize(&resindexhash);

	return result;
}

LOCAL UNITTEST_RESULT test_resindexhash_3()
{
	resindexhash_t resindexhash;
	W index = 1;
	W ret;
	UW attr, attr1 = 0x01010101;
	COLOR color, color1 = 0x10FF00FF;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	ret = resindexhash_initialize(&resindexhash);
	if (ret < 0) {
		printf("resindexhash_initialize fail\n");
		return UNITTEST_RESULT_FAIL;
	}
	ret = resindexhash_adddata(&resindexhash, index, attr1, color1);
	if (ret < 0) {
		printf("resindexhash_adddata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	ret = resindexhash_searchdata(&resindexhash, index, &attr, &color);
	if (ret != RESINDEXHASH_SEARCHDATA_FOUND) {
		printf("resindexhash_searchdata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	if (attr != attr1) {
		printf("resindexhash_searchdata result fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	if (color != color1) {
		printf("resindexhash_searchdata result fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	resindexhash_finalize(&resindexhash);

	return result;
}

LOCAL UNITTEST_RESULT test_resindexhash_4()
{
	resindexhash_t resindexhash;
	W index = 1;
	W ret;
	UW attr, attr1 = 0x01010101, attr2 = 0x10101010;
	COLOR color, color1 = 0x10FF00FF, color2 = 0x1000FF00;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	ret = resindexhash_initialize(&resindexhash);
	if (ret < 0) {
		printf("resindexhash_initialize fail\n");
		return UNITTEST_RESULT_FAIL;
	}
	ret = resindexhash_adddata(&resindexhash, index, attr1, color1);
	if (ret < 0) {
		printf("resindexhash_adddata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	ret = resindexhash_adddata(&resindexhash, index, attr2, color2);
	if (ret < 0) {
		printf("resindexhash_adddata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	ret = resindexhash_searchdata(&resindexhash, index, &attr, &color);
	if (ret != RESINDEXHASH_SEARCHDATA_FOUND) {
		printf("resindexhash_searchdata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	if (attr != attr2) {
		printf("resindexhash_searchdata result fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	if (color != color2) {
		printf("resindexhash_searchdata result fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	resindexhash_finalize(&resindexhash);

	return result;
}

LOCAL UNITTEST_RESULT test_resindexhash_5()
{
	resindexhash_t resindexhash;
	W index = 1;
	W ret;
	UW attr, attr1 = 0x01010101;
	COLOR color, color1 = 0x10FF00FF;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	ret = resindexhash_initialize(&resindexhash);
	if (ret < 0) {
		printf("resindexhash_initialize fail\n");
		return UNITTEST_RESULT_FAIL;
	}
	ret = resindexhash_adddata(&resindexhash, index, attr1, color1);
	if (ret < 0) {
		printf("resindexhash_adddata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	resindexhash_removedata(&resindexhash, index);
	ret = resindexhash_searchdata(&resindexhash, index, &attr, &color);
	if (ret != RESINDEXHASH_SEARCHDATA_NOTFOUND) {
		printf("resindexhash_searchdata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	resindexhash_finalize(&resindexhash);

	return result;
}

LOCAL UNITTEST_RESULT test_resindexhash_6()
{
	resindexhash_t resindexhash;
	W index = 1;
	W ret;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	ret = resindexhash_initialize(&resindexhash);
	if (ret < 0) {
		printf("resindexhash_initialize fail\n");
		return UNITTEST_RESULT_FAIL;
	}
	resindexhash_removedata(&resindexhash, index);
	resindexhash_finalize(&resindexhash);

	return result;
}

LOCAL UNITTEST_RESULT test_resindexhash_7()
{
	resindexhash_t resindexhash;
	W index1 = 1, index2 = 34;
	W ret;
	UW attr, attr1 = 0x01010101, attr2 = 0x10101010;
	COLOR color, color1 = 0x10FF00FF, color2 = 0x1000FF00;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	ret = resindexhash_initialize(&resindexhash);
	if (ret < 0) {
		printf("resindexhash_initialize fail\n");
		return UNITTEST_RESULT_FAIL;
	}
	ret = resindexhash_adddata(&resindexhash, index1, attr1, color1);
	if (ret < 0) {
		printf("resindexhash_adddata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	ret = resindexhash_adddata(&resindexhash, index2, attr2, color2);
	if (ret < 0) {
		printf("resindexhash_adddata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	ret = resindexhash_searchdata(&resindexhash, index1, &attr, &color);
	if (ret != RESINDEXHASH_SEARCHDATA_FOUND) {
		printf("resindexhash_searchdata 1 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	if (attr != attr1) {
		printf("resindexhash_searchdata 1 result fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	if (color != color1) {
		printf("resindexhash_searchdata 1 result fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	ret = resindexhash_searchdata(&resindexhash, index2, &attr, &color);
	if (ret != RESINDEXHASH_SEARCHDATA_FOUND) {
		printf("resindexhash_searchdata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	if (attr != attr2) {
		printf("resindexhash_searchdata 2 result fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	if (color != color2) {
		printf("resindexhash_searchdata 2 result fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	resindexhash_finalize(&resindexhash);

	return result;
}

LOCAL UNITTEST_RESULT test_resindexhash_8()
{
	resindexhash_t resindexhash;
	resindexhash_iterator_t iterator;
	W index;
	W ret, num;
	UW attr;
	COLOR color;
	Bool next;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	ret = resindexhash_initialize(&resindexhash);
	if (ret < 0) {
		printf("resindexhash_initialize fail\n");
		return UNITTEST_RESULT_FAIL;
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
		result = UNITTEST_RESULT_FAIL;
	}
	resindexhash_iterator_finalize(&iterator);


	resindexhash_finalize(&resindexhash);

	return result;
}

LOCAL UNITTEST_RESULT test_resindexhash_9()
{
	resindexhash_t resindexhash;
	resindexhash_iterator_t iterator;
	W index, index1 = 1;
	W ret, num;
	UW attr, attr1 = 0x01010101;
	COLOR color, color1 = 0x10FF00FF;
	Bool next;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	ret = resindexhash_initialize(&resindexhash);
	if (ret < 0) {
		printf("resindexhash_initialize fail\n");
		return UNITTEST_RESULT_FAIL;
	}
	ret = resindexhash_adddata(&resindexhash, index1, attr1, color1);
	if (ret < 0) {
		printf("resindexhash_adddata fail\n");
		result = UNITTEST_RESULT_FAIL;
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
				result = UNITTEST_RESULT_FAIL;
			}
			if (color != color1) {
				result = UNITTEST_RESULT_FAIL;
			}
		} else {
			result = UNITTEST_RESULT_FAIL;
		}
	}
	if (num != 1) {
		printf("resindexhash_iterator unexpected length\n");
		result = UNITTEST_RESULT_FAIL;
	}
	resindexhash_iterator_finalize(&iterator);


	resindexhash_finalize(&resindexhash);

	return result;
}

LOCAL UNITTEST_RESULT test_resindexhash_10()
{
	resindexhash_t resindexhash;
	resindexhash_iterator_t iterator;
	W index1 = 1, index2 = 34;
	W ret, num, index;
	UW attr, attr1 = 0x01010101, attr2 = 0x10101010;
	COLOR color, color1 = 0x10FF00FF, color2 = 0x1000FF00;
	Bool next;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	ret = resindexhash_initialize(&resindexhash);
	if (ret < 0) {
		printf("resindexhash_initialize fail\n");
		return UNITTEST_RESULT_FAIL;
	}
	ret = resindexhash_adddata(&resindexhash, index1, attr1, color1);
	if (ret < 0) {
		printf("resindexhash_adddata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	ret = resindexhash_adddata(&resindexhash, index2, attr2, color2);
	if (ret < 0) {
		printf("resindexhash_adddata fail\n");
		result = UNITTEST_RESULT_FAIL;
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
				result = UNITTEST_RESULT_FAIL;
			}
			if (color != color1) {
				result = UNITTEST_RESULT_FAIL;
			}
		} else if (index == index2) {
			if (attr != attr2) {
				result = UNITTEST_RESULT_FAIL;
			}
			if (color != color2) {
				result = UNITTEST_RESULT_FAIL;
			}
		} else {
			result = UNITTEST_RESULT_FAIL;
		}
	}
	if (num != 2) {
		printf("resindexhash_iterator unexpected length\n");
		result = UNITTEST_RESULT_FAIL;
	}
	resindexhash_iterator_finalize(&iterator);


	resindexhash_finalize(&resindexhash);

	return result;
}

EXPORT VOID test_resindexhash_main(unittest_driver_t *driver)
{
	UNITTEST_DRIVER_REGIST(driver, test_resindexhash_1);
	UNITTEST_DRIVER_REGIST(driver, test_resindexhash_2);
	UNITTEST_DRIVER_REGIST(driver, test_resindexhash_3);
	UNITTEST_DRIVER_REGIST(driver, test_resindexhash_4);
	UNITTEST_DRIVER_REGIST(driver, test_resindexhash_5);
	UNITTEST_DRIVER_REGIST(driver, test_resindexhash_6);
	UNITTEST_DRIVER_REGIST(driver, test_resindexhash_7);
	UNITTEST_DRIVER_REGIST(driver, test_resindexhash_8);
	UNITTEST_DRIVER_REGIST(driver, test_resindexhash_9);
	UNITTEST_DRIVER_REGIST(driver, test_resindexhash_10);
}
