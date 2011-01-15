/*
 * cache.c
 *
 * Copyright (c) 2009-2011 project bchan
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

#include	<basic.h>
#include	<bstdlib.h>
#include	<bstdio.h>
#include	<bstring.h>
#include	<errcode.h>
#include	<btron/btron.h>
#include	<bsys/queue.h>

#include    "cache.h"
#include    "residhash.h"
#include    "resindexhash.h"
#include    "wordlist.h"

typedef struct datcache_data_t_ datcache_data_t;

struct datcache_data_t_ {
	QUEUE queue;
	UB *data;
	W len;
};

#define DATCACHE_FLAG_DATAPPENDED 0x00000001
#define DATCACHE_FLAG_DATRESETED 0x00000002
#define DATCACHE_FLAG_IDINFOUPDATED 0x00000004
#define DATCACHE_FLAG_INDEXINFOUPDATED 0x00000008
#define DATCACHE_FLAG_LATESTHEADERUPDATED 0x00000010
#define DATCACHE_FLAG_NGWORDINFOUPDATED 0x00000020

struct datcache_t_ {
	UW flag;
	W fd;
	STIME mtime_open;
	ID semid;
	datcache_data_t datalist;
	W s_datsize;
	W recsize_open;
	datcache_datareadcontext_t *context;
	UB *retrinfo;
	W retrinfo_len;
	UB *host;
	W host_len;
	UB *board;
	W board_len;
	UB *thread;
	W thread_len;
	UB *latestheader;
	W latestheader_len;
	residhash_t residhash;
	resindexhash_t resindexhash;
	wordlist_t ngwordlist;
};

struct datcache_datareadcontext_t_ {
	datcache_t *datcache;
	datcache_data_t *current;
	W index;
};

struct datcache_ngwordreadcontext_t_ {
	wordlist_iterator_t iter;
};

LOCAL datcache_data_t* datcache_data_next(datcache_data_t *data)
{
	return (datcache_data_t*)data->queue.next;
}

LOCAL datcache_data_t* datcache_data_new(UB *data, W len)
{
	datcache_data_t *cache_data;

	cache_data = malloc(sizeof(datcache_data_t));
	if (cache_data == NULL) {
		return NULL;
	}
	cache_data->data = malloc(sizeof(UB)*len);
	if (cache_data->data == NULL) {
		free(cache_data);
		return NULL;
	}
	memcpy(cache_data->data, data, len);
	cache_data->len = len;

	return cache_data;
}

LOCAL VOID datcache_data_delete(datcache_data_t *cache_data)
{
	QueRemove(&(cache_data->queue));
	if (cache_data->data != NULL) {
		free(cache_data->data);
	}
	free(cache_data);
}

LOCAL VOID datcache_setappendedflag(datcache_t *cache)
{
	cache->flag = cache->flag | DATCACHE_FLAG_DATAPPENDED;
}

LOCAL VOID datcache_clearappendedflag(datcache_t *cache)
{
	cache->flag = cache->flag & ~DATCACHE_FLAG_DATAPPENDED;
}

LOCAL Bool datcache_issetappendedflag(datcache_t *cache)
{
	if ((cache->flag & DATCACHE_FLAG_DATAPPENDED) == 0) {
		return False;
	}
	return True;
}

LOCAL VOID datcache_setresetedflag(datcache_t *cache)
{
	cache->flag = cache->flag | DATCACHE_FLAG_DATRESETED;
}

LOCAL Bool datcache_issetresetedflag(datcache_t *cache)
{
	if ((cache->flag & DATCACHE_FLAG_DATRESETED) == 0) {
		return False;
	}
	return True;
}

LOCAL VOID datcache_setidinfoupdatedflag(datcache_t *cache)
{
	cache->flag = cache->flag | DATCACHE_FLAG_IDINFOUPDATED;
}

LOCAL Bool datcache_issetidinfoupdatedflag(datcache_t *cache)
{
	if ((cache->flag & DATCACHE_FLAG_IDINFOUPDATED) == 0) {
		return False;
	}
	return True;
}

LOCAL VOID datcache_setindexinfoupdatedflag(datcache_t *cache)
{
	cache->flag = cache->flag | DATCACHE_FLAG_INDEXINFOUPDATED;
}

LOCAL Bool datcache_issetindexinfoupdatedflag(datcache_t *cache)
{
	if ((cache->flag & DATCACHE_FLAG_INDEXINFOUPDATED) == 0) {
		return False;
	}
	return True;
}

LOCAL VOID datcache_setngwordinfoupdatedflag(datcache_t *cache)
{
	cache->flag = cache->flag | DATCACHE_FLAG_NGWORDINFOUPDATED;
}

LOCAL Bool datcache_issetngwordinfoupdatedflag(datcache_t *cache)
{
	if ((cache->flag & DATCACHE_FLAG_NGWORDINFOUPDATED) == 0) {
		return False;
	}
	return True;
}

LOCAL VOID datcache_setlatestheaderupdatedflag(datcache_t *cache)
{
	cache->flag = cache->flag | DATCACHE_FLAG_LATESTHEADERUPDATED;
}

LOCAL Bool datcache_issetlatestheaderupdatedflag(datcache_t *cache)
{
	if ((cache->flag & DATCACHE_FLAG_LATESTHEADERUPDATED) == 0) {
		return False;
	}
	return True;
}

EXPORT W datcache_appenddata(datcache_t *cache, UB *data, W len)
{
	datcache_data_t *cache_data;
	W err;

	err = wai_sem(cache->semid, T_FOREVER);
	if (err < 0) {
		return err;
	}

	if (cache->context != NULL) {
		sig_sem(cache->semid);
		return -1;
	}

	cache_data = datcache_data_new(data, len);
	if (cache_data == NULL) {
		sig_sem(cache->semid);
		return -1; /* TODO */
	}
	QueInsert(&(cache_data->queue), &(cache->datalist.queue));
	cache->s_datsize += len;

	datcache_setappendedflag(cache);

	sig_sem(cache->semid);

	return 0; /* TODO */
}

