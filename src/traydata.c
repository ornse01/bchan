/*
 * traydata.c
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

#include    "traydata.h"
#include    "layoutarray.h"
#include    "tadlib.h"

#include	<basic.h>
#include	<bstdlib.h>
#include	<bstdio.h>
#include	<tcode.h>

#ifdef BCHAN_CONFIG_DEBUG
# define DP(arg) printf arg
# define DP_ER(msg, err) printf("%s (%d/%x)\n", msg, err>>16, err)
#else
# define DP(arg) /**/
# define DP_ER(msg, err) /**/
#endif

struct dattraydata_t_ {
	datlayoutarray_t *layoutarray;
};

LOCAL W datlayout_res_totraytextdata_write_zenkaku(TC *str)
{
	TADSEG *seg = (TADSEG*)str;

	if (str == NULL) {
		return 10;
	}

	seg->id = 0xFF00|TS_TFONT;
	seg->len = 6;
	*(UH*)(str + 2) = 3 << 8;
	*(RATIO*)(str + 3) = 0x0101;
	*(RATIO*)(str + 4) = 0x0101;

	return 10;
}

LOCAL W datlayout_res_totraytextdata(datlayout_res_t *res, B *data, W data_len)
{
	W i = 0, num;
	TC *str = (TC*)data;

	if (data != NULL) {
		num = res->index + 1;
		i += datlayout_res_totraytextdata_write_zenkaku(str + i) / 2;
		i += tadlib_UW_to_str(num, str + i, data_len - i / 2);
		str[i++] = TK_KSP;
		i += tadlib_remove_TA_APPL(res->parser_res->name, res->parser_res->name_len, str + i, data_len - i / 2) / 2;
		str[i++] = TK_KSP;
		i += datlayout_res_totraytextdata_write_zenkaku(str + i) / 2;
		str[i++] = TK_LABR;
		i += tadlib_remove_TA_APPL(res->parser_res->mail, res->parser_res->mail_len, str + i, data_len - i / 2) / 2;
		i += datlayout_res_totraytextdata_write_zenkaku(str + i) / 2;
		str[i++] = TK_RABR;
		str[i++] = TK_KSP;
		i += tadlib_remove_TA_APPL(res->parser_res->date, res->parser_res->date_len, str + i, data_len - i / 2) / 2;
		str[i++] = TK_NL;
		i += datlayout_res_totraytextdata_write_zenkaku(str + i) / 2;
		i += tadlib_remove_TA_APPL(res->parser_res->body, res->parser_res->body_len, str + i, data_len - i / 2) / 2;
		str[i++] = TK_NL;
	} else {
		num = res->index + 1;
		i += datlayout_res_totraytextdata_write_zenkaku(NULL) / 2;
		i += tadlib_UW_to_str(num, NULL, 0);
		i++; /* TK_KSP */
		i += tadlib_remove_TA_APPL_calcsize(res->parser_res->name, res->parser_res->name_len) / 2;
		i++; /* TK_KSP */
		i += datlayout_res_totraytextdata_write_zenkaku(NULL) / 2;
		i++; /* TO_LABR */
		i += tadlib_remove_TA_APPL_calcsize(res->parser_res->mail, res->parser_res->mail_len) / 2;
		i += datlayout_res_totraytextdata_write_zenkaku(NULL) / 2;
		i++; /* TO_RABR */
		i++; /* TK_KSP */
		i += tadlib_remove_TA_APPL_calcsize(res->parser_res->date, res->parser_res->date_len) / 2;
		i++; /* TK_NL */
		i += datlayout_res_totraytextdata_write_zenkaku(NULL) / 2;
		i += tadlib_remove_TA_APPL_calcsize(res->parser_res->body, res->parser_res->body_len) / 2;
		i++; /* TK_NL */
	}

	return i * sizeof(TC);
}

EXPORT W dattraydata_resindextotraytextdata(dattraydata_t *traydata, W n, B *data, W data_len)
{
	datlayout_res_t *res;
	Bool exist;
	exist = datlayoutarray_getresbyindex(traydata->layoutarray, n, &res);
	if (exist == False) {
		return -1; /* TODO */
	}
	return datlayout_res_totraytextdata(res, data, data_len);
}

EXPORT W dattraydata_idtotraytextdata(dattraydata_t *traydata, TC *id, W id_len, B *data, W data_len)
{
	W i, len, result, sum = 0;
	Bool haveid;
	datlayout_res_t *res;

	len = datlayoutarray_length(traydata->layoutarray);
	for (i = 0; i < len; i++) {
		datlayoutarray_getresbyindex(traydata->layoutarray, i, &res);

		haveid = datlayout_res_issameid(res, id, id_len);
		if (haveid == True) {
			if (data == NULL) {
				result = datlayout_res_totraytextdata(res, NULL, 0);
				if (result < 0) {
					return result;
				}
			} else {
				result = datlayout_res_totraytextdata(res, data + sum, data_len - sum);
				if (result < 0) {
					return result;
				}
			}
			sum += result;
		}
	}

	return sum;
}

EXPORT dattraydata_t* dattraydata_new(datlayoutarray_t *layoutarray)
{
	dattraydata_t *traydata;

	traydata = (dattraydata_t *)malloc(sizeof(dattraydata_t));
	if (traydata == NULL) {
		return NULL;
	}
	traydata->layoutarray = layoutarray;

	return traydata;
}

EXPORT VOID dattraydata_delete(dattraydata_t *traydata)
{
	free(traydata);
}
