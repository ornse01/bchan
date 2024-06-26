/*
 * test_cookiedb.c
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

#include    "cookiedb.h"

#include    <btron/btron.h>
#include	<tcode.h>
#include    <bstdio.h>
#include    <bstdlib.h>
#include    <bstring.h>
#include    <errcode.h>

#include    <unittest_driver.h>

LOCAL W test_util_gen_file(LINK *lnk)
{
	LINK lnk0;
	W fd, err;
	TC name[] = {TK_t, TNULL};

	err = get_lnk(NULL, &lnk0, F_NORM);
	if (err < 0) {
		return err;
	}
	err = cre_fil(&lnk0, name, NULL, 0, F_FLOAT);
	if (err < 0) {
		return err;
	}
	fd = err;

	*lnk = lnk0;

	return fd;
}

LOCAL W test_util_write_file(LINK *lnk, W rectype, UH subtype, UB *data, W len)
{
	W fd, err = 0;

	fd = opn_fil(lnk, F_UPDATE, NULL);
	if (fd < 0) {
		return fd;
	}
	err = fnd_rec(fd, F_TOPEND, 1 << rectype, subtype, NULL);
	if (err == ER_REC) {
		err = ins_rec(fd, NULL, 0, rectype, subtype, 0);
		if (err < 0) {
			return -1;
		}
		err = see_rec(fd, -1, 0, NULL);
		if (err < 0) {
			return -1;
		}
	} else if (err < 0) {
		cls_fil(fd);
		return err;
	}
	err = loc_rec(fd, F_LOCK);
	if (err < 0) {
		cls_fil(fd);
		return err;
	}
	trc_rec(fd, 0);
	if (data != NULL) {
		err = wri_rec(fd, -1, data, len, NULL, NULL, 0);
	}
	err = loc_rec(fd, F_UNLOCK);
	cls_fil(fd);
	return err;
}

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

LOCAL Bool test_cookiedb_checkexist(UB *cookieheader, W len, testcookie_expected_t *expected, UB **next)
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
	if (next != NULL) {
		*next = p;
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

LOCAL W test_cookiedb_make_headerstring(cookiedb_t *db, UB *host, UB *path, Bool secure, STIME time, UB **header, W *header_len)
{
	cookiedb_writeheadercontext_t *wctx;
	Bool cont;
	UB *hstr, *str;
	W hstr_len, str_len;

	wctx = cookiedb_startheaderwrite(db, host, strlen(host), path, strlen(path), secure, time);
	if (wctx == NULL) {
		return False;
	}

	hstr = malloc(sizeof(UB));
	if (hstr == NULL) {
		return -1;
	}

	hstr[0] = '\0';
	hstr_len = 0;
	for (;;) {
		cont = cookiedb_writeheadercontext_makeheader(wctx, &str, &str_len);
		if (cont == False) {
			break;
		}

		hstr = realloc(hstr, hstr_len + str_len + 1);
		if (hstr == NULL) {
			cookiedb_endheaderwrite(db, wctx);
			return -1;
		}
		memcpy(hstr + hstr_len, str, str_len);
		hstr_len += str_len;
		hstr[hstr_len] = '\0';
	}

	cookiedb_endheaderwrite(db, wctx);

	*header = hstr;
	*header_len = hstr_len;

	return 0;
}

LOCAL Bool test_cookiedb_cmp_headerstring_expecteddataaray(UB *header, W header_len, testcookie_expected_t *expected, W len)
{
	Bool ok;
	W i;

	for (i = 0; i < len; i++) {
		ok = test_cookiedb_checkexist(header, header_len, expected + i, NULL);
		if (ok == False) {
			printf("fail by search NAME=VALUE %d\n", i);
			printf("     %s\n", header);
			return False;
		}
	}

	return True;
}

LOCAL Bool test_cookiedb_cmp_headerstring_expecteddataaray_order(UB *header, W header_len, testcookie_expected_t *expected, W len)
{
	Bool ok;
	W i;
	UB *str;

	str = header;
	for (i = 0; i < len; i++) {
		ok = test_cookiedb_checkexist(str, header_len - (header - str), expected + i, &str);
		if (ok == False) {
			printf("fail by search NAME=VALUE %d\n", i);
			printf("     %s\n", header);
			return False;
		}
	}

	return True;
}

LOCAL W test_cookiedb_count_headerstring_cookies(UB *header, W header_len)
{
	W i, c;

	for (i = 0; i < header_len; i++) {
		if (header[i] == ':') {
			break;
		}
	}
	if (i == header_len) {
		return 0;
	}
	c = 0;
	for (; i < header_len; i++) {
		for (; i < header_len; i++) {
			if (header[i] == '\r') {
				break;
			}
			if (header[i] != ' ') {
				c++;
				break;
			}
		}
		for (; i < header_len; i++) {
			if (header[i] == ';') {
				break;
			}
		}
	}

	return c;
}

LOCAL Bool test_cookiedb_check_headerstring_lastCRLF(UB *header, W header_len)
{
	if (strncmp(header + header_len - 2, "\r\n", 2) != 0) {
		return False;
	}
	return True;
}

LOCAL Bool text_cookiedb_check_expecteddataarray(cookiedb_t *db, UB *host, UB *path, Bool secure, STIME time, testcookie_expected_t *expected, W len)
{
	Bool ok;
	UB *hstr;
	W hstr_len;
	W c, err;

	err = test_cookiedb_make_headerstring(db, host, path, secure, time, &hstr, &hstr_len);
	if (err < 0) {
		return False;
	}

	ok = test_cookiedb_cmp_headerstring_expecteddataaray(hstr, hstr_len, expected, len);
	if (ok == False) {
		free(hstr);
		return False;
	}

	c = test_cookiedb_count_headerstring_cookies(hstr, hstr_len);
	if (c != len) {
		printf("fail by count cookies number\n");
		printf("     %s\n", hstr);
		free(hstr);
		return False;
	}
	if (c != 0) {
		ok = test_cookiedb_check_headerstring_lastCRLF(hstr, hstr_len);
		if (ok == False) {
			printf("fail by CRLF check\n");
			free(hstr);
			return False;
		}
	}

	free(hstr);

	return True;
}

LOCAL Bool text_cookiedb_check_expecteddataarray_order(cookiedb_t *db, UB *host, UB *path, Bool secure, STIME time, testcookie_expected_t *expected, W len)
{
	Bool ok;
	UB *hstr;
	W hstr_len;
	W c, err;

	err = test_cookiedb_make_headerstring(db, host, path, secure, time, &hstr, &hstr_len);
	if (err < 0) {
		return False;
	}

	ok = test_cookiedb_cmp_headerstring_expecteddataaray_order(hstr, hstr_len, expected, len);
	if (ok == False) {
		free(hstr);
		return False;
	}

	c = test_cookiedb_count_headerstring_cookies(hstr, hstr_len);
	if (c != len) {
		printf("fail by count cookies number\n");
		printf("     %s\n", hstr);
		free(hstr);
		return False;
	}
	if (c != 0) {
		ok = test_cookiedb_check_headerstring_lastCRLF(hstr, hstr_len);
		if (ok == False) {
			printf("fail by CRLF check\n");
			free(hstr);
			return False;
		}
	}

	free(hstr);

	return True;
}

#define TEST_COOKIEDB_SAMPLE_RECTYPE 30
#define TEST_COOKIEDB_SAMPLE_SUBTYPE 1

LOCAL UNITTEST_RESULT test_cookiedb_testingseparateinput(testcookie_input_t *inputdata, W inputdata_len, STIME inputtime, UB *outputhost, UB *outputpath, Bool outputsecure, STIME outputtime, testcookie_expected_t *expecteddata, W expecteddata_len)
{
	cookiedb_t *db;
	W fd, err;
	Bool ok;
	LINK test_lnk;

	fd = test_util_gen_file(&test_lnk);
	if (fd < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	db = cookiedb_new(&test_lnk, TEST_COOKIEDB_SAMPLE_RECTYPE, TEST_COOKIEDB_SAMPLE_SUBTYPE);
	if (db == NULL) {
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}

	err = test_cookiedb_input_testdataarray(db, inputtime, inputdata, inputdata_len);
	if (err < 0) {
		cookiedb_delete(db);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	ok = text_cookiedb_check_expecteddataarray(db, outputhost, outputpath, outputsecure, outputtime, expecteddata, expecteddata_len);
	if (ok == False) {
		cookiedb_delete(db);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}

	cookiedb_delete(db);
	del_fil(NULL, &test_lnk, 0);

	return UNITTEST_RESULT_PASS;
}

LOCAL UNITTEST_RESULT test_cookiedb_testingseparateinput_order(testcookie_input_t *inputdata, W inputdata_len, STIME inputtime, UB *outputhost, UB *outputpath, Bool outputsecure, STIME outputtime, testcookie_expected_t *expecteddata, W expecteddata_len)
{
	cookiedb_t *db;
	W fd, err;
	Bool ok;
	LINK test_lnk;

	fd = test_util_gen_file(&test_lnk);
	if (fd < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	db = cookiedb_new(&test_lnk, TEST_COOKIEDB_SAMPLE_RECTYPE, TEST_COOKIEDB_SAMPLE_SUBTYPE);
	if (db == NULL) {
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}

	err = test_cookiedb_input_testdataarray(db, inputtime, inputdata, inputdata_len);
	if (err < 0) {
		cookiedb_delete(db);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	ok = text_cookiedb_check_expecteddataarray_order(db, outputhost, outputpath, outputsecure, outputtime, expecteddata, expecteddata_len);
	if (ok == False) {
		cookiedb_delete(db);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}

	cookiedb_delete(db);
	del_fil(NULL, &test_lnk, 0);

	return UNITTEST_RESULT_PASS;
}

LOCAL UNITTEST_RESULT test_cookiedb_testingseparateinput_session(testcookie_input_t *inputdata, W inputdata_len, STIME inputtime, UB *outputhost, UB *outputpath, Bool outputsecure, STIME outputtime, testcookie_expected_t *expecteddata, W expecteddata_len)
{
	cookiedb_t *db;
	W fd, err;
	Bool ok;
	LINK test_lnk;

	fd = test_util_gen_file(&test_lnk);
	if (fd < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	db = cookiedb_new(&test_lnk, TEST_COOKIEDB_SAMPLE_RECTYPE, TEST_COOKIEDB_SAMPLE_SUBTYPE);
	if (db == NULL) {
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	err = test_cookiedb_input_testdataarray(db, inputtime, inputdata, inputdata_len);
	if (err < 0) {
		cookiedb_delete(db);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	err = cookiedb_writefile(db);
	if (err < 0) {
		printf("cookiedb_writefile error\n");
		cookiedb_delete(db);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	cookiedb_delete(db);

	db = cookiedb_new(&test_lnk, TEST_COOKIEDB_SAMPLE_RECTYPE, TEST_COOKIEDB_SAMPLE_SUBTYPE);
	if (db == NULL) {
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	err = cookiedb_readfile(db);
	if (err < 0) {
		printf("cookiedb_readfile error\n");
		cookiedb_delete(db);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	ok = text_cookiedb_check_expecteddataarray(db, outputhost, outputpath, outputsecure, outputtime, expecteddata, expecteddata_len);
	if (ok == False) {
		cookiedb_delete(db);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	cookiedb_delete(db);

	del_fil(NULL, &test_lnk, 0);

	return UNITTEST_RESULT_PASS;
}

LOCAL UNITTEST_RESULT test_cookiedb_testingseparateinput_file(testcookie_input_t *inputdata, W inputdata_len, STIME inputtime, UB *filedata, W filedata_len, UB *outputhost, UB *outputpath, Bool outputsecure, STIME outputtime, testcookie_expected_t *expecteddata, W expecteddata_len)
{
	cookiedb_t *db;
	W fd, err;
	Bool ok;
	LINK test_lnk;

	fd = test_util_gen_file(&test_lnk);
	if (fd < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	db = cookiedb_new(&test_lnk, TEST_COOKIEDB_SAMPLE_RECTYPE, TEST_COOKIEDB_SAMPLE_SUBTYPE);
	if (db == NULL) {
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}

	err = test_cookiedb_input_testdataarray(db, inputtime, inputdata, inputdata_len);
	if (err < 0) {
		cookiedb_delete(db);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}

	err = cookiedb_writefile(db);
	if (err < 0) {
		printf("cookiedb_writefile error\n");
		cookiedb_delete(db);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	err = test_util_write_file(&test_lnk, TEST_COOKIEDB_SAMPLE_RECTYPE, TEST_COOKIEDB_SAMPLE_SUBTYPE, filedata, filedata_len);
	if (err < 0) {
		cookiedb_delete(db);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	err = cookiedb_readfile(db);
	if (err < 0) {
		printf("cookiedb_readfile error\n");
		cookiedb_delete(db);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}

	ok = text_cookiedb_check_expecteddataarray(db, outputhost, outputpath, outputsecure, outputtime, expecteddata, expecteddata_len);
	if (ok == False) {
		cookiedb_delete(db);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}

	cookiedb_delete(db);
	del_fil(NULL, &test_lnk, 0);

	return UNITTEST_RESULT_PASS;
}

LOCAL UNITTEST_RESULT test_cookiedb_testingseparateinput_file_clear(testcookie_input_t *inputdata, W inputdata_len, STIME inputtime, UB *filedata, W filedata_len, UB *outputhost, UB *outputpath, Bool outputsecure, STIME outputtime, testcookie_expected_t *expecteddata, W expecteddata_len)
{
	cookiedb_t *db;
	W fd, err;
	Bool ok;
	LINK test_lnk;

	fd = test_util_gen_file(&test_lnk);
	if (fd < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	db = cookiedb_new(&test_lnk, TEST_COOKIEDB_SAMPLE_RECTYPE, TEST_COOKIEDB_SAMPLE_SUBTYPE);
	if (db == NULL) {
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}

	err = test_cookiedb_input_testdataarray(db, inputtime, inputdata, inputdata_len);
	if (err < 0) {
		cookiedb_delete(db);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}

	err = cookiedb_writefile(db);
	if (err < 0) {
		printf("cookiedb_writefile error\n");
		cookiedb_delete(db);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	err = test_util_write_file(&test_lnk, TEST_COOKIEDB_SAMPLE_RECTYPE, TEST_COOKIEDB_SAMPLE_SUBTYPE, filedata, filedata_len);
	if (err < 0) {
		cookiedb_delete(db);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	err = cookiedb_readfile(db);
	if (err < 0) {
		printf("cookiedb_readfile error\n");
		cookiedb_delete(db);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	cookiedb_clearallcookie(db);

	ok = text_cookiedb_check_expecteddataarray(db, outputhost, outputpath, outputsecure, outputtime, expecteddata, expecteddata_len);
	if (ok == False) {
		cookiedb_delete(db);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}

	cookiedb_delete(db);
	del_fil(NULL, &test_lnk, 0);

	return UNITTEST_RESULT_PASS;
}

/* check origin host condition. */

