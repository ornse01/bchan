/*
 * cookiedb.c
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

#include    "cookiedb.h"
#include    "httpdateparser.h"

#include	<basic.h>
#include	<bstdio.h>
#include	<bstdlib.h>
#include	<bstring.h>
#include	<bsys/queue.h>

#ifdef BCHAN_CONFIG_DEBUG
# define DP(arg) printf arg
# define DP_ER(msg, err) printf("%s (%d/%x)\n", msg, err>>16, err)
#else
# define DP(arg) /**/
# define DP_ER(msg, err) /**/
#endif

struct ascstr_t_ {
	UB *str;
	W len;
};
typedef struct ascstr_t_ ascstr_t;

EXPORT W ascstr_appendstr(ascstr_t *astr, UB *apd, W apd_len)
{
	UB *str;

	str = realloc(astr->str, astr->len + apd_len + 1);
	if (str == NULL) {
		return -1;
	}
	astr->str = str;

	memcpy(astr->str + astr->len, apd, apd_len);
	astr->len += apd_len;
	astr->str[astr->len] = '\0';

	return 0;
}

EXPORT W ascstr_initialize(ascstr_t *astr)
{
	astr->str = malloc(sizeof(UB));
	if (astr->str == NULL) {
		return -1;
	}
	astr->str[0] = '\0';
	astr->len = 0;
	return 0;
}

EXPORT VOID ascstr_finalize(ascstr_t *astr)
{
	free(astr->str);
}

struct httpcookie_t_ {
	QUEUE que;
	/* requested host */
	ascstr_t host;
	/* Set-Cookie value */
	ascstr_t attr; /* NAME string */
	ascstr_t name; /* VALUE string */
	ascstr_t comment;
	ascstr_t domain;
	Bool persistent;
	STIME expires;
	ascstr_t path;
	ascstr_t version;
	Bool secure;
};
typedef struct httpcookie_t_ httpcookie_t;

LOCAL Bool httpcookie_isvalueset(httpcookie_t *cookie)
{
	if (cookie->attr.len == 0) {
		return False;
	}
	if (cookie->name.len == 0) {
		return False;
	}
	return True;
}

LOCAL httpcookie_t* httpcookie_nextnode(httpcookie_t *cookie)
{
	return (httpcookie_t*)cookie->que.next;
}

LOCAL VOID httpcookie_setexpires(httpcookie_t *cookie, STIME expires)
{
	cookie->expires = expires;
}

LOCAL VOID httpcookie_QueRemove(httpcookie_t *cookie)
{
	QueRemove(&cookie->que);
	QueInit(&cookie->que);
}

LOCAL W httpcookie_initialize(httpcookie_t *cookie, UB *host, W host_len)
{
	W err;

	QueInit(&cookie->que);
	err = ascstr_initialize(&cookie->host);
	if (err < 0) {
		goto error_host;
	}
	ascstr_appendstr(&cookie->host, host, host_len);
	err = ascstr_initialize(&cookie->attr);
	if (err < 0) {
		goto error_attr;
	}
	err = ascstr_initialize(&cookie->name);
	if (err < 0) {
		goto error_name;
	}
	err = ascstr_initialize(&cookie->comment);
	if (err < 0) {
		goto error_comment;
	}
	err = ascstr_initialize(&cookie->domain);
	if (err < 0) {
		goto error_domain;
	}
	err = ascstr_initialize(&cookie->path);
	if (err < 0) {
		goto error_path;
	}
	err = ascstr_initialize(&cookie->version);
	if (err < 0) {
		goto error_version;
	}

	cookie->persistent = False;
	cookie->secure = False;
	cookie->expires = 0;

	return 0;

	ascstr_finalize(&cookie->version);
error_version:
	ascstr_finalize(&cookie->path);
error_path:
	ascstr_finalize(&cookie->domain);
error_domain:
	ascstr_finalize(&cookie->comment);
error_comment:
	ascstr_finalize(&cookie->name);
error_name:
	ascstr_finalize(&cookie->attr);
error_attr:
	ascstr_finalize(&cookie->host);
error_host:
	return err;
}

