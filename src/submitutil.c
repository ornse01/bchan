/*
 * submitutil.c
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

#include	<basic.h>
#include	<bstdio.h>
#include	<bstdlib.h>
#include	<bstring.h>

#include    "submitutil.h"
#include    "sjisstring.h"
#include    "parselib.h"

#define MAKEHEADER_ERR_LEN(dest, dest_len, src, len) \
   err = sjstring_appendasciistring(dest, dest_len, src, len); \
   if(err < 0){ \
     return err; \
   }

#define MAKEHEADER_ERR(dest, dest_len, src) MAKEHEADER_ERR_LEN(dest, dest_len, (src), strlen((src)))

#define MAKEHEADER_NUM_ERR(dest, dest_len, num) \
   err = sjstring_appendUWstring(dest, dest_len, num); \
   if(err < 0){ \
     return err; \
   }

EXPORT W submitutil_makeheaderstring(UB *host, W host_len, UB *board, W board_len, UB *thread, W thread_len, W content_length, UB **header, W *header_len)
{
	UB *str = NULL;
	W len = 0;
	W err;

	MAKEHEADER_ERR(&str, &len, "POST /test/bbs.cgi HTTP/1.1\r\n");
	MAKEHEADER_ERR(&str, &len, "Host: ");
	MAKEHEADER_ERR_LEN(&str, &len, host, host_len);
	MAKEHEADER_ERR(&str, &len, "\r\n");
	MAKEHEADER_ERR(&str, &len, "Accept: */*\r\n");
	MAKEHEADER_ERR(&str, &len, "Referer: http://");
	MAKEHEADER_ERR_LEN(&str, &len, host, host_len);
	MAKEHEADER_ERR(&str, &len, "/test/read.cgi/");
	MAKEHEADER_ERR_LEN(&str, &len, board, board_len);
	MAKEHEADER_ERR(&str, &len, "/");
	MAKEHEADER_ERR_LEN(&str, &len, thread, thread_len);
	MAKEHEADER_ERR(&str, &len, "/\r\n");
	MAKEHEADER_ERR(&str, &len, "Content-Length: ");
	MAKEHEADER_NUM_ERR(&str, &len, content_length);
	MAKEHEADER_ERR(&str, &len, "\r\n");
	MAKEHEADER_ERR(&str, &len, "Content-Type: application/x-www-form-urlencoded\r\n");
	MAKEHEADER_ERR(&str, &len, "Accept-Language: ja\r\nUser-Agent: Monazilla/1.00 (bchan/0.200)\r\nConnection: close\r\n\r\n");

	*header = str;
	*header_len = len;

	return 0;
}

