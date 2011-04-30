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

	c = 0;
	for (i = 0; i < header_len; i++) {
		if (header[i] == ';') {
			c++;
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
		free(hstr);
		printf("fail by count cookies number\n");
		return False;
	}
	ok = test_cookiedb_check_headerstring_lastCRLF(hstr, hstr_len);
	if (ok == False) {
		free(hstr);
		printf("fail by CRLF check\n");
		return False;
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
		free(hstr);
		printf("fail by count cookies number\n");
		return False;
	}
	ok = test_cookiedb_check_headerstring_lastCRLF(hstr, hstr_len);
	if (ok == False) {
		free(hstr);
		printf("fail by CRLF check\n");
		return False;
	}

	free(hstr);

	return True;
}

LOCAL TEST_RESULT test_cookiedb_testingseparateinput(testcookie_input_t *inputdata, W inputdata_len, STIME inputtime, UB *outputhost, UB *outputpath, Bool outputsecure, STIME outputtime, testcookie_expected_t *expecteddata, W expecteddata_len)
{
	cookiedb_t *db;
	W err;
	Bool ok;

	db = cookiedb_new(NULL);
	if (db == NULL) {
		return TEST_RESULT_FAIL;
	}

	err = test_cookiedb_input_testdataarray(db, inputtime, inputdata, inputdata_len);
	if (err < 0) {
		cookiedb_delete(db);
		return TEST_RESULT_FAIL;
	}
	ok = text_cookiedb_check_expecteddataarray(db, outputhost, outputpath, outputsecure, outputtime, expecteddata, expecteddata_len);
	if (ok == False) {
		cookiedb_delete(db);
		return TEST_RESULT_FAIL;
	}

	cookiedb_delete(db);

	return TEST_RESULT_PASS;
}

LOCAL TEST_RESULT test_cookiedb_testingseparateinput_order(testcookie_input_t *inputdata, W inputdata_len, STIME inputtime, UB *outputhost, UB *outputpath, Bool outputsecure, STIME outputtime, testcookie_expected_t *expecteddata, W expecteddata_len)
{
	cookiedb_t *db;
	W err;
	Bool ok;

	db = cookiedb_new(NULL);
	if (db == NULL) {
		return TEST_RESULT_FAIL;
	}

	err = test_cookiedb_input_testdataarray(db, inputtime, inputdata, inputdata_len);
	if (err < 0) {
		cookiedb_delete(db);
		return TEST_RESULT_FAIL;
	}
	ok = text_cookiedb_check_expecteddataarray_order(db, outputhost, outputpath, outputsecure, outputtime, expecteddata, expecteddata_len);
	if (ok == False) {
		cookiedb_delete(db);
		return TEST_RESULT_FAIL;
	}

	cookiedb_delete(db);

	return TEST_RESULT_PASS;
}

/* check origin host condition. */

LOCAL UB test_cookiedb_host1[] = ".2ch.net";
LOCAL UB test_cookiedb_name1[] = "NAME01";
LOCAL UB test_cookiedb_value1[] = "VALUE01";
LOCAL UB test_cookiedb_path1[] = "/";

LOCAL TEST_RESULT test_cookiedb_1()
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

LOCAL TEST_RESULT test_cookiedb_2()
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

LOCAL TEST_RESULT test_cookiedb_3()
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

LOCAL TEST_RESULT test_cookiedb_4()
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

LOCAL TEST_RESULT test_cookiedb_5()
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

LOCAL TEST_RESULT test_cookiedb_6()
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

LOCAL TEST_RESULT test_cookiedb_7()
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

LOCAL TEST_RESULT test_cookiedb_8()
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

LOCAL TEST_RESULT test_cookiedb_9()
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

LOCAL TEST_RESULT test_cookiedb_10()
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

LOCAL TEST_RESULT test_cookiedb_11()
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

LOCAL TEST_RESULT test_cookiedb_12()
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

LOCAL TEST_RESULT test_cookiedb_13()
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

LOCAL TEST_RESULT test_cookiedb_14()
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

LOCAL TEST_RESULT test_cookiedb_15()
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

LOCAL TEST_RESULT test_cookiedb_16()
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
	test_cookiedb_printresult(test_cookiedb_2, "test_cookiedb_2");
	test_cookiedb_printresult(test_cookiedb_3, "test_cookiedb_3");
	test_cookiedb_printresult(test_cookiedb_4, "test_cookiedb_4");
	test_cookiedb_printresult(test_cookiedb_5, "test_cookiedb_5");
	test_cookiedb_printresult(test_cookiedb_6, "test_cookiedb_6");
	test_cookiedb_printresult(test_cookiedb_7, "test_cookiedb_7");
	test_cookiedb_printresult(test_cookiedb_8, "test_cookiedb_8");
	test_cookiedb_printresult(test_cookiedb_9, "test_cookiedb_9");
	test_cookiedb_printresult(test_cookiedb_10, "test_cookiedb_10");
	test_cookiedb_printresult(test_cookiedb_11, "test_cookiedb_11");
	test_cookiedb_printresult(test_cookiedb_12, "test_cookiedb_12");
	test_cookiedb_printresult(test_cookiedb_13, "test_cookiedb_13");
	test_cookiedb_printresult(test_cookiedb_14, "test_cookiedb_14");
	test_cookiedb_printresult(test_cookiedb_15, "test_cookiedb_15");
	test_cookiedb_printresult(test_cookiedb_16, "test_cookiedb_16");
}
