/*
 * test_residhash.c
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

#include    "residhash.h"

#include    <btron/btron.h>
#include    <bstdio.h>
#include    <bstring.h>
#include    <tstring.h>

#include    <unittest_driver.h>

LOCAL UNITTEST_RESULT test_residhash_1()
{
	residhash_t residhash;
	W ret;

	ret = residhash_initialize(&residhash);
	if (ret < 0) {
		printf("residhash_initialize fail\n");
		return UNITTEST_RESULT_FAIL;
	}
	residhash_finalize(&residhash);

	return UNITTEST_RESULT_PASS;
}

LOCAL UNITTEST_RESULT test_residhash_2()
{
	residhash_t residhash;
	UB idstr[] = "yZXmy7Om0";
	TC idstr_tc[9];
	W ret, idstr_len = strlen(idstr);
	UW attr;
	COLOR color;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	sjstotcs(idstr_tc, idstr);

	ret = residhash_initialize(&residhash);
	if (ret < 0) {
		printf("residhash_initialize fail\n");
		return UNITTEST_RESULT_FAIL;
	}
	ret = residhash_searchdata(&residhash, idstr_tc, idstr_len, &attr, &color);
	if (ret != RESIDHASH_SEARCHDATA_NOTFOUND) {
		printf("residhash_searchdata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	residhash_finalize(&residhash);

	return result;
}

LOCAL UNITTEST_RESULT test_residhash_3()
{
	residhash_t residhash;
	UB idstr[] = "yZXmy7Om0";
	TC idstr_tc[9];
	W ret, idstr_len = strlen(idstr);
	UW attr, attr1 = 0x01010101;
	COLOR color, color1 = 0x10FF00FF;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	sjstotcs(idstr_tc, idstr);

	ret = residhash_initialize(&residhash);
	if (ret < 0) {
		printf("residhash_initialize fail\n");
		return UNITTEST_RESULT_FAIL;
	}
	ret = residhash_adddata(&residhash, idstr_tc, idstr_len, attr1, color1);
	if (ret < 0) {
		printf("residhash_adddata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	ret = residhash_searchdata(&residhash, idstr_tc, idstr_len, &attr, &color);
	if (ret != RESIDHASH_SEARCHDATA_FOUND) {
		printf("residhash_searchdata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	if (attr != attr1) {
		printf("residhash_searchdata result fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	if (color != color1) {
		printf("residhash_searchdata result fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	residhash_finalize(&residhash);

	return result;
}

LOCAL UNITTEST_RESULT test_residhash_4()
{
	residhash_t residhash;
	UB idstr[] = "yZXmy7Om0";
	TC idstr_tc[9];
	W ret, idstr_len = strlen(idstr);
	UW attr, attr1 = 0x01010101, attr2 = 0x10101010;
	COLOR color, color1 = 0x10FF00FF, color2 = 0x1000FF00;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	sjstotcs(idstr_tc, idstr);

	ret = residhash_initialize(&residhash);
	if (ret < 0) {
		printf("residhash_initialize fail\n");
		return UNITTEST_RESULT_FAIL;
	}
	ret = residhash_adddata(&residhash, idstr_tc, idstr_len, attr1, color1);
	if (ret < 0) {
		printf("residhash_adddata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	ret = residhash_adddata(&residhash, idstr_tc, idstr_len, attr2, color2);
	if (ret < 0) {
		printf("residhash_adddata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	ret = residhash_searchdata(&residhash, idstr_tc, idstr_len, &attr, &color);
	if (ret != RESIDHASH_SEARCHDATA_FOUND) {
		printf("residhash_searchdata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	if (attr != attr2) {
		printf("residhash_searchdata result fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	if (color != color2) {
		printf("residhash_searchdata result fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	residhash_finalize(&residhash);

	return result;
}

LOCAL UNITTEST_RESULT test_residhash_5()
{
	residhash_t residhash;
	UB idstr[] = "yZXmy7Om0";
	TC idstr_tc[9];
	W ret, idstr_len = strlen(idstr);
	UW attr, attr1 = 0x01010101;
	COLOR color, color1 = 0x10FF00FF;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	sjstotcs(idstr_tc, idstr);

	ret = residhash_initialize(&residhash);
	if (ret < 0) {
		printf("residhash_initialize fail\n");
		return UNITTEST_RESULT_FAIL;
	}
	ret = residhash_adddata(&residhash, idstr_tc, idstr_len, attr1, color1);
	if (ret < 0) {
		printf("residhash_adddata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	residhash_removedata(&residhash, idstr_tc, idstr_len);
	ret = residhash_searchdata(&residhash, idstr_tc, idstr_len, &attr, &color);
	if (ret != RESIDHASH_SEARCHDATA_NOTFOUND) {
		printf("residhash_searchdata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	residhash_finalize(&residhash);

	return result;
}

LOCAL UNITTEST_RESULT test_residhash_6()
{
	residhash_t residhash;
	UB idstr[] = "yZXmy7Om0";
	TC idstr_tc[9];
	W ret, idstr_len = strlen(idstr);
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	sjstotcs(idstr_tc, idstr);

	ret = residhash_initialize(&residhash);
	if (ret < 0) {
		printf("residhash_initialize fail\n");
		return UNITTEST_RESULT_FAIL;
	}
	residhash_removedata(&residhash, idstr_tc, idstr_len);
	residhash_finalize(&residhash);

	return result;
}

LOCAL UNITTEST_RESULT test_residhash_7()
{
	residhash_t residhash;
	UB idstr1[] = "yZXmy7Om0", idstr2[] = "GCQJ44Ao";
	TC idstr1_tc[9], idstr2_tc[8];
	W ret, idstr1_len = strlen(idstr1), idstr2_len = strlen(idstr2);
	UW attr, attr1 = 0x01010101, attr2 = 0x10101010;
	COLOR color, color1 = 0x10FF00FF, color2 = 0x1000FF00;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	sjstotcs(idstr1_tc, idstr1);
	sjstotcs(idstr2_tc, idstr2);

	ret = residhash_initialize(&residhash);
	if (ret < 0) {
		printf("residhash_initialize fail\n");
		return UNITTEST_RESULT_FAIL;
	}
	ret = residhash_adddata(&residhash, idstr1_tc, idstr1_len, attr1, color1);
	if (ret < 0) {
		printf("residhash_adddata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	ret = residhash_adddata(&residhash, idstr2_tc, idstr2_len, attr2, color2);
	if (ret < 0) {
		printf("residhash_adddata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	ret = residhash_searchdata(&residhash, idstr1_tc, idstr1_len, &attr, &color);
	if (ret != RESIDHASH_SEARCHDATA_FOUND) {
		printf("residhash_searchdata 1 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	if (attr != attr1) {
		printf("residhash_searchdata 1 result fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	if (color != color1) {
		printf("residhash_searchdata 1 result fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	ret = residhash_searchdata(&residhash, idstr2_tc, idstr2_len, &attr, &color);
	if (ret != RESIDHASH_SEARCHDATA_FOUND) {
		printf("residhash_searchdata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	if (attr != attr2) {
		printf("residhash_searchdata 2 result fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	if (color != color2) {
		printf("residhash_searchdata 2 result fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	residhash_finalize(&residhash);

	return result;
}

LOCAL UNITTEST_RESULT test_residhash_8()
{
	residhash_t residhash;
	residhash_iterator_t iterator;
	TC *idstr;
	W ret, num, idstr_len;
	UW attr;
	COLOR color;
	Bool next;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	ret = residhash_initialize(&residhash);
	if (ret < 0) {
		printf("residhash_initialize fail\n");
		return UNITTEST_RESULT_FAIL;
	}


	residhash_iterator_initialize(&iterator, &residhash);
	num = 0;
	for (;;) {
		next = residhash_iterator_next(&iterator, &idstr, &idstr_len, &attr, &color);
		if (next == False) {
			break;
		}
		num++;
	}
	if (num != 0) {
		printf("residhash_iterator unexpected length\n");
		result = UNITTEST_RESULT_FAIL;
	}
	residhash_iterator_finalize(&iterator);


	residhash_finalize(&residhash);

	return result;
}

LOCAL UNITTEST_RESULT test_residhash_9()
{
	residhash_t residhash;
	residhash_iterator_t iterator;
	UB idstr1[] = "yZXmy7Om0";
	TC idstr1_tc[9];
	TC *idstr;
	W ret, num, idstr_len, idstr1_len = strlen(idstr1);
	UW attr, attr1 = 0x01010101;
	COLOR color, color1 = 0x10FF00FF;
	Bool next;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	sjstotcs(idstr1_tc, idstr1);

	ret = residhash_initialize(&residhash);
	if (ret < 0) {
		printf("residhash_initialize fail\n");
		return UNITTEST_RESULT_FAIL;
	}
	ret = residhash_adddata(&residhash, idstr1_tc, idstr1_len, attr1, color1);
	if (ret < 0) {
		printf("residhash_adddata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}


	residhash_iterator_initialize(&iterator, &residhash);
	num = 0;
	for (;;) {
		next = residhash_iterator_next(&iterator, &idstr, &idstr_len, &attr, &color);
		if (next == False) {
			break;
		}
		num++;
		if ((idstr_len == idstr1_len)
			&&(tc_strncmp(idstr, idstr1_tc, idstr1_len) == 0)) {
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
		printf("residhash_iterator unexpected length\n");
		result = UNITTEST_RESULT_FAIL;
	}
	residhash_iterator_finalize(&iterator);


	residhash_finalize(&residhash);

	return result;
}

LOCAL UNITTEST_RESULT test_residhash_10()
{
	residhash_t residhash;
	residhash_iterator_t iterator;
	UB idstr1[] = "yZXmy7Om0", idstr2[] = "GCQJ44Ao";
	TC idstr1_tc[9], idstr2_tc[8];
	TC *idstr;
	W ret, num, idstr_len, idstr1_len = strlen(idstr1), idstr2_len = strlen(idstr2);
	UW attr, attr1 = 0x01010101, attr2 = 0x10101010;
	COLOR color, color1 = 0x10FF00FF, color2 = 0x1000FF00;
	Bool next;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	sjstotcs(idstr1_tc, idstr1);
	sjstotcs(idstr2_tc, idstr2);

	ret = residhash_initialize(&residhash);
	if (ret < 0) {
		printf("residhash_initialize fail\n");
		return UNITTEST_RESULT_FAIL;
	}
	ret = residhash_adddata(&residhash, idstr1_tc, idstr1_len, attr1, color1);
	if (ret < 0) {
		printf("residhash_adddata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	ret = residhash_adddata(&residhash, idstr2_tc, idstr2_len, attr2, color2);
	if (ret < 0) {
		printf("residhash_adddata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}


	residhash_iterator_initialize(&iterator, &residhash);
	num = 0;
	for (;;) {
		next = residhash_iterator_next(&iterator, &idstr, &idstr_len, &attr, &color);
		if (next == False) {
			break;
		}
		num++;
		if ((idstr_len == idstr1_len)
			&&(tc_strncmp(idstr, idstr1_tc, idstr1_len) == 0)) {
			if (attr != attr1) {
				result = UNITTEST_RESULT_FAIL;
			}
			if (color != color1) {
				result = UNITTEST_RESULT_FAIL;
			}
		} else if ((idstr_len == idstr2_len)
				   &&(tc_strncmp(idstr, idstr2_tc, idstr2_len) == 0)) {
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
		printf("residhash_iterator unexpected length\n");
		result = UNITTEST_RESULT_FAIL;
	}
	residhash_iterator_finalize(&iterator);


	residhash_finalize(&residhash);

	return result;
}

EXPORT VOID test_residhash_main(unittest_driver_t *driver)
{
	UNITTEST_DRIVER_REGIST(driver, test_residhash_1);
	UNITTEST_DRIVER_REGIST(driver, test_residhash_2);
	UNITTEST_DRIVER_REGIST(driver, test_residhash_3);
	UNITTEST_DRIVER_REGIST(driver, test_residhash_4);
	UNITTEST_DRIVER_REGIST(driver, test_residhash_5);
	UNITTEST_DRIVER_REGIST(driver, test_residhash_6);
	UNITTEST_DRIVER_REGIST(driver, test_residhash_7);
	UNITTEST_DRIVER_REGIST(driver, test_residhash_8);
	UNITTEST_DRIVER_REGIST(driver, test_residhash_9);
	UNITTEST_DRIVER_REGIST(driver, test_residhash_10);
}