EXPORT VOID datcache_cleardata(datcache_t *cache)
{
	datcache_data_t *cache_data;
	Bool ok;
	W err;

	err = wai_sem(cache->semid, T_FOREVER);
	if (err < 0) {
		return;
	}

	if (cache->context != NULL) {
		sig_sem(cache->semid);
		return;
	}

	for (;;) {
		ok = isQueEmpty(&(cache->datalist.queue));
		if (ok == True) {
			break;
		}
		cache_data = (datcache_data_t*)cache->datalist.queue.next;
		datcache_data_delete(cache_data);
	}
	free(cache->datalist.data);

	cache->datalist.data = NULL;
	cache->datalist.len = 0;
	cache->s_datsize = 0;

	datcache_clearappendedflag(cache);
	datcache_setresetedflag(cache);

	sig_sem(cache->semid);
}

EXPORT Bool datcache_datareadcontext_nextdata(datcache_datareadcontext_t *context, UB **bin, W *len)
{
	datcache_data_t *next;

	if (context->current == NULL) {
		return False;
	}

	*bin = context->current->data + context->index;
	*len = context->current->len - context->index;

	next = datcache_data_next(context->current);
	if (next == &(context->datcache->datalist)) {
		next = NULL;
	}
	context->current = next;
	context->index = 0;

	return True;
}

LOCAL datcache_datareadcontext_t* datcache_datareadcontext_new(datcache_t *cache)
{
	datcache_datareadcontext_t *context;

	context = malloc(sizeof(datcache_datareadcontext_t*));
	if (context == NULL) {
		return NULL;
	}
	context->datcache = cache;

	return context;
}

LOCAL VOID datcache_datareadcontext_delete(datcache_datareadcontext_t *context)
{
	free(context);
}

EXPORT datcache_datareadcontext_t* datcache_startdataread(datcache_t *cache, W start)
{
	datcache_datareadcontext_t *context;
	datcache_data_t *cache_data;
	W err,dest;

	err = wai_sem(cache->semid, T_FOREVER);
	if (err < 0) {
		return NULL;
	}

	if (cache->context != NULL) {
		sig_sem(cache->semid);
		return NULL;
	}

	context = datcache_datareadcontext_new(cache);
	if (context == NULL) {
		return NULL;
	}
	cache->context = context;

	if (start >= cache->s_datsize) {
		context->current = NULL;
		context->index = 0;
		return context;
	}

	cache_data = &(cache->datalist);
	dest = start;
	for (;;) {
		if (dest < cache_data->len) {
			break;
		}
		dest -= cache_data->len;
		cache_data = datcache_data_next(cache_data);
	}

	context->current = cache_data;
	context->index = dest;

	return context;
}