LOCAL VOID httpcookie_finalize(httpcookie_t *cookie)
{
	ascstr_finalize(&cookie->version);
	ascstr_finalize(&cookie->path);
	ascstr_finalize(&cookie->domain);
	ascstr_finalize(&cookie->comment);
	ascstr_finalize(&cookie->name);
	ascstr_finalize(&cookie->attr);
	ascstr_finalize(&cookie->host);
	QueRemove(&cookie->que);
}

LOCAL httpcookie_t* httpcookie_new(UB *host, W host_len)
{
	httpcookie_t *cookie;
	W err;

	cookie = malloc(sizeof(httpcookie_t));
	if (cookie == NULL) {
		return NULL;
	}
	err = httpcookie_initialize(cookie, host, host_len);
	if (err < 0) {
		free(cookie);
		return NULL;
	}
	return cookie;
}

LOCAL VOID httpcookie_delete(httpcookie_t *cookie)
{
	httpcookie_finalize(cookie);
	free(cookie);
}

struct cookie_persistentdb_t_ {
	QUEUE sentinel;
	LINK *lnk;
};
typedef struct cookie_persistentdb_t_ cookie_persistentdb_t;

struct cookie_volatiledb_t_ {
	QUEUE sentinel;
};
typedef struct cookie_volatiledb_t_ cookie_volatiledb_t;

struct cookiedb_t_ {
	cookie_volatiledb_t vdb;
	cookie_persistentdb_t pdb;
};

LOCAL W cookie_persistentdb_writefile(cookie_persistentdb_t *db)
{
	/* TODO */
	return -1;
}

LOCAL W cookie_persistentdb_readfile(cookie_persistentdb_t *db)
{
	/* TODO */
	return -1;
}

LOCAL VOID cookie_persistentdb_clear(cookie_persistentdb_t *db)
{
	httpcookie_t *cookie;
	Bool empty;

	for (;;) {
		empty = isQueEmpty(&db->sentinel);
		if (empty == True) {
			break;
		}
		cookie = (httpcookie_t*)db->sentinel.prev;
		httpcookie_delete(cookie);
	}

	/* TODO clear file */
}

LOCAL httpcookie_t* cookie_persistentdb_sentinelnode(cookie_persistentdb_t *db)
{
	return (httpcookie_t*)&db->sentinel;
}

LOCAL VOID cookie_persistentdb_insertcookie(cookie_persistentdb_t *db, httpcookie_t *cookie)
{
	QueInsert(&cookie->que, &db->sentinel);
}

LOCAL W cookie_persistentdb_initialize(cookie_persistentdb_t *db, LINK *db_lnk)
{
	QueInit(&db->sentinel);
	db->lnk = db_lnk;
	return 0;
}

LOCAL VOID cookie_persistentdb_finalize(cookie_persistentdb_t *db)
{
	httpcookie_t *cookie;
	Bool empty;

	for (;;) {
		empty = isQueEmpty(&db->sentinel);
		if (empty == True) {
			break;
		}
		cookie = (httpcookie_t*)db->sentinel.prev;
		httpcookie_delete(cookie);
	}
}

LOCAL VOID cookie_volatiledb_clear(cookie_volatiledb_t *db)
{
	httpcookie_t *cookie;
	Bool empty;

	for (;;) {
		empty = isQueEmpty(&db->sentinel);
		if (empty == True) {
			break;
		}
		cookie = (httpcookie_t*)db->sentinel.prev;
		httpcookie_delete(cookie);
	}
}

LOCAL httpcookie_t* cookie_volatiledb_sentinelnode(cookie_volatiledb_t *db)
{
	return (httpcookie_t*)&db->sentinel;
}

LOCAL VOID cookie_volatiledb_insertcookie(cookie_volatiledb_t *db, httpcookie_t *cookie)
{
	QueInsert(&cookie->que, &db->sentinel);
}

LOCAL W cookie_volatiledb_initialize(cookie_volatiledb_t *db)
{
	QueInit(&db->sentinel);
	return 0;
}