LOCAL UB test_cookiedb_host1[] = ".2ch.net";
LOCAL UB test_cookiedb_name1[] = "NAME01";
LOCAL UB test_cookiedb_value1[] = "VALUE01";
LOCAL UB test_cookiedb_path1[] = "/";

LOCAL UNITTEST_RESULT test_cookiedb_1()
{
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

	return test_cookiedb_testingseparateinput(data, 1, 0x1eec16c0, test_cookiedb_host1, test_cookiedb_path1, False, 0x1eec16c0, expected, 1);
}

LOCAL UNITTEST_RESULT test_cookiedb_2()
{
	testcookie_input_t data[] = {
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"PON", /* name */
			"xxxxx.yyyyy.zzzz.ad.jp", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"HAP", /* name */
			"XYZABCD", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
		{
			"PON", /* name */
			"xxxxx.yyyyy.zzzz.ad.jp", /* value */
		},
		{
			"HAP", /* name */
			"XYZABCD", /* value */
		}
	};

	return test_cookiedb_testingseparateinput(data, 2, 0x1eec16c0, "2ch.net", "/", False, 0x1eec16c0, expected, 2);
}

LOCAL UNITTEST_RESULT test_cookiedb_3()
{
	testcookie_input_t data[] = {
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"PON", /* name */
			"xxxxx.yyyyy.zzzz.ad.jp", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"HAP", /* name */
			"XYZABCD", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
		{
			"PON", /* name */
			"xxxxx.yyyyy.zzzz.ad.jp", /* value */
		}
	};

	return test_cookiedb_testingseparateinput(data, 2, 0x1eec16c0, "2ch.net", "/", False, 0x1eec16c0, expected, 1);
}

