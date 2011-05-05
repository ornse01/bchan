/*
 * setcookieheader.h
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

#include    <basic.h>

#include    "parselib.h"
#include    "httpdateparser.h"

#ifndef __SETCOOKIEHEADER_H__
#define __SETCOOKIEHEADER_H__

struct httpcookiegeneral_t_ {
	enum {
		HTTPCOOKIEGENERAL_STATE_SEARCH_ATTR,
		HTTPCOOKIEGENERAL_STATE_READ_ATTR,
		HTTPCOOKIEGENERAL_STATE_SEARCH_CH_EQ,
		HTTPCOOKIEGENERAL_STATE_SEARCH_VALUE,
		HTTPCOOKIEGENERAL_STATE_READ_VALUE,
	} state;
};
typedef struct httpcookiegeneral_t_ httpcookiegeneral_t;

enum SETCOOKIEPARSER_ATTR_T_ {
	SETCOOKIEPARSER_ATTR_COMMENT = 1,
	SETCOOKIEPARSER_ATTR_DOMAIN,
	SETCOOKIEPARSER_ATTR_EXPIRES,
	SETCOOKIEPARSER_ATTR_MAX_AGE,
	SETCOOKIEPARSER_ATTR_PATH,
	SETCOOKIEPARSER_ATTR_SECURE,
	SETCOOKIEPARSER_ATTR_VERSION,
	SETCOOKIEPARSER_ATTR_NAME,
};
typedef enum SETCOOKIEPARSER_ATTR_T_ SETCOOKIEPARSER_ATTR_T;

enum SETCOOKIEPARSER_RESULT_TYPE_T_ {
	SETCOOKIEPARSER_RESULT_TYPE_NAMEATTR,
	SETCOOKIEPARSER_RESULT_TYPE_VALUE,
	SETCOOKIEPARSER_RESULT_TYPE_SECUREATTR,
	SETCOOKIEPARSER_RESULT_TYPE_EXPIRESATTR,
};
typedef enum SETCOOKIEPARSER_RESULT_TYPE_T_ SETCOOKIEPARSER_RESULT_TYPE_T;

struct setcookieparser_result_t_ {
	SETCOOKIEPARSER_RESULT_TYPE_T type;
	union {
		struct {
			UB *str;
			W len;
		} name;
		struct {
			UB *str;
			W len;
			SETCOOKIEPARSER_ATTR_T attr;
		} value;
		struct {
			STIME time;
		} expires;
	} val;
};
typedef struct setcookieparser_result_t_ setcookieparser_result_t;

struct setcookieparser_t_ {
	enum {
		SETCOOKIEPARSER_STATE_SEARCH_ATTR,
		SETCOOKIEPARSER_STATE_READ_ATTR,
		SETCOOKIEPARSER_STATE_READ_NAMEATTR,
		SETCOOKIEPARSER_STATE_SKIP_AVPAIR,
		SETCOOKIEPARSER_STATE_VALUE_NAME,
		SETCOOKIEPARSER_STATE_VALUE_EXPIRES,
		SETCOOKIEPARSER_STATE_VALUE_SUPPORTED,
		SETCOOKIEPARSER_STATE_VALUE_UNSUPPORTED,
	} state;
	W attr;
	httpcookiegeneral_t lexer;
	tokenchecker_t attrchecker;
	rfc733dateparser_t dateparser;
	setcookieparser_result_t buffer[2];
	UB buf_str[1];
	DATE_TIM date;
};
typedef struct setcookieparser_t_ setcookieparser_t;

IMPORT W setcookieparser_initialize(setcookieparser_t *parser);
IMPORT VOID setcookieparser_finalize(setcookieparser_t *parser);
#define SETCOOKIEPARSER_CONTINUE 0
#define SETCOOKIEPARSER_ERROR -1
IMPORT W setcookieparser_inputchar(setcookieparser_t *parser, UB ch, setcookieparser_result_t **result, W *result_len);
IMPORT W setcookieparser_endinput(setcookieparser_t *parser, setcookieparser_result_t **result, W *result_len);

#endif
