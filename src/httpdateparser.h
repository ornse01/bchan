/*
 * httpdateparser.h
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
#include	<btron/clk.h>

#include    "parselib.h"

#ifndef __HTTPDATEPARSER_H__
#define __HTTPDATEPARSER_H__

typedef struct {
	enum {
		RFC733DATEPARSER_STATE_START,
		RFC733DATEPARSER_STATE_WEEKDAY,
		RFC733DATEPARSER_STATE_BEFORE_DAY,
		RFC733DATEPARSER_STATE_DAY,
		RFC733DATEPARSER_STATE_BEFORE_MONTH,
		RFC733DATEPARSER_STATE_MONTH,
		RFC733DATEPARSER_STATE_BEFORE_YEAR,
		RFC733DATEPARSER_STATE_YEAR,
		RFC733DATEPARSER_STATE_BEFORE_HOUR,
		RFC733DATEPARSER_STATE_HOUR,
		RFC733DATEPARSER_STATE_MINUTE,
		RFC733DATEPARSER_STATE_SECOND,
		RFC733DATEPARSER_STATE_BEFORE_ZONE,
		RFC733DATEPARSER_STATE_TOKEN_ZONE,
		RFC733DATEPARSER_STATE_VALUE_ZONE
	} state;
	tokenchecker_t read_weekday;
	tokenchecker_t read_month;
	tokenchecker_t read_tokenTIMEZONE;
	W localtimediff;
} rfc733dateparser_t;

#define HTTPDATEPARSER_CONTINUE 0
#define HTTPDATEPARSER_ERROR -1
#define HTTPDATEPARSER_DETERMINE 1

IMPORT VOID rfc733dateparser_initialize(rfc733dateparser_t *parsestate);
IMPORT VOID rfc733dateparser_finalize(rfc733dateparser_t *parsestate);
IMPORT W rfc733dateparser_inputchar(rfc733dateparser_t *parsestate, UB c, DATE_TIM *date);
EXPORT W rfc733dateparser_endinput(rfc733dateparser_t *parsestate, DATE_TIM *date);

#endif
