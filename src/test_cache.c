/*
 * test_cache.c
 *
 * Copyright (c) 2009-2015 project bchan
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

#include    "cache.h"

#include    <btron/btron.h>
#include	<tcode.h>
#include    <bstdio.h>
#include    <bstring.h>
#include    <bstdlib.h>
#include    <tstring.h>
#include	<errcode.h>

#include    <unittest_driver.h>

LOCAL UB test_cache_testdata_01[] = {"aaaaabbbbbcccccddddd"};
LOCAL UB test_cache_testdata_01_1[] = {"aaaaa"};
LOCAL UB test_cache_testdata_01_2[] = {"bbbbb"};
LOCAL UB test_cache_testdata_01_3[] = {"ccccc"};
LOCAL UB test_cache_testdata_01_4[] = {"ddddd"};
LOCAL UB test_cache_testdata_02[] = {"XXX abcdef\r\nAAAA: valueA\r\nBBBB: valueB\r\n\r\n"};
LOCAL UB test_cache_testdata_03[] = {"aaa.bbb.ccc.jp\nthread\nAAAAAAAAA\n"};
LOCAL UB test_cache_testdata_04[] = {"aaa.bbb.ccc.jp"};
LOCAL UB test_cache_testdata_05[] = {"thread"};
LOCAL UB test_cache_testdata_06[] = {"AAAAAAAAA"};
LOCAL UB test_cache_testdata_07[] = {"XXX abcdef\r\nAAAA: valueC\r\nBBBB: valueC\r\n\r\n"};
LOCAL UB test_cache_testdata_08[] = {"aaa.bbb.ccc.jp:15000\nthread\nAAAAAAAAA\n"};

LOCAL W test_cache_util_gen_file(LINK *lnk, VID *nvid)
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
		cls_fil(err);
		del_fil(NULL, &lnk0, 0);
		return err;
	}

	*lnk = lnk0;
	*nvid = err;

	return fd;
}

LOCAL Bool test_cache_util_cmp_ctx_str(datcache_datareadcontext_t *context, UB *data, W len)
{
	Bool cont;
	UB *bin_cache;
	W i = 0, len_cache, cmp;

	for (;;) {
		cont = datcache_datareadcontext_nextdata(context, &bin_cache, &len_cache);
		if (cont == False) {
			break;
		}
		cmp = memcmp(data + i, bin_cache, len_cache);
		if (cmp != 0) {
			return False;
		}
		i += len_cache;
		if (i > len) {
			return False;
		}
	}

	if (i != len) {
		return False;
	}

	return True;
}

LOCAL Bool test_cache_util_cmp_rec_bin(LINK *lnk, W rectype, W subtype, UB *bin, W len)
{
	W fd, err, r_len, cmp;
	UB *r_data;

	fd = opn_fil(lnk, F_READ, NULL);
	if (fd < 0) {
		return False;
	}

	err = fnd_rec(fd, F_TOPEND, 1 << rectype, subtype, NULL);
	if (err < 0) {
		cls_fil(fd);
		return False;
	}

	err = rea_rec(fd, 0, NULL, 0, &r_len, NULL);
	if (err < 0) {
		cls_fil(fd);
		return False;
	}

	if (len != r_len) {
		cls_fil(fd);
		return False;
	}

	r_data = malloc(r_len);
	if (r_data == NULL) {
		cls_fil(fd);
		return False;
	}

	err = rea_rec(fd, 0, r_data, r_len, NULL, NULL);
	if (err < 0) {
		free(r_data);
		cls_fil(fd);
		return False;
	}

	cmp = memcmp(r_data, bin, r_len);

	free(r_data);
	cls_fil(fd);

	if (cmp != 0) {
		return False;
	}

	return True;
}

/* test_cache_1 */

LOCAL UNITTEST_RESULT test_cache_1_testseq(VID vid, LINK *lnk)
{
	datcache_t *cache;
	datcache_datareadcontext_t *context;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;
	Bool ok;

	cache = datcache_new(vid);

	context = datcache_startdataread(cache, 0);
	if (context == NULL) {
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}
	ok = test_cache_util_cmp_ctx_str(context, test_cache_testdata_01, strlen(test_cache_testdata_01));
	if (ok == False) {
		result = UNITTEST_RESULT_FAIL;
	}
	datcache_enddataread(cache, context);

	datcache_delete(cache);

	return result;
}