LOCAL VOID cookie_volatiledb_finalize(cookie_volatiledb_t *db)
{
	httpcookie_t *cookie;
	Bool empty;

	for (;;) {
		empty = isQueEmpty(&db->sentinel);
		if (empty == True) {
			break;
		}
		cookie = (httpcookie_t*)db->sentinel.prev;
		httpcookie_delete(cookie);
	}
}

struct cookiedb_writeiterator_t_ {
	cookiedb_t *origin;
	ascstr_t host;
	ascstr_t path;
	Bool secure;
	STIME time;
	httpcookie_t *current;
	enum {
		COOKIEDB_WRITEITERATOR_STATE_VDB,
		COOKIEDB_WRITEITERATOR_STATE_PDB,
	} state;
};
typedef struct cookiedb_writeiterator_t_ cookiedb_writeiterator_t;

LOCAL W count_priod(ascstr_t *str)
{
	W i,count;

	count = 0;
	for (i = 0; i < str->len; i++) {
		if (str->str[i] == '.') {
			count++;
		}
	}

	return count;
}

LOCAL Bool check_specified_TLD(ascstr_t *domain)
{
	if (strncmp(domain->str + domain->len - 4, ".net", 4) == 0) {
		return True;
	}
	if (strncmp(domain->str + domain->len - 4, ".com", 4) == 0) {
		return True;
	}
	if (strncmp(domain->str + domain->len - 4, ".edu", 4) == 0) {
		return True;
	}
	if (strncmp(domain->str + domain->len - 4, ".org", 4) == 0) {
		return True;
	}
	if (strncmp(domain->str + domain->len - 4, ".gov", 4) == 0) {
		return True;
	}
	if (strncmp(domain->str + domain->len - 4, ".mil", 4) == 0) {
		return True;
	}
	if (strncmp(domain->str + domain->len - 4, ".int", 4) == 0) {
		return True;
	}
	/* TODO: other TLDs. */
	return False;
}

LOCAL Bool cookiedb_writeiterator_domaincheck(ascstr_t *send_host, ascstr_t *origin_host)
{
	W count;
	Bool ok;

	if (origin_host->len < send_host->len) {
		return False;
	}
	if (strncmp(origin_host->str + origin_host->len - send_host->len, send_host->str, send_host->len) != 0) {
		return False;
	}
	count = count_priod(send_host);
	ok = check_specified_TLD(send_host);
	if (ok == True) {
		if (count < 2) {
			return False;
		}
	} else {
		if (count < 3) {
			return False;
		}
	}

	return True;
}

LOCAL Bool cookiedb_writeitereator_pathcheck(ascstr_t *send_path, ascstr_t *origin_path)
{
	if (origin_path->len < send_path->len) {
		return False;
	}
	if (strncmp(origin_path->str, send_path->str, send_path->len) != 0) {
		return False;
	}
	return True;
}

LOCAL Bool cookiedb_writeiterator_checksendcondition(cookiedb_writeiterator_t *iter, httpcookie_t *cookie)
{
	Bool ok;

	if (cookie->secure == True) {
		if (iter->secure != True) {
			return False;
		}
	}
	if (cookie->persistent == True) {
		if (cookie->expires < iter->time) {
			return False;
		}
	}
	if (cookie->domain.len != 0) {
		ok = cookiedb_writeiterator_domaincheck(&iter->host, &cookie->domain);
	} else {
		ok = cookiedb_writeiterator_domaincheck(&iter->host, &cookie->host);
	}
	if (ok == False) {
		return False;
	}
	if (cookie->path.len != 0) {
		ok = cookiedb_writeitereator_pathcheck(&iter->path, &cookie->path);
	} else {
		/* TODO */
	}
	if (ok == False) {
		return False;
	}

	return True;
}

