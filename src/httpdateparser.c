/*
 * httpdateparser.c
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

#include    "httpdateparser.h"
#include    "parselib.h"

#include	<basic.h>
#include	<bstdio.h>
#include	<bctype.h>
#include	<btron/clk.h>

#ifdef BCHAN_CONFIG_DEBUG
# define DP(arg) printf arg
# define DP_ER(msg, err) printf("%s (%d/%x)\n", msg, err>>16, err)
#else
# define DP(arg) /**/
# define DP_ER(msg, err) /**/
#endif

#if 0
#define DP_STATE(arg) printf arg
#else
#define DP_STATE(arg) /**/
#endif

typedef enum {
	HTTP_MESSAGEVALUE_DATE_WEEKDAY_MON = 1,
	HTTP_MESSAGEVALUE_DATE_WEEKDAY_TUE,
	HTTP_MESSAGEVALUE_DATE_WEEKDAY_WED,
	HTTP_MESSAGEVALUE_DATE_WEEKDAY_THU,
	HTTP_MESSAGEVALUE_DATE_WEEKDAY_FRI,
	HTTP_MESSAGEVALUE_DATE_WEEKDAY_SAT,
	HTTP_MESSAGEVALUE_DATE_WEEKDAY_SUN,
	HTTP_MESSAGEVALUE_DATE_WEEKDAY_MONDAY,
	HTTP_MESSAGEVALUE_DATE_WEEKDAY_TUESDAY,
	HTTP_MESSAGEVALUE_DATE_WEEKDAY_WEDNESDAY,
	HTTP_MESSAGEVALUE_DATE_WEEKDAY_THURSDAY,
	HTTP_MESSAGEVALUE_DATE_WEEKDAY_FRIDAY,
	HTTP_MESSAGEVALUE_DATE_WEEKDAY_SATURDAY,
	HTTP_MESSAGEVALUE_DATE_WEEKDAY_SUNDAY
} http_messagevalue_date_weekday_t;

typedef enum {
	HTTP_MESSAGE_VALUE_DATE_MONTH_JAN = 1,
	HTTP_MESSAGE_VALUE_DATE_MONTH_FEB,
	HTTP_MESSAGE_VALUE_DATE_MONTH_MAR,
	HTTP_MESSAGE_VALUE_DATE_MONTH_APR,
	HTTP_MESSAGE_VALUE_DATE_MONTH_MAY,
	HTTP_MESSAGE_VALUE_DATE_MONTH_JUN,
	HTTP_MESSAGE_VALUE_DATE_MONTH_JUL,
	HTTP_MESSAGE_VALUE_DATE_MONTH_AUG,
	HTTP_MESSAGE_VALUE_DATE_MONTH_SEP,
	HTTP_MESSAGE_VALUE_DATE_MONTH_OCT,
	HTTP_MESSAGE_VALUE_DATE_MONTH_NOV,
	HTTP_MESSAGE_VALUE_DATE_MONTH_DEC
} http_messagevalue_date_month_t;

LOCAL tokenchecker_valuetuple_t nList_WEEKDAYS[] = {
	{"Fri",       HTTP_MESSAGEVALUE_DATE_WEEKDAY_FRI},
	{"Friday",    HTTP_MESSAGEVALUE_DATE_WEEKDAY_FRIDAY},
	{"Mon",       HTTP_MESSAGEVALUE_DATE_WEEKDAY_MON},
	{"Monday",    HTTP_MESSAGEVALUE_DATE_WEEKDAY_MONDAY},
	{"Sat",       HTTP_MESSAGEVALUE_DATE_WEEKDAY_SAT},
	{"Saturday",  HTTP_MESSAGEVALUE_DATE_WEEKDAY_SATURDAY},
	{"Sun",       HTTP_MESSAGEVALUE_DATE_WEEKDAY_SUN},
	{"Sunday",    HTTP_MESSAGEVALUE_DATE_WEEKDAY_SUNDAY},
	{"Thu",       HTTP_MESSAGEVALUE_DATE_WEEKDAY_THU},
	{"Thursday",  HTTP_MESSAGEVALUE_DATE_WEEKDAY_THURSDAY},
	{"Tue",       HTTP_MESSAGEVALUE_DATE_WEEKDAY_TUE},
	{"Tuesday",   HTTP_MESSAGEVALUE_DATE_WEEKDAY_TUESDAY},
	{"Wed",       HTTP_MESSAGEVALUE_DATE_WEEKDAY_WED},
	{"Wednesday", HTTP_MESSAGEVALUE_DATE_WEEKDAY_WEDNESDAY},
};
LOCAL B eToken_httpdate_weekday[] = " ,";