LOCAL UNITTEST_RESULT test_cache_1()
{
	LINK test_lnk;
	W fd, err;
	VID vid;
	UNITTEST_RESULT result;

	fd = test_cache_util_gen_file(&test_lnk, &vid);
	if (fd < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	err = ins_rec(fd, test_cache_testdata_01, strlen(test_cache_testdata_01), DATCACHE_RECORDTYPE_MAIN, 0, 0);
	if (err < 0) {
		cls_fil(fd);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	err = ins_rec(fd, test_cache_testdata_02, strlen(test_cache_testdata_02), DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_HEADER, 0);
	if (err < 0) {
		cls_fil(fd);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	err = ins_rec(fd, test_cache_testdata_03, strlen(test_cache_testdata_03), DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_RETRIEVE, 0);
	if (err < 0) {
		cls_fil(fd);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	result = test_cache_1_testseq(vid, &test_lnk);

	err = odel_vob(vid, 0);
	if (err < 0) {
		printf("error odel_vob:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}
	err = del_fil(NULL, &test_lnk, 0);
	if (err < 0) {
		printf("error del_fil:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}

	return result;
}

/* test_cache_2 */

LOCAL UNITTEST_RESULT test_cache_2_testseq(VID vid, LINK *lnk)
{
	W err;
	datcache_t *cache;
	datcache_datareadcontext_t *context;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;
	Bool ok;

	cache = datcache_new(vid);

	err = datcache_appenddata(cache, test_cache_testdata_01_2, strlen(test_cache_testdata_01_2));
	if (err < 0) {
		printf("datcache_appenddata error 1\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}
	err = datcache_appenddata(cache, test_cache_testdata_01_3, strlen(test_cache_testdata_01_3));
	if (err < 0) {
		printf("datcache_appenddata error 2\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}
	err = datcache_appenddata(cache, test_cache_testdata_01_4, strlen(test_cache_testdata_01_4));
	if (err < 0) {
		printf("datcache_appenddata error 3\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}

	context = datcache_startdataread(cache, 0);
	if (context == NULL) {
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}
	ok = test_cache_util_cmp_ctx_str(context, test_cache_testdata_01, strlen(test_cache_testdata_01));
	if (ok == False) {
		result = UNITTEST_RESULT_FAIL;
	}
	datcache_enddataread(cache, context);

	datcache_delete(cache);

	return result;
}

LOCAL UNITTEST_RESULT test_cache_2()
{
	LINK test_lnk;
	W fd, err;
	VID vid;
	UNITTEST_RESULT result;

	fd = test_cache_util_gen_file(&test_lnk, &vid);
	if (fd < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	err = ins_rec(fd, test_cache_testdata_01_1, strlen(test_cache_testdata_01_1), DATCACHE_RECORDTYPE_MAIN, 0, 0);
	if (err < 0) {
		cls_fil(fd);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	err = ins_rec(fd, test_cache_testdata_02, strlen(test_cache_testdata_02), DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_HEADER, 0);
	if (err < 0) {
		cls_fil(fd);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	err = ins_rec(fd, test_cache_testdata_03, strlen(test_cache_testdata_03), DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_RETRIEVE, 0);
	if (err < 0) {
		cls_fil(fd);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	result = test_cache_2_testseq(vid, &test_lnk);

	err = odel_vob(vid, 0);
	if (err < 0) {
		printf("error odel_vob:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}
	err = del_fil(NULL, &test_lnk, 0);
	if (err < 0) {
		printf("error del_fil:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}

	return result;
}

/* test_cache_3 */

LOCAL UNITTEST_RESULT test_cache_3_testseq(VID vid, LINK *lnk)
{
	datcache_t *cache;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;
	UB *header;
	W header_len, cmp;

	cache = datcache_new(vid);

	datcache_getlatestheader(cache, &header, &header_len);
	if (header_len != strlen(test_cache_testdata_02)) {
		printf("datcache_getlatestheader: length error\n");
		result = UNITTEST_RESULT_FAIL;
	}
	cmp = memcmp(header, test_cache_testdata_02, header_len);
	if (cmp != 0) {
		printf("datcache_getlatestheader: data error\n");
		result = UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	return result;
}

LOCAL UNITTEST_RESULT test_cache_3()
{
	LINK test_lnk;
	W fd, err;
	VID vid;
	UNITTEST_RESULT result;

	fd = test_cache_util_gen_file(&test_lnk, &vid);
	if (fd < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	err = ins_rec(fd, test_cache_testdata_01, strlen(test_cache_testdata_01), DATCACHE_RECORDTYPE_MAIN, 0, 0);
	if (err < 0) {
		cls_fil(fd);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	err = ins_rec(fd, test_cache_testdata_02, strlen(test_cache_testdata_02), DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_HEADER, 0);
	if (err < 0) {
		cls_fil(fd);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	err = ins_rec(fd, test_cache_testdata_03, strlen(test_cache_testdata_03), DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_RETRIEVE, 0);
	if (err < 0) {
		cls_fil(fd);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	result = test_cache_3_testseq(vid, &test_lnk);

	err = odel_vob(vid, 0);
	if (err < 0) {
		printf("error odel_vob:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}
	err = del_fil(NULL, &test_lnk, 0);
	if (err < 0) {
		printf("error del_fil:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}

	return result;
}

/* test_cache_4 */

LOCAL UNITTEST_RESULT test_cache_4_testseq(VID vid, LINK *lnk)
{
	datcache_t *cache;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;
	UB *header;
	W header_len;

	cache = datcache_new(vid);

	datcache_getlatestheader(cache, &header, &header_len);
	if (header_len != 0) {
		printf("datcache_getlatestheader: length error\n");
		result = UNITTEST_RESULT_FAIL;
	}
	if (header != NULL) {
		printf("datcache_getlatestheader: data error\n");
		result = UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	return result;
}

LOCAL UNITTEST_RESULT test_cache_4()
{
	LINK test_lnk;
	W fd, err;
	VID vid;
	UNITTEST_RESULT result;

	fd = test_cache_util_gen_file(&test_lnk, &vid);
	if (fd < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	err = ins_rec(fd, test_cache_testdata_01, strlen(test_cache_testdata_01), DATCACHE_RECORDTYPE_MAIN, 0, 0);
	if (err < 0) {
		cls_fil(fd);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	err = ins_rec(fd, test_cache_testdata_03, strlen(test_cache_testdata_03), DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_RETRIEVE, 0);
	if (err < 0) {
		cls_fil(fd);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	result = test_cache_4_testseq(vid, &test_lnk);

	err = odel_vob(vid, 0);
	if (err < 0) {
		printf("error odel_vob:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}
	err = del_fil(NULL, &test_lnk, 0);
	if (err < 0) {
		printf("error del_fil:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}

	return result;
}

/* test_cache_5 */

LOCAL UNITTEST_RESULT test_cache_5_testseq(VID vid, LINK *lnk)
{
	datcache_t *cache;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;
	UB *header;
	W err, header_len, cmp;

	cache = datcache_new(vid);

	err = datcache_updatelatestheader(cache, test_cache_testdata_07, strlen(test_cache_testdata_07));
	if (err < 0) {
		printf("datcache_updatelataestheade error\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}

	datcache_getlatestheader(cache, &header, &header_len);
	if (header_len != strlen(test_cache_testdata_07)) {
		printf("datcache_getlatestheader: length error\n");
		result = UNITTEST_RESULT_FAIL;
	}
	cmp = memcmp(header, test_cache_testdata_07, header_len);
	if (cmp != 0) {
		printf("datcache_getlatestheader: data error\n");
		result = UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	return result;
}

LOCAL UNITTEST_RESULT test_cache_5()
{
	LINK test_lnk;
	W fd, err;
	VID vid;
	UNITTEST_RESULT result;

	fd = test_cache_util_gen_file(&test_lnk, &vid);
	if (fd < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	err = ins_rec(fd, test_cache_testdata_01, strlen(test_cache_testdata_01), DATCACHE_RECORDTYPE_MAIN, 0, 0);
	if (err < 0) {
		cls_fil(fd);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	err = ins_rec(fd, test_cache_testdata_02, strlen(test_cache_testdata_02), DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_HEADER, 0);
	if (err < 0) {
		cls_fil(fd);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	err = ins_rec(fd, test_cache_testdata_03, strlen(test_cache_testdata_03), DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_RETRIEVE, 0);
	if (err < 0) {
		cls_fil(fd);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	result = test_cache_5_testseq(vid, &test_lnk);

	err = odel_vob(vid, 0);
	if (err < 0) {
		printf("error odel_vob:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}
	err = del_fil(NULL, &test_lnk, 0);
	if (err < 0) {
		printf("error del_fil:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}

	return result;
}

/* test_cache_6 */

LOCAL UNITTEST_RESULT test_cache_6_testseq(VID vid, LINK *lnk)
{
	datcache_t *cache;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;
	UB *header;
	W err, header_len, cmp;

	cache = datcache_new(vid);

	err = datcache_updatelatestheader(cache, test_cache_testdata_07, strlen(test_cache_testdata_07));
	if (err < 0) {
		printf("datcache_updatelataestheade error\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}

	datcache_getlatestheader(cache, &header, &header_len);
	if (header_len != strlen(test_cache_testdata_07)) {
		printf("datcache_getlatestheader: length error\n");
		result = UNITTEST_RESULT_FAIL;
	}
	cmp = memcmp(header, test_cache_testdata_07, header_len);
	if (cmp != 0) {
		printf("datcache_getlatestheader: data error\n");
		result = UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	return result;
}

LOCAL UNITTEST_RESULT test_cache_6()
{
	LINK test_lnk;
	W fd, err;
	VID vid;
	UNITTEST_RESULT result;

	fd = test_cache_util_gen_file(&test_lnk, &vid);
	if (fd < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	err = ins_rec(fd, test_cache_testdata_01, strlen(test_cache_testdata_01), DATCACHE_RECORDTYPE_MAIN, 0, 0);
	if (err < 0) {
		cls_fil(fd);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	err = ins_rec(fd, test_cache_testdata_03, strlen(test_cache_testdata_03), DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_RETRIEVE, 0);
	if (err < 0) {
		cls_fil(fd);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	result = test_cache_6_testseq(vid, &test_lnk);

	err = odel_vob(vid, 0);
	if (err < 0) {
		printf("error odel_vob:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}
	err = del_fil(NULL, &test_lnk, 0);
	if (err < 0) {
		printf("error del_fil:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}

	return result;
}

/* test_cache_7 */

LOCAL UNITTEST_RESULT test_cache_7_testseq(VID vid, LINK *lnk)
{
	datcache_t *cache;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;
	UB *host, *borad, *thread;
	W cmp, host_len, borad_len, thread_len;
	UH port;

	cache = datcache_new(vid);

	datcache_gethost(cache, &host, &host_len);
	if (host_len != strlen(test_cache_testdata_04)) {
		printf("datcache_gethost: length error\n");
		result = UNITTEST_RESULT_FAIL;
	}
	cmp = memcmp(host, test_cache_testdata_04, host_len);
	if (cmp != 0) {
		printf("datcache_gethost: data error\n");
		result = UNITTEST_RESULT_FAIL;
	}

	datcache_getport(cache, &port);
	if (port != 80) {
		printf("datcache_getport: data error\n");
		result = UNITTEST_RESULT_FAIL;
	}

	datcache_getborad(cache, &borad, &borad_len);
	if (borad_len != strlen(test_cache_testdata_05)) {
		printf("datcache_getboard: length error\n");
		result = UNITTEST_RESULT_FAIL;
	}
	cmp = memcmp(borad, test_cache_testdata_05, borad_len);
	if (cmp != 0) {
		printf("datcache_getboard: data error\n");
		result = UNITTEST_RESULT_FAIL;
	}

	datcache_getthread(cache, &thread, &thread_len);
	if (thread_len != strlen(test_cache_testdata_06)) {
		printf("datcache_getthread: length error\n");
		result = UNITTEST_RESULT_FAIL;
	}
	cmp = memcmp(thread, test_cache_testdata_06, thread_len);
	if (cmp != 0) {
		printf("datcache_getthread: data error\n");
		result = UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	return result;
}

LOCAL UNITTEST_RESULT test_cache_7()
{
	LINK test_lnk;
	W fd, err;
	VID vid;
	UNITTEST_RESULT result;

	fd = test_cache_util_gen_file(&test_lnk, &vid);
	if (fd < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	err = ins_rec(fd, test_cache_testdata_01, strlen(test_cache_testdata_01), DATCACHE_RECORDTYPE_MAIN, 0, 0);
	if (err < 0) {
		cls_fil(fd);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	err = ins_rec(fd, test_cache_testdata_02, strlen(test_cache_testdata_02), DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_HEADER, 0);
	if (err < 0) {
		cls_fil(fd);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	err = ins_rec(fd, test_cache_testdata_03, strlen(test_cache_testdata_03), DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_RETRIEVE, 0);
	if (err < 0) {
		cls_fil(fd);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	result = test_cache_7_testseq(vid, &test_lnk);

	err = odel_vob(vid, 0);
	if (err < 0) {
		printf("error odel_vob:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}
	err = del_fil(NULL, &test_lnk, 0);
	if (err < 0) {
		printf("error del_fil:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}

	return result;
}

/* test_cache_8 */

LOCAL UNITTEST_RESULT test_cache_8_testseq(VID vid, LINK *lnk)
{
	datcache_t *cache;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;
	UB *host, *borad, *thread;
	W host_len, borad_len, thread_len;
	UH port;

	cache = datcache_new(vid);

	datcache_gethost(cache, &host, &host_len);
	if (host_len != 0) {
		printf("datcache_gethost: length error\n");
		result = UNITTEST_RESULT_FAIL;
	}
	if (host != NULL) {
		printf("datcache_gethost: data error\n");
		result = UNITTEST_RESULT_FAIL;
	}

	datcache_getport(cache, &port);
	if (port != 80) {
		printf("datcache_getport: data error\n");
		result = UNITTEST_RESULT_FAIL;
	}

	datcache_getborad(cache, &borad, &borad_len);
	if (borad_len != 0) {
		printf("datcache_getboard: length error\n");
		result = UNITTEST_RESULT_FAIL;
	}
	if (borad != NULL) {
		printf("datcache_getboard: data error\n");
		result = UNITTEST_RESULT_FAIL;
	}

	datcache_getthread(cache, &thread, &thread_len);
	if (thread_len != 0) {
		printf("datcache_getthread: length error\n");
		result = UNITTEST_RESULT_FAIL;
	}
	if (thread != NULL) {
		printf("datcache_getthread: data error\n");
		result = UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	return result;
}

LOCAL UNITTEST_RESULT test_cache_8()
{
	LINK test_lnk;
	W fd, err;
	VID vid;
	UNITTEST_RESULT result;

	fd = test_cache_util_gen_file(&test_lnk, &vid);
	if (fd < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	err = ins_rec(fd, test_cache_testdata_01, strlen(test_cache_testdata_01), DATCACHE_RECORDTYPE_MAIN, 0, 0);
	if (err < 0) {
		cls_fil(fd);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	err = ins_rec(fd, test_cache_testdata_02, strlen(test_cache_testdata_02), DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_HEADER, 0);
	if (err < 0) {
		cls_fil(fd);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	result = test_cache_8_testseq(vid, &test_lnk);

	err = odel_vob(vid, 0);
	if (err < 0) {
		printf("error odel_vob:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}
	err = del_fil(NULL, &test_lnk, 0);
	if (err < 0) {
		printf("error del_fil:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}

	return result;
}

/* test_cache_9 */

LOCAL UNITTEST_RESULT test_cache_9()
{
	LINK test_lnk;
	W fd, err;
	VID vid;
	datcache_t *cache;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;
	Bool ok;

	fd = test_cache_util_gen_file(&test_lnk, &vid);
	if (fd < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	err = ins_rec(fd, test_cache_testdata_02, strlen(test_cache_testdata_02), DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_HEADER, 0);
	if (err < 0) {
		cls_fil(fd);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	err = ins_rec(fd, test_cache_testdata_03, strlen(test_cache_testdata_03), DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_RETRIEVE, 0);
	if (err < 0) {
		cls_fil(fd);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	cache = datcache_new(vid);

	err = datcache_appenddata(cache, test_cache_testdata_01_1, strlen(test_cache_testdata_01_1));
	if (err < 0) {
		printf("datcache_appenddata error 1\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}
	err = datcache_appenddata(cache, test_cache_testdata_01_2, strlen(test_cache_testdata_01_2));
	if (err < 0) {
		printf("datcache_appenddata error 2\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}
	err = datcache_appenddata(cache, test_cache_testdata_01_3, strlen(test_cache_testdata_01_3));
	if (err < 0) {
		printf("datcache_appenddata error 3\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}
	err = datcache_appenddata(cache, test_cache_testdata_01_4, strlen(test_cache_testdata_01_4));
	if (err < 0) {
		printf("datcache_appenddata error 4\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}

	err = datcache_writefile(cache);
	if (err < 0) {
		printf("datcache_writefile error\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	ok = test_cache_util_cmp_rec_bin(&test_lnk, DATCACHE_RECORDTYPE_MAIN, 0, test_cache_testdata_01, strlen(test_cache_testdata_01));
	if (ok == False) {
		printf("main data error\n");
		result = UNITTEST_RESULT_FAIL;
	}
	ok = test_cache_util_cmp_rec_bin(&test_lnk, DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_HEADER, test_cache_testdata_02, strlen(test_cache_testdata_02));
	if (ok == False) {
		printf("info data error\n");
		result = UNITTEST_RESULT_FAIL;
	}

	err = odel_vob(vid, 0);
	if (err < 0) {
		printf("error odel_vob:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}
	err = del_fil(NULL, &test_lnk, 0);
	if (err < 0) {
		printf("error del_fil:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}

	return result;
}

/* test_cache_10 */

LOCAL UNITTEST_RESULT test_cache_10()
{
	LINK test_lnk;
	W fd, err;
	VID vid;
	datcache_t *cache;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;
	Bool ok;

	fd = test_cache_util_gen_file(&test_lnk, &vid);
	if (fd < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	err = ins_rec(fd, test_cache_testdata_01_1, strlen(test_cache_testdata_01_1), DATCACHE_RECORDTYPE_MAIN, 0, 0);
	if (err < 0) {
		cls_fil(fd);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	err = ins_rec(fd, test_cache_testdata_02, strlen(test_cache_testdata_02), DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_HEADER, 0);
	if (err < 0) {
		cls_fil(fd);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	err = ins_rec(fd, test_cache_testdata_03, strlen(test_cache_testdata_03), DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_RETRIEVE, 0);
	if (err < 0) {
		cls_fil(fd);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	cache = datcache_new(vid);

	err = datcache_appenddata(cache, test_cache_testdata_01_2, strlen(test_cache_testdata_01_2));
	if (err < 0) {
		printf("datcache_appenddata error 2\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}
	err = datcache_appenddata(cache, test_cache_testdata_01_3, strlen(test_cache_testdata_01_3));
	if (err < 0) {
		printf("datcache_appenddata error 3\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}
	err = datcache_appenddata(cache, test_cache_testdata_01_4, strlen(test_cache_testdata_01_4));
	if (err < 0) {
		printf("datcache_appenddata error 4\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}

	err = datcache_writefile(cache);
	if (err < 0) {
		printf("datcache_writefile error\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	ok = test_cache_util_cmp_rec_bin(&test_lnk, DATCACHE_RECORDTYPE_MAIN, 0, test_cache_testdata_01, strlen(test_cache_testdata_01));
	if (ok == False) {
		printf("main data error\n");
		result = UNITTEST_RESULT_FAIL;
	}
	ok = test_cache_util_cmp_rec_bin(&test_lnk, DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_HEADER, test_cache_testdata_02, strlen(test_cache_testdata_02));
	if (ok == False) {
		printf("info data error\n");
		result = UNITTEST_RESULT_FAIL;
	}

	err = odel_vob(vid, 0);
	if (err < 0) {
		printf("error odel_vob:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}
	err = del_fil(NULL, &test_lnk, 0);
	if (err < 0) {
		printf("error del_fil:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}

	return result;
}

/* test_cache_11 */

LOCAL UNITTEST_RESULT test_cache_11()
{
	LINK test_lnk;
	W fd, err;
	VID vid;
	datcache_t *cache;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;
	Bool ok;

	fd = test_cache_util_gen_file(&test_lnk, &vid);
	if (fd < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	err = ins_rec(fd, test_cache_testdata_01, strlen(test_cache_testdata_01), DATCACHE_RECORDTYPE_MAIN, 0, 0);
	if (err < 0) {
		cls_fil(fd);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	err = ins_rec(fd, test_cache_testdata_02, strlen(test_cache_testdata_02), DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_HEADER, 0);
	if (err < 0) {
		cls_fil(fd);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	err = ins_rec(fd, test_cache_testdata_03, strlen(test_cache_testdata_03), DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_RETRIEVE, 0);
	if (err < 0) {
		cls_fil(fd);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	cache = datcache_new(vid);

	err = datcache_updatelatestheader(cache, test_cache_testdata_07, strlen(test_cache_testdata_07));
	if (err < 0) {
		printf("datcache_updatelataestheade error\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}

	err = datcache_writefile(cache);
	if (err < 0) {
		printf("datcache_writefile error\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	ok = test_cache_util_cmp_rec_bin(&test_lnk, DATCACHE_RECORDTYPE_MAIN, 0, test_cache_testdata_01, strlen(test_cache_testdata_01));
	if (ok == False) {
		printf("main data error\n");
		result = UNITTEST_RESULT_FAIL;
	}
	ok = test_cache_util_cmp_rec_bin(&test_lnk, DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_HEADER, test_cache_testdata_07, strlen(test_cache_testdata_07));
	if (ok == False) {
		printf("info data error\n");
		result = UNITTEST_RESULT_FAIL;
	}

	err = odel_vob(vid, 0);
	if (err < 0) {
		printf("error odel_vob:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}
	err = del_fil(NULL, &test_lnk, 0);
	if (err < 0) {
		printf("error del_fil:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}

	return result;
}

/* test_cache_12 */

LOCAL UNITTEST_RESULT test_cache_12_testseq(VID vid, LINK *lnk)
{
	datcache_t *cache;
	datcache_datareadcontext_t *context;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;
	UB *bin_cache;
	W len_cache;
	Bool ok;

	cache = datcache_new(vid);

	context = datcache_startdataread(cache, strlen(test_cache_testdata_01) + 5);
	if (context == NULL) {
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}
	ok = datcache_datareadcontext_nextdata(context, &bin_cache, &len_cache);
	if (ok == True) {
		result = UNITTEST_RESULT_FAIL;
	}
	datcache_enddataread(cache, context);

	datcache_delete(cache);

	return result;
}

LOCAL UNITTEST_RESULT test_cache_12()
{
	LINK test_lnk;
	W fd, err;
	VID vid;
	UNITTEST_RESULT result;

	fd = test_cache_util_gen_file(&test_lnk, &vid);
	if (fd < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	err = ins_rec(fd, test_cache_testdata_01, strlen(test_cache_testdata_01), DATCACHE_RECORDTYPE_MAIN, 0, 0);
	if (err < 0) {
		cls_fil(fd);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	err = ins_rec(fd, test_cache_testdata_02, strlen(test_cache_testdata_02), DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_HEADER, 0);
	if (err < 0) {
		cls_fil(fd);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	err = ins_rec(fd, test_cache_testdata_03, strlen(test_cache_testdata_03), DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_RETRIEVE, 0);
	if (err < 0) {
		cls_fil(fd);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	result = test_cache_12_testseq(vid, &test_lnk);

	err = odel_vob(vid, 0);
	if (err < 0) {
		printf("error odel_vob:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}
	err = del_fil(NULL, &test_lnk, 0);
	if (err < 0) {
		printf("error del_fil:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}

	return result;
}

/* test_cache_13 */

LOCAL UNITTEST_RESULT test_cache_13_testseq(VID vid, LINK *lnk)
{
	W err;
	datcache_t *cache;
	datcache_datareadcontext_t *context;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;
	UB *bin_cache;
	W len_cache;
	Bool ok;

	cache = datcache_new(vid);

	err = datcache_appenddata(cache, test_cache_testdata_01_2, strlen(test_cache_testdata_01_2));
	if (err < 0) {
		printf("datcache_appenddata error 1\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}
	err = datcache_appenddata(cache, test_cache_testdata_01_3, strlen(test_cache_testdata_01_3));
	if (err < 0) {
		printf("datcache_appenddata error 2\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}
	err = datcache_appenddata(cache, test_cache_testdata_01_4, strlen(test_cache_testdata_01_4));
	if (err < 0) {
		printf("datcache_appenddata error 3\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}

	context = datcache_startdataread(cache, strlen(test_cache_testdata_01)+5);
	if (context == NULL) {
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}
	ok = datcache_datareadcontext_nextdata(context, &bin_cache, &len_cache);
	if (ok == True) {
		result = UNITTEST_RESULT_FAIL;
	}
	datcache_enddataread(cache, context);

	datcache_delete(cache);

	return result;
}

LOCAL UNITTEST_RESULT test_cache_13()
{
	LINK test_lnk;
	W fd, err;
	VID vid;
	UNITTEST_RESULT result;

	fd = test_cache_util_gen_file(&test_lnk, &vid);
	if (fd < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	err = ins_rec(fd, test_cache_testdata_01_1, strlen(test_cache_testdata_01_1), DATCACHE_RECORDTYPE_MAIN, 0, 0);
	if (err < 0) {
		cls_fil(fd);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	err = ins_rec(fd, test_cache_testdata_02, strlen(test_cache_testdata_02), DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_HEADER, 0);
	if (err < 0) {
		cls_fil(fd);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	err = ins_rec(fd, test_cache_testdata_03, strlen(test_cache_testdata_03), DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_RETRIEVE, 0);
	if (err < 0) {
		cls_fil(fd);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	result = test_cache_13_testseq(vid, &test_lnk);

	err = odel_vob(vid, 0);
	if (err < 0) {
		printf("error odel_vob:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}
	err = del_fil(NULL, &test_lnk, 0);
	if (err < 0) {
		printf("error del_fil:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}

	return result;
}

/* test_cache_14 */

LOCAL UNITTEST_RESULT test_cache_14_testseq(VID vid, LINK *lnk)
{
	W err;
	datcache_t *cache;
	datcache_datareadcontext_t *context;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;
	UB *bin_cache;
	W len_cache;
	Bool ok;

	cache = datcache_new(vid);

	err = datcache_appenddata(cache, test_cache_testdata_01_1, strlen(test_cache_testdata_01_1));
	if (err < 0) {
		printf("datcache_appenddata error 1\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}
	err = datcache_appenddata(cache, test_cache_testdata_01_2, strlen(test_cache_testdata_01_2));
	if (err < 0) {
		printf("datcache_appenddata error 2\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}
	err = datcache_appenddata(cache, test_cache_testdata_01_3, strlen(test_cache_testdata_01_3));
	if (err < 0) {
		printf("datcache_appenddata error 3\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}
	err = datcache_appenddata(cache, test_cache_testdata_01_4, strlen(test_cache_testdata_01_4));
	if (err < 0) {
		printf("datcache_appenddata error 4\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}

	context = datcache_startdataread(cache, strlen(test_cache_testdata_01)+5);
	if (context == NULL) {
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}
	ok = datcache_datareadcontext_nextdata(context, &bin_cache, &len_cache);
	if (ok == True) {
		result = UNITTEST_RESULT_FAIL;
	}
	datcache_enddataread(cache, context);

	datcache_delete(cache);

	return result;
}

LOCAL UNITTEST_RESULT test_cache_14()
{
	LINK test_lnk;
	W fd, err;
	VID vid;
	UNITTEST_RESULT result;

	fd = test_cache_util_gen_file(&test_lnk, &vid);
	if (fd < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	err = ins_rec(fd, test_cache_testdata_02, strlen(test_cache_testdata_02), DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_HEADER, 0);
	if (err < 0) {
		cls_fil(fd);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	err = ins_rec(fd, test_cache_testdata_03, strlen(test_cache_testdata_03), DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_RETRIEVE, 0);
	if (err < 0) {
		cls_fil(fd);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	result = test_cache_14_testseq(vid, &test_lnk);

	err = odel_vob(vid, 0);
	if (err < 0) {
		printf("error odel_vob:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}
	err = del_fil(NULL, &test_lnk, 0);
	if (err < 0) {
		printf("error del_fil:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}

	return result;
}

/* test_cache_15 */

LOCAL UNITTEST_RESULT test_cache_15()
{
	LINK test_lnk;
	W fd, err;
	VID vid;
	datcache_t *cache;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	fd = test_cache_util_gen_file(&test_lnk, &vid);
	if (fd < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	cache = datcache_new(vid);

	err = datcache_writefile(cache);
	if (err < 0) {
		printf("datcache_writefile error\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	fd = opn_fil(&test_lnk, F_READ|F_WRITE, NULL);
	if (fd < 0) {
		printf("main data error\n");
		return UNITTEST_RESULT_FAIL;
	}
	err = fnd_rec(fd, F_TOPEND, 1 << DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_HEADER, NULL);
	if (err != ER_REC) {
		printf("found HEADER record\n");
		result = UNITTEST_RESULT_FAIL;
	}
	err = fnd_rec(fd, F_TOPEND, 1 << DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_RETRIEVE, NULL);
	if (err != ER_REC) {
		printf("found RETRIEVE record\n");
		result = UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	err = odel_vob(vid, 0);
	if (err < 0) {
		printf("error odel_vob:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}
	err = del_fil(NULL, &test_lnk, 0);
	if (err < 0) {
		printf("error del_fil:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}

	return result;
}

/* test_cache_16 */

LOCAL UNITTEST_RESULT test_cache_16_testseq(VID vid, LINK *lnk)
{
	datcache_t *cache;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;
	UB *host, *borad, *thread;
	W cmp, host_len, borad_len, thread_len;
	UH port;

	cache = datcache_new(vid);

	datcache_gethost(cache, &host, &host_len);
	if (host_len != strlen(test_cache_testdata_04)) {
		printf("datcache_gethost: length error\n");
		result = UNITTEST_RESULT_FAIL;
	}
	cmp = memcmp(host, test_cache_testdata_04, host_len);
	if (cmp != 0) {
		printf("datcache_gethost: data error\n");
		result = UNITTEST_RESULT_FAIL;
	}

	datcache_getport(cache, &port);
	if (port != 15000) {
		printf("datcache_getport: data error\n");
		result = UNITTEST_RESULT_FAIL;
	}

	datcache_getborad(cache, &borad, &borad_len);
	if (borad_len != strlen(test_cache_testdata_05)) {
		printf("datcache_getboard: length error\n");
		result = UNITTEST_RESULT_FAIL;
	}
	cmp = memcmp(borad, test_cache_testdata_05, borad_len);
	if (cmp != 0) {
		printf("datcache_getboard: data error\n");
		result = UNITTEST_RESULT_FAIL;
	}

	datcache_getthread(cache, &thread, &thread_len);
	if (thread_len != strlen(test_cache_testdata_06)) {
		printf("datcache_getthread: length error\n");
		result = UNITTEST_RESULT_FAIL;
	}
	cmp = memcmp(thread, test_cache_testdata_06, thread_len);
	if (cmp != 0) {
		printf("datcache_getthread: data error\n");
		result = UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	return result;
}

LOCAL UNITTEST_RESULT test_cache_16()
{
	LINK test_lnk;
	W fd, err;
	VID vid;
	UNITTEST_RESULT result;

	fd = test_cache_util_gen_file(&test_lnk, &vid);
	if (fd < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	err = ins_rec(fd, test_cache_testdata_01, strlen(test_cache_testdata_01), DATCACHE_RECORDTYPE_MAIN, 0, 0);
	if (err < 0) {
		cls_fil(fd);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	err = ins_rec(fd, test_cache_testdata_02, strlen(test_cache_testdata_02), DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_HEADER, 0);
	if (err < 0) {
		cls_fil(fd);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	err = ins_rec(fd, test_cache_testdata_08, strlen(test_cache_testdata_08), DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_RETRIEVE, 0);
	if (err < 0) {
		cls_fil(fd);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	result = test_cache_16_testseq(vid, &test_lnk);

	err = odel_vob(vid, 0);
	if (err < 0) {
		printf("error odel_vob:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}
	err = del_fil(NULL, &test_lnk, 0);
	if (err < 0) {
		printf("error del_fil:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}

	return result;
}

/* append and write file test */

LOCAL W test_cache_writecheck_testseq_appenddata(datcache_t *cache, Bool clear)
{
	W err;

	err = datcache_appenddata(cache, test_cache_testdata_01_2, strlen(test_cache_testdata_01_2));
	if (err < 0) {
		printf("datcache_appenddata error 1\n");
		return err;
	}
	err = datcache_appenddata(cache, test_cache_testdata_01_3, strlen(test_cache_testdata_01_3));
	if (err < 0) {
		printf("datcache_appenddata error 2\n");
		return err;
	}
	err = datcache_appenddata(cache, test_cache_testdata_01_4, strlen(test_cache_testdata_01_4));
	if (err < 0) {
		printf("datcache_appenddata error 3\n");
		return err;
	}

	if (clear == False) {
		return 0;
	}

	datcache_cleardata(cache);

	err = datcache_appenddata(cache, test_cache_testdata_01_1, strlen(test_cache_testdata_01_1));
	if (err < 0) {
		printf("datcache_appenddata error 4\n");
		return err;
	}
	err = datcache_appenddata(cache, test_cache_testdata_01_2, strlen(test_cache_testdata_01_2));
	if (err < 0) {
		printf("datcache_appenddata error 5\n");
		return err;
	}
	err = datcache_appenddata(cache, test_cache_testdata_01_3, strlen(test_cache_testdata_01_3));
	if (err < 0) {
		printf("datcache_appenddata error 6\n");
		return err;
	}
	err = datcache_appenddata(cache, test_cache_testdata_01_4, strlen(test_cache_testdata_01_4));
	if (err < 0) {
		printf("datcache_appenddata error 7\n");
		return err;
	}

	return 0;
}

LOCAL W test_cache_writecheck_testseq_modsize(W size_diff, LINK *lnk)
{
	W fd, prev_size, err;
	UB *bin;

	fd = opn_fil(lnk, F_READ|F_WRITE, NULL);
	if (fd < 0) {
		printf("opn_fil error\n");
		return fd;
	}
	err = fnd_rec(fd, F_TOPEND, 1 << DATCACHE_RECORDTYPE_MAIN, 0, NULL);
	if (err < 0) {
		printf("fnd_rec error\n");
		cls_fil(fd);
		return err;
	}
	if (size_diff < 0) {
		err = rea_rec(fd, 0, NULL, 0, &prev_size, NULL);
		if (err < 0) {
			printf("rea_rec error\n");
			cls_fil(fd);
			return err;
		}
		err = trc_rec(fd, prev_size + size_diff);
		if (err < 0) {
			printf("trc_rec error %d\n", err >> 16);
			cls_fil(fd);
			return err;
		}
	} else {
		bin = malloc(size_diff);
		if (bin == NULL) {
			printf("malloc error\n");
			cls_fil(fd);
			return -1;
		}
		memset(bin, 0, size_diff);
		err = wri_rec(fd, -1, bin, size_diff, NULL, NULL, 0);
		if (err < 0) {
			printf("wri_rec error %d\n", err >> 16);
			free(bin);
			cls_fil(fd);
			return err;
		}
		free(bin);
	}
	cls_fil(fd);

	return 0;
}

LOCAL UNITTEST_RESULT test_cache_writecheck_testseq(W time_diff, W size_diff, Bool clear, Bool expected, VID vid, LINK *lnk)
{
	W err;
	datcache_t *cache;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;
	Bool ok;
	F_TIME ftime;
	F_STATE fstate;

	cache = datcache_new(vid);
	if (cache == NULL) {
		printf("datcache_new error\n");
		return UNITTEST_RESULT_FAIL;
	}

	err = test_cache_writecheck_testseq_appenddata(cache, clear);
	if (err < 0) {
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}

	err = fil_sts(lnk, NULL, &fstate, NULL);
	if (err < 0) {
		printf("fil_sts error\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}
	if (size_diff != 0) {
		err = test_cache_writecheck_testseq_modsize(size_diff, lnk);
		if (err < 0) {
			printf("recsize modify error\n");
			datcache_delete(cache);
			return UNITTEST_RESULT_FAIL;
		}
	}
	ftime.f_ltime = -1;
	ftime.f_atime = -1;
	ftime.f_mtime = fstate.f_mtime + time_diff;
	err = chg_ftm(lnk, &ftime);
	if (err < 0) {
		printf("chg_ftm error\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}

	err = datcache_writefile(cache);
	if (err < 0) {
		printf("datcache_writefile error\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	ok = test_cache_util_cmp_rec_bin(lnk, DATCACHE_RECORDTYPE_MAIN, 0, test_cache_testdata_01, strlen(test_cache_testdata_01));
	if (ok != expected) {
		printf("main data error\n");
		result = UNITTEST_RESULT_FAIL;
	}	

	return result;
}

LOCAL UNITTEST_RESULT test_cache_writecheck(W time_diff, W size_diff, Bool clear, Bool expected)
{
	LINK test_lnk;
	W fd, err;
	VID vid;
	UNITTEST_RESULT result;

	fd = test_cache_util_gen_file(&test_lnk, &vid);
	if (fd < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	err = ins_rec(fd, test_cache_testdata_01_1, strlen(test_cache_testdata_01_1), DATCACHE_RECORDTYPE_MAIN, 0, 0);
	if (err < 0) {
		cls_fil(fd);
		del_fil(NULL, &test_lnk, 0);
		return UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	result = test_cache_writecheck_testseq(time_diff, size_diff, clear, expected, vid, &test_lnk);

	err = odel_vob(vid, 0);
	if (err < 0) {
		printf("error odel_vob:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}
	err = del_fil(NULL, &test_lnk, 0);
	if (err < 0) {
		printf("error del_fil:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}

	return result;
}

LOCAL UNITTEST_RESULT test_cache_append_1()
{
	return test_cache_writecheck(0, 0, False, True);
}

LOCAL UNITTEST_RESULT test_cache_append_2()
{
	return test_cache_writecheck(0, 3, False, False);
}

LOCAL UNITTEST_RESULT test_cache_append_3()
{
	return test_cache_writecheck(0, -3, False, False);
}

LOCAL UNITTEST_RESULT test_cache_append_4()
{
	return test_cache_writecheck(300, 0, False, False);
}

LOCAL UNITTEST_RESULT test_cache_append_5()
{
	return test_cache_writecheck(300, 3, False, False);
}

LOCAL UNITTEST_RESULT test_cache_append_6()
{
	return test_cache_writecheck(300, -3, False, False);
}

LOCAL UNITTEST_RESULT test_cache_append_7()
{
	return test_cache_writecheck(-300, 0, False, False);
}

LOCAL UNITTEST_RESULT test_cache_append_8()
{
	return test_cache_writecheck(-300, 3, False, False);
}

LOCAL UNITTEST_RESULT test_cache_append_9()
{
	return test_cache_writecheck(-300, -3, False, False);
}

LOCAL UNITTEST_RESULT test_cache_reload_1()
{
	return test_cache_writecheck(0, 0, True, True);
}

LOCAL UNITTEST_RESULT test_cache_reload_2()
{
	return test_cache_writecheck(0, 3, True, True);
}

LOCAL UNITTEST_RESULT test_cache_reload_3()
{
	return test_cache_writecheck(0, -3, True, True);
}

LOCAL UNITTEST_RESULT test_cache_reload_4()
{
	return test_cache_writecheck(300, 0, True, True);
}

LOCAL UNITTEST_RESULT test_cache_reload_5()
{
	return test_cache_writecheck(300, 3, True, True);
}

LOCAL UNITTEST_RESULT test_cache_reload_6()
{
	return test_cache_writecheck(300, -3, True, True);
}

LOCAL UNITTEST_RESULT test_cache_reload_7()
{
	return test_cache_writecheck(-300, 0, True, True);
}

LOCAL UNITTEST_RESULT test_cache_reload_8()
{
	return test_cache_writecheck(-300, 3, True, True);
}

LOCAL UNITTEST_RESULT test_cache_reload_9()
{
	return test_cache_writecheck(-300, -3, True, True);
}

/* test_cache_residinfo_1 */

LOCAL UNITTEST_RESULT test_cache_residinfo_1()
{
	LINK test_lnk;
	W fd, err;
	VID vid;
	datcache_t *cache;
	UB idstr1[] = "yZXmy7Om0", idstr2[] = "GCQJ44Ao", idstr3[] = "V.jwWEDO";
	TC idstr1_tc[9], idstr2_tc[8], idstr3_tc[8];
	W ret, idstr1_len = strlen(idstr1), idstr2_len = strlen(idstr2), idstr3_len = strlen(idstr3);
	UW attr, attr1 = 0x01010101, attr2 = 0x10101010, attr3 = 0x00001111;
	COLOR color, color1 = 0x10FF00FF, color2 = 0x1000FF00, color3 = 0x10000000;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	sjstotcs(idstr1_tc, idstr1);
	sjstotcs(idstr2_tc, idstr2);
	sjstotcs(idstr3_tc, idstr3);

	fd = test_cache_util_gen_file(&test_lnk, &vid);
	if (fd < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	cache = datcache_new(vid);

	err = datcache_addresiddata(cache, idstr1_tc, idstr1_len, attr1, color1);
	if (err < 0) {
		printf("datcache_addresiddata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	err = datcache_addresiddata(cache, idstr2_tc, idstr2_len, attr2, color2);
	if (err < 0) {
		printf("datcache_addresiddata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	err = datcache_addresiddata(cache, idstr3_tc, idstr3_len, attr3, color3);
	if (err < 0) {
		printf("datcache_addresiddata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}

	err = datcache_writefile(cache);
	if (err < 0) {
		printf("datcache_writefile error\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	cache = datcache_new(vid);

	ret = datcache_searchresiddata(cache, idstr1_tc, idstr1_len, &attr, &color);
	if (ret != DATCACHE_SEARCHRESIDDATA_FOUND) {
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
	ret = datcache_searchresiddata(cache, idstr2_tc, idstr2_len, &attr, &color);
	if (ret != DATCACHE_SEARCHRESIDDATA_FOUND) {
		printf("residhash_searchdata 2 fail\n");
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
	ret = datcache_searchresiddata(cache, idstr3_tc, idstr3_len, &attr, &color);
	if (ret != DATCACHE_SEARCHRESIDDATA_FOUND) {
		printf("residhash_searchdata 3 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	if (attr != attr3) {
		printf("residhash_searchdata 3 result fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	if (color != color3) {
		printf("residhash_searchdata 3 result fail\n");
		result = UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	err = odel_vob(vid, 0);
	if (err < 0) {
		printf("error odel_vob:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}
	err = del_fil(NULL, &test_lnk, 0);
	if (err < 0) {
		printf("error del_fil:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}

	return result;
}

/* test_cache_residinfo_2 */

LOCAL UNITTEST_RESULT test_cache_residinfo_2()
{
	LINK test_lnk;
	W fd, err;
	VID vid;
	datcache_t *cache;
	UB idstr1[] = "yZXmy7Om0", idstr2[] = "GCQJ44Ao", idstr3[] = "V.jwWEDO";
	TC idstr1_tc[9], idstr2_tc[8], idstr3_tc[8];
	W ret, idstr1_len = strlen(idstr1), idstr2_len = strlen(idstr2), idstr3_len = strlen(idstr3);
	UW attr, attr1 = 0x01010101, attr2 = 0x10101010;
	COLOR color, color1 = 0x10FF00FF, color2 = 0x1000FF00;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	sjstotcs(idstr1_tc, idstr1);
	sjstotcs(idstr2_tc, idstr2);
	sjstotcs(idstr3_tc, idstr3);

	fd = test_cache_util_gen_file(&test_lnk, &vid);
	if (fd < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	cache = datcache_new(vid);

	err = datcache_addresiddata(cache, idstr1_tc, idstr1_len, attr1, color1);
	if (err < 0) {
		printf("datcache_addresiddata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	err = datcache_addresiddata(cache, idstr2_tc, idstr2_len, attr2, color2);
	if (err < 0) {
		printf("datcache_addresiddata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}

	err = datcache_writefile(cache);
	if (err < 0) {
		printf("datcache_writefile error\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	cache = datcache_new(vid);

	ret = datcache_searchresiddata(cache, idstr1_tc, idstr1_len, &attr, &color);
	if (ret != DATCACHE_SEARCHRESIDDATA_FOUND) {
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
	ret = datcache_searchresiddata(cache, idstr2_tc, idstr2_len, &attr, &color);
	if (ret != DATCACHE_SEARCHRESIDDATA_FOUND) {
		printf("residhash_searchdata 2 fail\n");
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
	ret = datcache_searchresiddata(cache, idstr3_tc, idstr3_len, &attr, &color);
	if (ret != DATCACHE_SEARCHRESIDDATA_NOTFOUND) {
		printf("residhash_searchdata 3 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	err = odel_vob(vid, 0);
	if (err < 0) {
		printf("error odel_vob:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}
	err = del_fil(NULL, &test_lnk, 0);
	if (err < 0) {
		printf("error del_fil:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}

	return result;
}

/* test_cache_residinfo_3 */

LOCAL UNITTEST_RESULT test_cache_residinfo_3()
{
	LINK test_lnk;
	W fd, err;
	VID vid;
	datcache_t *cache;
	UB idstr1[] = "yZXmy7Om0", idstr2[] = "GCQJ44Ao", idstr3[] = "V.jwWEDO";
	TC idstr1_tc[9], idstr2_tc[8], idstr3_tc[8];
	W ret, idstr1_len = strlen(idstr1), idstr2_len = strlen(idstr2), idstr3_len = strlen(idstr3);
	UW attr, attr1 = 0x01010101;
	COLOR color, color1 = 0x10FF00FF;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	sjstotcs(idstr1_tc, idstr1);
	sjstotcs(idstr2_tc, idstr2);
	sjstotcs(idstr3_tc, idstr3);

	fd = test_cache_util_gen_file(&test_lnk, &vid);
	if (fd < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	cache = datcache_new(vid);

	err = datcache_addresiddata(cache, idstr1_tc, idstr1_len, attr1, color1);
	if (err < 0) {
		printf("datcache_addresiddata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}

	err = datcache_writefile(cache);
	if (err < 0) {
		printf("datcache_writefile error\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	cache = datcache_new(vid);

	ret = datcache_searchresiddata(cache, idstr1_tc, idstr1_len, &attr, &color);
	if (ret != DATCACHE_SEARCHRESIDDATA_FOUND) {
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
	ret = datcache_searchresiddata(cache, idstr2_tc, idstr2_len, &attr, &color);
	if (ret != DATCACHE_SEARCHRESIDDATA_NOTFOUND) {
		printf("residhash_searchdata 2 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	ret = datcache_searchresiddata(cache, idstr3_tc, idstr3_len, &attr, &color);
	if (ret != DATCACHE_SEARCHRESIDDATA_NOTFOUND) {
		printf("residhash_searchdata 3 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	err = odel_vob(vid, 0);
	if (err < 0) {
		printf("error odel_vob:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}
	err = del_fil(NULL, &test_lnk, 0);
	if (err < 0) {
		printf("error del_fil:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}

	return result;
}

/* test_cache_residinfo_4 */

LOCAL UNITTEST_RESULT test_cache_residinfo_4()
{
	LINK test_lnk;
	W fd, err;
	VID vid;
	datcache_t *cache;
	UB idstr1[] = "yZXmy7Om0", idstr2[] = "GCQJ44Ao", idstr3[] = "V.jwWEDO";
	TC idstr1_tc[9], idstr2_tc[8], idstr3_tc[8];
	W ret, idstr1_len = strlen(idstr1), idstr2_len = strlen(idstr2), idstr3_len = strlen(idstr3);
	UW attr;
	COLOR color;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	sjstotcs(idstr1_tc, idstr1);
	sjstotcs(idstr2_tc, idstr2);
	sjstotcs(idstr3_tc, idstr3);

	fd = test_cache_util_gen_file(&test_lnk, &vid);
	if (fd < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	cache = datcache_new(vid);

	err = datcache_writefile(cache);
	if (err < 0) {
		printf("datcache_writefile error\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	cache = datcache_new(vid);

	ret = datcache_searchresiddata(cache, idstr1_tc, idstr1_len, &attr, &color);
	if (ret != DATCACHE_SEARCHRESIDDATA_NOTFOUND) {
		printf("residhash_searchdata 1 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	ret = datcache_searchresiddata(cache, idstr2_tc, idstr2_len, &attr, &color);
	if (ret != DATCACHE_SEARCHRESIDDATA_NOTFOUND) {
		printf("residhash_searchdata 2 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	ret = datcache_searchresiddata(cache, idstr3_tc, idstr3_len, &attr, &color);
	if (ret != DATCACHE_SEARCHRESIDDATA_NOTFOUND) {
		printf("residhash_searchdata 3 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	err = odel_vob(vid, 0);
	if (err < 0) {
		printf("error odel_vob:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}
	err = del_fil(NULL, &test_lnk, 0);
	if (err < 0) {
		printf("error del_fil:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}

	return result;
}

/* test_cache_residinfo_5 */

LOCAL UNITTEST_RESULT test_cache_residinfo_5()
{
	LINK test_lnk;
	W fd, err;
	VID vid;
	datcache_t *cache;
	UB idstr1[] = "yZXmy7Om0", idstr2[] = "GCQJ44Ao", idstr3[] = "V.jwWEDO";
	TC idstr1_tc[9], idstr2_tc[8], idstr3_tc[8];
	W ret, idstr1_len = strlen(idstr1), idstr2_len = strlen(idstr2), idstr3_len = strlen(idstr3);
	UW attr, attr1 = 0x01010101, attr2 = 0x10101010, attr3 = 0x00001111;
	COLOR color, color1 = 0x10FF00FF, color2 = 0x1000FF00, color3 = 0x10000000;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	sjstotcs(idstr1_tc, idstr1);
	sjstotcs(idstr2_tc, idstr2);
	sjstotcs(idstr3_tc, idstr3);

	fd = test_cache_util_gen_file(&test_lnk, &vid);
	if (fd < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	cache = datcache_new(vid);

	err = datcache_addresiddata(cache, idstr1_tc, idstr1_len, attr1, color1);
	if (err < 0) {
		printf("datcache_addresiddata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	err = datcache_addresiddata(cache, idstr2_tc, idstr2_len, attr2, color2);
	if (err < 0) {
		printf("datcache_addresiddata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	err = datcache_addresiddata(cache, idstr3_tc, idstr3_len, attr3, color3);
	if (err < 0) {
		printf("datcache_addresiddata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}

	err = datcache_writefile(cache);
	if (err < 0) {
		printf("datcache_writefile error\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	cache = datcache_new(vid);
	datcache_removeresiddata(cache, idstr3_tc, idstr3_len);
	err = datcache_writefile(cache);
	if (err < 0) {
		printf("datcache_writefile error\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}
	datcache_delete(cache);

	cache = datcache_new(vid);

	ret = datcache_searchresiddata(cache, idstr1_tc, idstr1_len, &attr, &color);
	if (ret != DATCACHE_SEARCHRESIDDATA_FOUND) {
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
	ret = datcache_searchresiddata(cache, idstr2_tc, idstr2_len, &attr, &color);
	if (ret != DATCACHE_SEARCHRESIDDATA_FOUND) {
		printf("residhash_searchdata 2 fail\n");
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
	ret = datcache_searchresiddata(cache, idstr3_tc, idstr3_len, &attr, &color);
	if (ret != DATCACHE_SEARCHRESIDDATA_NOTFOUND) {
		printf("residhash_searchdata 3 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	err = odel_vob(vid, 0);
	if (err < 0) {
		printf("error odel_vob:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}
	err = del_fil(NULL, &test_lnk, 0);
	if (err < 0) {
		printf("error del_fil:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}

	return result;
}

/* test_cache_residinfo_6 */

LOCAL UNITTEST_RESULT test_cache_residinfo_6()
{
	LINK test_lnk;
	W fd, err;
	VID vid;
	datcache_t *cache;
	UB idstr1[] = "yZXmy7Om0", idstr2[] = "GCQJ44Ao", idstr3[] = "V.jwWEDO";
	TC idstr1_tc[9], idstr2_tc[8], idstr3_tc[8];
	W ret, idstr1_len = strlen(idstr1), idstr2_len = strlen(idstr2), idstr3_len = strlen(idstr3);
	UW attr, attr1 = 0x01010101, attr2 = 0x10101010, attr3 = 0x00001111;
	COLOR color, color1 = 0x10FF00FF, color2 = 0x1000FF00, color3 = 0x10000000;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	sjstotcs(idstr1_tc, idstr1);
	sjstotcs(idstr2_tc, idstr2);
	sjstotcs(idstr3_tc, idstr3);

	fd = test_cache_util_gen_file(&test_lnk, &vid);
	if (fd < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	cache = datcache_new(vid);

	err = datcache_addresiddata(cache, idstr1_tc, idstr1_len, attr1, color1);
	if (err < 0) {
		printf("datcache_addresiddata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	err = datcache_addresiddata(cache, idstr2_tc, idstr2_len, attr2, color2);
	if (err < 0) {
		printf("datcache_addresiddata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	err = datcache_addresiddata(cache, idstr3_tc, idstr3_len, attr3, color3);
	if (err < 0) {
		printf("datcache_addresiddata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}

	err = datcache_writefile(cache);
	if (err < 0) {
		printf("datcache_writefile error\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	cache = datcache_new(vid);
	datcache_removeresiddata(cache, idstr2_tc, idstr2_len);
	datcache_removeresiddata(cache, idstr3_tc, idstr3_len);
	err = datcache_writefile(cache);
	if (err < 0) {
		printf("datcache_writefile error\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}
	datcache_delete(cache);

	cache = datcache_new(vid);

	ret = datcache_searchresiddata(cache, idstr1_tc, idstr1_len, &attr, &color);
	if (ret != DATCACHE_SEARCHRESIDDATA_FOUND) {
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
	ret = datcache_searchresiddata(cache, idstr2_tc, idstr2_len, &attr, &color);
	if (ret != DATCACHE_SEARCHRESIDDATA_NOTFOUND) {
		printf("residhash_searchdata 2 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	ret = datcache_searchresiddata(cache, idstr3_tc, idstr3_len, &attr, &color);
	if (ret != DATCACHE_SEARCHRESIDDATA_NOTFOUND) {
		printf("residhash_searchdata 3 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	err = odel_vob(vid, 0);
	if (err < 0) {
		printf("error odel_vob:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}
	err = del_fil(NULL, &test_lnk, 0);
	if (err < 0) {
		printf("error del_fil:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}

	return result;
}

/* test_cache_residinfo_7 */

LOCAL UNITTEST_RESULT test_cache_residinfo_7()
{
	LINK test_lnk;
	W fd, err;
	VID vid;
	datcache_t *cache;
	UB idstr1[] = "yZXmy7Om0", idstr2[] = "GCQJ44Ao", idstr3[] = "V.jwWEDO";
	TC idstr1_tc[9], idstr2_tc[8], idstr3_tc[8];
	W ret, idstr1_len = strlen(idstr1), idstr2_len = strlen(idstr2), idstr3_len = strlen(idstr3);
	UW attr, attr1 = 0x01010101, attr2 = 0x10101010, attr3 = 0x00001111;
	COLOR color, color1 = 0x10FF00FF, color2 = 0x1000FF00, color3 = 0x10000000;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	sjstotcs(idstr1_tc, idstr1);
	sjstotcs(idstr2_tc, idstr2);
	sjstotcs(idstr3_tc, idstr3);

	fd = test_cache_util_gen_file(&test_lnk, &vid);
	if (fd < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	cache = datcache_new(vid);

	err = datcache_addresiddata(cache, idstr1_tc, idstr1_len, attr1, color1);
	if (err < 0) {
		printf("datcache_addresiddata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	err = datcache_addresiddata(cache, idstr2_tc, idstr2_len, attr2, color2);
	if (err < 0) {
		printf("datcache_addresiddata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	err = datcache_addresiddata(cache, idstr3_tc, idstr3_len, attr3, color3);
	if (err < 0) {
		printf("datcache_addresiddata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}

	err = datcache_writefile(cache);
	if (err < 0) {
		printf("datcache_writefile error\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	cache = datcache_new(vid);
	datcache_removeresiddata(cache, idstr1_tc, idstr1_len);
	datcache_removeresiddata(cache, idstr2_tc, idstr2_len);
	datcache_removeresiddata(cache, idstr3_tc, idstr3_len);
	err = datcache_writefile(cache);
	if (err < 0) {
		printf("datcache_writefile error\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}
	datcache_delete(cache);

	cache = datcache_new(vid);

	ret = datcache_searchresiddata(cache, idstr1_tc, idstr1_len, &attr, &color);
	if (ret != DATCACHE_SEARCHRESIDDATA_NOTFOUND) {
		printf("residhash_searchdata 1 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	ret = datcache_searchresiddata(cache, idstr2_tc, idstr2_len, &attr, &color);
	if (ret != DATCACHE_SEARCHRESIDDATA_NOTFOUND) {
		printf("residhash_searchdata 2 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	ret = datcache_searchresiddata(cache, idstr3_tc, idstr3_len, &attr, &color);
	if (ret != DATCACHE_SEARCHRESIDDATA_NOTFOUND) {
		printf("residhash_searchdata 3 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	err = odel_vob(vid, 0);
	if (err < 0) {
		printf("error odel_vob:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}
	err = del_fil(NULL, &test_lnk, 0);
	if (err < 0) {
		printf("error del_fil:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}

	return result;
}

/* test_cache_resindexinfo_1 */

LOCAL UNITTEST_RESULT test_cache_resindexinfo_1()
{
	LINK test_lnk;
	W fd, err;
	VID vid;
	datcache_t *cache;
	W index1 = 3, index2 = 543, index3 = 349;
	W ret;
	UW attr, attr1 = 0x01010101, attr2 = 0x10101010, attr3 = 0x00001111;
	COLOR color, color1 = 0x10FF00FF, color2 = 0x1000FF00, color3 = 0x10000000;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	fd = test_cache_util_gen_file(&test_lnk, &vid);
	if (fd < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	cache = datcache_new(vid);

	err = datcache_addresindexdata(cache, index1, attr1, color1);
	if (err < 0) {
		printf("datcache_addresindexdata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	err = datcache_addresindexdata(cache, index2, attr2, color2);
	if (err < 0) {
		printf("datcache_addresindexdata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	err = datcache_addresindexdata(cache, index3, attr3, color3);
	if (err < 0) {
		printf("datcache_addresindexdata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}

	err = datcache_writefile(cache);
	if (err < 0) {
		printf("datcache_writefile error\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	cache = datcache_new(vid);

	ret = datcache_searchresindexdata(cache, index1, &attr, &color);
	if (ret != DATCACHE_SEARCHRESINDEXDATA_FOUND) {
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
	ret = datcache_searchresindexdata(cache, index2, &attr, &color);
	if (ret != DATCACHE_SEARCHRESINDEXDATA_FOUND) {
		printf("resindexhash_searchdata 2 fail\n");
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
	ret = datcache_searchresindexdata(cache, index3, &attr, &color);
	if (ret != DATCACHE_SEARCHRESINDEXDATA_FOUND) {
		printf("resindexhash_searchdata 3 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	if (attr != attr3) {
		printf("resindexhash_searchdata 3 result fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	if (color != color3) {
		printf("resindexhash_searchdata 3 result fail\n");
		result = UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	err = odel_vob(vid, 0);
	if (err < 0) {
		printf("error odel_vob:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}
	err = del_fil(NULL, &test_lnk, 0);
	if (err < 0) {
		printf("error del_fil:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}

	return result;
}

/* test_cache_resindexinfo_2 */

LOCAL UNITTEST_RESULT test_cache_resindexinfo_2()
{
	LINK test_lnk;
	W fd, err;
	VID vid;
	datcache_t *cache;
	W index1 = 3, index2 = 543, index3 = 349;
	W ret;
	UW attr, attr1 = 0x01010101, attr2 = 0x10101010;
	COLOR color, color1 = 0x10FF00FF, color2 = 0x1000FF00;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	fd = test_cache_util_gen_file(&test_lnk, &vid);
	if (fd < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	cache = datcache_new(vid);

	err = datcache_addresindexdata(cache, index1, attr1, color1);
	if (err < 0) {
		printf("datcache_addresindexdata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	err = datcache_addresindexdata(cache, index2, attr2, color2);
	if (err < 0) {
		printf("datcache_addresindexdata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}

	err = datcache_writefile(cache);
	if (err < 0) {
		printf("datcache_writefile error\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	cache = datcache_new(vid);

	ret = datcache_searchresindexdata(cache, index1, &attr, &color);
	if (ret != DATCACHE_SEARCHRESINDEXDATA_FOUND) {
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
	ret = datcache_searchresindexdata(cache, index2, &attr, &color);
	if (ret != DATCACHE_SEARCHRESINDEXDATA_FOUND) {
		printf("resindexhash_searchdata 2 fail\n");
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
	ret = datcache_searchresindexdata(cache, index3, &attr, &color);
	if (ret != DATCACHE_SEARCHRESINDEXDATA_NOTFOUND) {
		printf("resindexhash_searchdata 3 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	err = odel_vob(vid, 0);
	if (err < 0) {
		printf("error odel_vob:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}
	err = del_fil(NULL, &test_lnk, 0);
	if (err < 0) {
		printf("error del_fil:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}

	return result;
}

/* test_cache_resindexinfo_3 */

LOCAL UNITTEST_RESULT test_cache_resindexinfo_3()
{
	LINK test_lnk;
	W fd, err;
	VID vid;
	datcache_t *cache;
	W index1 = 3, index2 = 543, index3 = 349;
	W ret;
	UW attr, attr1 = 0x01010101;
	COLOR color, color1 = 0x10FF00FF;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	fd = test_cache_util_gen_file(&test_lnk, &vid);
	if (fd < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	cache = datcache_new(vid);

	err = datcache_addresindexdata(cache, index1, attr1, color1);
	if (err < 0) {
		printf("datcache_addresindexdata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}

	err = datcache_writefile(cache);
	if (err < 0) {
		printf("datcache_writefile error\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	cache = datcache_new(vid);

	ret = datcache_searchresindexdata(cache, index1, &attr, &color);
	if (ret != DATCACHE_SEARCHRESINDEXDATA_FOUND) {
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
	ret = datcache_searchresindexdata(cache, index2, &attr, &color);
	if (ret != DATCACHE_SEARCHRESINDEXDATA_NOTFOUND) {
		printf("resindexhash_searchdata 2 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	ret = datcache_searchresindexdata(cache, index3, &attr, &color);
	if (ret != DATCACHE_SEARCHRESINDEXDATA_NOTFOUND) {
		printf("resindexhash_searchdata 3 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	err = odel_vob(vid, 0);
	if (err < 0) {
		printf("error odel_vob:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}
	err = del_fil(NULL, &test_lnk, 0);
	if (err < 0) {
		printf("error del_fil:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}

	return result;
}

/* test_cache_resindexinfo_4 */

LOCAL UNITTEST_RESULT test_cache_resindexinfo_4()
{
	LINK test_lnk;
	W fd, err;
	VID vid;
	datcache_t *cache;
	W index1 = 3, index2 = 543, index3 = 349;
	W ret;
	UW attr;
	COLOR color;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	fd = test_cache_util_gen_file(&test_lnk, &vid);
	if (fd < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	cache = datcache_new(vid);

	err = datcache_writefile(cache);
	if (err < 0) {
		printf("datcache_writefile error\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	cache = datcache_new(vid);

	ret = datcache_searchresindexdata(cache, index1, &attr, &color);
	if (ret != DATCACHE_SEARCHRESINDEXDATA_NOTFOUND) {
		printf("resindexhash_searchdata 1 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	ret = datcache_searchresindexdata(cache, index2, &attr, &color);
	if (ret != DATCACHE_SEARCHRESINDEXDATA_NOTFOUND) {
		printf("resindexhash_searchdata 2 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	ret = datcache_searchresindexdata(cache, index3, &attr, &color);
	if (ret != DATCACHE_SEARCHRESINDEXDATA_NOTFOUND) {
		printf("resindexhash_searchdata 3 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	err = odel_vob(vid, 0);
	if (err < 0) {
		printf("error odel_vob:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}
	err = del_fil(NULL, &test_lnk, 0);
	if (err < 0) {
		printf("error del_fil:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}

	return result;
}

/* test_cache_resindexinfo_5 */

LOCAL UNITTEST_RESULT test_cache_resindexinfo_5()
{
	LINK test_lnk;
	W fd, err;
	VID vid;
	datcache_t *cache;
	W index1 = 3, index2 = 543, index3 = 349;
	W ret;
	UW attr, attr1 = 0x01010101, attr2 = 0x10101010, attr3 = 0x00001111;
	COLOR color, color1 = 0x10FF00FF, color2 = 0x1000FF00, color3 = 0x10000000;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	fd = test_cache_util_gen_file(&test_lnk, &vid);
	if (fd < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	cache = datcache_new(vid);

	err = datcache_addresindexdata(cache, index1, attr1, color1);
	if (err < 0) {
		printf("datcache_addresindexdata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	err = datcache_addresindexdata(cache, index2, attr2, color2);
	if (err < 0) {
		printf("datcache_addresindexdata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	err = datcache_addresindexdata(cache, index3, attr3, color3);
	if (err < 0) {
		printf("datcache_addresindexdata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}

	err = datcache_writefile(cache);
	if (err < 0) {
		printf("datcache_writefile error\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	cache = datcache_new(vid);
	datcache_removeresindexdata(cache, index3);
	err = datcache_writefile(cache);
	if (err < 0) {
		printf("datcache_writefile error\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}
	datcache_delete(cache);

	cache = datcache_new(vid);

	ret = datcache_searchresindexdata(cache, index1, &attr, &color);
	if (ret != DATCACHE_SEARCHRESINDEXDATA_FOUND) {
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
	ret = datcache_searchresindexdata(cache, index2, &attr, &color);
	if (ret != DATCACHE_SEARCHRESINDEXDATA_FOUND) {
		printf("resindexhash_searchdata 2 fail\n");
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
	ret = datcache_searchresindexdata(cache, index3, &attr, &color);
	if (ret != DATCACHE_SEARCHRESINDEXDATA_NOTFOUND) {
		printf("resindexhash_searchdata 3 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	err = odel_vob(vid, 0);
	if (err < 0) {
		printf("error odel_vob:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}
	err = del_fil(NULL, &test_lnk, 0);
	if (err < 0) {
		printf("error del_fil:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}

	return result;
}

/* test_cache_resindexinfo_6 */

LOCAL UNITTEST_RESULT test_cache_resindexinfo_6()
{
	LINK test_lnk;
	W fd, err;
	VID vid;
	datcache_t *cache;
	W index1 = 3, index2 = 543, index3 = 349;
	W ret;
	UW attr, attr1 = 0x01010101, attr2 = 0x10101010, attr3 = 0x00001111;
	COLOR color, color1 = 0x10FF00FF, color2 = 0x1000FF00, color3 = 0x10000000;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	fd = test_cache_util_gen_file(&test_lnk, &vid);
	if (fd < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	cache = datcache_new(vid);

	err = datcache_addresindexdata(cache, index1, attr1, color1);
	if (err < 0) {
		printf("datcache_addresindexdata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	err = datcache_addresindexdata(cache, index2, attr2, color2);
	if (err < 0) {
		printf("datcache_addresindexdata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	err = datcache_addresindexdata(cache, index3, attr3, color3);
	if (err < 0) {
		printf("datcache_addresindexdata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}

	err = datcache_writefile(cache);
	if (err < 0) {
		printf("datcache_writefile error\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	cache = datcache_new(vid);
	datcache_removeresindexdata(cache, index2);
	datcache_removeresindexdata(cache, index3);
	err = datcache_writefile(cache);
	if (err < 0) {
		printf("datcache_writefile error\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}
	datcache_delete(cache);

	cache = datcache_new(vid);

	ret = datcache_searchresindexdata(cache, index1, &attr, &color);
	if (ret != DATCACHE_SEARCHRESINDEXDATA_FOUND) {
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
	ret = datcache_searchresindexdata(cache, index2, &attr, &color);
	if (ret != DATCACHE_SEARCHRESINDEXDATA_NOTFOUND) {
		printf("resindexhash_searchdata 2 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	ret = datcache_searchresindexdata(cache, index3, &attr, &color);
	if (ret != DATCACHE_SEARCHRESINDEXDATA_NOTFOUND) {
		printf("resindexhash_searchdata 3 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	err = odel_vob(vid, 0);
	if (err < 0) {
		printf("error odel_vob:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}
	err = del_fil(NULL, &test_lnk, 0);
	if (err < 0) {
		printf("error del_fil:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}

	return result;
}

/* test_cache_resindexinfo_7 */

LOCAL UNITTEST_RESULT test_cache_resindexinfo_7()
{
	LINK test_lnk;
	W fd, err;
	VID vid;
	datcache_t *cache;
	W index1 = 3, index2 = 543, index3 = 349;
	W ret;
	UW attr, attr1 = 0x01010101, attr2 = 0x10101010, attr3 = 0x00001111;
	COLOR color, color1 = 0x10FF00FF, color2 = 0x1000FF00, color3 = 0x10000000;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	fd = test_cache_util_gen_file(&test_lnk, &vid);
	if (fd < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	cache = datcache_new(vid);

	err = datcache_addresindexdata(cache, index1, attr1, color1);
	if (err < 0) {
		printf("datcache_addresindexdata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	err = datcache_addresindexdata(cache, index2, attr2, color2);
	if (err < 0) {
		printf("datcache_addresindexdata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	err = datcache_addresindexdata(cache, index3, attr3, color3);
	if (err < 0) {
		printf("datcache_addresindexdata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}

	err = datcache_writefile(cache);
	if (err < 0) {
		printf("datcache_writefile error\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	cache = datcache_new(vid);
	datcache_removeresindexdata(cache, index1);
	datcache_removeresindexdata(cache, index2);
	datcache_removeresindexdata(cache, index3);
	err = datcache_writefile(cache);
	if (err < 0) {
		printf("datcache_writefile error\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}
	datcache_delete(cache);

	cache = datcache_new(vid);

	ret = datcache_searchresindexdata(cache, index1, &attr, &color);
	if (ret != DATCACHE_SEARCHRESINDEXDATA_NOTFOUND) {
		printf("resindexhash_searchdata 1 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	ret = datcache_searchresindexdata(cache, index2, &attr, &color);
	if (ret != DATCACHE_SEARCHRESINDEXDATA_NOTFOUND) {
		printf("resindexhash_searchdata 2 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	ret = datcache_searchresindexdata(cache, index3, &attr, &color);
	if (ret != DATCACHE_SEARCHRESINDEXDATA_NOTFOUND) {
		printf("resindexhash_searchdata 3 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	err = odel_vob(vid, 0);
	if (err < 0) {
		printf("error odel_vob:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}
	err = del_fil(NULL, &test_lnk, 0);
	if (err < 0) {
		printf("error del_fil:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}

	return result;
}

/* test_cache_ngwordinfo_1 */

LOCAL UNITTEST_RESULT test_cache_ngwordinfo_1()
{
	LINK test_lnk;
	W fd, err;
	VID vid;
	datcache_t *cache;
	UB ngword1[] = "yZXmy7Om0", ngword2[] = "GCQJ44Ao", ngword3[] = "V.jwWEDO";
	TC ngword1_tc[9], ngword2_tc[8], ngword3_tc[8];
	W ngword1_len = strlen(ngword1), ngword2_len = strlen(ngword2), ngword3_len = strlen(ngword3);
	Bool found;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	sjstotcs(ngword1_tc, ngword1);
	sjstotcs(ngword2_tc, ngword2);
	sjstotcs(ngword3_tc, ngword3);

	fd = test_cache_util_gen_file(&test_lnk, &vid);
	if (fd < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	cache = datcache_new(vid);

	err = datcache_appendngword(cache, ngword1_tc, ngword1_len);
	if (err < 0) {
		printf("datcache_appendngword fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	err = datcache_appendngword(cache, ngword2_tc, ngword2_len);
	if (err < 0) {
		printf("datcache_appendngword fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	err = datcache_appendngword(cache, ngword3_tc, ngword3_len);
	if (err < 0) {
		printf("datcache_appendngword fail\n");
		result = UNITTEST_RESULT_FAIL;
	}

	err = datcache_writefile(cache);
	if (err < 0) {
		printf("datcache_writefile error\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	cache = datcache_new(vid);

	found = datcache_checkngwordexist(cache, ngword1_tc, ngword1_len);
	if (found != True) {
		printf("datcache_checkngwordexist 1 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	found = datcache_checkngwordexist(cache, ngword2_tc, ngword2_len);
	if (found != True) {
		printf("datcache_checkngwordexist 2 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	found = datcache_checkngwordexist(cache, ngword3_tc, ngword3_len);
	if (found != True) {
		printf("datcache_checkngwordexist 3 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	err = odel_vob(vid, 0);
	if (err < 0) {
		printf("error odel_vob:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}
	err = del_fil(NULL, &test_lnk, 0);
	if (err < 0) {
		printf("error del_fil:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}

	return result;
}

/* test_cache_ngwordinfo_2 */

LOCAL UNITTEST_RESULT test_cache_ngwordinfo_2()
{
	LINK test_lnk;
	W fd, err;
	VID vid;
	datcache_t *cache;
	UB ngword1[] = "yZXmy7Om0", ngword2[] = "GCQJ44Ao", ngword3[] = "V.jwWEDO";
	TC ngword1_tc[9], ngword2_tc[8], ngword3_tc[8];
	W ngword1_len = strlen(ngword1), ngword2_len = strlen(ngword2), ngword3_len = strlen(ngword3);
	Bool found;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	sjstotcs(ngword1_tc, ngword1);
	sjstotcs(ngword2_tc, ngword2);
	sjstotcs(ngword3_tc, ngword3);

	fd = test_cache_util_gen_file(&test_lnk, &vid);
	if (fd < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	cache = datcache_new(vid);

	err = datcache_appendngword(cache, ngword1_tc, ngword1_len);
	if (err < 0) {
		printf("datcache_addresiddata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	err = datcache_appendngword(cache, ngword2_tc, ngword2_len);
	if (err < 0) {
		printf("datcache_addresiddata fail\n");
		result = UNITTEST_RESULT_FAIL;
	}

	err = datcache_writefile(cache);
	if (err < 0) {
		printf("datcache_writefile error\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	cache = datcache_new(vid);

	found = datcache_checkngwordexist(cache, ngword1_tc, ngword1_len);
	if (found != True) {
		printf("datcache_checkngwordexist 1 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	found = datcache_checkngwordexist(cache, ngword2_tc, ngword2_len);
	if (found != True) {
		printf("datcache_checkngwordexist 2 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	found = datcache_checkngwordexist(cache, ngword3_tc, ngword3_len);
	if (found != False) {
		printf("datcache_checkngwordexist 3 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	err = odel_vob(vid, 0);
	if (err < 0) {
		printf("error odel_vob:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}
	err = del_fil(NULL, &test_lnk, 0);
	if (err < 0) {
		printf("error del_fil:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}

	return result;
}

/* test_cache_ngwordinfo_3 */

LOCAL UNITTEST_RESULT test_cache_ngwordinfo_3()
{
	LINK test_lnk;
	W fd, err;
	VID vid;
	datcache_t *cache;
	UB ngword1[] = "yZXmy7Om0", ngword2[] = "GCQJ44Ao", ngword3[] = "V.jwWEDO";
	TC ngword1_tc[9], ngword2_tc[8], ngword3_tc[8];
	W ngword1_len = strlen(ngword1), ngword2_len = strlen(ngword2), ngword3_len = strlen(ngword3);
	Bool found;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	sjstotcs(ngword1_tc, ngword1);
	sjstotcs(ngword2_tc, ngword2);
	sjstotcs(ngword3_tc, ngword3);

	fd = test_cache_util_gen_file(&test_lnk, &vid);
	if (fd < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	cache = datcache_new(vid);

	err = datcache_appendngword(cache, ngword1_tc, ngword1_len);
	if (err < 0) {
		printf("datcache_appendngword fail\n");
		result = UNITTEST_RESULT_FAIL;
	}

	err = datcache_writefile(cache);
	if (err < 0) {
		printf("datcache_writefile error\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	cache = datcache_new(vid);

	found = datcache_checkngwordexist(cache, ngword1_tc, ngword1_len);
	if (found != True) {
		printf("datcache_checkngwordexist 1 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	found = datcache_checkngwordexist(cache, ngword2_tc, ngword2_len);
	if (found != False) {
		printf("datcache_checkngwordexist 2 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	found = datcache_checkngwordexist(cache, ngword3_tc, ngword3_len);
	if (found != False) {
		printf("datcache_checkngwordexist 3 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	err = odel_vob(vid, 0);
	if (err < 0) {
		printf("error odel_vob:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}
	err = del_fil(NULL, &test_lnk, 0);
	if (err < 0) {
		printf("error del_fil:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}

	return result;
}

/* test_cache_ngwordinfo_4 */

LOCAL UNITTEST_RESULT test_cache_ngwordinfo_4()
{
	LINK test_lnk;
	W fd, err;
	VID vid;
	datcache_t *cache;
	UB ngword1[] = "yZXmy7Om0", ngword2[] = "GCQJ44Ao", ngword3[] = "V.jwWEDO";
	TC ngword1_tc[9], ngword2_tc[8], ngword3_tc[8];
	W ngword1_len = strlen(ngword1), ngword2_len = strlen(ngword2), ngword3_len = strlen(ngword3);
	Bool found;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	sjstotcs(ngword1_tc, ngword1);
	sjstotcs(ngword2_tc, ngword2);
	sjstotcs(ngword3_tc, ngword3);

	fd = test_cache_util_gen_file(&test_lnk, &vid);
	if (fd < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	cache = datcache_new(vid);

	err = datcache_writefile(cache);
	if (err < 0) {
		printf("datcache_writefile error\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	cache = datcache_new(vid);

	found = datcache_checkngwordexist(cache, ngword1_tc, ngword1_len);
	if (found != False) {
		printf("datcache_checkngwordexist 1 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	found = datcache_checkngwordexist(cache, ngword2_tc, ngword2_len);
	if (found != False) {
		printf("datcache_checkngwordexist 2 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	found = datcache_checkngwordexist(cache, ngword3_tc, ngword3_len);
	if (found != False) {
		printf("datcache_checkngwordexist 3 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	err = odel_vob(vid, 0);
	if (err < 0) {
		printf("error odel_vob:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}
	err = del_fil(NULL, &test_lnk, 0);
	if (err < 0) {
		printf("error del_fil:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}

	return result;
}

/* test_cache_ngwordinfo_5 */

LOCAL UNITTEST_RESULT test_cache_ngwordinfo_5()
{
	LINK test_lnk;
	W fd, err;
	VID vid;
	datcache_t *cache;
	UB ngword1[] = "yZXmy7Om0", ngword2[] = "GCQJ44Ao", ngword3[] = "V.jwWEDO";
	TC ngword1_tc[9], ngword2_tc[8], ngword3_tc[8];
	W ngword1_len = strlen(ngword1), ngword2_len = strlen(ngword2), ngword3_len = strlen(ngword3);
	Bool found;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	sjstotcs(ngword1_tc, ngword1);
	sjstotcs(ngword2_tc, ngword2);
	sjstotcs(ngword3_tc, ngword3);

	fd = test_cache_util_gen_file(&test_lnk, &vid);
	if (fd < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	cache = datcache_new(vid);

	err = datcache_appendngword(cache, ngword1_tc, ngword1_len);
	if (err < 0) {
		printf("datcache_appendngword fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	err = datcache_appendngword(cache, ngword2_tc, ngword2_len);
	if (err < 0) {
		printf("datcache_appendngword fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	err = datcache_appendngword(cache, ngword3_tc, ngword3_len);
	if (err < 0) {
		printf("datcache_appendngword fail\n");
		result = UNITTEST_RESULT_FAIL;
	}

	err = datcache_writefile(cache);
	if (err < 0) {
		printf("datcache_writefile error\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	cache = datcache_new(vid);
	datcache_removengword(cache, ngword3_tc, ngword3_len);
	err = datcache_writefile(cache);
	if (err < 0) {
		printf("datcache_writefile error\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}
	datcache_delete(cache);

	cache = datcache_new(vid);

	found = datcache_checkngwordexist(cache, ngword1_tc, ngword1_len);
	if (found != True) {
		printf("datcache_checkngwordexist 1 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	found = datcache_checkngwordexist(cache, ngword2_tc, ngword2_len);
	if (found != True) {
		printf("datcache_checkngwordexist 2 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	found = datcache_checkngwordexist(cache, ngword3_tc, ngword3_len);
	if (found != False) {
		printf("datcache_checkngwordexist 3 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	err = odel_vob(vid, 0);
	if (err < 0) {
		printf("error odel_vob:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}
	err = del_fil(NULL, &test_lnk, 0);
	if (err < 0) {
		printf("error del_fil:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}

	return result;
}

/* test_cache_ngwordinfo_6 */

LOCAL UNITTEST_RESULT test_cache_ngwordinfo_6()
{
	LINK test_lnk;
	W fd, err;
	VID vid;
	datcache_t *cache;
	UB ngword1[] = "yZXmy7Om0", ngword2[] = "GCQJ44Ao", ngword3[] = "V.jwWEDO";
	TC ngword1_tc[9], ngword2_tc[8], ngword3_tc[8];
	W ngword1_len = strlen(ngword1), ngword2_len = strlen(ngword2), ngword3_len = strlen(ngword3);
	Bool found;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	sjstotcs(ngword1_tc, ngword1);
	sjstotcs(ngword2_tc, ngword2);
	sjstotcs(ngword3_tc, ngword3);

	fd = test_cache_util_gen_file(&test_lnk, &vid);
	if (fd < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	cache = datcache_new(vid);
	err = datcache_appendngword(cache, ngword1_tc, ngword1_len);
	if (err < 0) {
		printf("datcache_appendngword fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	err = datcache_appendngword(cache, ngword2_tc, ngword2_len);
	if (err < 0) {
		printf("datcache_appendngword fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	err = datcache_appendngword(cache, ngword3_tc, ngword3_len);
	if (err < 0) {
		printf("datcache_appendngword fail\n");
		result = UNITTEST_RESULT_FAIL;
	}

	err = datcache_writefile(cache);
	if (err < 0) {
		printf("datcache_writefile error\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	cache = datcache_new(vid);
	datcache_removengword(cache, ngword2_tc, ngword2_len);
	datcache_removengword(cache, ngword3_tc, ngword3_len);
	err = datcache_writefile(cache);
	if (err < 0) {
		printf("datcache_writefile error\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}
	datcache_delete(cache);

	cache = datcache_new(vid);

	found = datcache_checkngwordexist(cache, ngword1_tc, ngword1_len);
	if (found != True) {
		printf("datcache_checkngwordexist 1 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	found = datcache_checkngwordexist(cache, ngword2_tc, ngword2_len);
	if (found != False) {
		printf("datcache_checkngwordexist 2 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	found = datcache_checkngwordexist(cache, ngword3_tc, ngword3_len);
	if (found != False) {
		printf("datcache_checkngwordexist 3 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	err = odel_vob(vid, 0);
	if (err < 0) {
		printf("error odel_vob:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}
	err = del_fil(NULL, &test_lnk, 0);
	if (err < 0) {
		printf("error del_fil:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}

	return result;
}

/* test_cache_ngwordinfo_7 */

LOCAL UNITTEST_RESULT test_cache_ngwordinfo_7()
{
	LINK test_lnk;
	W fd, err;
	VID vid;
	datcache_t *cache;
	UB ngword1[] = "yZXmy7Om0", ngword2[] = "GCQJ44Ao", ngword3[] = "V.jwWEDO";
	TC ngword1_tc[9], ngword2_tc[8], ngword3_tc[8];
	W ngword1_len = strlen(ngword1), ngword2_len = strlen(ngword2), ngword3_len = strlen(ngword3);
	Bool found;
	UNITTEST_RESULT result = UNITTEST_RESULT_PASS;

	sjstotcs(ngword1_tc, ngword1);
	sjstotcs(ngword2_tc, ngword2);
	sjstotcs(ngword3_tc, ngword3);

	fd = test_cache_util_gen_file(&test_lnk, &vid);
	if (fd < 0) {
		return UNITTEST_RESULT_FAIL;
	}
	cls_fil(fd);

	cache = datcache_new(vid);
	err = datcache_appendngword(cache, ngword1_tc, ngword1_len);
	if (err < 0) {
		printf("datcache_appendngword fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	err = datcache_appendngword(cache, ngword2_tc, ngword2_len);
	if (err < 0) {
		printf("datcache_appendngword fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	err = datcache_appendngword(cache, ngword3_tc, ngword3_len);
	if (err < 0) {
		printf("datcache_appendngword fail\n");
		result = UNITTEST_RESULT_FAIL;
	}

	err = datcache_writefile(cache);
	if (err < 0) {
		printf("datcache_writefile error\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	cache = datcache_new(vid);
	datcache_removengword(cache, ngword1_tc, ngword1_len);
	datcache_removengword(cache, ngword2_tc, ngword2_len);
	datcache_removengword(cache, ngword3_tc, ngword3_len);
	err = datcache_writefile(cache);
	if (err < 0) {
		printf("datcache_writefile error\n");
		datcache_delete(cache);
		return UNITTEST_RESULT_FAIL;
	}
	datcache_delete(cache);

	cache = datcache_new(vid);

	found = datcache_checkngwordexist(cache, ngword1_tc, ngword1_len);
	if (found != False) {
		printf("datcache_checkngwordexist 1 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	found = datcache_checkngwordexist(cache, ngword2_tc, ngword2_len);
	if (found != False) {
		printf("datcache_checkngwordexist 2 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}
	found = datcache_checkngwordexist(cache, ngword3_tc, ngword3_len);
	if (found != False) {
		printf("datcache_checkngwordexist 3 fail\n");
		result = UNITTEST_RESULT_FAIL;
	}

	datcache_delete(cache);

	err = odel_vob(vid, 0);
	if (err < 0) {
		printf("error odel_vob:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}
	err = del_fil(NULL, &test_lnk, 0);
	if (err < 0) {
		printf("error del_fil:%d\n", err >> 16);
		result = UNITTEST_RESULT_FAIL;
	}

	return result;
}

IMPORT VOID test_cache_main(unittest_driver_t *driver)
{
	UNITTEST_DRIVER_REGIST(driver, test_cache_1);
	UNITTEST_DRIVER_REGIST(driver, test_cache_2);
	UNITTEST_DRIVER_REGIST(driver, test_cache_3);
	UNITTEST_DRIVER_REGIST(driver, test_cache_4);
	UNITTEST_DRIVER_REGIST(driver, test_cache_5);
	UNITTEST_DRIVER_REGIST(driver, test_cache_6);
	UNITTEST_DRIVER_REGIST(driver, test_cache_7);
	UNITTEST_DRIVER_REGIST(driver, test_cache_8);
	UNITTEST_DRIVER_REGIST(driver, test_cache_9);
	UNITTEST_DRIVER_REGIST(driver, test_cache_10);
	UNITTEST_DRIVER_REGIST(driver, test_cache_11);
	UNITTEST_DRIVER_REGIST(driver, test_cache_12);
	UNITTEST_DRIVER_REGIST(driver, test_cache_13);
	UNITTEST_DRIVER_REGIST(driver, test_cache_14);
	UNITTEST_DRIVER_REGIST(driver, test_cache_15);
	UNITTEST_DRIVER_REGIST(driver, test_cache_16);
	UNITTEST_DRIVER_REGIST(driver, test_cache_residinfo_1);
	UNITTEST_DRIVER_REGIST(driver, test_cache_residinfo_2);
	UNITTEST_DRIVER_REGIST(driver, test_cache_residinfo_3);
	UNITTEST_DRIVER_REGIST(driver, test_cache_residinfo_4);
	UNITTEST_DRIVER_REGIST(driver, test_cache_residinfo_5);
	UNITTEST_DRIVER_REGIST(driver, test_cache_residinfo_6);
	UNITTEST_DRIVER_REGIST(driver, test_cache_residinfo_7);
	UNITTEST_DRIVER_REGIST(driver, test_cache_resindexinfo_1);
	UNITTEST_DRIVER_REGIST(driver, test_cache_resindexinfo_2);
	UNITTEST_DRIVER_REGIST(driver, test_cache_resindexinfo_3);
	UNITTEST_DRIVER_REGIST(driver, test_cache_resindexinfo_4);
	UNITTEST_DRIVER_REGIST(driver, test_cache_resindexinfo_5);
	UNITTEST_DRIVER_REGIST(driver, test_cache_resindexinfo_6);
	UNITTEST_DRIVER_REGIST(driver, test_cache_resindexinfo_7);
	UNITTEST_DRIVER_REGIST(driver, test_cache_ngwordinfo_1);
	UNITTEST_DRIVER_REGIST(driver, test_cache_ngwordinfo_2);
	UNITTEST_DRIVER_REGIST(driver, test_cache_ngwordinfo_3);
	UNITTEST_DRIVER_REGIST(driver, test_cache_ngwordinfo_4);
	UNITTEST_DRIVER_REGIST(driver, test_cache_ngwordinfo_5);
	UNITTEST_DRIVER_REGIST(driver, test_cache_ngwordinfo_6);
	UNITTEST_DRIVER_REGIST(driver, test_cache_ngwordinfo_7);
	UNITTEST_DRIVER_REGIST(driver, test_cache_append_1);
	UNITTEST_DRIVER_REGIST(driver, test_cache_append_2);
	UNITTEST_DRIVER_REGIST(driver, test_cache_append_3);
	UNITTEST_DRIVER_REGIST(driver, test_cache_append_4);
	UNITTEST_DRIVER_REGIST(driver, test_cache_append_5);
	UNITTEST_DRIVER_REGIST(driver, test_cache_append_6);
	UNITTEST_DRIVER_REGIST(driver, test_cache_append_7);
	UNITTEST_DRIVER_REGIST(driver, test_cache_append_8);
	UNITTEST_DRIVER_REGIST(driver, test_cache_append_9);
	UNITTEST_DRIVER_REGIST(driver, test_cache_reload_1);
	UNITTEST_DRIVER_REGIST(driver, test_cache_reload_2);
	UNITTEST_DRIVER_REGIST(driver, test_cache_reload_3);
	UNITTEST_DRIVER_REGIST(driver, test_cache_reload_4);
	UNITTEST_DRIVER_REGIST(driver, test_cache_reload_5);
	UNITTEST_DRIVER_REGIST(driver, test_cache_reload_6);
	UNITTEST_DRIVER_REGIST(driver, test_cache_reload_7);
	UNITTEST_DRIVER_REGIST(driver, test_cache_reload_8);
	UNITTEST_DRIVER_REGIST(driver, test_cache_reload_9);
}