LOCAL Bool cookiedb_writeiterator_next(cookiedb_writeiterator_t *iter, httpcookie_t **cookie)
{
	httpcookie_t *senti;
	Bool send;

	if (iter->state == COOKIEDB_WRITEITERATOR_STATE_VDB) {
		senti = cookie_volatiledb_sentinelnode(&iter->origin->vdb);
		for (;;) {
			if (iter->current == senti) {
				break;
			}
			send = cookiedb_writeiterator_checksendcondition(iter, iter->current);
			if (send == True) {
				break;
			}
			iter->current = httpcookie_nextnode(iter->current);
		}
		if (iter->current != senti) {
			*cookie = iter->current;
			iter->current = httpcookie_nextnode(iter->current);
			return True;
		} else {
			iter->state = COOKIEDB_WRITEITERATOR_STATE_PDB;
			senti = cookie_persistentdb_sentinelnode(&iter->origin->pdb);
			iter->current = httpcookie_nextnode(senti);
		}
	}
	if (iter->state == COOKIEDB_WRITEITERATOR_STATE_PDB) {
		senti = cookie_persistentdb_sentinelnode(&iter->origin->pdb);
		for (;;) {
			if (iter->current == senti) {
				break;
			}
			send = cookiedb_writeiterator_checksendcondition(iter, iter->current);
			if (send == True) {
				break;
			}
			iter->current = httpcookie_nextnode(iter->current);
		}
		if (iter->current != senti) {
			*cookie = iter->current;
			iter->current = httpcookie_nextnode(iter->current);
			return True;
		}
	}

	return False;
}

LOCAL W cookiedb_writeiterator_initialize(cookiedb_writeiterator_t *iter, cookiedb_t *db, UB *host, W host_len, UB *path, W path_len, Bool secure, STIME time)
{
	W err;
	httpcookie_t *senti;

	err = ascstr_initialize(&iter->host);
	if (err < 0) {
		return err;
	}
	err = ascstr_appendstr(&iter->host, host, host_len);
	if (err < 0) {
		ascstr_finalize(&iter->host);
		return err;
	}
	err = ascstr_initialize(&iter->path);
	if (err < 0) {
		ascstr_finalize(&iter->host);
		return err;
	}
	err = ascstr_appendstr(&iter->path, path, path_len);
	if (err < 0) {
		ascstr_finalize(&iter->path);
		ascstr_finalize(&iter->host);
		return err;
	}

	iter->origin = db;
	iter->secure = secure;
	iter->time = time;
	iter->state = COOKIEDB_WRITEITERATOR_STATE_VDB;
	senti = cookie_volatiledb_sentinelnode(&db->vdb);
	iter->current = httpcookie_nextnode(senti);

	return 0;
}

LOCAL VOID cookiedb_writeiterator_finalize(cookiedb_writeiterator_t *iter)
{
	ascstr_finalize(&iter->path);
	ascstr_finalize(&iter->host);
}

struct cookiedb_writeheadercontext_t_ {
	cookiedb_writeiterator_t iter;
	enum {
		COOKIEDB_WRITEITERATORCONTEXT_STATE_START,
		COOKIEDB_WRITEITERATORCONTEXT_STATE_NAME,
		COOKIEDB_WRITEITERATORCONTEXT_STATE_EQUAL,
		COOKIEDB_WRITEITERATORCONTEXT_STATE_VALUE,
		COOKIEDB_WRITEITERATORCONTEXT_STATE_COLON,
		COOKIEDB_WRITEITERATORCONTEXT_STATE_CRLF,
		COOKIEDB_WRITEITERATORCONTEXT_STATE_END,
	} state;
	httpcookie_t *current;
};

LOCAL UB cookiedb_writeheader_context_headername[] = "Cookie: ";
LOCAL UB cookiedb_writeheader_context_equal[] = "=";
LOCAL UB cookiedb_writeheader_context_colon[] = "; ";
LOCAL UB cookiedb_writeheader_context_crlf[] = "\r\n";

