/*
 * sjisstring.c
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
#include	<bctype.h>
#include	<tstring.h>

#include    "sjisstring.h"

EXPORT W sjstring_appendasciistring(UB **dest, W *dest_len, UB *str, W len)
{
	if (len <= 0) {
		return 0;
	}

	*dest = realloc(*dest, *dest_len + len + 1);
	if (*dest == NULL) {
		*dest_len = 0;
		return -1;
	}
	memcpy((*dest)+*dest_len, str, len);
	*dest_len += len;
	(*dest)[*dest_len] = '\0';

	return 0;
}

LOCAL UB dec[] = "0123456789";

EXPORT W sjstring_appendUWstring(UB **dest, W *dlen, UW n)
{
	W i = 0,digit,draw = 0;
	UB str[10];

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

	return sjstring_appendasciistring(dest, dlen, str, i);
}

/* rfc2396.txt */
LOCAL W urlencode_needlength(UB *bin, W len)
{
	W i, ret = 0;

	for (i=0;i<len;i++) {
		if (bin[i] == 0x2E) { /* '.' */
			ret++;
		} else if (bin[i] == 0x2D) { /* '-' */
			ret++;
		} else if (bin[i] == 0x5F) { /* '_' */
			ret++;
		} else if (bin[i] == 0x20) { /* ' ' */
			ret++;
		} else if (isalnum(bin[i])) {
			ret++;
		} else {
			ret+=3;
		}
	}

	return ret;
}

LOCAL W urlencode_convert(UB *dest, W dest_len, UB *src, W src_len)
{
	W i,j = 0;
	static UB num[] = "0123456789ABCDEF";

	for (i=0;i<src_len;i++) {
		if (src[i] == 0x2E) { /* '.' */
			dest[j++] = src[i];
		} else if (src[i] == 0x2D) { /* '-' */
			dest[j++] = src[i];
		} else if (src[i] == 0x5F) { /* '_' */
			dest[j++] = src[i];
		} else if (src[i] == 0x20) { /* ' ' */
			dest[j++] = 0x2B; /* '+' */
		} else if (isalnum(src[i])) {
			dest[j++] = src[i];
		} else {
			dest[j++] = 0x25; /* '%' */
			if (j == dest_len) {
				break;
			}
			dest[j++] = num[(src[i] >> 4)];
			if (j == dest_len) {
				break;
			}
			dest[j++] = num[(src[i] & 0xF)];
		}
		if (j == dest_len) {
			break;
		}
	}

	return j;
}

EXPORT W sjstring_appendurlencodestring(UB **dest, W *dest_len, UB *str, W len)
{
	W encoded_len;

	if (len == 0) {
		return 0;
	}

	encoded_len = urlencode_needlength(str, len);

	*dest = realloc(*dest, *dest_len + encoded_len + 1);
	if (*dest == NULL) {
		return -1; /* TODO */
	}

	urlencode_convert(*dest + *dest_len, encoded_len, str, len);

	*dest_len += encoded_len;
	(*dest)[*dest_len] = '\0';

	return 0;
}

EXPORT W sjstring_appendconvartingTCstring(UB **dest, W *dest_len, TC *str, W len)
{
	W i,j,converted_len = 0, ret;
	UB conv[2];

	for (i = 0; i < len; i++) {
		ret = tctosj(NULL, str[i]);
		if (ret == -1) {
			return -1; /* TODO */
		}
		converted_len += ret;
	}

	*dest = realloc(*dest, *dest_len + converted_len + 1);
	if (*dest == NULL) {
		return -1; /* TODO */
	}

	j = 0;
	for (i = 0; i < len; i++) {
		ret = tctosj(conv, str[i]);
		if (ret == -1) {
			return -1; /* TODO */
		}
		memcpy(*(dest)+*dest_len+j, conv, ret);
		j += ret;
	}

	*dest_len += converted_len;
	(*dest)[*dest_len] = '\0';

	return 0;
}

EXPORT W sjstring_appendformpoststring(UB **dest, W *dest_len, UB *str, W len)
{
	UB ch0,si;
	W i,err;
 
	si = 0;
	for (i = 0; i < len; i++) {
		ch0 = str[i];
		if (((0x81 <= ch0)&&(ch0 <= 0x9f))
			||((0xe0 <= ch0)&&(ch0 <= 0xef))) {
			i++;
			continue;
		}

		if (ch0 == '<') {
			err = sjstring_appendurlencodestring(dest, dest_len, str+si, i-si);
			if (err < 0) {
				return err;
			}
			err = sjstring_appendurlencodestring(dest, dest_len, "&lt;", 4);
			if (err < 0) {
				return err;
			}
			si = i+1;
		} else if (ch0 == '>') {
			err = sjstring_appendurlencodestring(dest, dest_len, str+si, i-si);
			if (err < 0) {
				return err;
			}
			err = sjstring_appendurlencodestring(dest, dest_len, "&gt;", 4);
			if (err < 0) {
				return err;
			}
			si = i+1;
		} else if (ch0 == '"') {
			err = sjstring_appendurlencodestring(dest, dest_len, str+si, i-si);
			if (err < 0) {
				return err;
			}
			err = sjstring_appendurlencodestring(dest, dest_len, "&quot;", 6);
			if (err < 0) {
				return err;
			}
			si = i+1;
		}
	}

	return sjstring_appendurlencodestring(dest, dest_len, str+si, i-si);
}

