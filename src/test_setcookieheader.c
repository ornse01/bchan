/*
 * test_setcookieheader.c
 *
 * Copyright (c) 2011-2015 project bchan
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

#include    "setcookieheader.h"

#include    <btron/btron.h>
#include    <bstdio.h>
#include    <bstring.h>

#include    <unittest_driver.h>

LOCAL UB test_setcookieheader_testdata_01[] = " PON=xAjpuk10.tky.hoge.co.jp; expires=Friday, 01-Jan-2016 00:00:00 GMT; path=/";
LOCAL UB test_setcookieheader_testdata_02[] = " HAP=0000000; expires=Friday, 01-Jan-2016 00:00:00 GMT; path=/; domain=.2ch.net";
LOCAL UB test_setcookieheader_testdata_03[] = " num=123456; expires=Sun, 10-Jun-2001 12:00:00 GMT; path=/HTTP/";
LOCAL UB test_setcookieheader_testdata_04[] = " param2=GHIJKL; expires=Mon, 31-Dec-2001 23:59:59 GMT; path=/; secure";
LOCAL UB test_setcookieheader_testdata_05[] = " NAME=; Expires=Friday, 01-Jan-2016 00:00:00 GMT; Path=/";

typedef struct {
	UB *attr_name_exptd;
	UB *value_name_exptd;
	UB *value_comment_exptd;
	UB *value_domain_exptd;
	UB *value_max_age_exptd;
	UB *value_path_exptd;
	UB *value_version_exptd;
	Bool issecure_exptd;
	STIME expires_exptd;

	W i_a_name;
	W i_v_name;
	W i_v_comment;
	W i_v_domain;
	W i_v_max_age;
	W i_v_path;
	W i_v_version;
	Bool b_secure;
	STIME t_v_expires;
} test_cookieresult_t;

LOCAL VOID test_cookieresult_initialize(test_cookieresult_t *result, UB *name, UB *val_name, UB *comment, UB *domain, STIME expires, UB *max_age, UB *path, UB *version, Bool secure)
{
	result->attr_name_exptd = name;
	result->value_name_exptd = val_name;
	result->value_comment_exptd = comment;
	result->value_domain_exptd = domain;
	result->value_max_age_exptd = max_age;
	result->value_path_exptd = path;
	result->value_version_exptd = version;
	result->issecure_exptd = secure;
	result->expires_exptd = expires;
	result->i_a_name = 0;
	result->i_v_name = 0;
	result->i_v_comment = 0;
	result->i_v_domain = 0;
	result->i_v_max_age = 0;
	result->i_v_path = 0;
	result->i_v_version = 0;
	result->b_secure = False;
	result->t_v_expires = 0;
}

LOCAL VOID test_cookieresult_checkstring(UB *target, W *target_len, UB *rcv, W rcv_len)
{
	W ret;

	ret = strncmp(target + *target_len, rcv, rcv_len);
	if (ret != 0) {
		return;
	}
	*target_len += rcv_len;
}

LOCAL VOID test_cookieresult_attr_name(test_cookieresult_t *result, UB *str, W len)
{
	test_cookieresult_checkstring(result->attr_name_exptd, &result->i_a_name, str, len);
}

LOCAL VOID test_cookieresult_value_name(test_cookieresult_t *result, UB *str, W len)
{
	test_cookieresult_checkstring(result->value_name_exptd, &result->i_v_name, str, len);
}

LOCAL VOID test_cookieresult_value_comment(test_cookieresult_t *result, UB *str, W len)
{
	test_cookieresult_checkstring(result->value_comment_exptd, &result->i_v_comment, str, len);
}

LOCAL VOID test_cookieresult_value_domain(test_cookieresult_t *result, UB *str, W len)
{
	test_cookieresult_checkstring(result->value_domain_exptd, &result->i_v_domain, str, len);
}

LOCAL VOID test_cookieresult_value_max_age(test_cookieresult_t *result, UB *str, W len)
{
	test_cookieresult_checkstring(result->value_max_age_exptd, &result->i_v_max_age, str, len);
}

LOCAL VOID test_cookieresult_value_path(test_cookieresult_t *result, UB *str, W len)
{
	test_cookieresult_checkstring(result->value_path_exptd, &result->i_v_path, str, len);
}

LOCAL VOID test_cookieresult_value_version(test_cookieresult_t *result, UB *str, W len)
{
	test_cookieresult_checkstring(result->value_version_exptd, &result->i_v_version, str, len);
}

LOCAL VOID test_cookieresult_secure(test_cookieresult_t *result)
{
	result->b_secure = True;
}

LOCAL VOID test_cookieresult_expires(test_cookieresult_t *result, STIME time)
{
	result->t_v_expires = time;
}

LOCAL VOID test_cookieresult_inputresult(test_cookieresult_t *result, setcookieparser_result_t *res)
{
	if (res->type == SETCOOKIEPARSER_RESULT_TYPE_NAMEATTR) {
		test_cookieresult_attr_name(result, res->val.name.str, res->val.name.len);
	} else if (res->type == SETCOOKIEPARSER_RESULT_TYPE_VALUE) {
		switch (res->val.value.attr) {
		case SETCOOKIEPARSER_ATTR_COMMENT:
			test_cookieresult_value_comment(result, res->val.name.str, res->val.name.len);
			break;
		case SETCOOKIEPARSER_ATTR_DOMAIN:
			test_cookieresult_value_domain(result, res->val.name.str, res->val.name.len);
			break;
		case SETCOOKIEPARSER_ATTR_MAX_AGE:
			test_cookieresult_value_max_age(result, res->val.name.str, res->val.name.len);
			break;
		case SETCOOKIEPARSER_ATTR_PATH:
			test_cookieresult_value_path(result, res->val.name.str, res->val.name.len);
			break;
		case SETCOOKIEPARSER_ATTR_VERSION:
			test_cookieresult_value_version(result, res->val.name.str, res->val.name.len);
			break;
		case SETCOOKIEPARSER_ATTR_NAME:
			test_cookieresult_value_name(result, res->val.name.str, res->val.name.len);
			break;
		case SETCOOKIEPARSER_ATTR_EXPIRES:
		case SETCOOKIEPARSER_ATTR_SECURE:
		default:
			printf("invalid attr value\n");
			break;
		}
	} else if (res->type == SETCOOKIEPARSER_RESULT_TYPE_SECUREATTR) {
		test_cookieresult_secure(result);
	} else if (res->type == SETCOOKIEPARSER_RESULT_TYPE_EXPIRESATTR) {
		test_cookieresult_expires(result, res->val.expires.time);
	} else {
		printf("invalid value\n");
	}
}

LOCAL VOID test_cookieresult_inputresult_array(test_cookieresult_t *result, setcookieparser_result_t *res, W len)
{
	W i;
	for (i = 0; i < len; i++) {
		test_cookieresult_inputresult(result, res + i);
	}
}

LOCAL Bool test_cookieresult_checkexpected(test_cookieresult_t *result)
{
	if (strlen(result->attr_name_exptd) != result->i_a_name) {
		printf("NAME is not expected\n");
		return False;
	}
	if (strlen(result->value_name_exptd) != result->i_v_name) {
		printf("VALUE is not expected\n");
		return False;
	}
	if (strlen(result->value_comment_exptd) != result->i_v_comment) {
		printf("comment is not expected\n");
		return False;
	}
	if (strlen(result->value_domain_exptd) != result->i_v_domain) {
		printf("domain is not expected\n");
		return False;
	}
	if (strlen(result->value_max_age_exptd) != result->i_v_max_age) {
		printf("max-age is not expected\n");
		return False;
	}
	if (strlen(result->value_path_exptd) != result->i_v_path) {
		printf("path is not expected\n");
		return False;
	}
	if (strlen(result->value_version_exptd) != result->i_v_version) {
		printf("version is not expected\n");
		return False;
	}
	if (result->issecure_exptd != result->b_secure) {
		printf("secure is not expected\n");
		return False;
	}
	if (result->expires_exptd != result->t_v_expires) {
		printf("expires is not expected\n");
		return False;
	}
	return True;
}

#if 0

LOCAL VOID print_setcookieparser_result(setcookieparser_result_t *result)
{
	W i;

	if (result->type == SETCOOKIEPARSER_RESULT_TYPE_NAMEATTR) {
		printf("result type: NAME ATTR, str = %08x, len = %d\n", (UW)result->val.name.str, result->val.name.len);
		printf("             ");
		for (i = 0; i < result->val.name.len; i++) {
			printf("%c", result->val.name.str[i]);
		}
		printf("\n");
	} else if (result->type == SETCOOKIEPARSER_RESULT_TYPE_VALUE) {
		printf("result type: VALUE, str = %08x, len = %d, attr = ", (UW)result->val.value.str, result->val.value.len);
		switch (result->val.value.attr) {
		case SETCOOKIEPARSER_ATTR_COMMENT:
			printf("COMMENT\n");
			break;
		case SETCOOKIEPARSER_ATTR_DOMAIN:
			printf("DOMAIN\n");
			break;
		case SETCOOKIEPARSER_ATTR_EXPIRES:
			printf("EXPIRES\n");
			break;
		case SETCOOKIEPARSER_ATTR_MAX_AGE:
			printf("MAX_AGE\n");
			break;
		case SETCOOKIEPARSER_ATTR_PATH:
			printf("PATH\n");
			break;
		case SETCOOKIEPARSER_ATTR_SECURE:
			printf("SECURE\n");
			break;
		case SETCOOKIEPARSER_ATTR_VERSION:
			printf("VERSION\n");
			break;
		case SETCOOKIEPARSER_ATTR_NAME:
			printf("NAME\n");
			break;
		default:
			printf("invalid attr value\n");
			break;
		}
		printf("             ");
		for (i = 0; i < result->val.name.len; i++) {
			printf("%c", result->val.name.str[i]);
		}
		printf("\n");

	} else if (result->type == SETCOOKIEPARSER_RESULT_TYPE_SECUREATTR) {
		printf("result type: SECURE ATTR\n");
	} else {
		printf("invalid type\n");
	}
}

LOCAL VOID print_setcookieparser_result_array(setcookieparser_result_t *result, W len)
{
	W i;
	for (i = 0; i < len; i++) {
		print_setcookieparser_result(result + i);
	}
}

#endif

LOCAL UNITTEST_RESULT test_setcookieheader_1()
{
	setcookieparser_t parser;
	W i,err,len,res_len;
	setcookieparser_result_t *res;
	test_cookieresult_t check;

	test_cookieresult_initialize(&check, "PON", "xAjpuk10.tky.hoge.co.jp", "", "", /*"Friday, 01-Jan-2016 00:00:00"*/0x3a4e7700, "", "/", "", False);

	err = setcookieparser_initialize(&parser);
	if (err < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	len = strlen(test_setcookieheader_testdata_01);
	for (i = 0; i < len; i++) {
		setcookieparser_inputchar(&parser, test_setcookieheader_testdata_01[i], &res, &res_len);
		test_cookieresult_inputresult_array(&check, res, res_len);
	}
	setcookieparser_endinput(&parser, &res, &res_len);
	test_cookieresult_inputresult_array(&check, res, res_len);
	setcookieparser_finalize(&parser);

	if (test_cookieresult_checkexpected(&check) == False) {
		return UNITTEST_RESULT_FAIL;
	}

	return UNITTEST_RESULT_PASS;
}