LOCAL tokenchecker_valuetuple_t nList_MONTHS[] = {
	{"Apr", HTTP_MESSAGE_VALUE_DATE_MONTH_APR},
	{"April", HTTP_MESSAGE_VALUE_DATE_MONTH_APR},
	{"Aug", HTTP_MESSAGE_VALUE_DATE_MONTH_AUG},
	{"August", HTTP_MESSAGE_VALUE_DATE_MONTH_AUG},
	{"Dec", HTTP_MESSAGE_VALUE_DATE_MONTH_DEC},
	{"December", HTTP_MESSAGE_VALUE_DATE_MONTH_DEC},
	{"Feb", HTTP_MESSAGE_VALUE_DATE_MONTH_FEB},
	{"February", HTTP_MESSAGE_VALUE_DATE_MONTH_FEB},
	{"Jan", HTTP_MESSAGE_VALUE_DATE_MONTH_JAN},
	{"January" , HTTP_MESSAGE_VALUE_DATE_MONTH_JAN},
	{"Jul", HTTP_MESSAGE_VALUE_DATE_MONTH_JUL},
	{"July", HTTP_MESSAGE_VALUE_DATE_MONTH_JUL},
	{"Jun", HTTP_MESSAGE_VALUE_DATE_MONTH_JUN},
	{"June", HTTP_MESSAGE_VALUE_DATE_MONTH_JUN},
	{"Mar", HTTP_MESSAGE_VALUE_DATE_MONTH_MAR},
	{"March" , HTTP_MESSAGE_VALUE_DATE_MONTH_MAR},
	{"May", HTTP_MESSAGE_VALUE_DATE_MONTH_MAY},
	{"Nov", HTTP_MESSAGE_VALUE_DATE_MONTH_NOV},
	{"November", HTTP_MESSAGE_VALUE_DATE_MONTH_NOV},
	{"Oct", HTTP_MESSAGE_VALUE_DATE_MONTH_OCT},
	{"October", HTTP_MESSAGE_VALUE_DATE_MONTH_OCT},
	{"Sep", HTTP_MESSAGE_VALUE_DATE_MONTH_SEP},
	{"September" , HTTP_MESSAGE_VALUE_DATE_MONTH_SEP},
};
LOCAL B eToken_httpdate_month[] = " -";

typedef enum {
	HTTPDATE_TIMEZONE_GMT = 1,
	HTTPDATE_TIMEZONE_NST,
	HTTPDATE_TIMEZONE_AST,
	HTTPDATE_TIMEZONE_ADT,
	HTTPDATE_TIMEZONE_EST,
	HTTPDATE_TIMEZONE_EDT,
	HTTPDATE_TIMEZONE_CST,
	HTTPDATE_TIMEZONE_CDT,
	HTTPDATE_TIMEZONE_MST,
	HTTPDATE_TIMEZONE_MDT,
	HTTPDATE_TIMEZONE_PST,
	HTTPDATE_TIMEZONE_PDT,
	HTTPDATE_TIMEZONE_YST,
	HTTPDATE_TIMEZONE_YDT,
	HTTPDATE_TIMEZONE_HST,
	HTTPDATE_TIMEZONE_HDT,
	HTTPDATE_TIMEZONE_BST,
	HTTPDATE_TIMEZONE_BDT,
} httpdate_timezone_t;

/* TODO: Military token*/
LOCAL tokenchecker_valuetuple_t nList_tokenTIMEZONE[] = {
	{"ADT", HTTPDATE_TIMEZONE_ADT},
	{"AST", HTTPDATE_TIMEZONE_AST},
	{"BDT", HTTPDATE_TIMEZONE_BDT},
	{"BST", HTTPDATE_TIMEZONE_BST},
	{"CDT", HTTPDATE_TIMEZONE_CDT},
	{"CST", HTTPDATE_TIMEZONE_CST},
	{"EDT", HTTPDATE_TIMEZONE_EDT},
	{"EST", HTTPDATE_TIMEZONE_EST},
	{"GMT", HTTPDATE_TIMEZONE_GMT},
	{"HDT", HTTPDATE_TIMEZONE_HDT},
	{"HST", HTTPDATE_TIMEZONE_HST},
	{"MDT", HTTPDATE_TIMEZONE_MDT},
	{"MST", HTTPDATE_TIMEZONE_MST},
	{"NST", HTTPDATE_TIMEZONE_NST},
	{"PDT", HTTPDATE_TIMEZONE_PDT},
	{"PST", HTTPDATE_TIMEZONE_PST},
	{"YDT", HTTPDATE_TIMEZONE_YDT},
	{"YST", HTTPDATE_TIMEZONE_YST},
};
LOCAL B eToken_httpdate_tokenTIMEZONE[] = " \r\n";

