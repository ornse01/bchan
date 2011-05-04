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
#include    "psvlexer.h"
#include    "parselib.h"

#include	<basic.h>
#include	<bstdio.h>
#include	<bstdlib.h>
#include	<bstring.h>
#include	<bctype.h>
#include	<errcode.h>
#include	<btron/btron.h>
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

EXPORT W ascstr_cmp(ascstr_t *s1, ascstr_t *s2)
{
	W i,len;

	len = s1->len > s2->len ? s2->len : s1->len;
	for (i = 0; i < len; i++) {
		if (s1->str[i] > s2->str[i]) {
			return 1;
		} else if (s1->str[i] < s2->str[i]) {
			return -1;
		}
	}

	/* check terminate char: '\0' */
	if (s1->str[i] > s2->str[i]) {
		return 1;
	} else if (s1->str[i] < s2->str[i]) {
		return -1;
	}
	return 0; /* same length */
}

/*
 * match example
 *  astr:    XXXXYYYYZZZZ
 *  suffix:       YYYZZZZ
 *
 */
EXPORT Bool ascstr_suffixcmp(ascstr_t *astr, ascstr_t *suffix)
{
	if (astr->len < suffix->len) {
		return False;
	}
	if (strncmp(astr->str + astr->len - suffix->len, suffix->str, suffix->len) != 0) {
		return False;
	}

	return True;
}

/*
 * match example
 *  astr:    XXXXYYYYZZZZ
 *  prefix:  XXXXYY
 *
 */