LOCAL UNITTEST_RESULT test_cookiedb_4()
{
	testcookie_input_t data[] = {
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"PON", /* name */
			"xxxxx.yyyyy.zzzz.ad.jp", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"HAP", /* name */
			"XYZABCD", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
		{
			"HAP", /* name */
			"XYZABCD", /* value */
		},
		{
			"PON", /* name */
			"xxxxx.yyyyy.zzzz.ad.jp", /* value */
		}
	};

	return test_cookiedb_testingseparateinput(data, 2, 0x1eec16c0, "www.2ch.net", "/", False, 0x1eec16c0, expected, 2);
}

/* check path condition */

LOCAL UNITTEST_RESULT test_cookiedb_5()
{
	testcookie_input_t data[] = {
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"PON", /* name */
			"xxxxx.yyyyy.zzzz.ad.jp", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"PON", /* name */
			"XYZABCD", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
		{
			"PON", /* name */
			"XYZABCD", /* value */
		}
	};

	return test_cookiedb_testingseparateinput(data, 2, 0x1eec16c0, "www.2ch.net", "/", False, 0x1eec16c0, expected, 1);
}

LOCAL UNITTEST_RESULT test_cookiedb_6()
{
	testcookie_input_t data[] = {
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"PON", /* name */
			"xxxxx.yyyyy.zzzz.ad.jp", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"PON", /* name */
			"XYZABCD", /* value */
			NULL, /* domain */
			"/", /* path */
			False, /* secure */
			0 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
		{
			"PON", /* name */
			"XYZABCD", /* value */
		}
	};

	return test_cookiedb_testingseparateinput(data, 2, 0x1eec16c0, "www.2ch.net", "/", False, 0x1eec16c0, expected, 1);
}

LOCAL UNITTEST_RESULT test_cookiedb_7()
{
	testcookie_input_t data[] = {
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"PON", /* name */
			"xxxxx.yyyyy.zzzz.ad.jp", /* value */
			NULL, /* domain */
			"/", /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"PON", /* name */
			"XYZABCD", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
		{
			"PON", /* name */
			"XYZABCD", /* value */
		}
	};

	return test_cookiedb_testingseparateinput(data, 2, 0x1eec16c0, "www.2ch.net", "/", False, 0x1eec16c0, expected, 1);
}

LOCAL UNITTEST_RESULT test_cookiedb_8()
{
	testcookie_input_t data[] = {
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"PON", /* name */
			"xxxxx.yyyyy.zzzz.ad.jp", /* value */
			NULL, /* domain */
			"/aaa/bbb", /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"PON", /* name */
			"XYZABCD", /* value */
			NULL, /* domain */
			"/aaa/bbb", /* path */
			False, /* secure */
			0 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
		{
			"PON", /* name */
			"XYZABCD", /* value */
		}
	};

	return test_cookiedb_testingseparateinput(data, 2, 0x1eec16c0, "www.2ch.net", "/", False, 0x1eec16c0, expected, 1);
}

/* check cookie order by path */