EXPORT VOID datcache_enddataread(datcache_t *cache, datcache_datareadcontext_t *context)
{
	cache->context = NULL;
	datcache_datareadcontext_delete(context);

	sig_sem(cache->semid);
}

EXPORT VOID datcache_getlatestheader(datcache_t *cache, UB **header, W *len)
{
	*header = cache->latestheader;
	*len = cache->latestheader_len;
}

EXPORT W datcache_updatelatestheader(datcache_t *cache, UB *header, W len)
{
	UB *latestheader0;

	latestheader0 = realloc(cache->latestheader, len + 1);
	if (latestheader0 == NULL) {
		return -1;
	}

	cache->latestheader = latestheader0;
	memcpy(cache->latestheader, header, len);
	cache->latestheader_len = len;
	cache->latestheader[cache->latestheader_len] = '\0';

	datcache_setlatestheaderupdatedflag(cache);

	return 0;
}

EXPORT VOID datcache_gethost(datcache_t *cache, UB **host, W *len)
{
	*host = cache->host;
	*len = cache->host_len;
}

EXPORT VOID datcache_getborad(datcache_t *cache, UB **borad, W *len)
{
	*borad = cache->board;
	*len = cache->board_len;
}

EXPORT VOID datcache_getthread(datcache_t *cache, UB **thread, W *len)
{
	*thread = cache->thread;
	*len = cache->thread_len;
}

LOCAL VOID datcache_setupretrinfo(datcache_t *cache, UB *retrinfo, W len)
{
	W i=0;

	cache->retrinfo = retrinfo;
	cache->retrinfo_len = len;
	cache->host = NULL;
	cache->host_len = 0;
	cache->board = NULL;
	cache->board_len = 0;
	cache->thread = NULL;
	cache->thread_len = 0;

	if (cache->retrinfo == NULL) {
		return;
	}

	cache->host = cache->retrinfo;
	for (; i < len; i++) {
		if (cache->retrinfo[i] == '\n') {
			break;
		}
		cache->host_len++;
	}

	i++;
	cache->board = cache->retrinfo + i;
	for (; i < len; i++) {
		if (cache->retrinfo[i] == '\n') {
			break;
		}
		cache->board_len++;
	}

	i++;
	cache->thread = cache->retrinfo + i;
	for (; i < len; i++) {
		if (cache->retrinfo[i] == '\n') {
			break;
		}
		cache->thread_len++;
	}
}

LOCAL W datcache_preparerec_forwritefile(W fd, W rectype, W subtype, Bool need_truncute)
{
	W err;

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
		return -1; /* TODO */
	} else {
		if (need_truncute == True) {
			err = trc_rec(fd, 0);
			if (err < 0) {
				return -1;
			}
		}
	}

	return 0; /* TODO */
}

LOCAL W datcache_deleterec_forwritefile(W fd, W rectype, W subtype)
{
	W err;

	err = fnd_rec(fd, F_TOPEND, 1 << rectype, subtype, NULL);
	if (err == ER_REC) {
		return 0; /* TODO */
	} else if (err < 0) {
		return -1; /* TODO */
	}

	err = del_rec(fd);
	if (err < 0) {
		return -1; /* TODO */
	}

	return 0; /* TODO */
}

EXPORT W datcache_datasize(datcache_t *cache)
{
	return cache->s_datsize;
}

LOCAL W datcache_writefile_latestheader(datcache_t *cache)
{
	W err;

	if (cache->latestheader_len <= 0) {
		return 0;
	}

	err = datcache_preparerec_forwritefile(cache->fd, DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_HEADER, True);
	if (err < 0) {
		return err;
	}
	err = wri_rec(cache->fd, -1, cache->latestheader, cache->latestheader_len, NULL, NULL, 0);
	if (err < 0) {
		return err;
	}

	return 0;
}