EXPORT Bool cookiedb_writeheadercontext_makeheader(cookiedb_writeheadercontext_t *context, UB **str, W *len)
{
	Bool cont;

	switch (context->state) {
	case COOKIEDB_WRITEITERATORCONTEXT_STATE_START:
		cont = cookiedb_writeiterator_next(&context->iter, &context->current);
		if (cont == False) {
			return False;
		}
		*str = cookiedb_writeheader_context_headername;
		*len = 8;
		context->state = COOKIEDB_WRITEITERATORCONTEXT_STATE_NAME;
		return True;
	case COOKIEDB_WRITEITERATORCONTEXT_STATE_NAME:
		*str = context->current->attr.str;
		*len = context->current->attr.len;
		context->state = COOKIEDB_WRITEITERATORCONTEXT_STATE_EQUAL;
		return True;
	case COOKIEDB_WRITEITERATORCONTEXT_STATE_EQUAL:
		*str = cookiedb_writeheader_context_equal;
		*len = 1;
		context->state = COOKIEDB_WRITEITERATORCONTEXT_STATE_VALUE;
		return True;
	case COOKIEDB_WRITEITERATORCONTEXT_STATE_VALUE:
		*str = context->current->name.str;
		*len = context->current->name.len;
		context->state = COOKIEDB_WRITEITERATORCONTEXT_STATE_COLON;
		return True;
	case COOKIEDB_WRITEITERATORCONTEXT_STATE_COLON:
		*str = cookiedb_writeheader_context_colon;
		*len = 2;
		cont = cookiedb_writeiterator_next(&context->iter, &context->current);
		if (cont == False) {
			context->state = COOKIEDB_WRITEITERATORCONTEXT_STATE_CRLF;
		} else {
			context->state = COOKIEDB_WRITEITERATORCONTEXT_STATE_NAME;
		}
		return True;
	case COOKIEDB_WRITEITERATORCONTEXT_STATE_CRLF:
		*str = cookiedb_writeheader_context_crlf;
		*len = 2;
		context->state = COOKIEDB_WRITEITERATORCONTEXT_STATE_END;
		return True;
	case COOKIEDB_WRITEITERATORCONTEXT_STATE_END:
		return False;
	}

	return False;
}

LOCAL cookiedb_writeheadercontext_t* cookiedb_writeheadercontext_new(cookiedb_t *db, UB *host, W host_len, UB *path, W path_len, Bool secure, STIME time)
{
	cookiedb_writeheadercontext_t *context;
	W err;

	context = (cookiedb_writeheadercontext_t*)malloc(sizeof(cookiedb_writeheadercontext_t));
	if (context == NULL) {
		return NULL;
	}
	err = cookiedb_writeiterator_initialize(&context->iter, db, host, host_len, path, path_len, secure, time);
	if (err < 0) {
		free(context);
		return NULL;
	}
	context->state = COOKIEDB_WRITEITERATORCONTEXT_STATE_START;

	return context;
}

LOCAL VOID cookiedb_writeheadercontext_delete(cookiedb_writeheadercontext_t *context)
{
	cookiedb_writeiterator_finalize(&context->iter);
	free(context);
}

EXPORT cookiedb_writeheadercontext_t* cookiedb_startheaderwrite(cookiedb_t *db, UB *host, W host_len, UB *path, W path_len, Bool secure, STIME time)
{
	cookiedb_writeheadercontext_t *context;

	context = cookiedb_writeheadercontext_new(db, host, host_len, path, path_len, secure, time);
	if (context == NULL) {
		return NULL;
	}

	return context;
}

EXPORT VOID cookiedb_endheaderwrite(cookiedb_t *db, cookiedb_writeheadercontext_t *context)
{
	cookiedb_writeheadercontext_delete(context);
}

struct cookiedb_readheadercontext_t_ {
	ascstr_t host;
	STIME current;
	QUEUE sentinel;
	enum {
		COOKIEDB_READHEADERCONTEXT_STATE_START,
		COOKIEDB_READHEADERCONTEXT_STATE_NAMEATTR,
		COOKIEDB_READHEADERCONTEXT_STATE_OTHERVAL,
	} state;
	httpcookie_t *reading;
};

LOCAL VOID cookiedb_readheadercontext_insertcookie(cookiedb_readheadercontext_t *context, httpcookie_t *cookie)
{
	QueInsert(&cookie->que, &context->sentinel);
}