EXPORT W rfc733dateparser_inputchar(rfc733dateparser_t *parsestate, UB c, DATE_TIM *date)
{
	W ret, val;

	switch(parsestate->state){
	case RFC733DATEPARSER_STATE_START:
		if (isdigit(c)) {
			parsestate->state = RFC733DATEPARSER_STATE_DAY;
			date->d_day = c - '0';
			return HTTPDATEPARSER_CONTINUE;
		}
		parsestate->state = RFC733DATEPARSER_STATE_WEEKDAY;
	case RFC733DATEPARSER_STATE_WEEKDAY:
		ret = tokenchecker_inputchar(&(parsestate->read_weekday), c, &val);
		if (ret == TOKENCHECKER_CONTINUE) {
			return HTTPDATEPARSER_CONTINUE;
		}
		if (ret != TOKENCHECKER_DETERMINE) {
			return HTTPDATEPARSER_ERROR;
		}
		if(c != ','){
			return HTTPDATEPARSER_ERROR;
		}
		parsestate->state = RFC733DATEPARSER_STATE_BEFORE_DAY;
		return HTTPDATEPARSER_CONTINUE;
	case RFC733DATEPARSER_STATE_BEFORE_DAY:
		if (c == ' ') {
			return HTTPDATEPARSER_CONTINUE;
		}
		if (!isdigit(c)) {
			return HTTPDATEPARSER_ERROR;
		}
		parsestate->state = RFC733DATEPARSER_STATE_DAY;
		date->d_day = c - '0';
		return HTTPDATEPARSER_CONTINUE;
	case RFC733DATEPARSER_STATE_DAY:
		if (isdigit(c)) {
			date->d_day = date->d_day*10 + c - '0';
			return HTTPDATEPARSER_CONTINUE;
		}
		if ((c == ' ')||(c == '-')) {
			parsestate->state = RFC733DATEPARSER_STATE_BEFORE_MONTH;
			return HTTPDATEPARSER_CONTINUE;
		}
		return HTTPDATEPARSER_ERROR;
	case RFC733DATEPARSER_STATE_BEFORE_MONTH:
		if (c == ' ') {
			return HTTPDATEPARSER_CONTINUE;
		}
		if (!isalpha(c)) {
			return HTTPDATEPARSER_ERROR;
		}
		parsestate->state = RFC733DATEPARSER_STATE_MONTH;
	case RFC733DATEPARSER_STATE_MONTH:
		ret = tokenchecker_inputchar(&(parsestate->read_month), c, &val);
		if (ret == TOKENCHECKER_CONTINUE) {
			return HTTPDATEPARSER_CONTINUE;
		}
		if (ret != TOKENCHECKER_DETERMINE) {
			return HTTPDATEPARSER_ERROR;
		}
		date->d_month = val;
		parsestate->state = RFC733DATEPARSER_STATE_BEFORE_YEAR;
		return HTTPDATEPARSER_CONTINUE;
	case RFC733DATEPARSER_STATE_BEFORE_YEAR:
		if (c == ' ') {
			return HTTPDATEPARSER_CONTINUE;
		}
		if (!isdigit(c)) {
			return HTTPDATEPARSER_ERROR;
		}
		parsestate->state = RFC733DATEPARSER_STATE_YEAR;
		date->d_year = c - '0';
		return HTTPDATEPARSER_CONTINUE;
	case RFC733DATEPARSER_STATE_YEAR:
		if (isdigit(c)) {
			date->d_year = date->d_year*10 + c - '0';
			return HTTPDATEPARSER_CONTINUE;
		}
		if (c == ' ') {
			date->d_year -= 1900;
			parsestate->state = RFC733DATEPARSER_STATE_BEFORE_HOUR;
			return HTTPDATEPARSER_CONTINUE;
		}
		return HTTPDATEPARSER_ERROR;
	case RFC733DATEPARSER_STATE_BEFORE_HOUR:
		if (c == ' ') {
			return HTTPDATEPARSER_CONTINUE;
		}
		if (!isdigit(c)) {
			return HTTPDATEPARSER_ERROR;
		}
		date->d_hour = c - '0';
		parsestate->state = RFC733DATEPARSER_STATE_HOUR;
		return HTTPDATEPARSER_CONTINUE;
	case RFC733DATEPARSER_STATE_HOUR:
		if (isdigit(c)) {
			date->d_hour = date->d_hour*10 + c - '0';
			return HTTPDATEPARSER_CONTINUE;
		}
		if (c == ':') {
			parsestate->state = RFC733DATEPARSER_STATE_MINUTE;
			date->d_min = 0;
			return HTTPDATEPARSER_CONTINUE;
		}
		return HTTPDATEPARSER_ERROR;
	case RFC733DATEPARSER_STATE_MINUTE:
		if (isdigit(c)) {
			date->d_min = date->d_min*10 + c - '0';
			return HTTPDATEPARSER_CONTINUE;
		}
		if (c == ':') {
			parsestate->state = RFC733DATEPARSER_STATE_SECOND;
			date->d_sec = 0;
			return HTTPDATEPARSER_CONTINUE;
		}
		return HTTPDATEPARSER_ERROR;
	case RFC733DATEPARSER_STATE_SECOND:
		if (isdigit(c)) {
			date->d_sec = date->d_sec*10 + c - '0';
			return HTTPDATEPARSER_CONTINUE;
		}
		if (c == ' ') {
			parsestate->state = RFC733DATEPARSER_STATE_BEFORE_ZONE;
			return HTTPDATEPARSER_CONTINUE;
		}
		return HTTPDATEPARSER_ERROR;
	case RFC733DATEPARSER_STATE_BEFORE_ZONE:
		if (c == ' ') {
			return HTTPDATEPARSER_CONTINUE;
		}
		if ((c == '+')&&(c == '-')) {
			parsestate->state = RFC733DATEPARSER_STATE_VALUE_ZONE;
			return HTTPDATEPARSER_CONTINUE;
		}
		if (!isalpha(c)) {
			return HTTPDATEPARSER_ERROR;
		}
		parsestate->state = RFC733DATEPARSER_STATE_TOKEN_ZONE;
	case RFC733DATEPARSER_STATE_TOKEN_ZONE:
		ret = tokenchecker_inputchar(&(parsestate->read_tokenTIMEZONE), c, &val);
		if (ret == TOKENCHECKER_CONTINUE) {
			return HTTPDATEPARSER_CONTINUE;
		}
		if (ret != TOKENCHECKER_DETERMINE) {
			return HTTPDATEPARSER_ERROR;
		}
		return HTTPDATEPARSER_DETERMINE;
	case RFC733DATEPARSER_STATE_VALUE_ZONE:
		/* TODO */
		return HTTPDATEPARSER_ERROR;
	}

	return HTTPDATEPARSER_ERROR;
}