LOCAL W datcache_writefile_residhash(datcache_t *cache)
{
	Bool cont, updated;
	W err, len;
	residhash_iterator_t iter;
	TC *idstr;
	W idstr_len;
	UW attr;
	COLOR color;
	UB bin[10];

	updated = datcache_issetidinfoupdatedflag(cache);
	if (updated == False) {
		return 0;
	}

	len = residhash_datanum(&cache->residhash);
	if (len > 0) {
		err = datcache_preparerec_forwritefile(cache->fd, DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_RESIDINFO, True);
		if (err < 0) {
			return err;
		}
		residhash_iterator_initialize(&iter, &cache->residhash);
		for (;;) {
			cont = residhash_iterator_next(&iter, &idstr, &idstr_len, &attr, &color);
			if (cont == False) {
				break;
			}
			*(UH*)bin = idstr_len * 2;
			err = wri_rec(cache->fd, -1, bin, 2, NULL, NULL, 0);
			if (err < 0) {
				return err;
			}
			err = wri_rec(cache->fd, -1, (UB*)idstr, idstr_len * 2, NULL, NULL, 0);
			if (err < 0) {
				return err;
			}
			*(UH*)bin = 8;
			*(UW*)(bin + 2) = attr;
			*(COLOR*)(bin + 2 + 4) = color;
			err = wri_rec(cache->fd, -1, bin, 10, NULL, NULL, 0);
			if (err < 0) {
				return err;
			}
		}
		residhash_iterator_finalize(&iter);
	} else {
		err = datcache_deleterec_forwritefile(cache->fd, DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_RESIDINFO);
		if (err < 0) {
			return err;
		}
	}

	return 0;
}

LOCAL W datcache_writefile_resindexhash(datcache_t *cache)
{
	Bool cont, updated;
	W err, len, index;
	resindexhash_iterator_t iter;
	UW attr;
	COLOR color;
	UB bin[10];

	updated = datcache_issetindexinfoupdatedflag(cache);
	if (updated == False) {
		return 0;
	}

	len = resindexhash_datanum(&cache->resindexhash);
	if (len > 0) {
		err = datcache_preparerec_forwritefile(cache->fd, DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_RESINDEXINFO, True);
		if (err < 0) {
			return err;
		}
		resindexhash_iterator_initialize(&iter, &cache->resindexhash);
		for (;;) {
			cont = resindexhash_iterator_next(&iter, &index, &attr, &color);
			if (cont == False) {
				break;
			}
			if (index > 65535) {
				continue;
			}
			*(UH*)bin = 2;
			err = wri_rec(cache->fd, -1, bin, 2, NULL, NULL, 0);
			if (err < 0) {
				return err;
			}
			*(UH*)bin = index & 0xFFFF;
			err = wri_rec(cache->fd, -1, bin, 2, NULL, NULL, 0);
			if (err < 0) {
				return err;
			}
			*(UH*)bin = 8;
			*(UW*)(bin + 2) = attr;
			*(COLOR*)(bin + 2 + 4) = color;
			err = wri_rec(cache->fd, -1, bin, 10, NULL, NULL, 0);
			if (err < 0) {
				return err;
			}
		}
		resindexhash_iterator_finalize(&iter);
	} else {
		err = datcache_deleterec_forwritefile(cache->fd, DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_RESINDEXINFO);
		if (err < 0) {
			return err;
		}
	}

	return 0;
}