LOCAL UNITTEST_RESULT test_cookiedb_9()
{
	testcookie_input_t data[] = {
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"PON", /* name */
			"xxxxx.yyyyy.zzzz.ad.jp", /* value */
			NULL, /* domain */
			"/", /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"HAP", /* name */
			"XYZABCD", /* value */
			NULL, /* domain */
			"/aaa/bbb/", /* path */
			False, /* secure */
			0 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
		{
			"HAP", /* name */
			"XYZABCD", /* value */
		},
		{
			"PON", /* name */
			"xxxxx.yyyyy.zzzz.ad.jp", /* value */
		}
	};

	return test_cookiedb_testingseparateinput_order(data, 2, 0x1eec16c0, "2ch.net", "/aaa/bbb/index.html", False, 0x1eec16c0, expected, 2);
}

LOCAL UNITTEST_RESULT test_cookiedb_10()
{
	testcookie_input_t data[] = {
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"PON", /* name */
			"xxxxx.yyyyy.zzzz.ad.jp", /* value */
			NULL, /* domain */
			"/aaa/bbb/", /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"HAP", /* name */
			"XYZABCD", /* value */
			NULL, /* domain */
			"/", /* path */
			False, /* secure */
			0 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
		{
			"PON", /* name */
			"xxxxx.yyyyy.zzzz.ad.jp", /* value */
		},
		{
			"HAP", /* name */
			"XYZABCD", /* value */
		}
	};

	return test_cookiedb_testingseparateinput_order(data, 2, 0x1eec16c0, "2ch.net", "/aaa/bbb/index.html", False, 0x1eec16c0, expected, 2);
}

LOCAL UNITTEST_RESULT test_cookiedb_11()
{
	testcookie_input_t data[] = {
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"PON", /* name */
			"xxxxx.yyyyy.zzzz.ad.jp", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/aaa/", /* origin_path */
			"HAP", /* name */
			"XYZABCD", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
		{
			"HAP", /* name */
			"XYZABCD", /* value */
		},
		{
			"PON", /* name */
			"xxxxx.yyyyy.zzzz.ad.jp", /* value */
		}
	};

	return test_cookiedb_testingseparateinput_order(data, 2, 0x1eec16c0, "2ch.net", "/aaa/bbb/index.html", False, 0x1eec16c0, expected, 2);
}