EXPORT W cookiedb_readheadercontext_appendchar_attr(cookiedb_readheadercontext_t *context, UB ch)
{
	if (context->state != COOKIEDB_READHEADERCONTEXT_STATE_NAMEATTR) {
		context->state = COOKIEDB_READHEADERCONTEXT_STATE_NAMEATTR;
		if (context->reading != NULL) {
			cookiedb_readheadercontext_insertcookie(context, context->reading);
		}
		context->reading = httpcookie_new(context->host.str, context->host.len);
		if (context->reading == NULL) {
			return -1; /* TODO: error value */
		}
	}
	return ascstr_appendstr(&context->reading->attr, &ch, 1);
}

LOCAL W cookiedb_readheadercontext_appdatechar_common(cookiedb_readheadercontext_t *context, ascstr_t *target, UB ch)
{
	if (context->state == COOKIEDB_READHEADERCONTEXT_STATE_START) {
		return 0;
	}
	context->state = COOKIEDB_READHEADERCONTEXT_STATE_OTHERVAL;
	return ascstr_appendstr(target, &ch, 1);
}

EXPORT W cookiedb_readheadercontext_appendchar_name(cookiedb_readheadercontext_t *context, UB ch)
{
	return cookiedb_readheadercontext_appdatechar_common(context, &context->reading->name, ch);
}

EXPORT W cookiedb_readheadercontext_appendchar_comment(cookiedb_readheadercontext_t *context, UB ch)
{
	return cookiedb_readheadercontext_appdatechar_common(context, &context->reading->comment, ch);
}

EXPORT W cookiedb_readheadercontext_appendchar_domain(cookiedb_readheadercontext_t *context, UB ch)
{
	return cookiedb_readheadercontext_appdatechar_common(context, &context->reading->domain, ch);
}

EXPORT W cookiedb_readheadercontext_appendchar_path(cookiedb_readheadercontext_t *context, UB ch)
{
	return cookiedb_readheadercontext_appdatechar_common(context, &context->reading->path, ch);
}

EXPORT W cookiedb_readheadercontext_appendchar_version(cookiedb_readheadercontext_t *context, UB ch)
{
	return cookiedb_readheadercontext_appdatechar_common(context, &context->reading->version, ch);
}

EXPORT W cookiedb_readheadercontext_setsecure(cookiedb_readheadercontext_t *context)
{
	if (context->state == COOKIEDB_READHEADERCONTEXT_STATE_START) {
		return 0;
	}
	context->state = COOKIEDB_READHEADERCONTEXT_STATE_OTHERVAL;

	context->reading->secure = True;

	return 0;
}

EXPORT W cookiedb_readheadercontext_setexpires(cookiedb_readheadercontext_t *context, STIME expires)
{
	if (context->state == COOKIEDB_READHEADERCONTEXT_STATE_START) {
		return 0;
	}
	context->state = COOKIEDB_READHEADERCONTEXT_STATE_OTHERVAL;

	httpcookie_setexpires(context->reading, expires);

	return 0;
}

EXPORT W cookiedb_readheadercontext_setmaxage(cookiedb_readheadercontext_t *context, W seconds)
{
	if (seconds <= 0) {
		return 0;
	}

	httpcookie_setexpires(context->reading, context->current + seconds);

	return 0;
}

LOCAL httpcookie_t* cookiedb_readheadercontext_prevcookie(cookiedb_readheadercontext_t *context)
{
	Bool empty;
	empty = isQueEmpty(&context->sentinel);
	if (empty == True) {
		return NULL;
	}
	return (httpcookie_t*)context->sentinel.prev;
}

LOCAL cookiedb_readheadercontext_t* cookiedb_readheadercontext_new(UB *host, W host_len, STIME time)
{
	cookiedb_readheadercontext_t *context;
	W err;

	context = (cookiedb_readheadercontext_t*)malloc(sizeof(cookiedb_readheadercontext_t));
	if (context == NULL) {
		return NULL;
	}
	err = ascstr_initialize(&context->host);
	if (err < 0) {
		free(context);
		return NULL;
	}
	err = ascstr_appendstr(&context->host, host, host_len);
	if (err < 0) {
		ascstr_finalize(&context->host);
		free(context);
		return NULL;
	}
	context->reading = NULL;
	QueInit(&context->sentinel);
	context->state = COOKIEDB_READHEADERCONTEXT_STATE_START;

	return context;
}