LOCAL W datcache_writefile_ngwordlist(datcache_t *cache)
{
	Bool cont, updated, empty;
	W err;
	wordlist_iterator_t iter;
	TC *str;
	W str_len;
	UB bin[10];

	updated = datcache_issetngwordinfoupdatedflag(cache);
	if (updated == False) {
		return 0;
	}

	empty = wordlist_isempty(&cache->ngwordlist);
	if (empty == False) {
		err = datcache_preparerec_forwritefile(cache->fd, DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_NGWORDINFO, True);
		if (err < 0) {
			wordlist_iterator_finalize(&iter);
			return err;
		}

		wordlist_iterator_initialize(&iter, &cache->ngwordlist);
		for (;;) {
			cont = wordlist_iterator_next(&iter, &str, &str_len);
			if (cont == False) {
				break;
			}

			*(UH*)bin = str_len * 2;
			err = wri_rec(cache->fd, -1, bin, 2, NULL, NULL, 0);
			if (err < 0) {
				return err;
			}
			err = wri_rec(cache->fd, -1, (UB*)str, str_len * 2, NULL, NULL, 0);
			if (err < 0) {
				return err;
			}
			*(UH*)bin = 2;
			*(UH*)(bin + 2) = 0;
			err = wri_rec(cache->fd, -1, bin, 4, NULL, NULL, 0);
			if (err < 0) {
				return err;
			}
		}
		wordlist_iterator_finalize(&iter);
	} else {
		err = datcache_deleterec_forwritefile(cache->fd, DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_NGWORDINFO);
		if (err < 0) {
			return err;
		}
	}

	return 0;
}

LOCAL W datcache_writedat_appended(datcache_t *cache)
{
	W err, size;
	F_STATE fstate;
	datcache_data_t *cache_data;

	err = datcache_preparerec_forwritefile(cache->fd, DATCACHE_RECORDTYPE_MAIN, 0, False);
	if (err < 0) {
		return err;
	}

	err = loc_rec(cache->fd, F_LOCK);
	if (err < 0) {
		return err;
	}

	err = ofl_sts(cache->fd, NULL, &fstate, NULL);
	if (err < 0) {
		loc_rec(cache->fd, F_UNLOCK);
		return err;
	}
	if (cache->mtime_open != fstate.f_mtime) {
		loc_rec(cache->fd, F_UNLOCK);
		return 0;
	}
	err = rea_rec(cache->fd, 0, NULL, 0, &size, NULL);
	if (err < 0) {
		loc_rec(cache->fd, F_UNLOCK);
		return err;
	}
	if (cache->recsize_open != size) {
		loc_rec(cache->fd, F_UNLOCK);
		return 0;
	}

	cache_data = datcache_data_next(&(cache->datalist));
	for (;;) {
		if (cache_data == &(cache->datalist)) {
			break;
		}

		err = wri_rec(cache->fd, -1, cache_data->data, cache_data->len, NULL, NULL, 0);
		if (err < 0) {
			loc_rec(cache->fd, F_UNLOCK);
			return err;
		}

		cache_data = datcache_data_next(cache_data);
	}

	loc_rec(cache->fd, F_UNLOCK);

	return 0;
}

LOCAL W datcache_writedat_allupdateded(datcache_t *cache)
{
	W err;
	datcache_data_t *cache_data;

	if (cache->s_datsize <= 0) {
		return 0;
	}

	err = datcache_preparerec_forwritefile(cache->fd, DATCACHE_RECORDTYPE_MAIN, 0, True);
	if (err < 0) {
		return err;
	}

	err = loc_rec(cache->fd, F_LOCK);
	if (err < 0) {
		return err;
	}

	cache_data = &(cache->datalist);
	for (;;) {
		err = wri_rec(cache->fd, -1, cache_data->data, cache_data->len, NULL, NULL, 0);
		if (err < 0) {
			loc_rec(cache->fd, F_UNLOCK);
			return err;
		}
		cache_data = datcache_data_next(cache_data);
		if (cache_data == &(cache->datalist)) {
			break;
		}
	}

	loc_rec(cache->fd, F_UNLOCK);

	return 0;
}

EXPORT W datcache_writefile(datcache_t *cache)
{
	W err;
	Bool appended, reseted, header_updated;

	appended = datcache_issetappendedflag(cache);
	reseted = datcache_issetresetedflag(cache);
	header_updated = datcache_issetlatestheaderupdatedflag(cache);

	if (appended == True) {
		if (reseted == True) {
			err = datcache_writedat_allupdateded(cache);
		} else {
			err = datcache_writedat_appended(cache);
		}
		if (err < 0) {
			return err;
		}
	}

	if (header_updated == True) {
		err = datcache_writefile_latestheader(cache);
		if (err < 0) {
			return err;
		}
	}

	err = datcache_writefile_residhash(cache);
	if (err < 0) {
		return err;
	}
	err = datcache_writefile_resindexhash(cache);
	if (err < 0) {
		return err;
	}
	err = datcache_writefile_ngwordlist(cache);
	if (err < 0) {
		return err;
	}

	return 0;
}