EXPORT Bool ascstr_prefixcmp(ascstr_t *astr, ascstr_t *prefix)
{
	if (astr->len < prefix->len) {
		return False;
	}
	if (strncmp(astr->str, prefix->str, prefix->len) != 0) {
		return False;
	}

	return True;
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
	/* requested host and path */
	ascstr_t origin_host;
	ascstr_t origin_path;
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

LOCAL ascstr_t* httpcookie_getvaliddomain(httpcookie_t *cookie)
{
	if (cookie->domain.len != 0) {
		return &cookie->domain;
	}
	return &cookie->origin_host;
}

LOCAL ascstr_t* httpcookie_getvalidpath(httpcookie_t *cookie)
{
	if (cookie->path.len != 0) {
		return &cookie->path;
	}
	return &cookie->origin_path;
}

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

/* check replace condition. */
LOCAL Bool httpcookie_issamekey(httpcookie_t *cookie1, httpcookie_t *cookie2)
{
	W cmp;
	ascstr_t *as1, *as2;

	cmp = ascstr_cmp(&cookie1->attr, &cookie2->attr);
	if (cmp != 0) {
		return False;
	}
	as1 = httpcookie_getvaliddomain(cookie1);
	as2 = httpcookie_getvaliddomain(cookie2);
	cmp = ascstr_cmp(as1, as2);
	if (cmp != 0) {
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
	cookie->persistent = True;
	cookie->expires = expires;
}

LOCAL VOID httpcookie_QueRemove(httpcookie_t *cookie)
{
	QueRemove(&cookie->que);
	QueInit(&cookie->que);
}

LOCAL VOID httpcookie_QueInsert(httpcookie_t *entry, httpcookie_t *que)
{
	QueInsert(&entry->que, &que->que);
}

LOCAL W httpcookie_initialize(httpcookie_t *cookie, UB *host, W host_len, UB *path, W path_len)
{
	W err;

	QueInit(&cookie->que);
	err = ascstr_initialize(&cookie->origin_host);
	if (err < 0) {
		goto error_originhost;
	}
	err = ascstr_appendstr(&cookie->origin_host, host, host_len);
	if (err < 0) {
		goto error_originhost_append;
	}
	err = ascstr_initialize(&cookie->origin_path);
	if (err < 0) {
		goto error_originpath;
	}
	err = ascstr_appendstr(&cookie->origin_path, path, path_len);
	if (err < 0) {
		goto error_originpath_append;
	}
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
error_originpath_append:
	ascstr_finalize(&cookie->origin_path);
error_originpath:
error_originhost_append:
	ascstr_finalize(&cookie->origin_host);
error_originhost:
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
	ascstr_finalize(&cookie->origin_path);
	ascstr_finalize(&cookie->origin_host);
	QueRemove(&cookie->que);
}

LOCAL httpcookie_t* httpcookie_new(UB *host, W host_len, UB *path, W path_len)
{
	httpcookie_t *cookie;
	W err;

	cookie = malloc(sizeof(httpcookie_t));
	if (cookie == NULL) {
		return NULL;
	}
	err = httpcookie_initialize(cookie, host, host_len, path, path_len);
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

struct cookiedb_t_ {
	QUEUE sentinel;
	LINK *lnk;
	W rectype;
	UH subtype;
};

LOCAL httpcookie_t* cookiedb_sentinelnode(cookiedb_t *db)
{
	return (httpcookie_t*)&db->sentinel;
}

struct cookiedb_writeiterator_t_ {
	cookiedb_t *origin;
	ascstr_t host;
	ascstr_t path;
	Bool secure;
	STIME time;
	httpcookie_t *current;
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

LOCAL Bool cookiedb_writeiterator_checksendcondition_domaincheck(cookiedb_writeiterator_t *iter, httpcookie_t *cookie)
{
	Bool ok;

	if (cookie->domain.len != 0) {
		if (cookie->domain.len == (iter->host.len + 1)) {
			/* for
			 *  domain = .xxx.yyy.zzz
			 *  origin =  xxx.yyy.zzz
			 */
			if (strncmp(cookie->domain.str + 1, iter->host.str, iter->host.len) != 0) {
				return False;
			}
		} else {
			ok = ascstr_suffixcmp(&iter->host, &cookie->domain);
			if (ok == False) {
				return False;
			}
		}
		/* count period number is not need for counting in insertion to queue. */
	} else {
		ok = ascstr_suffixcmp(&iter->host, &cookie->origin_host);
		if (ok == False) {
			return False;
		}
	}

	return True;
}

LOCAL Bool cookiedb_writeitereator_pathcheck(cookiedb_writeiterator_t *iter, httpcookie_t *cookie)
{
	Bool ok;
	if (cookie->path.len != 0) {
		ok = ascstr_prefixcmp(&iter->path, &cookie->path);
	} else {
		ok = ascstr_prefixcmp(&iter->path, &cookie->origin_path);
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
	ok = cookiedb_writeiterator_checksendcondition_domaincheck(iter, cookie);
	if (ok == False) {
		return False;
	}
	ok = cookiedb_writeitereator_pathcheck(iter, cookie);
	if (ok == False) {
		return False;
	}

	return True;
}

LOCAL Bool cookiedb_writeiterator_next(cookiedb_writeiterator_t *iter, httpcookie_t **cookie)
{
	httpcookie_t *senti;
	Bool send;

	senti = cookiedb_sentinelnode(iter->origin);
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
	senti = cookiedb_sentinelnode(db);
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
		cont = cookiedb_writeiterator_next(&context->iter, &context->current);
		if (cont == False) {
			*len = 1;
			context->state = COOKIEDB_WRITEITERATORCONTEXT_STATE_CRLF;
		} else {
			*len = 2;
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
	ascstr_t path;
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
		context->reading = httpcookie_new(context->host.str, context->host.len, context->path.str, context->path.len);
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

LOCAL cookiedb_readheadercontext_t* cookiedb_readheadercontext_new(UB *host, W host_len, UB *path, W path_len, STIME time)
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
	err = ascstr_initialize(&context->path);
	if (err < 0) {
		ascstr_finalize(&context->host);
		free(context);
		return NULL;
	}
	err = ascstr_appendstr(&context->path, path, path_len);
	if (err < 0) {
		ascstr_finalize(&context->path);
		ascstr_finalize(&context->host);
		free(context);
		return NULL;
	}
	context->reading = NULL;
	QueInit(&context->sentinel);
	context->state = COOKIEDB_READHEADERCONTEXT_STATE_START;
	context->current = time;

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

	ascstr_finalize(&context->path);
	ascstr_finalize(&context->host);
	free(context);
}

EXPORT cookiedb_readheadercontext_t* cookiedb_startheaderread(cookiedb_t *db, UB *host, W host_len, UB *path, W path_len, STIME time)
{
	cookiedb_readheadercontext_t *context;

	context = cookiedb_readheadercontext_new(host, host_len, path, path_len, time);
	if (context == NULL) {
		return NULL;
	}

	return context;
}

LOCAL VOID cookiedb_insertcookie(cookiedb_t *db, httpcookie_t *cookie)
{
	httpcookie_t *senti, *node;
	ascstr_t *as_a, *as_n;
	W cmp;
	Bool samekey;

	senti = cookiedb_sentinelnode(db);
	node = httpcookie_nextnode(senti);
	for (;;) {
		if (node == senti) {
			httpcookie_QueInsert(cookie, senti);
			break;
		}
		as_n = httpcookie_getvalidpath(node);
		as_a = httpcookie_getvalidpath(cookie);
		cmp = ascstr_cmp(as_n, as_a);
		if (cmp == 0) {
			samekey = httpcookie_issamekey(node, cookie);
			if (samekey == True) {
				httpcookie_QueInsert(cookie, node);
				httpcookie_delete(node);
				break;
			}
		} else if (cmp < 0) {
			httpcookie_QueInsert(cookie, node);
			break;
		}
		node = httpcookie_nextnode(node);
	}
}

LOCAL Bool cookiedb_checkinsertioncondition(cookiedb_t *db, httpcookie_t *cookie, STIME current)
{
	W count;
	Bool ok;

	/* domain check */
	if (cookie->domain.len != 0) {
		if (cookie->domain.str[0] != '.') { /* is not error and add period? */
			return False;
		}
		/* same as cookiedb_writeiterator_checksendcondition */
		count = count_priod(&cookie->domain);
		ok = check_specified_TLD(&cookie->domain);
		if (ok == True) {
			if (count < 2) {
				return False;
			}
		} else {
			if (count < 3) {
				return False;
			}
		}
		/* domain and request host check */
		if (cookie->domain.len == (cookie->origin_host.len + 1)) {
			/* for
			 *  domain = .xxx.yyy.zzz
			 *  origin =  xxx.yyy.zzz
			 */
			if (strncmp(cookie->domain.str + 1, cookie->origin_host.str, cookie->origin_host.len) != 0) {
				return False;
			}
		} else {
			ok = ascstr_suffixcmp(&cookie->origin_host, &cookie->domain);
			if (ok == False) {
				return False;
			}
		}
	}

	/* expire check */
	if (cookie->expires == 0) {
		return True;
	}
	if (cookie->expires >= current) {
		return True;
	}

	return False;
}

LOCAL VOID cookiedb_insertwithcheck(cookiedb_t *db, httpcookie_t *cookie, STIME current)
{
	Bool save;
	save = cookiedb_checkinsertioncondition(db, cookie, current);
	if (save == True) {
		cookiedb_insertcookie(db, cookie);
	} else {
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
		cookiedb_insertwithcheck(db, cookie, context->current);
	}
	if (context->reading != NULL) {
		ok = httpcookie_isvalueset(context->reading);
		if (ok == True) {
			httpcookie_QueRemove(context->reading);
			cookiedb_insertwithcheck(db, context->reading, context->current);
		} else {
			httpcookie_delete(context->reading);
		}
		context->reading = NULL;
	}

	cookiedb_readheadercontext_delete(context);
}

EXPORT VOID cookiedb_clearallcookie(cookiedb_t *db)
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

LOCAL W cookiedb_append_astr_recode(W fd, ascstr_t *astr)
{
	return wri_rec(fd, -1, astr->str, astr->len, NULL, NULL, 0);
}

/* tmp */
LOCAL UB dec[] = "0123456789";
LOCAL W UW_to_str(UW n, UB *str)
{
	W i = 0,digit,draw = 0;

	digit = n / 1000000000 % 10;
	if ((digit != 0)||(draw != 0)) {
		str[i++] = dec[digit];
		draw = 1;
	}
	digit = n / 100000000 % 10;
	if ((digit != 0)||(draw != 0)) {
		str[i++] = dec[digit];
		draw = 1;
	}
	digit = n / 10000000 % 10;
	if ((digit != 0)||(draw != 0)) {
		str[i++] = dec[digit];
		draw = 1;
	}
	digit = n / 1000000 % 10;
	if ((digit != 0)||(draw != 0)) {
		str[i++] = dec[digit];
		draw = 1;
	}
	digit = n / 100000 % 10;
	if ((digit != 0)||(draw != 0)) {
		str[i++] = dec[digit];
		draw = 1;
	}
	digit = n / 10000 % 10;
	if ((digit != 0)||(draw != 0)) {
		str[i++] = dec[digit];
		draw = 1;
	}
	digit = n / 1000 % 10;
	if ((digit != 0)||(draw != 0)) {
		str[i++] = dec[digit];
		draw = 1;
	}
	digit = n / 100 % 10;
	if ((digit != 0)||(draw != 0)) {
		str[i++] = dec[digit];
		draw = 1;
	}
	digit = n / 10 % 10;
	if ((digit != 0)||(draw != 0)) {
		str[i++] = dec[digit];
		draw = 1;
	}
	digit = n % 10;
	str[i++] = dec[digit];

	return i;
}

LOCAL W cookiedb_append_STIME_recode(W fd, STIME time)
{
	UB str[10];
	W len;

	len = UW_to_str(time, str);
	return wri_rec(fd, -1, str, len, NULL, NULL, 0);
}

LOCAL W cookiedb_writecookietorecord(httpcookie_t *cookie, W fd)
{
	W err;
	UB paren[2] = "<>";
	UB secure[6] = "secure";
	UB nl[1] = "\n";
	ascstr_t a_paren = {paren, 2};
	ascstr_t a_secure = {secure, 6};
	ascstr_t a_nl = {nl, 1};

	err = cookiedb_append_astr_recode(fd, &cookie->origin_host);
	if (err < 0) {
		return err;
	}
	err = cookiedb_append_astr_recode(fd, &a_paren);
	if (err < 0) {
		return err;
	}
	err = cookiedb_append_astr_recode(fd, &cookie->origin_path);
	if (err < 0) {
		return err;
	}
	err = cookiedb_append_astr_recode(fd, &a_paren);
	if (err < 0) {
		return err;
	}
	err = cookiedb_append_astr_recode(fd, &cookie->attr);
	if (err < 0) {
		return err;
	}
	err = cookiedb_append_astr_recode(fd, &a_paren);
	if (err < 0) {
		return err;
	}
	err = cookiedb_append_astr_recode(fd, &cookie->name);
	if (err < 0) {
		return err;
	}
	err = cookiedb_append_astr_recode(fd, &a_paren);
	if (err < 0) {
		return err;
	}
	err = cookiedb_append_astr_recode(fd, &cookie->comment);
	if (err < 0) {
		return err;
	}
	err = cookiedb_append_astr_recode(fd, &a_paren);
	if (err < 0) {
		return err;
	}
	err = cookiedb_append_astr_recode(fd, &cookie->domain);
	if (err < 0) {
		return err;
	}
	err = cookiedb_append_astr_recode(fd, &a_paren);
	if (err < 0) {
		return err;
	}
	err = cookiedb_append_STIME_recode(fd, cookie->expires);
	if (err < 0) {
		return err;
	}
	err = cookiedb_append_astr_recode(fd, &a_paren);
	if (err < 0) {
		return err;
	}
	err = cookiedb_append_astr_recode(fd, &cookie->path);
	if (err < 0) {
		return err;
	}
	err = cookiedb_append_astr_recode(fd, &a_paren);
	if (err < 0) {
		return err;
	}
	err = cookiedb_append_astr_recode(fd, &cookie->version);
	if (err < 0) {
		return err;
	}
	err = cookiedb_append_astr_recode(fd, &a_paren);
	if (err < 0) {
		return err;
	}
	if (cookie->secure == True) {
		err = cookiedb_append_astr_recode(fd, &a_secure);
		if (err < 0) {
			return err;
		}
	}
	err = cookiedb_append_astr_recode(fd, &a_paren);
	if (err < 0) {
		return err;
	}
	err = cookiedb_append_astr_recode(fd, &a_nl);
	if (err < 0) {
		return err;
	}

	return 0;
}

LOCAL W cookiedb_writecookiestorecord(cookiedb_t *db, W fd)
{
	httpcookie_t *senti, *node;
	W err;

	senti = cookiedb_sentinelnode(db);
	node = httpcookie_nextnode(senti);
	for (; node != senti; node = httpcookie_nextnode(node)) {
		if (node->persistent == False) {
			continue;
		}
		err = cookiedb_writecookietorecord(node, fd);
		if (err < 0) {
			return err;
		}
	}
	return 0;
}

EXPORT W cookiedb_writefile(cookiedb_t *db)
{
	W fd, err = 0;

	fd = opn_fil(db->lnk, F_UPDATE, NULL);
	if (fd < 0) {
		return fd;
	}
	err = fnd_rec(fd, F_TOPEND, 1 << db->rectype, db->subtype, NULL);
	if (err == ER_REC) {
		err = ins_rec(fd, NULL, 0, db->rectype, db->subtype, 0);
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
	err = cookiedb_writecookiestorecord(db, fd);
	if (err < 0) {
		printf("cookiedb_writecookiestorecord error\n");
		trc_rec(fd, 0);
	}
	err = loc_rec(fd, F_UNLOCK);
	cls_fil(fd);
	return err;
}

struct cookiedb_readfilecontext_t_ {
	enum {
		COOKIEDB_READFILECONTEXT_STATE_ORIGIN_HOST,
		COOKIEDB_READFILECONTEXT_STATE_ORIGIN_PATH,
		COOKIEDB_READFILECONTEXT_STATE_ATTR,
		COOKIEDB_READFILECONTEXT_STATE_NAME,
		COOKIEDB_READFILECONTEXT_STATE_COMMENT,
		COOKIEDB_READFILECONTEXT_STATE_DOMAIN,
		COOKIEDB_READFILECONTEXT_STATE_EXPIRES,
		COOKIEDB_READFILECONTEXT_STATE_EXPIRES_ERROR,
		COOKIEDB_READFILECONTEXT_STATE_PATH,
		COOKIEDB_READFILECONTEXT_STATE_VERSION,
		COOKIEDB_READFILECONTEXT_STATE_SECURE,
		COOKIEDB_READFILECONTEXT_STATE_OTHER,
	} state;
	cookiedb_t *db;
	httpcookie_t *buf;
	UW time;
	tokenchecker_t securecheck;
};
typedef struct cookiedb_readfilecontext_t_ cookiedb_readfilecontext_t;

LOCAL Bool cookiedb_readfilecontext_cookiecheck(httpcookie_t *cookie)
{
	if (cookie->origin_host.len == 0) {
		return False;
	}
	if (cookie->origin_path.len == 0) {
		return False;
	}
	if (cookie->attr.len == 0) {
		return False;
	}
	if (cookie->name.len == 0) {
		return False;
	}
	if (cookie->persistent == False) {
		return False;
	}
	return True;
}

LOCAL W cookiedb_readfilecontext_inputcommand(cookiedb_readfilecontext_t *ctx, psvlexer_result_t *cmd)
{
	W i,err,val,ret;
	UB c;
	Bool ok;

	if (cmd->type == PSVLEXER_RESULT_FIELDEND) {
		ok = cookiedb_readfilecontext_cookiecheck(ctx->buf);
		if (ok == False) {
			httpcookie_delete(ctx->buf);
		} else {
			cookiedb_insertcookie(ctx->db, ctx->buf);
		}
		ctx->buf = httpcookie_new(NULL, 0, NULL, 0);
		if (ctx->buf == NULL) {
			return -1; /* TODO */
		}
		ctx->state = COOKIEDB_READFILECONTEXT_STATE_ORIGIN_HOST;
	}

	switch (ctx->state) {
	case COOKIEDB_READFILECONTEXT_STATE_ORIGIN_HOST:
		if (cmd->type == PSVLEXER_RESULT_SEPARATION) {
			ctx->state = COOKIEDB_READFILECONTEXT_STATE_ORIGIN_PATH;
		} else if (cmd->type == PSVLEXER_RESULT_VALUE) {
			err = ascstr_appendstr(&ctx->buf->origin_host, cmd->str, cmd->len);
			if (err < 0) {
				return err;
			}
		}
		break;
	case COOKIEDB_READFILECONTEXT_STATE_ORIGIN_PATH:
		if (cmd->type == PSVLEXER_RESULT_SEPARATION) {
			ctx->state = COOKIEDB_READFILECONTEXT_STATE_ATTR;
		} else if (cmd->type == PSVLEXER_RESULT_VALUE) {
			err = ascstr_appendstr(&ctx->buf->origin_path, cmd->str, cmd->len);
			if (err < 0) {
				return err;
			}
		}
		break;
	case COOKIEDB_READFILECONTEXT_STATE_ATTR:
		if (cmd->type == PSVLEXER_RESULT_SEPARATION) {
			ctx->state = COOKIEDB_READFILECONTEXT_STATE_NAME;
		} else if (cmd->type == PSVLEXER_RESULT_VALUE) {
			err = ascstr_appendstr(&ctx->buf->attr, cmd->str, cmd->len);
			if (err < 0) {
				return err;
			}
		}
		break;
	case COOKIEDB_READFILECONTEXT_STATE_NAME:
		if (cmd->type == PSVLEXER_RESULT_SEPARATION) {
			ctx->state = COOKIEDB_READFILECONTEXT_STATE_COMMENT;
		} else if (cmd->type == PSVLEXER_RESULT_VALUE) {
			err = ascstr_appendstr(&ctx->buf->name, cmd->str, cmd->len);
			if (err < 0) {
				return err;
			}
		}
		break;
	case COOKIEDB_READFILECONTEXT_STATE_COMMENT:
		if (cmd->type == PSVLEXER_RESULT_SEPARATION) {
			ctx->state = COOKIEDB_READFILECONTEXT_STATE_DOMAIN;
		} else if (cmd->type == PSVLEXER_RESULT_VALUE) {
			err = ascstr_appendstr(&ctx->buf->comment, cmd->str, cmd->len);
			if (err < 0) {
				return err;
			}
		}
		break;
	case COOKIEDB_READFILECONTEXT_STATE_DOMAIN:
		if (cmd->type == PSVLEXER_RESULT_SEPARATION) {
			ctx->state = COOKIEDB_READFILECONTEXT_STATE_EXPIRES;
			ctx->time = 0;
		} else if (cmd->type == PSVLEXER_RESULT_VALUE) {
			err = ascstr_appendstr(&ctx->buf->domain, cmd->str, cmd->len);
			if (err < 0) {
				return err;
			}
		}
		break;
	case COOKIEDB_READFILECONTEXT_STATE_EXPIRES:
		if (cmd->type == PSVLEXER_RESULT_SEPARATION) {
			ctx->state = COOKIEDB_READFILECONTEXT_STATE_PATH;
			httpcookie_setexpires(ctx->buf, ctx->time);
		} else if (cmd->type == PSVLEXER_RESULT_VALUE) {
			for (i = 0; i < cmd->len; i++) {
				c = cmd->str[i];
				if (isdigit(c)) {
					ctx->time = ctx->time * 10 + c - '0';
				} else {
					ctx->state = COOKIEDB_READFILECONTEXT_STATE_EXPIRES_ERROR;
					break;
				}
			}
		}
		break;
	case COOKIEDB_READFILECONTEXT_STATE_EXPIRES_ERROR:
		if (cmd->type == PSVLEXER_RESULT_SEPARATION) {
			ctx->state = COOKIEDB_READFILECONTEXT_STATE_PATH;
		}
		break;
	case COOKIEDB_READFILECONTEXT_STATE_PATH:
		if (cmd->type == PSVLEXER_RESULT_SEPARATION) {
			ctx->state = COOKIEDB_READFILECONTEXT_STATE_VERSION;
		} else if (cmd->type == PSVLEXER_RESULT_VALUE) {
			err = ascstr_appendstr(&ctx->buf->path, cmd->str, cmd->len);
			if (err < 0) {
				return err;
			}
		}
		break;
	case COOKIEDB_READFILECONTEXT_STATE_VERSION:
		if (cmd->type == PSVLEXER_RESULT_SEPARATION) {
			ctx->state = COOKIEDB_READFILECONTEXT_STATE_SECURE;
			tokenchecker_clear(&ctx->securecheck);
		} else if (cmd->type == PSVLEXER_RESULT_VALUE) {
			err = ascstr_appendstr(&ctx->buf->version, cmd->str, cmd->len);
			if (err < 0) {
				return err;
			}
		}
		break;
	case COOKIEDB_READFILECONTEXT_STATE_SECURE:
		if (cmd->type == PSVLEXER_RESULT_SEPARATION) {
			ctx->state = COOKIEDB_READFILECONTEXT_STATE_OTHER;
			ret = tokenchecker_endinput(&ctx->securecheck, &val);
			if (ret == TOKENCHECKER_DETERMINE) {
				ctx->buf->secure = True;
			}
		} else if (cmd->type == PSVLEXER_RESULT_VALUE) {
			for (i = 0; i < cmd->len; i++) {
				tokenchecker_inputchar(&ctx->securecheck, cmd->str[i], &val);
			}
		}
		break;
	case COOKIEDB_READFILECONTEXT_STATE_OTHER:
		break;
	}

	return 0;
}

LOCAL tokenchecker_valuetuple_t nList_secure[] = {
	{"secure", 1}
};
LOCAL B eToken_secure[] = "<";

LOCAL W cookiedb_readfilecontext_initialize(cookiedb_readfilecontext_t *ctx, cookiedb_t *db)
{
	ctx->buf = httpcookie_new(NULL, 0, NULL, 0);
	if (ctx->buf == NULL) {
		return -1; /* TODO */
	}
	ctx->db = db;
	ctx->state = COOKIEDB_READFILECONTEXT_STATE_ORIGIN_HOST;
	tokenchecker_initialize(&ctx->securecheck, nList_secure, 1, eToken_secure);
	return 0;
}

LOCAL VOID cookiedb_readfilecontext_finalize(cookiedb_readfilecontext_t *ctx)
{
	tokenchecker_finalize(&ctx->securecheck);
	if (ctx->buf != NULL) {
		httpcookie_delete(ctx->buf);
	}
}

LOCAL W cookiedb_readcookiesfrommemory(cookiedb_t *db, UB *data, W len)
{
	psvlexer_t lexer;
	psvlexer_result_t *result;
	cookiedb_readfilecontext_t context;
	W i,j,err,result_len;

	err = psvlexer_initialize(&lexer);
	if (err < 0) {
		return err;
	}
	err = cookiedb_readfilecontext_initialize(&context, db);
	if (err < 0) {
		psvlexer_finalize(&lexer);
		return err;
	}
	for (i = 0; i < len; i++) {
		psvlexer_inputchar(&lexer, data + i, 1, &result, &result_len);
		err = 0;
		for (j = 0; j < result_len; j++) {
			err = cookiedb_readfilecontext_inputcommand(&context, result + j);
			if (err < 0) {
				break;
			}
		}
		if (err < 0) {
			break;
		}
	}
	cookiedb_readfilecontext_finalize(&context);
	psvlexer_finalize(&lexer);

	return 0;
}

LOCAL W cookiedb_readcookiesfromrecord(cookiedb_t *db, W fd)
{
	W err, r_len;
	UB *r_data;

	err = rea_rec(fd, 0, NULL, 0, &r_len, NULL);
	if (err < 0) {
		return err;
	}
	r_data = malloc(r_len);
	if (r_data == NULL) {
		return err;
	}
	err = rea_rec(fd, 0, r_data, r_len, NULL, NULL);
	if (err < 0) {
		free(r_data);
		return err;
	}

/*
	{
		W i;
		printf("r_len = %d\n", r_len);
		for (i = 0; i < r_len; i++) {
			printf("%c", r_data[i]);
		}
	}
*/

	err = cookiedb_readcookiesfrommemory(db, r_data, r_len);

	free(r_data);

	return err;
}

LOCAL VOID cookiedb_clearpersistentcookie(cookiedb_t *db)
{
	httpcookie_t *senti, *node, *node2;

	senti = cookiedb_sentinelnode(db);
	node = httpcookie_nextnode(senti);
	for (; node != senti;) {
		if (node->persistent == True) {
			node2 = httpcookie_nextnode(node);
			httpcookie_delete(node);
			node = node2;
		} else {
			node = httpcookie_nextnode(node);
		}
	}
}

EXPORT W cookiedb_readfile(cookiedb_t *db)
{
	W fd, err = 0;

	fd = opn_fil(db->lnk, F_UPDATE, NULL);
	if (fd < 0) {
		return fd;
	}
	err = fnd_rec(fd, F_TOPEND, 1 << db->rectype, db->subtype, NULL);
	if (err == ER_REC) {
		cls_fil(fd);
		return 0;
	} else if (err < 0) {
		cls_fil(fd);
		return err;
	}
	err = loc_rec(fd, F_LOCK);
	if (err < 0) {
		cls_fil(fd);
		return err;
	}
	cookiedb_clearpersistentcookie(db);
	err = cookiedb_readcookiesfromrecord(db, fd);
	if (err < 0) {
		trc_rec(fd, 0);
	}
	err = loc_rec(fd, F_UNLOCK);
	cls_fil(fd);
	return err;
}

LOCAL W cookiedb_initialize(cookiedb_t *db, LINK *db_lnk, W rectype, UH subtype)
{
	QueInit(&db->sentinel);
	db->lnk = db_lnk;
	db->rectype = rectype;
	db->subtype = subtype;
	return 0;
}

LOCAL VOID cookiedb_finalize(cookiedb_t *db)
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

EXPORT cookiedb_t* cookiedb_new(LINK *db_lnk, W rectype, UH subtype)
{
	cookiedb_t *db;
	W err;

	db = malloc(sizeof(cookiedb_t));
	if (db == NULL) {
		return NULL;
	}
	err = cookiedb_initialize(db, db_lnk, rectype, subtype);
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