LOCAL VOID cookiedb_readheadercontext_delete(cookiedb_readheadercontext_t *context)
{
	httpcookie_t *cookie;

	for (;;) {
		cookie = cookiedb_readheadercontext_prevcookie(context);
		if (cookie == NULL) {
			break;
		}
		httpcookie_delete(cookie);
	}
	if (context->reading != NULL) {
		httpcookie_delete(context->reading);
	}

	ascstr_finalize(&context->host);
	free(context);
}

EXPORT cookiedb_readheadercontext_t* cookiedb_startheaderread(cookiedb_t *db, UB *host, W host_len, STIME time)
{
	cookiedb_readheadercontext_t *context;

	context = cookiedb_readheadercontext_new(host, host_len, time);
	if (context == NULL) {
		return NULL;
	}

	return context;
}

LOCAL VOID cookiedb_inserteachdb(cookiedb_t *db, httpcookie_t *cookie, STIME current)
{
	/* TODO: domain chack */
	if (cookie->expires == 0) {
		cookie_volatiledb_insertcookie(&db->vdb, cookie);
	} else if (cookie->expires < current) {
		cookie_persistentdb_insertcookie(&db->pdb, cookie);
	} else { /* cookie->expires >= current */
		httpcookie_delete(cookie);
	}
}

EXPORT VOID cookiedb_endheaderread(cookiedb_t *db, cookiedb_readheadercontext_t *context)
{
	httpcookie_t *cookie;
	Bool ok;

	for (;;) {
		cookie = cookiedb_readheadercontext_prevcookie(context);
		if (cookie == NULL) {
			break;
		}
		httpcookie_QueRemove(cookie);
		cookiedb_inserteachdb(db, cookie, context->current);
	}
	if (context->reading != NULL) {
		ok = httpcookie_isvalueset(context->reading);
		if (ok == True) {
			httpcookie_QueRemove(context->reading);
			cookiedb_inserteachdb(db, context->reading, context->current);
		} else {
			httpcookie_delete(context->reading);
		}
		context->reading = NULL;
	}

	cookiedb_readheadercontext_delete(context);
}

EXPORT VOID cookiedb_clearallcookie(cookiedb_t *db)
{
	cookie_volatiledb_clear(&db->vdb);
	cookie_persistentdb_clear(&db->pdb);
}

EXPORT W cookiedb_writefile(cookiedb_t *db)
{
	return cookie_persistentdb_writefile(&db->pdb);
}

EXPORT W cookiedb_readfile(cookiedb_t *db)
{
	return cookie_persistentdb_readfile(&db->pdb);
}

LOCAL W cookiedb_initialize(cookiedb_t *db, LINK *db_lnk)
{
	W err;

	err = cookie_volatiledb_initialize(&db->vdb);
	if (err < 0) {
		return err;
	}
	err = cookie_persistentdb_initialize(&db->pdb, db_lnk);
	if (err < 0) {
		cookie_volatiledb_finalize(&db->vdb);
		return err;
	}

	return 0;
}

LOCAL VOID cookiedb_finalize(cookiedb_t *db)
{
	cookie_persistentdb_finalize(&db->pdb);
	cookie_volatiledb_finalize(&db->vdb);
}

EXPORT cookiedb_t* cookiedb_new(LINK *db_lnk)
{
	cookiedb_t *db;
	W err;

	db = malloc(sizeof(cookiedb_t));
	if (db == NULL) {
		return NULL;
	}
	err = cookiedb_initialize(db, db_lnk);
	if (err < 0) {
		free(db);
		return NULL;
	}

	return db;
}

EXPORT VOID cookiedb_delete(cookiedb_t *db)
{
	cookiedb_finalize(db);
	free(db);
}
