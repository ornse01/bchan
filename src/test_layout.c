/*
 * test_layout.c
 *
 * Copyright (c) 2009-2010 project bchan
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
#include	<btron/dp.h>
#include	<tcode.h>
#include    <bstdio.h>
#include    <bstring.h>
#include    <bstdlib.h>

#include    "test.h"

#include    "layout.h"
#include    "parser.h"
#include    "cache.h"

LOCAL UB test_layout_testdata_01[] = {
	0x81, 0xa0, 0x81, 0xa0, 0x81, 0xa0, 0x81, 0xa0,
	0x28, 0x83, 0x6c, 0x81, 0x5b, 0x83, 0x80, 0x96,
	0xb3, 0x82, 0xb5, 0x29, 0x3c, 0x3e, 0x3c, 0x3e,
	0x32, 0x30, 0x30, 0x39, 0x2f, 0x31, 0x30, 0x2f,
	0x31, 0x36, 0x28, 0x8b, 0xe0, 0x29, 0x20, 0x32,
	0x31, 0x3a, 0x32, 0x38, 0x3a, 0x34, 0x31, 0x20,
	0x3c, 0x3e, 0x20, 0x61, 0x62, 0x63, 0x20, 0x3c,
	0x62, 0x72, 0x3e, 0x20, 0x78, 0x79, 0x7a, 0x20,
	0x3c, 0x3e, 0x82, 0xbd, 0x82, 0xa2, 0x82, 0xc6,
	0x82, 0xe9, 0x0a, 0x81, 0xa0, 0x81, 0xa0, 0x81,
	0xa0, 0x81, 0xa0, 0x28, 0x83, 0x6c, 0x81, 0x5b,
	0x83, 0x80, 0x96, 0xb3, 0x82, 0xb5, 0x29, 0x3c,
	0x3e, 0x3c, 0x3e, 0x32, 0x30, 0x30, 0x39, 0x2f,
	0x31, 0x30, 0x2f, 0x31, 0x36, 0x28, 0x8b, 0xe0,
	0x29, 0x20, 0x32, 0x32, 0x3a, 0x31, 0x39, 0x3a,
	0x34, 0x36, 0x20, 0x3c, 0x3e, 0x20, 0x3c, 0x61,
	0x20, 0x68, 0x72, 0x65, 0x66, 0x3d, 0x22, 0x2e,
	0x2e, 0x2f, 0x74, 0x65, 0x73, 0x74, 0x2f, 0x72,
	0x65, 0x61, 0x64, 0x2e, 0x63, 0x67, 0x69, 0x2f,
	0x64, 0x75, 0x6d, 0x6d, 0x79, 0x2f, 0x78, 0x78,
	0x78, 0x78, 0x78, 0x78, 0x78, 0x78, 0x78, 0x78,
	0x2f, 0x31, 0x22, 0x20, 0x74, 0x61, 0x72, 0x67,
	0x65, 0x74, 0x3d, 0x22, 0x5f, 0x62, 0x6c, 0x61,
	0x6e, 0x6b, 0x22, 0x3e, 0x26, 0x67, 0x74, 0x3b,
	0x26, 0x67, 0x74, 0x3b, 0x31, 0x3c, 0x2f, 0x61,
	0x3e, 0x20, 0x3c, 0x3e, 0x0a, 0x81, 0xa0, 0x81,
	0xa0, 0x81, 0xa0, 0x81, 0xa0, 0x28, 0x83, 0x6c,
	0x81, 0x5b, 0x83, 0x80, 0x96, 0xb3, 0x82, 0xb5,
	0x29, 0x3c, 0x3e, 0x3c, 0x3e, 0x32, 0x30, 0x30,
	0x39, 0x2f, 0x31, 0x30, 0x2f, 0x31, 0x36, 0x28,
	0x8b, 0xe0, 0x29, 0x20, 0x32, 0x32, 0x3a, 0x32,
	0x34, 0x3a, 0x35, 0x37, 0x20, 0x3c, 0x3e, 0x20,
	0x3c, 0x62, 0x72, 0x3e, 0x20, 0x20, 0x3c, 0x62,
	0x72, 0x3e, 0x20, 0x20, 0x3c, 0x62, 0x72, 0x3e,
	0x20, 0x3c, 0x3e, 0x0a, 0x81, 0xa0, 0x81, 0xa0,
	0x81, 0xa0, 0x81, 0xa0, 0x28, 0x83, 0x6c, 0x81,
	0x5b, 0x83, 0x80, 0x96, 0xb3, 0x82, 0xb5, 0x29,
	0x3c, 0x3e, 0x73, 0x61, 0x67, 0x65, 0x3c, 0x3e,
	0x32, 0x30, 0x30, 0x39, 0x2f, 0x31, 0x30, 0x2f,
	0x31, 0x36, 0x28, 0x8b, 0xe0, 0x29, 0x20, 0x32,
	0x32, 0x3a, 0x33, 0x32, 0x3a, 0x31, 0x36, 0x20,
	0x3c, 0x3e, 0x20, 0x83, 0x58, 0x83, 0x8c, 0x82,
	0xbd, 0x82, 0xc1, 0x82, 0xbd, 0x82, 0xcc, 0x82,
	0xa9, 0x82, 0x97, 0x20, 0x3c, 0x62, 0x72, 0x3e,
	0x20, 0x20, 0x3c, 0x62, 0x72, 0x3e, 0x20, 0x3c,
	0x3e, 0x0a, 0x81, 0xa0, 0x81, 0xa0, 0x81, 0xa0,
	0x81, 0xa0, 0x28, 0x83, 0x6c, 0x81, 0x5b, 0x83,
	0x80, 0x96, 0xb3, 0x82, 0xb5, 0x29, 0x3c, 0x3e,
	0x73, 0x61, 0x67, 0x65, 0x3c, 0x3e, 0x32, 0x30,
	0x30, 0x39, 0x2f, 0x31, 0x30, 0x2f, 0x31, 0x37,
	0x28, 0x93, 0x79, 0x29, 0x20, 0x30, 0x34, 0x3a,
	0x34, 0x38, 0x3a, 0x31, 0x39, 0x20, 0x3c, 0x3e,
	0x20, 0x74, 0x65, 0x73, 0x74, 0x20, 0x74, 0x65,
	0x73, 0x74, 0x20, 0x74, 0x65, 0x73, 0x74, 0x20,
	0x3c, 0x3e, 0x0a, 0x00
};

LOCAL W test_parser_util_gen_file(LINK *lnk, VID *nvid)
{
	LINK lnk0;
	W fd, err;
	TC name[] = {TK_t, TNULL};
	VOBJSEG vseg = {
		{{0,0,100,20}},
		16, 16,
		0x10000000, 0x10000000, 0x10FFFFFF, 0x10FFFFFF,
		0
	};

	err = get_lnk(NULL, &lnk0, F_NORM);
	if (err < 0) {
		return err;
	}
	err = cre_fil(&lnk0, name, NULL, 0, F_FLOAT);
	if (err < 0) {
		return err;
	}
	fd = err;
	err = oreg_vob((VLINK*)&lnk0, (VP)&vseg, -1, V_NODISP);
	if (err < 0) {
		cls_fil(fd);
		del_fil(NULL, &lnk0, 0);
		return err;
	}

	*lnk = lnk0;
	*nvid = err;

	return fd;
}

LOCAL BMP* test_layout_util_alloc_BMP()
{
	W size;
	BMP* dest;

	dest = malloc(sizeof(UW)+sizeof(UH)*2+sizeof(RECT)+sizeof(UB*)*1);
	dest->planes = 1;
	dest->pixbits = (32 << 8)|28;
	dest->rowbytes = 100*4;
	dest->bounds = (RECT){{0,0,100,100}};

	size = ((dest->bounds.c.right - dest->bounds.c.left) * (dest->pixbits >> 8) + 15) / 16 * 2 * (dest->bounds.c.bottom - dest->bounds.c.top);

	dest->baseaddr[0] = malloc(size);

	return dest;
}

LOCAL VOID test_layout_util_free_BMP(BMP *bmp)
{
	free(bmp->baseaddr[0]);
	free(bmp);
}

LOCAL TEST_RESULT test_layout_1()
{
	LINK test_lnk;
	BMP *bmp;
	GID gid;
	W fd, err;
	VID vid;
	datlayout_t *layout;
	datcache_t *cache;
	datparser_t *parser;
	datparser_res_t *res = NULL;
	TEST_RESULT result = TEST_RESULT_PASS;

	fd = test_parser_util_gen_file(&test_lnk, &vid);
	if (fd < 0) {
		return TEST_RESULT_FAIL;
	}
	cls_fil(fd);
	bmp = test_layout_util_alloc_BMP();
	if (bmp == NULL) {
		return TEST_RESULT_FAIL;
	}
	gid = gopn_mem(NULL, bmp, NULL);
	if (gid < 0) {
		return TEST_RESULT_FAIL;
	}

	cache = datcache_new(vid);
	datcache_appenddata(cache, test_layout_testdata_01, strlen(test_layout_testdata_01));

	parser = datparser_new(cache);
	layout = datlayout_new(gid);

	for (;;) {
		err = datparser_getnextres(parser, &res);
		if (err != 1) {
			break;
		}
		if (res != NULL) {
			datlayout_appendres(layout, res);
		} else {
			break;
		}
	}

	datlayout_delete(layout);
	datparser_delete(parser);

	datcache_delete(cache);

	gcls_env(gid);
	test_layout_util_free_BMP(bmp);
	err = odel_vob(vid, 0);
	if (err < 0) {
		printf("error odel_vob:%d\n", err >> 16);
		result = TEST_RESULT_FAIL;
	}
	err = del_fil(NULL, &test_lnk, 0);
	if (err < 0) {
		printf("error del_fil:%d\n", err >> 16);
		result = TEST_RESULT_FAIL;
	}

	return result;
}

/* tmp */
#include    "tadlib.h"