LOCAL UNITTEST_RESULT test_setcookieheader_2()
{
	setcookieparser_t parser;
	W i,err,len,res_len;
	setcookieparser_result_t *res;
	test_cookieresult_t check;

	test_cookieresult_initialize(&check, "HAP", "0000000", "", ".2ch.net", /*"Friday, 01-Jan-2016 00:00:00 GMT"*/0x3a4e7700, "", "/", "", False);

	err = setcookieparser_initialize(&parser);
	if (err < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	len = strlen(test_setcookieheader_testdata_02);
	for (i = 0; i < len; i++) {
		setcookieparser_inputchar(&parser, test_setcookieheader_testdata_02[i], &res, &res_len);
		test_cookieresult_inputresult_array(&check, res, res_len);
	}
	setcookieparser_endinput(&parser, &res, &res_len);
	test_cookieresult_inputresult_array(&check, res, res_len);
	setcookieparser_finalize(&parser);

	if (test_cookieresult_checkexpected(&check) == False) {
		return UNITTEST_RESULT_FAIL;
	}

	return UNITTEST_RESULT_PASS;
}

LOCAL UNITTEST_RESULT test_setcookieheader_3()
{
	setcookieparser_t parser;
	W i,err,len,res_len;
	setcookieparser_result_t *res;
	test_cookieresult_t check;

	test_cookieresult_initialize(&check, "num", "123456", "", "", /*"Sun, 10-Jun-2001 12:00:00 GMT"*/0x1eec16c0, "", "/HTTP/", "", False);

	err = setcookieparser_initialize(&parser);
	if (err < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	len = strlen(test_setcookieheader_testdata_03);
	for (i = 0; i < len; i++) {
		setcookieparser_inputchar(&parser, test_setcookieheader_testdata_03[i], &res, &res_len);
		test_cookieresult_inputresult_array(&check, res, res_len);
	}
	setcookieparser_endinput(&parser, &res, &res_len);
	test_cookieresult_inputresult_array(&check, res, res_len);
	setcookieparser_finalize(&parser);

	if (test_cookieresult_checkexpected(&check) == False) {
		return UNITTEST_RESULT_FAIL;
	}

	return UNITTEST_RESULT_PASS;
}

LOCAL UNITTEST_RESULT test_setcookieheader_4()
{
	setcookieparser_t parser;
	W i,err,len,res_len;
	setcookieparser_result_t *res;
	test_cookieresult_t check;

	test_cookieresult_initialize(&check, "param2", "GHIJKL", "", "", /*"Mon, 31-Dec-2001 23:59:59 GMT"*/0x1ff9b17f, "", "/", "", True);

	err = setcookieparser_initialize(&parser);
	if (err < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	len = strlen(test_setcookieheader_testdata_04);
	for (i = 0; i < len; i++) {
		setcookieparser_inputchar(&parser, test_setcookieheader_testdata_04[i], &res, &res_len);
		test_cookieresult_inputresult_array(&check, res, res_len);
	}
	setcookieparser_endinput(&parser, &res, &res_len);
	test_cookieresult_inputresult_array(&check, res, res_len);
	setcookieparser_finalize(&parser);

	if (test_cookieresult_checkexpected(&check) == False) {
		return UNITTEST_RESULT_FAIL;
	}

	return UNITTEST_RESULT_PASS;
}

LOCAL UNITTEST_RESULT test_setcookieheader_5()
{
	setcookieparser_t parser;
	W i,err,len,res_len;
	setcookieparser_result_t *res;
	test_cookieresult_t check;

	test_cookieresult_initialize(&check, "NAME", "", "", "", /*"Friday, 01-Jan-2016 00:00:00"*/0x3a4e7700, "", "/", "", False);

	err = setcookieparser_initialize(&parser);
	if (err < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	len = strlen(test_setcookieheader_testdata_05);
	for (i = 0; i < len; i++) {
		setcookieparser_inputchar(&parser, test_setcookieheader_testdata_05[i], &res, &res_len);
		test_cookieresult_inputresult_array(&check, res, res_len);
	}
	setcookieparser_endinput(&parser, &res, &res_len);
	test_cookieresult_inputresult_array(&check, res, res_len);
	setcookieparser_finalize(&parser);

	if (test_cookieresult_checkexpected(&check) == False) {
		return UNITTEST_RESULT_FAIL;
	}

	return UNITTEST_RESULT_PASS;
}

EXPORT VOID test_setcookieheader_main(unittest_driver_t *driver)
{
	UNITTEST_DRIVER_REGIST(driver, test_setcookieheader_1);
	UNITTEST_DRIVER_REGIST(driver, test_setcookieheader_2);
	UNITTEST_DRIVER_REGIST(driver, test_setcookieheader_3);
	UNITTEST_DRIVER_REGIST(driver, test_setcookieheader_4);
	UNITTEST_DRIVER_REGIST(driver, test_setcookieheader_5);
}