EXPORT UB* sjstring_searchchar(UB *str, W len, UB ch)
{
	UB ch0;
	W i;

	/* not lower byte of 2 bytes character in Shift_JIS. */
	if (!(((0x40 <= ch)&&(ch <= 0x7e))||((0x80 < ch)&&(ch < 0xfc)))) {
		for (i = 0; i < len; i++) {
			if (str[i] == ch) {
				return str + i;
			}
			if (str[i] == '\0') {
				return NULL;
			}
		}
		return NULL;
	}

	for (i = 0; i < len; i++) {
		ch0 = str[i];
		if (((0x81 <= ch0)&&(ch0 <= 0x9f))
			||((0xe0 <= ch0)&&(ch0 <= 0xef))) {
			i++;
			continue;
		}
		if (ch0 == ch) {
			return str + i;
		}
		if (ch0 == '\0') {
			return NULL;
		}
	}

	return NULL;
}

EXPORT W sjstring_totcs(UB *sjstr, UB sjstr_len, TC *tcstr)
{
	W i, ret, tclen = 0;

	if (tcstr == NULL) {
		for (i = 0; i < sjstr_len; i++) {
			ret = sjtotc(NULL, sjstr + i);
			switch (ret) {
			case -1:
				return -1;
			case 0:
				break;
			case 1:
				tclen++;
				break;
			case 2:
				tclen++;
				i++;
				break;
			}
		}
		return tclen;
	}

	for (i = 0; i < sjstr_len; i++) {
		ret = sjtotc(tcstr + tclen, sjstr + i);
		switch (ret) {
		case -1:
			return -1;
		case 0:
			break;
		case 1:
			tclen++;
			break;
		case 2:
			tclen++;
			i++;
			break;
		}
	}

	return tclen;
}

/* from rfc1738. 2.2. URL Character Encoding Issues */
/* should be more speed up. */
EXPORT Bool sjstring_isurlusablecharacter(UB ch)
{
	/* No corresponding graphic US-ASCII */
	if ((0x00 <= ch)&&(ch <= 0x1F)) {
		return False;
	}
	if (ch == 0x7F) {
		return False;
	}
	if ((0x80 <= ch)&&(ch <= 0xFF)) {
		return False;
	}

	/* Unsafe */
	if (ch == '<') {
		return False;
	}
	if (ch == '>') {
		return False;
	}
	if (ch == '"') {
		return False;
	}
	if (ch == '{') {
		return False;
	}
	if (ch == '}') {
		return False;
	}
	if (ch == '|') {
		return False;
	}
	if (ch == '\\') {
		return False;
	}
	if (ch == '^') {
		return False;
	}
	if (ch == '~') {
		return False;
	}
	if (ch == '[') {
		return False;
	}
	if (ch == ']') {
		return False;
	}
	if (ch == '`') {
		return False;
	}

	/* Reserved */
	/*  ignore ";", "/", "?", ":", "@", "=" and "&" */
	/*  to handling these, need parsing. */
	/* special characters */
	if (ch == '$') {
		return False;
	}
	if (ch == '-') {
		return False;
	}
	if (ch == '_') {
		return False;
	}
	if (ch == '.') {
		return False;
	}
	if (ch == '+') {
		return False;
	}
	if (ch == '!') {
		return False;
	}
	if (ch == '*') {
		return False;
	}
	if (ch == '\'') {
		return False;
	}
	if (ch == '(') {
		return False;
	}
	if (ch == ')') {
		return False;
	}
	if (ch == ',') {
		return False;
	}

	/* whitespace */
	/*  should be ignore? */
	/*   by "APPENDIX: Recommendations for URLs in Context" */
	/*  to handling these, need parsing. */
	if (ch == ' ') {
		return False;
	}
	if (ch == '\t') {
		return False;
	}
	if (ch == '\n') {
		return False;
	}
	if (ch == '\r') {
		return False;
	}

	return True;
}

#ifdef BCHAN_CONFIG_DEBUG
#include	<tstring.h>

EXPORT VOID SJSTRING_DP(UB *str, W len)
{
	W i, print_len;
	TC *print;

	print_len = sjstotcs(NULL, str);
	print = malloc(sizeof(TC)*(print_len + 1));
	if (print == NULL) {
		return;
	}

	sjstotcs(print, str);
	print[print_len] = TNULL;

	for (i=0;i<print_len;i++) {
		printf("%C", print[i]);
	}

	free(print);
}
#endif