LOCAL UB test_layout_datepart_01[] = {
"name<>sage<>2008/12/12(X) 17:28:35<> body <>title
name<><>2008/12/19(X) 18:03:53 ID:XXXXXXX<> body <>
name<>sage<>2008/12/12(X) 20:28:38 ID:???<> body <>
name<>sage<>2008/12/13(X) 02:11:04 ID:XXXXXXX BE:999999999-DIA(20000)<> body <>
name<>sage<>2008/12/16(X) 22:08:27 ID:??? BE:999999999-DIA(20000)<> body <>
name<>sage<>2008/12/13(X) 02:11:04 BE:999999999-DIA(20000)<> body <>
name<>sage<>2008/12/16(X) 22:08:27 BE:999999999-DIA(20000)<> body <>
"
};

LOCAL TEST_RESULT test_layout_datepart_1()
{
	LINK test_lnk;
	TC *date, *id, *beid;
	W date_len, id_len, beid_len;
	W i, fd, err;
	VID vid;
	datcache_t *cache;
	datparser_t *parser;
	datparser_res_t *res = NULL;
	TEST_RESULT result = TEST_RESULT_PASS;

	fd = test_parser_util_gen_file(&test_lnk, &vid);
	if (fd < 0) {
		return TEST_RESULT_FAIL;
	}
	cls_fil(fd);

	cache = datcache_new(vid);
	datcache_appenddata(cache, test_layout_datepart_01, strlen(test_layout_datepart_01));

	parser = datparser_new(cache);

	for (i = 0;; i++) {
		err = datparser_getnextres(parser, &res);
		if (err != 1) {
			break;
		}
		if (res != NULL) {
			tadlib_separete_datepart(res->date, res->date_len, &date, &date_len, &id, &id_len, &beid, &beid_len);
			//printf("date len = %d, id len = %d, beid len = %d\n", date_len, id_len, beid_len);
			switch (i) { /* TODO: should compair result TAD. */
			case 0:
				if (date_len != 27) {
					result = TEST_RESULT_FAIL;
				}
				if (id_len != 0) {
					result = TEST_RESULT_FAIL;
				}
				if (beid_len != 0) {
					result = TEST_RESULT_FAIL;
				}
				break;
			case 1:
				if (date_len != 27) {
					result = TEST_RESULT_FAIL;
				}
				if (id_len != 10) {
					result = TEST_RESULT_FAIL;
				}
				if (beid_len != 0) {
					result = TEST_RESULT_FAIL;
				}
				break;
			case 2:
				if (date_len != 27) {
					result = TEST_RESULT_FAIL;
				}
				if (id_len != 6) {
					result = TEST_RESULT_FAIL;
				}
				if (beid_len != 0) {
					result = TEST_RESULT_FAIL;
				}
				break;
			case 3:
				if (date_len != 27) {
					result = TEST_RESULT_FAIL;
				}
				if (id_len != 10) {
					result = TEST_RESULT_FAIL;
				}
				if (beid_len != 23) {
					result = TEST_RESULT_FAIL;
				}
				break;
			case 4:
				if (date_len != 27) {
					result = TEST_RESULT_FAIL;
				}
				if (id_len != 6) {
					result = TEST_RESULT_FAIL;
				}
				if (beid_len != 23) {
					result = TEST_RESULT_FAIL;
				}
				break;
			case 5:
				if (date_len != 27) {
					result = TEST_RESULT_FAIL;
				}
				if (id_len != 0) {
					result = TEST_RESULT_FAIL;
				}
				if (beid_len != 23) {
					result = TEST_RESULT_FAIL;
				}
				break;
			case 6:
				if (date_len != 27) {
					result = TEST_RESULT_FAIL;
				}
				if (id_len != 0) {
					result = TEST_RESULT_FAIL;
				}
				if (beid_len != 23) {
					result = TEST_RESULT_FAIL;
				}
				break;
			default:
				result = TEST_RESULT_FAIL;
				break;
			}
		} else {
			break;
		}
	}

	datparser_delete(parser);

	datcache_delete(cache);

	err = odel_vob(vid, 0);
	if (err < 0) {
		printf("error odel_vob:%d\n", err >> 16);
		result = TEST_RESULT_FAIL;
	}
	err = del_fil(NULL, &test_lnk, 0);
	if (err < 0) {
		printf("error del_fil:%d\n", err >> 16);
		result = TEST_RESULT_FAIL;
	}

	return result;
}

LOCAL VOID test_layout_printresult(TEST_RESULT (*proc)(), B *test_name)
{
	TEST_RESULT result;

	printf("test_layout: %s\n", test_name);
	printf("---------------------------------------------\n");
	result = proc();
	if (result == TEST_RESULT_PASS) {
		printf("--pass---------------------------------------\n");
	} else {
		printf("--fail---------------------------------------\n");
	}
	printf("---------------------------------------------\n");
}

EXPORT VOID test_layout_main()
{
	test_layout_printresult(test_layout_1, "test_layout_1");
	test_layout_printresult(test_layout_datepart_1, "test_layout_datepart_1");
}
