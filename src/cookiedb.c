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
	ascstr_t attr;
	ascstr_t name;
	ascstr_t comment;
	ascstr_t domain;
	Bool persistent;
	STIME expire;
	ascstr_t path;
	ascstr_t version;
	Bool secure;
};
typedef struct httpcookie_t_ httpcookie_t;

LOCAL W httpcookie_setexpire(httpcookie_t *cookie, DATE_TIM *date)
{
	return set_tod(date, &cookie->expire, 0);
}

LOCAL VOID httpcookie_QueRemove(httpcookie_t *cookie)
{
	QueRemove(&cookie->que);
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
};
typedef struct cookie_persistentdb_t_ cookie_persistentdb_t;

struct cookie_volatiledb_t_ {
	QUEUE sentinel;
};
typedef struct cookie_volatiledb_t_ cookie_volatiledb_t;

struct cookiedb_t_ {
	cookie_volatiledb_t vdb;
	cookiedb_readheadercontext_t *readcontext;
};

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

struct cookiedb_readheadercontext_t_ {
	ascstr_t host;
	STIME current;
	QUEUE sentinel;
	enum {
		COOKIEDB_READHEADERCONTEXT_STATE_START,
		COOKIEDB_READHEADERCONTEXT_STATE_NAMEATTR,
		COOKIEDB_READHEADERCONTEXT_STATE_EXPIRE,
		COOKIEDB_READHEADERCONTEXT_STATE_OTHERVAL,
	} state;
	rfc733dateparser_t dateparser;
	DATE_TIM expire_buf;
	httpcookie_t *reading;
};

LOCAL VOID cookiedb_readheadercontext_insertcookie(cookiedb_readheadercontext_t *context, httpcookie_t *cookie)
{
	QueInsert(&cookie->que, &context->sentinel);
}

EXPORT W cookiedb_readheadercontext_appendchar_attr(cookiedb_readheadercontext_t *context, UB ch)
{
	W err;
	if (context->state == COOKIEDB_READHEADERCONTEXT_STATE_EXPIRE) {
		err = rfc733dateparser_endinput(&context->dateparser, &context->expire_buf);
		rfc733dateparser_finalize(&context->dateparser);
		if (err == HTTPDATEPARSER_DETERMINE) {
			httpcookie_setexpire(context->reading, &context->expire_buf);
		}
	}
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
	W err;
	if (context->state == COOKIEDB_READHEADERCONTEXT_STATE_START) {
		return 0;
	}
	if (context->state == COOKIEDB_READHEADERCONTEXT_STATE_EXPIRE) {
		err = rfc733dateparser_endinput(&context->dateparser, &context->expire_buf);
		rfc733dateparser_finalize(&context->dateparser);
		if (err == HTTPDATEPARSER_DETERMINE) {
			httpcookie_setexpire(context->reading, &context->expire_buf);
		}
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

EXPORT W cookiedb_readheadercontext_appendchar_expires(cookiedb_readheadercontext_t *context, UB ch)
{
	W ret;

	if (context->state == COOKIEDB_READHEADERCONTEXT_STATE_START) {
		return 0;
	}
	if (context->state != COOKIEDB_READHEADERCONTEXT_STATE_EXPIRE) {
		rfc733dateparser_initialize(&context->dateparser);
		memset(&context->expire_buf, 0, sizeof(DATE_TIM));
		context->state = COOKIEDB_READHEADERCONTEXT_STATE_EXPIRE;
	}
	ret = rfc733dateparser_inputchar(&context->dateparser, ch, &context->expire_buf);
	/* TODO: format error handling */

	return 0;
}

EXPORT W cookiedb_readheadercontext_appendchar_max_age(cookiedb_readheadercontext_t *context, UB ch)
{
	/* TODO */
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
	if (context->state == COOKIEDB_READHEADERCONTEXT_STATE_EXPIRE) {
	}
	context->state = COOKIEDB_READHEADERCONTEXT_STATE_OTHERVAL;

	context->reading->secure = True;

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
	context->reading = httpcookie_new(host, host_len);
	if (context->reading == NULL) {
		ascstr_finalize(&context->host);
		free(context);
		return NULL;
	}
	QueInit(&context->sentinel);
	context->reading = NULL;
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
	if (context->reading == NULL) {
		httpcookie_delete(context->reading);
	}

	ascstr_finalize(&context->host);
	free(context);
}

EXPORT cookiedb_readheadercontext_t* cookiedb_startheaderread(cookiedb_t *db, UB *host, W host_len, STIME time)
{
	cookiedb_readheadercontext_t *context;

	if (db->readcontext != NULL) {
		return NULL;
	}

	context = cookiedb_readheadercontext_new(host, host_len, time);
	if (context == NULL) {
		return NULL;
	}
	db->readcontext = context;

	return context;
}

EXPORT VOID cookiedb_endheaderread(cookiedb_t *db, cookiedb_readheadercontext_t *context)
{
	httpcookie_t *cookie;

	for (;;) {
		cookie = cookiedb_readheadercontext_prevcookie(context);
		if (cookie == NULL) {
			break;
		}
		httpcookie_QueRemove(cookie);
		cookie_volatiledb_insertcookie(&db->vdb, cookie);
	}
	if (context->reading != NULL) {
		httpcookie_QueRemove(context->reading);
		cookie_volatiledb_insertcookie(&db->vdb, context->reading);
		context->reading = NULL;
	}

	cookiedb_readheadercontext_delete(context);
	db->readcontext = NULL;
}

EXPORT W cookiedb_clearallcookie(cookiedb_t *db)
{
}

LOCAL W cookiedb_initialize(cookiedb_t *db)
{
	W err;

	err = cookie_volatiledb_initialize(&db->vdb);
	if (err < 0) {
		return err;
	}
	db->readcontext = NULL;

	return 0;
}

LOCAL VOID cookiedb_finalize(cookiedb_t *db)
{
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
	err = cookiedb_initialize(db);
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