EXPORT W datcache_addresiddata(datcache_t *cache, TC *idstr, W idstr_len, UW attr, COLOR color)
{
	datcache_setidinfoupdatedflag(cache);
	return residhash_adddata(&cache->residhash, idstr, idstr_len, attr, color);
}

EXPORT W datcache_searchresiddata(datcache_t *cache, TC *idstr, W idstr_len, UW *attr, COLOR *color)
{
	return residhash_searchdata(&cache->residhash, idstr, idstr_len, attr, color);
}

EXPORT VOID datcache_removeresiddata(datcache_t *cache, TC *idstr, W idstr_len)
{
	datcache_setidinfoupdatedflag(cache);
	residhash_removedata(&cache->residhash, idstr, idstr_len);
}

EXPORT W datcache_addresindexdata(datcache_t *cache, W index, UW attr, COLOR color)
{
	datcache_setindexinfoupdatedflag(cache);
	return resindexhash_adddata(&cache->resindexhash, index, attr, color);
}

EXPORT W datcache_searchresindexdata(datcache_t *cache, W index, UW *attr, COLOR *color)
{
	return resindexhash_searchdata(&cache->resindexhash, index, attr, color);
}

EXPORT VOID datcache_removeresindexdata(datcache_t *cache, W index)
{
	datcache_setindexinfoupdatedflag(cache);
	resindexhash_removedata(&cache->resindexhash, index);
}

EXPORT W datcache_appendngword(datcache_t *cache, TC *str, W len)
{
	datcache_setngwordinfoupdatedflag(cache);
	return wordlist_appendword(&cache->ngwordlist, str, len);
}

EXPORT VOID datcache_removengword(datcache_t *cache, TC *str, W len)
{
	datcache_setngwordinfoupdatedflag(cache);
	wordlist_removeword(&cache->ngwordlist, str, len);
}

EXPORT Bool datcache_checkngwordexist(datcache_t *cache, TC *str, W len)
{
	return wordlist_checkexistbyword(&cache->ngwordlist, str, len);
}

EXPORT datcache_ngwordreadcontext_t* datcache_startngwordread(datcache_t *cache)
{
	datcache_ngwordreadcontext_t *context;

	context = malloc(sizeof(datcache_ngwordreadcontext_t));
	if (context == NULL) {
		return NULL;
	}
	wordlist_iterator_initialize(&context->iter, &cache->ngwordlist);
	return context;
}

EXPORT VOID datcache_endngwordread(datcache_t *cache, datcache_ngwordreadcontext_t *context)
{
	wordlist_iterator_finalize(&context->iter);
	free(context);
}

EXPORT Bool datcache_ngwordreadcontext_nextdata(datcache_ngwordreadcontext_t *context, TC **str, W *len)
{
	return wordlist_iterator_next(&context->iter, str, len);
}

LOCAL W datcache_getrec_fromfile(W fd, W rectype, W subtype, UB **data, W *size)
{
	W err, size0;
	UB *data0;

	err = fnd_rec(fd, F_TOPEND, 1 << rectype, subtype, NULL);
	if (err == ER_REC) {
		*data = NULL;
		*size = 0;
		return 0;
	}
	if (err < 0) {
		return -1; /* TODO */
	}
	err = rea_rec(fd, 0, NULL, 0, &size0, NULL);
	if (err < 0) {
		return -1; /* TODO */
	}
	if (size0 == 0) {
		*data = NULL;
		*size = 0;
		return 0;
	}
	data0 = malloc(size0);
	if (data0 == NULL) {
		return -1; /* TODO */
	}
	err = rea_rec(fd, 0, data0, size0, NULL, NULL);
	if (err < 0) {
		free(data0);
		return -1; /* TODO */
	}

	*data = data0;
	*size = size0;

	return 0;
}

LOCAL W datcache_getdat_fromfile(W fd, UB **data, W *size)
{
	return datcache_getrec_fromfile(fd, DATCACHE_RECORDTYPE_MAIN, 0, data, size);
}