EXPORT submitutil_poststatus_t submitutil_checkresponse(UB *body, W len)
{
	W cmp;
	UB *ptr, *title_last, *found;
	UB title_true[] = {0x82, 0xdc, 0x82, 0xb5, 0x82, 0xbd, '\0'};
	UB title_error[] = {0x82, 0x64, 0x82, 0x71, 0x82, 0x71, '\0'};
	UB title_cookie[] = {0x8a, 0x6d, 0x94, 0x46, '\0'};
	UB title_ocha[] = {0x82, 0xa8, 0x92, 0x83, '\0'};

	ptr = body;
	for (;;) {
		ptr = strstr(ptr, "<!--");
		if (ptr == NULL) {
			break;
		}
		ptr++;
		for (;ptr < body + len;ptr++) {
			if (*ptr != ' ') {
				break;
			}
		}
		cmp = strcmp(ptr, "2ch_X:true");
		if (cmp == 0) {
			return submitutil_poststatus_true;
		}
		cmp = strcmp(ptr, "2ch_X:false");
		if (cmp == 0) {
			return submitutil_poststatus_false;
		}
		cmp = strcmp(ptr, "2ch_X:error");
		if (cmp == 0) {
			return submitutil_poststatus_error;
		}
		cmp = strcmp(ptr, "2ch_X:check");
		if (cmp == 0) {
			return submitutil_poststatus_check;
		}
		cmp = strcmp(ptr, "2ch_X:cookie");
		if (cmp == 0) {
			return submitutil_poststatus_cookie;
		}
	}

	for (ptr = body;ptr < body + len; ptr++) {
		if (*ptr == '<') {
			cmp = strncasecmp(ptr, "<title>", 7);
			if (cmp != 0) {
				continue;
			}
			ptr += 7;
			title_last = strstr(ptr, "</");
			if (title_last == NULL) {
				return submitutil_poststatus_notfound;
			}
			found = strstr(ptr, title_true);
			if ((found != NULL)&&(found < title_last)) {
				return submitutil_poststatus_true;
			}
			found = strstr(ptr, title_error);
			if ((found != NULL)&&(found < title_last)) {
				return submitutil_poststatus_error;
			}
			found = strstr(ptr, title_cookie);
			if ((found != NULL)&&(found < title_last)) {
				return submitutil_poststatus_cookie;
			}
			found = strstr(ptr, title_ocha);
			if ((found != NULL)&&(found < title_last)) {
				return submitutil_poststatus_error;
			}
		}
	}

	return submitutil_poststatus_notfound;
}

#ifdef BCHAN_CONFIG_DEBUG
EXPORT VOID SUBMITUTIL_POSTSTATUS_DP(submitutil_poststatus_t status)
{
	switch (status) {
	case submitutil_poststatus_notfound:
		printf("submitutil_poststatus_notfound\n");
		break;
	case submitutil_poststatus_true:
		printf("submitutil_poststatus_true\n");
		break;
	case submitutil_poststatus_false:
		printf("submitutil_poststatus_false\n");
		break;
	case submitutil_poststatus_error:
		printf("submitutil_poststatus_error\n");
		break;
	case submitutil_poststatus_check:
		printf("submitutil_poststatus_check\n");
		break;
	case submitutil_poststatus_cookie:
		printf("submitutil_poststatus_cookie\n");
		break;
	}
}
#endif

LOCAL W submitutil_appendstring_chref_before_urlencode(UB **dest, W *dlen, UB *src, W slen)
{
	charreferparser_t parser;
	charreferparser_result_t result;
	UB *amp, *rem, ch_b;
	W i, rem_len, ch_w, err;

	rem = src;
	rem_len = slen;

	for (;;) {
		amp = sjstring_searchchar(rem, rem_len, '&');
		if (amp == NULL) {
			return sjstring_appendurlencodestring(dest, dlen, rem, rem_len);
		}
		err = sjstring_appendurlencodestring(dest, dlen, rem, amp - rem);
		if (err < 0) {
			return err;
		}

		rem_len -= amp - rem;

		err = charreferparser_initialize(&parser);
		if (err < 0) {
			return err;
		}
		for (i = 0; i < rem_len; i++) {
			result = charreferparser_parsechar(&parser, amp[i]);
			if (result == CHARREFERPARSER_RESULT_DETERMINE) {
				ch_w = charreferparser_getcharnumber(&parser);
				if ((ch_w >= 0)&&(ch_w < 256)) {
					ch_b = ch_w & 0xff;
					if (ch_b == 0x0a) { /* need more check ? */
						UB ch_b2 = 0x0d;
						err = sjstring_appendurlencodestring(dest, dlen, &ch_b2, 1);
						if (err < 0) {
							return err;
						}
					}
					err = sjstring_appendurlencodestring(dest, dlen, &ch_b, 1);
					if (err < 0) {
						return err;
					}
				} else {
					err = sjstring_appendurlencodestring(dest, dlen, amp, i+1);
					if (err < 0) {
						return err;
					}
				}
				i++;
				break;
			}
			if (result == CHARREFERPARSER_RESULT_INVALID) {
				break;
			}
		}
		charreferparser_finalize(&parser);

		rem_len -= i;
		rem = amp + i;

		if (rem_len <= 0) {
			break;
		}
	}

	return 0;
}