LOCAL UNITTEST_RESULT test_cookiedb_12()
{
	testcookie_input_t data[] = {
		{
			"2ch.net", /* origin_host */
			"/aaa/bbb/", /* origin_path */
			"PON", /* name */
			"xxxxx.yyyyy.zzzz.ad.jp", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"HAP", /* name */
			"XYZABCD", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
		{
			"PON", /* name */
			"xxxxx.yyyyy.zzzz.ad.jp", /* value */
		},
		{
			"HAP", /* name */
			"XYZABCD", /* value */
		}
	};

	return test_cookiedb_testingseparateinput_order(data, 2, 0x1eec16c0, "2ch.net", "/aaa/bbb/index.html", False, 0x1eec16c0, expected, 2);
}

/* check domain condition. */

LOCAL UNITTEST_RESULT test_cookiedb_13()
{
	testcookie_input_t data[] = {
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"AAA", /* name */
			"BBB", /* value */
			".xxx.www.2ch.net", /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"CCC", /* name */
			"DDD", /* value */
			".www.2ch.net", /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"EEE", /* name */
			"FFF", /* value */
			".2ch.net", /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
		{
			"CCC", /* name */
			"DDD", /* value */
		},
		{
			"EEE", /* name */
			"FFF", /* value */
		}
	};

	return test_cookiedb_testingseparateinput(data, 3, 0x1eec16c0, "xxx.www.2ch.net", "/", False, 0x1eec16c0, expected, 2);
}

LOCAL UNITTEST_RESULT test_cookiedb_14()
{
	testcookie_input_t data[] = {
		{
			"www.xxx.xx.jp", /* origin_host */
			"/", /* origin_path */
			"AAA", /* name */
			"BBB", /* value */
			".xxx.www.xxx.xx.jp", /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"www.xxx.xx.jp", /* origin_host */
			"/", /* origin_path */
			"CCC", /* name */
			"DDD", /* value */
			".www.xxx.xx.jp", /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"www.xxx.xx.jp", /* origin_host */
			"/", /* origin_path */
			"EEE", /* name */
			"FFF", /* value */
			".xxx.xx.jp", /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
		{
			"CCC", /* name */
			"DDD", /* value */
		},
		{
			"EEE", /* name */
			"FFF", /* value */
		}
	};

	return test_cookiedb_testingseparateinput(data, 3, 0x1eec16c0, "xxx.www.xxx.xx.jp", "/", False, 0x1eec16c0, expected, 2);
}

LOCAL UNITTEST_RESULT test_cookiedb_15()
{
	testcookie_input_t data[] = {
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"AAA", /* name */
			"BBB", /* value */
			".xxx.www.2ch.net", /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"CCC", /* name */
			"DDD", /* value */
			".www.2ch.net", /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"EEE", /* name */
			"FFF", /* value */
			".2ch.net", /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
		{
			"EEE", /* name */
			"FFF", /* value */
		}
	};

	return test_cookiedb_testingseparateinput(data, 3, 0x1eec16c0, "xxx.yyy.2ch.net", "/", False, 0x1eec16c0, expected, 1);
}

LOCAL UNITTEST_RESULT test_cookiedb_16()
{
	testcookie_input_t data[] = {
		{
			"www.xxx.xx.jp", /* origin_host */
			"/", /* origin_path */
			"AAA", /* name */
			"BBB", /* value */
			".xxx.www.xxx.xx.jp", /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"www.xxx.xx.jp", /* origin_host */
			"/", /* origin_path */
			"CCC", /* name */
			"DDD", /* value */
			".www.xxx.xx.jp", /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"www.xxx.xx.jp", /* origin_host */
			"/", /* origin_path */
			"EEE", /* name */
			"FFF", /* value */
			".xxx.xx.jp", /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
		{
			"EEE", /* name */
			"FFF", /* value */
		}
	};

	return test_cookiedb_testingseparateinput(data, 3, 0x1eec16c0, "yyy.zzz.xxx.xx.jp", "/", False, 0x1eec16c0, expected, 1);
}

LOCAL UNITTEST_RESULT test_cookiedb_17()
{
	testcookie_input_t data[] = {
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"AAA", /* name */
			"BBB", /* value */
			".xxx.www.2ch.net", /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"CCC", /* name */
			"DDD", /* value */
			".www.2ch.net", /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"EEE", /* name */
			"FFF", /* value */
			".2ch.net", /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
		{
			"CCC", /* name */
			"DDD", /* value */
		},
		{
			"EEE", /* name */
			"FFF", /* value */
		}
	};

	return test_cookiedb_testingseparateinput(data, 3, 0x1eec16c0, "www.2ch.net", "/", False, 0x1eec16c0, expected, 2);
}

LOCAL UNITTEST_RESULT test_cookiedb_18()
{
	testcookie_input_t data[] = {
		{
			"www.xxx.xx.jp", /* origin_host */
			"/", /* origin_path */
			"AAA", /* name */
			"BBB", /* value */
			".xxx.www.xxx.xx.jp", /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"www.xxx.xx.jp", /* origin_host */
			"/", /* origin_path */
			"CCC", /* name */
			"DDD", /* value */
			".www.xxx.xx.jp", /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"www.xxx.xx.jp", /* origin_host */
			"/", /* origin_path */
			"EEE", /* name */
			"FFF", /* value */
			".xxx.xx.jp", /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
		{
			"CCC", /* name */
			"DDD", /* value */
		},
		{
			"EEE", /* name */
			"FFF", /* value */
		}
	};

	return test_cookiedb_testingseparateinput(data, 3, 0x1eec16c0, "www.xxx.xx.jp", "/", False, 0x1eec16c0, expected, 2);
}

LOCAL UNITTEST_RESULT test_cookiedb_19()
{
	testcookie_input_t data[] = {
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"AAA", /* name */
			"BBB", /* value */
			".xxx.www.2ch.net", /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"CCC", /* name */
			"DDD", /* value */
			".www.2ch.net", /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"EEE", /* name */
			"FFF", /* value */
			".2ch.net", /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
		{
			"EEE", /* name */
			"FFF", /* value */
		}
	};

	return test_cookiedb_testingseparateinput(data, 3, 0x1eec16c0, "2ch.net", "/", False, 0x1eec16c0, expected, 1);
}

LOCAL UNITTEST_RESULT test_cookiedb_20()
{
	testcookie_input_t data[] = {
		{
			"www.xxx.xx.jp", /* origin_host */
			"/", /* origin_path */
			"AAA", /* name */
			"BBB", /* value */
			".xxx.www.xxx.xx.jp", /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"www.xxx.xx.jp", /* origin_host */
			"/", /* origin_path */
			"CCC", /* name */
			"DDD", /* value */
			".www.xxx.xx.jp", /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"www.xxx.xx.jp", /* origin_host */
			"/", /* origin_path */
			"EEE", /* name */
			"FFF", /* value */
			".xxx.xx.jp", /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
		{
			"EEE", /* name */
			"FFF", /* value */
		}
	};

	return test_cookiedb_testingseparateinput(data, 3, 0x1eec16c0, "xxx.xx.jp", "/", False, 0x1eec16c0, expected, 1);
}

LOCAL UNITTEST_RESULT test_cookiedb_21()
{
	testcookie_input_t data[] = {
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"AAA", /* name */
			"BBB", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"CCC", /* name */
			"DDD", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"EEE", /* name */
			"FFF", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
	};

	return test_cookiedb_testingseparateinput(data, 3, 0x1eec16c0, "yyy.zzz.xxx.xx.jp", "/", False, 0x1eec16c0, expected, 0);
}

LOCAL UNITTEST_RESULT test_cookiedb_22()
{
	testcookie_input_t data[] = {
		{
			"www.xxx.xx.jp", /* origin_host */
			"/", /* origin_path */
			"AAA", /* name */
			"BBB", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"www.xxx.xx.jp", /* origin_host */
			"/", /* origin_path */
			"CCC", /* name */
			"DDD", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"www.xxx.xx.jp", /* origin_host */
			"/", /* origin_path */
			"EEE", /* name */
			"FFF", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
	};

	return test_cookiedb_testingseparateinput(data, 3, 0x1eec16c0, "xxx.yyy.2ch.net", "/", False, 0x1eec16c0, expected, 0);
}

LOCAL UNITTEST_RESULT test_cookiedb_23()
{
	testcookie_input_t data[] = {
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"AAA", /* name */
			"BBB", /* value */
			".xxx.www.2ch.net", /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"CCC", /* name */
			"DDD", /* value */
			".www.2ch.net", /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"EEE", /* name */
			"FFF", /* value */
			".2ch.net", /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
	};

	return test_cookiedb_testingseparateinput(data, 3, 0x1eec16c0, "xxx.www.xxx.xx.jp", "/", False, 0x1eec16c0, expected, 0);
}

LOCAL UNITTEST_RESULT test_cookiedb_24()
{
	testcookie_input_t data[] = {
		{
			"www.xxx.xx.jp", /* origin_host */
			"/", /* origin_path */
			"AAA", /* name */
			"BBB", /* value */
			".xxx.www.xxx.xx.jp", /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"www.xxx.xx.jp", /* origin_host */
			"/", /* origin_path */
			"CCC", /* name */
			"DDD", /* value */
			".www.xxx.xx.jp", /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"www.xxx.xx.jp", /* origin_host */
			"/", /* origin_path */
			"EEE", /* name */
			"FFF", /* value */
			".xxx.xx.jp", /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
	};

	return test_cookiedb_testingseparateinput(data, 3, 0x1eec16c0, "xxx.www.2ch.net", "/", False, 0x1eec16c0, expected, 0);
}

LOCAL UNITTEST_RESULT test_cookiedb_25()
{
	testcookie_input_t data[] = {
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"AAA", /* name */
			"BBB", /* value */
			".xxx.www.xxx.xx.jp", /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
	};

	return test_cookiedb_testingseparateinput(data, 1, 0x1eec16c0, "www.xxx.xx.jp", "/", False, 0x1eec16c0, expected, 0);
}

LOCAL UNITTEST_RESULT test_cookiedb_26()
{
	testcookie_input_t data[] = {
		{
			"www.xxx.xx.jp", /* origin_host */
			"/", /* origin_path */
			"AAA", /* name */
			"BBB", /* value */
			".xxx.www.2ch.net", /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
	};

	return test_cookiedb_testingseparateinput(data, 1, 0x1eec16c0, "yyy.xxx.www.2ch.net", "/", False, 0x1eec16c0, expected, 0);
}

LOCAL UNITTEST_RESULT test_cookiedb_27()
{
	testcookie_input_t data[] = {
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"AAA", /* name */
			"BBB", /* value */
			".xxx.www.xxx.xx.jp", /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
	};

	return test_cookiedb_testingseparateinput(data, 1, 0x1eec16c0, "xxx.www.2ch.net", "/", False, 0x1eec16c0, expected, 0);
}

LOCAL UNITTEST_RESULT test_cookiedb_28()
{
	testcookie_input_t data[] = {
		{
			"www.xxx.xx.jp", /* origin_host */
			"/", /* origin_path */
			"AAA", /* name */
			"BBB", /* value */
			".xxx.www.2ch.net", /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
	};

	return test_cookiedb_testingseparateinput(data, 1, 0x1eec16c0, "yyy.www.xxx.xx.jp", "/", False, 0x1eec16c0, expected, 0);
}

/* expires test */

LOCAL UNITTEST_RESULT test_cookiedb_29()
{
	testcookie_input_t data[] = {
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"AAA", /* name */
			"BBB", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"CCC", /* name */
			"DDD", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0x10000000 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"EEE", /* name */
			"FFF", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0x20000000 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"GGG", /* name */
			"HHH", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0x30000000 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
		{
			"AAA", /* name */
			"BBB", /* value */
		},
		{
			"CCC", /* name */
			"DDD", /* value */
		},
		{
			"EEE", /* name */
			"FFF", /* value */
		},
		{
			"GGG", /* name */
			"HHH", /* value */
		}
	};

	return test_cookiedb_testingseparateinput(data, 4, 0x0eec16c0, "2ch.net", "/", False, 0x0eec16c0, expected, 4);
}

LOCAL UNITTEST_RESULT test_cookiedb_30()
{
	testcookie_input_t data[] = {
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"AAA", /* name */
			"BBB", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"CCC", /* name */
			"DDD", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0x10000000 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"EEE", /* name */
			"FFF", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0x20000000 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"GGG", /* name */
			"HHH", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0x30000000 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
		{
			"AAA", /* name */
			"BBB", /* value */
		},
		{
			"EEE", /* name */
			"FFF", /* value */
		},
		{
			"GGG", /* name */
			"HHH", /* value */
		}
	};

	return test_cookiedb_testingseparateinput(data, 4, 0x1eec16c0, "2ch.net", "/", False, 0x1eec16c0, expected, 3);
}

LOCAL UNITTEST_RESULT test_cookiedb_31()
{
	testcookie_input_t data[] = {
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"AAA", /* name */
			"BBB", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"CCC", /* name */
			"DDD", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0x10000000 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"EEE", /* name */
			"FFF", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0x20000000 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"GGG", /* name */
			"HHH", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0x30000000 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
		{
			"AAA", /* name */
			"BBB", /* value */
		},
		{
			"GGG", /* name */
			"HHH", /* value */
		}
	};

	return test_cookiedb_testingseparateinput(data, 4, 0x1eec16c0, "2ch.net", "/", False, 0x2eec16c0, expected, 2);
}

LOCAL UNITTEST_RESULT test_cookiedb_32()
{
	testcookie_input_t data[] = {
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"AAA", /* name */
			"BBB", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"CCC", /* name */
			"DDD", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0x10000000 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"EEE", /* name */
			"FFF", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0x20000000 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"GGG", /* name */
			"HHH", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0x30000000 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
		{
			"AAA", /* name */
			"BBB", /* value */
		}
	};

	return test_cookiedb_testingseparateinput(data, 4, 0x1eec16c0, "2ch.net", "/", False, 0x3eec16c0, expected, 1);
}

/* expires test over sessions */

LOCAL UNITTEST_RESULT test_cookiedb_33()
{
	testcookie_input_t data[] = {
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"AAA", /* name */
			"BBB", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"CCC", /* name */
			"DDD", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0x10000000 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"EEE", /* name */
			"FFF", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0x20000000 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"GGG", /* name */
			"HHH", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0x30000000 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
		{
			"CCC", /* name */
			"DDD", /* value */
		},
		{
			"EEE", /* name */
			"FFF", /* value */
		},
		{
			"GGG", /* name */
			"HHH", /* value */
		}
	};

	return test_cookiedb_testingseparateinput_session(data, 4, 0x0eec16c0, "2ch.net", "/", False, 0x0eec16c0, expected, 3);
}

LOCAL UNITTEST_RESULT test_cookiedb_34()
{
	testcookie_input_t data[] = {
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"AAA", /* name */
			"BBB", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"CCC", /* name */
			"DDD", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0x10000000 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"EEE", /* name */
			"FFF", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0x20000000 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"GGG", /* name */
			"HHH", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0x30000000 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
		{
			"EEE", /* name */
			"FFF", /* value */
		},
		{
			"GGG", /* name */
			"HHH", /* value */
		}
	};

	return test_cookiedb_testingseparateinput_session(data, 4, 0x1eec16c0, "2ch.net", "/", False, 0x1eec16c0, expected, 2);
}

LOCAL UNITTEST_RESULT test_cookiedb_35()
{
	testcookie_input_t data[] = {
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"AAA", /* name */
			"BBB", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"CCC", /* name */
			"DDD", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0x10000000 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"EEE", /* name */
			"FFF", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0x20000000 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"GGG", /* name */
			"HHH", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0x30000000 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
		{
			"GGG", /* name */
			"HHH", /* value */
		}
	};

	return test_cookiedb_testingseparateinput_session(data, 4, 0x1eec16c0, "2ch.net", "/", False, 0x2eec16c0, expected, 1);
}

LOCAL UNITTEST_RESULT test_cookiedb_36()
{
	testcookie_input_t data[] = {
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"AAA", /* name */
			"BBB", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"CCC", /* name */
			"DDD", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0x10000000 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"EEE", /* name */
			"FFF", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0x20000000 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"GGG", /* name */
			"HHH", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0x30000000 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
	};

	return test_cookiedb_testingseparateinput_session(data, 4, 0x1eec16c0, "2ch.net", "/", False, 0x3eec16c0, expected, 0);
}

/* check path condition over sessions */

LOCAL UNITTEST_RESULT test_cookiedb_37()
{
	testcookie_input_t data[] = {
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"PON", /* name */
			"xxxxx.yyyyy.zzzz.ad.jp", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0x20000000 /* expires */
		},
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"PON", /* name */
			"XYZABCD", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0x20000000 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
		{
			"PON", /* name */
			"XYZABCD", /* value */
		}
	};

	return test_cookiedb_testingseparateinput_session(data, 2, 0x1eec16c0, "www.2ch.net", "/", False, 0x1eec16c0, expected, 1);
}

LOCAL UNITTEST_RESULT test_cookiedb_38()
{
	testcookie_input_t data[] = {
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"PON", /* name */
			"xxxxx.yyyyy.zzzz.ad.jp", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0x20000000 /* expires */
		},
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"PON", /* name */
			"XYZABCD", /* value */
			NULL, /* domain */
			"/", /* path */
			False, /* secure */
			0x20000000 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
		{
			"PON", /* name */
			"XYZABCD", /* value */
		}
	};

	return test_cookiedb_testingseparateinput_session(data, 2, 0x1eec16c0, "www.2ch.net", "/", False, 0x1eec16c0, expected, 1);
}

LOCAL UNITTEST_RESULT test_cookiedb_39()
{
	testcookie_input_t data[] = {
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"PON", /* name */
			"xxxxx.yyyyy.zzzz.ad.jp", /* value */
			NULL, /* domain */
			"/", /* path */
			False, /* secure */
			0x20000000 /* expires */
		},
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"PON", /* name */
			"XYZABCD", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0x20000000 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
		{
			"PON", /* name */
			"XYZABCD", /* value */
		}
	};

	return test_cookiedb_testingseparateinput_session(data, 2, 0x1eec16c0, "www.2ch.net", "/", False, 0x1eec16c0, expected, 1);
}

LOCAL UNITTEST_RESULT test_cookiedb_40()
{
	testcookie_input_t data[] = {
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"PON", /* name */
			"xxxxx.yyyyy.zzzz.ad.jp", /* value */
			NULL, /* domain */
			"/aaa/bbb", /* path */
			False, /* secure */
			0x20000000 /* expires */
		},
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"PON", /* name */
			"XYZABCD", /* value */
			NULL, /* domain */
			"/aaa/bbb", /* path */
			False, /* secure */
			0x20000000 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
		{
			"PON", /* name */
			"XYZABCD", /* value */
		}
	};

	return test_cookiedb_testingseparateinput_session(data, 2, 0x1eec16c0, "www.2ch.net", "/", False, 0x1eec16c0, expected, 1);
}

/* check domain condition over sessions */

LOCAL UNITTEST_RESULT test_cookiedb_41()
{
	testcookie_input_t data[] = {
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"AAA", /* name */
			"BBB", /* value */
			".xxx.www.2ch.net", /* domain */
			NULL, /* path */
			False, /* secure */
			0x20000000 /* expires */
		},
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"CCC", /* name */
			"DDD", /* value */
			".www.2ch.net", /* domain */
			NULL, /* path */
			False, /* secure */
			0x20000000 /* expires */
		},
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"EEE", /* name */
			"FFF", /* value */
			".2ch.net", /* domain */
			NULL, /* path */
			False, /* secure */
			0x20000000 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
		{
			"CCC", /* name */
			"DDD", /* value */
		},
		{
			"EEE", /* name */
			"FFF", /* value */
		}
	};

	return test_cookiedb_testingseparateinput_session(data, 3, 0x1eec16c0, "xxx.www.2ch.net", "/", False, 0x1eec16c0, expected, 2);
}

LOCAL UNITTEST_RESULT test_cookiedb_42()
{
	testcookie_input_t data[] = {
		{
			"www.xxx.xx.jp", /* origin_host */
			"/", /* origin_path */
			"AAA", /* name */
			"BBB", /* value */
			".xxx.www.xxx.xx.jp", /* domain */
			NULL, /* path */
			False, /* secure */
			0x20000000 /* expires */
		},
		{
			"www.xxx.xx.jp", /* origin_host */
			"/", /* origin_path */
			"CCC", /* name */
			"DDD", /* value */
			".www.xxx.xx.jp", /* domain */
			NULL, /* path */
			False, /* secure */
			0x20000000 /* expires */
		},
		{
			"www.xxx.xx.jp", /* origin_host */
			"/", /* origin_path */
			"EEE", /* name */
			"FFF", /* value */
			".xxx.xx.jp", /* domain */
			NULL, /* path */
			False, /* secure */
			0x20000000 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
		{
			"CCC", /* name */
			"DDD", /* value */
		},
		{
			"EEE", /* name */
			"FFF", /* value */
		}
	};

	return test_cookiedb_testingseparateinput_session(data, 3, 0x1eec16c0, "xxx.www.xxx.xx.jp", "/", False, 0x1eec16c0, expected, 2);
}

LOCAL UNITTEST_RESULT test_cookiedb_43()
{
	testcookie_input_t data[] = {
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"AAA", /* name */
			"BBB", /* value */
			".xxx.www.2ch.net", /* domain */
			NULL, /* path */
			False, /* secure */
			0x20000000 /* expires */
		},
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"CCC", /* name */
			"DDD", /* value */
			".www.2ch.net", /* domain */
			NULL, /* path */
			False, /* secure */
			0x20000000 /* expires */
		},
		{
			"www.2ch.net", /* origin_host */
			"/", /* origin_path */
			"EEE", /* name */
			"FFF", /* value */
			".2ch.net", /* domain */
			NULL, /* path */
			False, /* secure */
			0x20000000 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
		{
			"EEE", /* name */
			"FFF", /* value */
		}
	};

	return test_cookiedb_testingseparateinput_session(data, 3, 0x1eec16c0, "xxx.yyy.2ch.net", "/", False, 0x1eec16c0, expected, 1);
}

LOCAL UNITTEST_RESULT test_cookiedb_44()
{
	testcookie_input_t data[] = {
		{
			"www.xxx.xx.jp", /* origin_host */
			"/", /* origin_path */
			"AAA", /* name */
			"BBB", /* value */
			".xxx.www.xxx.xx.jp", /* domain */
			NULL, /* path */
			False, /* secure */
			0x20000000 /* expires */
		},
		{
			"www.xxx.xx.jp", /* origin_host */
			"/", /* origin_path */
			"CCC", /* name */
			"DDD", /* value */
			".www.xxx.xx.jp", /* domain */
			NULL, /* path */
			False, /* secure */
			0x20000000 /* expires */
		},
		{
			"www.xxx.xx.jp", /* origin_host */
			"/", /* origin_path */
			"EEE", /* name */
			"FFF", /* value */
			".xxx.xx.jp", /* domain */
			NULL, /* path */
			False, /* secure */
			0x20000000 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
		{
			"EEE", /* name */
			"FFF", /* value */
		}
	};

	return test_cookiedb_testingseparateinput_session(data, 3, 0x1eec16c0, "yyy.zzz.xxx.xx.jp", "/", False, 0x1eec16c0, expected, 1);
}

/* expires test with cookiedb_readfile() */

LOCAL UNITTEST_RESULT test_cookiedb_45()
{
	testcookie_input_t data[] = {
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"AAA", /* name */
			"BBB", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"CCC", /* name */
			"DDD", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0x10000000 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"EEE", /* name */
			"FFF", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0x20000000 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"GGG", /* name */
			"HHH", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0x30000000 /* expires */
		},
	};
	UB filedata[] = {"2ch.net<>/<>III<>JJJ<>comment<><>1300000000<><><><>\n2ch.net<>/<>KKK<>LLL<>comment<><>1300000000<><><><>\n"};
	testcookie_expected_t expected[] = {
		{
			"AAA", /* name */
			"BBB", /* value */
		},
		{
			"III", /* name */
			"JJJ", /* value */
		},
		{
			"KKK", /* name */
			"LLL", /* value */
		}
	};

	return test_cookiedb_testingseparateinput_file(data, 4, 0x0eec16c0, filedata, strlen(filedata), "2ch.net", "/", False, 0x0eec16c0, expected, 3);
}

LOCAL UNITTEST_RESULT test_cookiedb_46()
{
	testcookie_input_t data[] = {
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"AAA", /* name */
			"BBB", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"CCC", /* name */
			"DDD", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0x10000000 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"EEE", /* name */
			"FFF", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0x20000000 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"GGG", /* name */
			"HHH", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0x30000000 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
		{
			"AAA", /* name */
			"BBB", /* value */
		},
	};

	return test_cookiedb_testingseparateinput_file(data, 4, 0x1eec16c0, NULL, 0, "2ch.net", "/", False, 0x1eec16c0, expected, 1);
}

LOCAL UNITTEST_RESULT test_cookiedb_47()
{
	testcookie_input_t data[] = {
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"AAA", /* name */
			"BBB", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"CCC", /* name */
			"DDD", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
	};
	UB filedata[] = {"2ch.net<>/<>III<>JJJ<>comment<><>1300000000<><><><>\n2ch.net<>/<>KKK<>LLL<>comment<><>1300000000<><><><>\n"};
	testcookie_expected_t expected[] = {
		{
			"AAA", /* name */
			"BBB", /* value */
		},
		{
			"CCC", /* name */
			"DDD", /* value */
		},
		{
			"III", /* name */
			"JJJ", /* value */
		},
		{
			"KKK", /* name */
			"LLL", /* value */
		}
	};

	return test_cookiedb_testingseparateinput_file(data, 2, 0x0eec16c0, filedata, strlen(filedata), "2ch.net", "/", False, 0x0eec16c0, expected, 4);
}

LOCAL UNITTEST_RESULT test_cookiedb_48()
{
	testcookie_input_t data[] = {
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"AAA", /* name */
			"BBB", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"CCC", /* name */
			"DDD", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
		{
			"AAA", /* name */
			"BBB", /* value */
		},
		{
			"CCC", /* name */
			"DDD", /* value */
		},
	};

	return test_cookiedb_testingseparateinput_file(data, 2, 0x1eec16c0, NULL, 0, "2ch.net", "/", False, 0x1eec16c0, expected, 2);
}

/* clear test */

LOCAL UNITTEST_RESULT test_cookiedb_49()
{
	testcookie_input_t data[] = {
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"AAA", /* name */
			"BBB", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"CCC", /* name */
			"DDD", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0x10000000 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"EEE", /* name */
			"FFF", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0x20000000 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"GGG", /* name */
			"HHH", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0x30000000 /* expires */
		},
	};
	UB filedata[] = {"2ch.net<>/<>III<>JJJ<>comment<><>1300000000<><><><>\n2ch.net<>/<>KKK<>LLL<>comment<><>1300000000<><><><>\n"};
	testcookie_expected_t expected[] = {
	};

	return test_cookiedb_testingseparateinput_file_clear(data, 4, 0x0eec16c0, filedata, strlen(filedata), "2ch.net", "/", False, 0x0eec16c0, expected, 0);
}

LOCAL UNITTEST_RESULT test_cookiedb_50()
{
	testcookie_input_t data[] = {
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"AAA", /* name */
			"BBB", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"CCC", /* name */
			"DDD", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0x10000000 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"EEE", /* name */
			"FFF", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0x20000000 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"GGG", /* name */
			"HHH", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0x30000000 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
	};

	return test_cookiedb_testingseparateinput_file_clear(data, 4, 0x1eec16c0, NULL, 0, "2ch.net", "/", False, 0x1eec16c0, expected, 0);
}

LOCAL UNITTEST_RESULT test_cookiedb_51()
{
	testcookie_input_t data[] = {
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"AAA", /* name */
			"BBB", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"CCC", /* name */
			"DDD", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
	};
	UB filedata[] = {"2ch.net<>/<>III<>JJJ<>comment<><>1300000000<><><><>\n2ch.net<>/<>KKK<>LLL<>comment<><>1300000000<><><><>\n"};
	testcookie_expected_t expected[] = {
	};

	return test_cookiedb_testingseparateinput_file_clear(data, 2, 0x0eec16c0, filedata, strlen(filedata), "2ch.net", "/", False, 0x0eec16c0, expected, 0);
}

LOCAL UNITTEST_RESULT test_cookiedb_52()
{
	testcookie_input_t data[] = {
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"AAA", /* name */
			"BBB", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"2ch.net", /* origin_host */
			"/", /* origin_path */
			"CCC", /* name */
			"DDD", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
	};

	return test_cookiedb_testingseparateinput_file_clear(data, 2, 0x1eec16c0, NULL, 0, "2ch.net", "/", False, 0x1eec16c0, expected, 0);
}

/* empty value test */

LOCAL UNITTEST_RESULT test_cookiedb_53()
{
	testcookie_input_t data[] = {
		{
			"0ch.net", /* origin_host */
			"/", /* origin_path */
			"PON", /* name */
			"", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
		{
			"0ch.net", /* origin_host */
			"/", /* origin_path */
			"HAP", /* name */
			NULL, /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
	};
	testcookie_expected_t expected[] = {
		{
			"PON", /* name */
			"", /* value */
		},
		{
			"HAP", /* name */
			"", /* value */
		}
	};

	return test_cookiedb_testingseparateinput(data, 2, 0x1eec16c0, "0ch.net", "/", False, 0x1eec16c0, expected, 2);
}

/* empty value test with cookiedb_readfile() */

LOCAL UNITTEST_RESULT test_cookiedb_54()
{
	testcookie_input_t data[] = {
		{
			"0ch.net", /* origin_host */
			"/", /* origin_path */
			"AAA", /* name */
			"BBB", /* value */
			NULL, /* domain */
			NULL, /* path */
			False, /* secure */
			0 /* expires */
		},
	};
	UB filedata[] = {"0ch.net<>/<>III<><>comment<><>1300000000<><><><>\n"};
	testcookie_expected_t expected[] = {
		{
			"AAA", /* name */
			"BBB", /* value */
		},
		{
			"III", /* name */
			"", /* value */
		}
	};

	return test_cookiedb_testingseparateinput_file(data, 1, 0x0eec16c0, filedata, strlen(filedata), "0ch.net", "/", False, 0x0eec16c0, expected, 2);
}

EXPORT VOID test_cookiedb_main(unittest_driver_t *driver)
{
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_1);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_2);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_3);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_4);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_5);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_6);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_7);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_8);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_9);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_10);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_11);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_12);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_13);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_14);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_15);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_16);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_17);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_18);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_19);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_20);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_21);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_22);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_23);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_24);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_25);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_26);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_27);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_28);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_29);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_30);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_31);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_32);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_33);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_34);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_35);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_36);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_37);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_38);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_39);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_40);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_41);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_42);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_43);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_44);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_45);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_46);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_47);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_48);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_49);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_50);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_51);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_52);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_53);
	UNITTEST_DRIVER_REGIST(driver, test_cookiedb_54);
}
