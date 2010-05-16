/*
 * cache.c
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

#include	<basic.h>
#include	<bstdlib.h>
#include	<bstdio.h>
#include	<bstring.h>
#include	<errcode.h>
#include	<btron/btron.h>
#include	<bsys/queue.h>

#include    "cache.h"
#include    "residhash.h"

typedef struct datcache_data_t_ datcache_data_t;

struct datcache_data_t_ {
	QUEUE queue;
	UB *data;
	W len;
};

struct datcache_t_ {
	W fd;
	ID semid;
	datcache_data_t datalist;
	W s_datsize;
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
};

struct datcache_datareadcontext_t_ {
	datcache_t *datcache;
	datcache_data_t *current;
	W index;
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

LOCAL W datcache_preparerec_forwritefile(W fd, W rectype, W subtype)
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
		err = trc_rec(fd, 0);
		if (err < 0) {
			return -1;
		}
	}

	return 0; /* TODO */
}

EXPORT W datcache_datasize(datcache_t *cache)
{
	return cache->s_datsize;
}

EXPORT W datcache_writefile(datcache_t *cache)
{
	W err;
	datcache_data_t *cache_data;

	/* TODO: blush up implementation for speed up. */
	if (cache->s_datsize > 0) {
		err = datcache_preparerec_forwritefile(cache->fd, DATCACHE_RECORDTYPE_MAIN, 0);
		if (err < 0) {
			return err;
		}
		cache_data = &(cache->datalist);
		for (;;) {
			err = wri_rec(cache->fd, -1, cache_data->data, cache_data->len, NULL, NULL, 0);
			if (err < 0) {
				return err;
			}
			cache_data = datcache_data_next(cache_data);
			if (cache_data == &(cache->datalist)) {
				break;
			}
		}
	}

	if (cache->latestheader_len > 0) {
		err = datcache_preparerec_forwritefile(cache->fd, DATCACHE_RECORDTYPE_INFO, DATCACHE_RECORDSUBTYPE_HEADER);
		if (err < 0) {
			return err;
		}
		err = wri_rec(cache->fd, -1, cache->latestheader, cache->latestheader_len, NULL, NULL, 0);
		if (err < 0) {
			return err;
		}
	}

	return 0;
}

EXPORT W datcache_addresiddata(datcache_t *cache, TC *idstr, W idstr_len, UW attr, COLOR color)
{
	return residhash_adddata(&cache->residhash, idstr, idstr_len, attr, color);
}

EXPORT W datcache_searchresiddata(datcache_t *cache, TC *idstr, W idstr_len, UW *attr, COLOR *color)
{
	return residhash_searchdata(&cache->residhash, idstr, idstr_len, attr, color);
}

EXPORT VOID datcache_removeresiddata(datcache_t *cache, TC *idstr, W idstr_len)
{
	residhash_removedata(&cache->residhash, idstr, idstr_len);
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

EXPORT datcache_t* datcache_new(VID vid)
{
	datcache_t *cache;
	ID semid;
	W fd,err;
	W size, retrinfo_len, header_len;
	UB *rawdat, *retrinfo, *header;

	fd = oopn_obj(vid, NULL, F_READ|F_WRITE, NULL);
	if (fd < 0) {
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
	cache->fd = fd;
	cache->semid = semid;
	QueInit(&(cache->datalist.queue));
	cache->datalist.data = rawdat;
	cache->datalist.len = size;
	cache->s_datsize = size;
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

	return cache;
}

EXPORT VOID datcache_delete(datcache_t *cache)
{
	datcache_data_t *cache_data;
	Bool ok;

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