EXPORT W submitutil_makenextrequestbody(UB *prev_body, W prev_body_len, UB **next_body, W *next_len)
{
	UB *ptr;
	UB *newbody = NULL;
	UB *name = NULL, *value = NULL;
	W name_len = 0, value_len = 0;
	W newbody_len = 0;
	W err;
	Bool first = True;

	for (ptr = prev_body; ptr < prev_body + prev_body_len;) {
		ptr = strstr(ptr, "<input ");
		if (ptr == NULL) {
			break;
		}
		name = NULL;
		value = NULL;
		name_len = 0;
		value_len = 0;
		for (;ptr < prev_body + prev_body_len;) {
			for (;ptr < prev_body + prev_body_len;ptr++) {
				if (*ptr != ' ') {
					break;
				}
			}
			if (*ptr == '>') {
				if ((name == NULL)||(name_len == 0)) {
					break;
				}
				if (first != True) {
					err = sjstring_appendasciistring(&newbody, &newbody_len, "&", 1);
					if (err < 0) {
						return err;
					}
				}
				first = False;
				err = sjstring_appendasciistring(&newbody, &newbody_len, name, name_len);
				if (err < 0) {
					return err;
				}
				sjstring_appendasciistring(&newbody, &newbody_len, "=", 1);
				if (err < 0) {
					return err;
				}
				err = submitutil_appendstring_chref_before_urlencode(&newbody, &newbody_len, value, value_len);
				if (err < 0) {
					return err;
				}
				break;
			}
			/* check attribute */
			if (strncasecmp(ptr, "name=\"", 6) == 0) {
				ptr += 6;
				name = ptr;
				for (;ptr < prev_body + prev_body_len;ptr++) {
					if (*ptr == '"') {
						break;
					}
				}
				name_len = ptr - name;
				ptr++;
			} else if (strncasecmp(ptr, "name=", 5) == 0) {
				ptr += 5;
				name = ptr;
				for (;ptr < prev_body + prev_body_len;ptr++) {
					if ((*ptr == ' ')||(*ptr == '>')) {
						break;
					}
				}
				name_len = ptr - name;
			} else if (strncasecmp(ptr, "value=\"", 7) == 0) {
				ptr += 7;
				value = ptr;
				for (;ptr < prev_body + prev_body_len;ptr++) {
					if (*ptr == '"') {
						break;
					}
				}
				value_len = ptr - value;
				ptr++;
			} else if (strncasecmp(ptr, "value=", 6) == 0) {
				ptr += 6;
				value = ptr;
				for (;ptr < prev_body + prev_body_len;ptr++) {
					if ((*ptr == ' ')||(*ptr == '>')) {
						break;
					}
				}
				value_len = ptr - value;
			} else {
				/* skip attribute */
				for (;ptr < prev_body + prev_body_len;ptr++) {
					if (*ptr == ' ') {
						break;
					}
				}
			}
		}
	}

	*next_body = newbody;
	*next_len = newbody_len;

	return 0;
}

LOCAL W submitutil_makecookieheader(UB **str, W *len, UB *prev_header, W prev_header_len)
{
	UB *ptr, *start;
	W cookie_len, err;
	Bool first = True;

	for (ptr = prev_header; ptr < prev_header + prev_header_len;) {
		ptr = strstr(ptr, "Set-Cookie:");
		if (ptr == NULL) {
			break;
		}
		ptr += 11;
		for (;ptr < prev_header + prev_header_len; ptr++) {
			if (*ptr != ' ') {
				break;
			}
		}
		start = ptr;
		for (;ptr < prev_header + prev_header_len; ptr++) {
			if (*ptr == ';') {
				break;
			}
		}
		cookie_len = ptr + 1 - start;
		if (first == True) {
			MAKEHEADER_ERR(str, len, "Cookie: ");
			first = False;
		} else {
			MAKEHEADER_ERR(str, len, " ");
		}
		MAKEHEADER_ERR_LEN(str, len, start, cookie_len);
	}

	if (first == False) {
		MAKEHEADER_ERR(str, len, "\r\n");
	}

	return 0;
}