EXPORT W rfc733dateparser_endinput(rfc733dateparser_t *parsestate, DATE_TIM *date)
{
	W ret, val;

	switch (parsestate->state) {
	case RFC733DATEPARSER_STATE_TOKEN_ZONE:
		ret = tokenchecker_inputchar(&(parsestate->read_tokenTIMEZONE), ' ', &val);
		if (ret != TOKENCHECKER_DETERMINE) {
			return HTTPDATEPARSER_ERROR;
		}
		return HTTPDATEPARSER_DETERMINE;
	case RFC733DATEPARSER_STATE_VALUE_ZONE:
		/* TODO */
		return HTTPDATEPARSER_ERROR;
	default:
		return HTTPDATEPARSER_ERROR;
	}

	return HTTPDATEPARSER_ERROR;
}

EXPORT VOID rfc733dateparser_initialize(rfc733dateparser_t *parsestate)
{
	tokenchecker_initialize(&parsestate->read_weekday, nList_WEEKDAYS, 14, eToken_httpdate_weekday);
	tokenchecker_initialize(&parsestate->read_month, nList_MONTHS, 23, eToken_httpdate_month);
	tokenchecker_initialize(&parsestate->read_tokenTIMEZONE, nList_tokenTIMEZONE, 18, eToken_httpdate_tokenTIMEZONE);
	parsestate->state = RFC733DATEPARSER_STATE_START;
	parsestate->localtimediff = 0;
}

EXPORT VOID rfc733dateparser_finalize(rfc733dateparser_t *parsestate)
{
	tokenchecker_finalize(&parsestate->read_tokenTIMEZONE);
	tokenchecker_finalize(&parsestate->read_month);
	tokenchecker_finalize(&parsestate->read_weekday);
}