LOCAL W datcache_getretrinfo_fromfile(W fd, UB **data, W *size)
{
	return datcache_getrec_fromfile(fd, DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_RETRIEVE, data, size);
}

LOCAL W datcache_getheader_fromfile(W fd, UB **data, W *size)
{
	return datcache_getrec_fromfile(fd, DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_HEADER, data, size);
}

LOCAL W datcache_readresidinfo(datcache_t *cache)
{
	UB *recdata;
	W recsize, err = 0, idstr_len, i;
	TC *idstr;
	UW attr;
	COLOR color;
	UH chunksize;

	err = datcache_getrec_fromfile(cache->fd, DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_RESIDINFO, &recdata, &recsize);
	if (err < 0) {
		return err;
	}

	if (recsize == 0) {
		return 0; /* TODO */
	}

	for (i = 0; i < recsize; ) {
		chunksize = *(UH*)(recdata + i);
		i += 2;
		idstr = (TC*)(recdata + i);
		idstr_len = chunksize / 2;
		i += chunksize;
		chunksize = *(UH*)(recdata + i);
		i += 2;
		if (chunksize >= 4) {
			attr = *(UW*)(recdata + i);
			if (chunksize >= 8) {
				color = *(COLOR*)(recdata + i + 4);
			} else {
				color = -1;
			}
			err = residhash_adddata(&cache->residhash, idstr, idstr_len, attr, color);
			if (err < 0) {
				break;
			}
		}
		i += chunksize;
	}

	free(recdata);

	return err;
}

LOCAL W datcache_readresindexinfo(datcache_t *cache)
{
	UB *recdata;
	W recsize, err = 0, i, index;
	UW attr;
	COLOR color;
	UH chunksize;

	err = datcache_getrec_fromfile(cache->fd, DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_RESINDEXINFO, &recdata, &recsize);
	if (err < 0) {
		return err;
	}

	if (recsize == 0) {
		return 0; /* TODO */
	}

	for (i = 0; i < recsize; ) {
		chunksize = *(UH*)(recdata + i);
		i += 2;
		index = *(UH*)(recdata + i);
		i += chunksize;
		chunksize = *(UH*)(recdata + i);
		i += 2;
		if (chunksize >= 4) {
			attr = *(UW*)(recdata + i);
			if (chunksize >= 8) {
				color = *(COLOR*)(recdata + i + 4);
			} else {
				color = -1;
			}
			err = resindexhash_adddata(&cache->resindexhash, index, attr, color);
			if (err < 0) {
				break;
			}
		}
		i += chunksize;
	}

	free(recdata);

	return err;
}

LOCAL W datcache_readngwordinfo(datcache_t *cache)
{
	UB *recdata;
	W recsize, err = 0, str_len, i;
	TC *str;
	UH chunksize;

	err = datcache_getrec_fromfile(cache->fd, DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_NGWORDINFO, &recdata, &recsize);
	if (err < 0) {
		return err;
	}

	if (recsize == 0) {
		return 0; /* TODO */
	}

	for (i = 0; i < recsize; ) {
		chunksize = *(UH*)(recdata + i);
		i += 2;
		str = (TC*)(recdata + i);
		str_len = chunksize / 2;
		i += chunksize;
		chunksize = *(UH*)(recdata + i);
		i += 2;
		err = wordlist_appendword(&cache->ngwordlist, str, str_len);
		if (err < 0) {
			break;
		}
		i += chunksize;
	}

	free(recdata);

	return err;
}