EXPORT W submitutil_makenextheader(UB *host, W host_len, UB *board, W board_len, UB *thread, W thread_len, W content_length, UB *prev_header, W prev_header_len, UB **header, W *header_len)
{
	UB *str = NULL;
	W len = 0;
	W err;

	MAKEHEADER_ERR(&str, &len, "POST /test/bbs.cgi HTTP/1.1\r\n");
	MAKEHEADER_ERR(&str, &len, "Host: ");
	MAKEHEADER_ERR_LEN(&str, &len, host, host_len);
	MAKEHEADER_ERR(&str, &len, "\r\n");
	MAKEHEADER_ERR(&str, &len, "Accept: */*\r\n");
	MAKEHEADER_ERR(&str, &len, "Referer: http://");
	MAKEHEADER_ERR_LEN(&str, &len, host, host_len);
	MAKEHEADER_ERR(&str, &len, "/test/read.cgi/");
	MAKEHEADER_ERR_LEN(&str, &len, board, board_len);
	MAKEHEADER_ERR(&str, &len, "/");
	MAKEHEADER_ERR_LEN(&str, &len, thread, thread_len);
	MAKEHEADER_ERR(&str, &len, "/\r\n");
	MAKEHEADER_ERR(&str, &len, "Content-Length: ");
	MAKEHEADER_NUM_ERR(&str, &len, content_length);
	MAKEHEADER_ERR(&str, &len, "\r\n");
	err = submitutil_makecookieheader(&str, &len, prev_header, prev_header_len);
	if (err < 0) {
		return err;
	}
	MAKEHEADER_ERR(&str, &len, "Content-Type: application/x-www-form-urlencoded\r\n");
	MAKEHEADER_ERR(&str, &len, "Accept-Language: ja\r\nUser-Agent: Monazilla/1.00 (bchan/0.200)\r\nConnection: close\r\n\r\n");

	*header = str;
	*header_len = len;

	return 0;
}

LOCAL UB* submitutil_makeerrormessage_search_b_elm(UB *body, W body_len)
{
	UB *ptr;
	W i;

	for (i = 0; i < body_len - 2; i++) {
		if (body[i] != '<') { /* tmp */
			continue;
		}
		if ((body[i+1] == 'b')||(body[i+1] == 'B')) {
			if (body[i+2] == '>') {
				return body + i + 3;
			}
			if (body[i+2] == ' ') {
				ptr = sjstring_searchchar(body + i, body_len - i, '>');
				if (ptr == NULL) {
					return NULL;
				}
				return ptr + 1;
			}
		}
	}

	return NULL;
}

EXPORT W submitutil_makeerrormessage(UB *body, W body_len, TC **msg, W *msg_len)
{
	UB *ptr, *start, *end;
	TC *ret;
	W rem_len, ret_len;

	*msg = NULL;
	*msg_len = 0;

	ptr = submitutil_makeerrormessage_search_b_elm(body, body_len);
	if (ptr == NULL) {
		return 0; /* TODO */
	}
	start = ptr;
	rem_len = start - body;

	end = sjstring_searchchar(start, rem_len, '<');
	if (end == NULL) {
		return 0; /* TODO */
	}

	ret_len = sjstring_totcs(start, end - start, NULL);
	if (ret_len < 0) {
		return 0;
	}

	ret = malloc(sizeof(TC)*ret_len);
	if (ret == NULL) {
		return -1; /* TODO */
	}
	sjstring_totcs(start, end - start, ret);

	*msg = ret;
	*msg_len = ret_len;

	return 0;
}
