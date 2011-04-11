/*
 * test_cookiedb.c
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
#include    <bstdlib.h>
#include    <bstring.h>

#include    "test.h"

#include    "cookiedb.h"

struct testcookie_input_t_ {
	UB *origin_host;
	UB *origin_path;
	UB *name;
	UB *value;
	UB *domain;
	UB *path;
	Bool secure;
	STIME expires;
};
typedef struct testcookie_input_t_ testcookie_input_t;

struct testcookie_expected_t_ {
	UB *name;
	UB *value;
};
typedef struct testcookie_expected_t_ testcookie_expected_t;

LOCAL W test_cookiedb_inputdata_nohost(cookiedb_readheadercontext_t *ctx, testcookie_input_t *data)
{
	W i, err;

	if (data->name != NULL) {
		for (i = 0; i < strlen(data->name); i++) {
			err = cookiedb_readheadercontext_appendchar_attr(ctx, data->name[i]);
			if (err < 0) {
				return err;
			}
		}
	}
	if (data->value != NULL) {
		for (i = 0; i < strlen(data->value); i++) {
			err = cookiedb_readheadercontext_appendchar_name(ctx, data->value[i]);
			if (err < 0) {
				return err;
			}
		}
	}
	if (data->domain != NULL) {
		for (i = 0; i < strlen(data->domain); i++) {
			err = cookiedb_readheadercontext_appendchar_domain(ctx, data->domain[i]);
			if (err < 0) {
				return err;
			}
		}
	}
	if (data->path != NULL) {
		for (i = 0; i < strlen(data->path); i++) {
			err = cookiedb_readheadercontext_appendchar_path(ctx, data->path[i]);
			if (err < 0) {
				return err;
			}
		}
	}
	if (data->secure == True) {
		err = cookiedb_readheadercontext_setsecure(ctx);
		if (err < 0) {
			return err;
		}
	}
	if (data->expires != 0) {
		err = cookiedb_readheadercontext_setexpires(ctx, data->expires);
		if (err < 0) {
			return err;
		}
	}

	return 0;
}

LOCAL W test_cookiedb_inputdata(cookiedb_t *db, STIME time, testcookie_input_t *data)
{
	cookiedb_readheadercontext_t *ctx;
	W err;

	ctx = cookiedb_startheaderread(db, data->origin_host, strlen(data->origin_host), data->origin_path, strlen(data->origin_path), time);
	if (ctx == NULL) {
		return -1;
	}
	err = test_cookiedb_inputdata_nohost(ctx, data);
	cookiedb_endheaderread(db, ctx);
	return err;
}

LOCAL Bool test_cookiedb_checkexist(UB *cookieheader, W len, testcookie_expected_t *expected)
{
	char *p;

	p = strstr(cookieheader, expected->name);
	if (p == NULL) {
		return False;
	}
	p += strlen(expected->name);
	if (*p != '=') {
		return False;
	}
	p++;
	if (strncmp(p, expected->value, strlen(expected->value)) != 0) {
		return False;
	}
	return True;
}

LOCAL W test_cookiedb_input_testdataarray(cookiedb_t *db, STIME time, testcookie_input_t *data, W len)
{
	W i, err;
	for (i = 0; i < len; i++) {
		err = test_cookiedb_inputdata(db, time, data + i);
		if (err < 0) {
			return err;
		}
	}
	return 0;
}

LOCAL Bool text_cookiedb_check_expecteddataarray(cookiedb_t *db, UB *host, UB *path, Bool secure, STIME time, testcookie_expected_t *expected, W len)
{
	cookiedb_writeheadercontext_t *wctx;
	Bool cont, ok;
	UB *hstr, *str;
	W hstr_len, str_len;
	W i, c;

	wctx = cookiedb_startheaderwrite(db, host, strlen(host), path, strlen(path), secure, time);
	if (wctx == NULL) {
		return False;
	}

	hstr = NULL;
	hstr_len = 0;
	for (;;) {
		cont = cookiedb_writeheadercontext_makeheader(wctx, &str, &str_len);
		if (cont == False) {
			break;
		}

		hstr = realloc(hstr, hstr_len + str_len + 1);
		if (hstr == NULL) {
			cookiedb_endheaderwrite(db, wctx);
			return False;
		}
		memcpy(hstr + hstr_len, str, str_len);
		hstr_len += str_len;
		hstr[hstr_len] = '\0';
	}

	cookiedb_endheaderwrite(db, wctx);

	for (i = 0; i < len; i++) {
		ok = test_cookiedb_checkexist(hstr, hstr_len, expected + i);
		if (ok == False) {
			free(hstr);
			printf("fail by search NAME=VALUE %d\n", i);
			return False;
		}
	}

	c = 0;
	for (i = 0; i < hstr_len; i++) {
		if (hstr[i] == ';') {
			c++;
		}
	}
	if (c != len) {
		free(hstr);
		printf("fail by count cookies number\n");
		return False;
	}
	if (strncmp(hstr + hstr_len - 2, "\r\n", 2) != 0) {
		free(hstr);
		printf("fail by CRLF check\n");
		return False;
	}

	free(hstr);

	return True;
}

LOCAL UB test_cookiedb_host1[] = ".2ch.net";
LOCAL UB test_cookiedb_name1[] = "NAME01";
LOCAL UB test_cookiedb_value1[] = "VALUE01";
LOCAL UB test_cookiedb_path1[] = "/";

LOCAL TEST_RESULT test_cookiedb_1()
{
	cookiedb_t *db;
	W err;
	Bool ok;
	testcookie_input_t data[] = {
		{
			test_cookiedb_host1, /* origin_host */
			test_cookiedb_path1, /* origin_path */
			test_cookiedb_name1, /* name */
			test_cookiedb_value1, /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
		{
			test_cookiedb_name1, /* name */
			test_cookiedb_value1, /* value */
		}
	};

	db = cookiedb_new(NULL);
	if (db == NULL) {
		return TEST_RESULT_FAIL;
	}

	err = test_cookiedb_input_testdataarray(db, 0x1eec16c0, data, 1);
	if (err < 0) {
		cookiedb_delete(db);
		return TEST_RESULT_FAIL;
	}
	ok = text_cookiedb_check_expecteddataarray(db, test_cookiedb_host1, test_cookiedb_path1, False, 0x1eec16c0, expected, 1);
	if (ok == False) {
		cookiedb_delete(db);
		return TEST_RESULT_FAIL;
	}

	cookiedb_delete(db);

	return TEST_RESULT_PASS;
}

LOCAL VOID test_cookiedb_printresult(TEST_RESULT (*proc)(), B *test_name)
{
	TEST_RESULT result;

	printf("test_cookiedb: %s\n", test_name);
	printf("---------------------------------------------\n");
	result = proc();
	if (result == TEST_RESULT_PASS) {
		printf("--pass---------------------------------------\n");
	} else {
		printf("--fail---------------------------------------\n");
	}
	printf("---------------------------------------------\n");
}

EXPORT VOID test_cookiedb_main()
{
	test_cookiedb_printresult(test_cookiedb_1, "test_cookiedb_1");
}