EXPORT datcache_t* datcache_new(VID vid)
{
	datcache_t *cache;
	ID semid;
	W fd,err;
	W size, retrinfo_len, header_len;
	UB *rawdat, *retrinfo, *header;
	F_STATE fstate;

	fd = oopn_obj(vid, NULL, F_READ|F_WRITE, NULL);
	if (fd < 0) {
		return NULL;
	}
	err = ofl_sts(fd, NULL, &fstate, NULL);
	if (err < 0) {
		return NULL;
	}
	semid = cre_sem(1, SEM_EXCL|DELEXIT);
	if (semid < 0) {
		cls_fil(fd);
		return NULL;
	}
	err = datcache_getdat_fromfile(fd, &rawdat, &size);
	if (err < 0) {
		del_sem(semid);
		cls_fil(fd);
		return NULL;
	}
	err = datcache_getretrinfo_fromfile(fd, &retrinfo, &retrinfo_len);
	if (err < 0) {
		if (rawdat != NULL) {
			free(rawdat);
		}
		del_sem(semid);
		cls_fil(fd);
		return NULL;
	}
	err = datcache_getheader_fromfile(fd, &header, &header_len);
	if (err < 0) {
		if (retrinfo != NULL) {
			free(retrinfo);
		}
		if (rawdat != NULL) {
			free(rawdat);
		}
		del_sem(semid);
		cls_fil(fd);
		return NULL;
	}

	cache = (datcache_t*)malloc(sizeof(datcache_t));
	if (cache == NULL) {
		free(retrinfo);
		free(rawdat);
		del_sem(semid);
		cls_fil(fd);
		return NULL;
	}
	cache->flag = 0;
	cache->fd = fd;
	cache->mtime_open = fstate.f_mtime;
	cache->semid = semid;
	QueInit(&(cache->datalist.queue));
	cache->datalist.data = rawdat;
	cache->datalist.len = size;
	cache->s_datsize = size;
	cache->recsize_open = size;
	cache->context = NULL;

	datcache_setupretrinfo(cache, retrinfo, retrinfo_len);
	cache->latestheader = header;
	cache->latestheader_len = header_len;

	err = residhash_initialize(&cache->residhash);
	if (err < 0) {
		free(retrinfo);
		free(rawdat);
		del_sem(semid);
		cls_fil(fd);
		free(cache);
		return NULL;
	}
	err = resindexhash_initialize(&cache->resindexhash);
	if (err < 0) {
		residhash_finalize(&cache->residhash);
		free(retrinfo);
		free(rawdat);
		del_sem(semid);
		cls_fil(fd);
		free(cache);
		return NULL;
	}
	err = wordlist_initialize(&cache->ngwordlist);
	if (err < 0) {
		resindexhash_finalize(&cache->resindexhash);
		residhash_finalize(&cache->residhash);
		free(retrinfo);
		free(rawdat);
		del_sem(semid);
		cls_fil(fd);
		free(cache);
		return NULL;
	}

	err = datcache_readresidinfo(cache);
	if (err < 0) {
		wordlist_finalize(&cache->ngwordlist);
		resindexhash_finalize(&cache->resindexhash);
		residhash_finalize(&cache->residhash);
		free(retrinfo);
		free(rawdat);
		del_sem(semid);
		cls_fil(fd);
		free(cache);
		return NULL;
	}
	err = datcache_readresindexinfo(cache);
	if (err < 0) {
		wordlist_finalize(&cache->ngwordlist);
		resindexhash_finalize(&cache->resindexhash);
		residhash_finalize(&cache->residhash);
		free(retrinfo);
		free(rawdat);
		del_sem(semid);
		cls_fil(fd);
		free(cache);
		return NULL;
	}
	err = datcache_readngwordinfo(cache);
	if (err < 0) {
		wordlist_finalize(&cache->ngwordlist);
		resindexhash_finalize(&cache->resindexhash);
		residhash_finalize(&cache->residhash);
		free(retrinfo);
		free(rawdat);
		del_sem(semid);
		cls_fil(fd);
		free(cache);
		return NULL;
	}

	return cache;
}

EXPORT VOID datcache_delete(datcache_t *cache)
{
	datcache_data_t *cache_data;
	Bool ok;

	wordlist_finalize(&cache->ngwordlist);
	resindexhash_finalize(&cache->resindexhash);
	residhash_finalize(&cache->residhash);

	if (cache->latestheader != NULL) {
		free(cache->latestheader);
	}
	if (cache->retrinfo != NULL) {
		free(cache->retrinfo);
	}

	for (;;) {
		ok = isQueEmpty(&(cache->datalist.queue));
		if (ok == True) {
			break;
		}
		cache_data = datcache_data_next(&cache->datalist);
		datcache_data_delete(cache_data);
	}
	free(cache->datalist.data);
	del_sem(cache->semid);
	cls_fil(cache->fd);
	free(cache);
}
