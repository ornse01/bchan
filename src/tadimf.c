/*
 * tadimf.c
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
#include	<tcode.h>

#include    "tadimf.h"
#include    "tadlib.h"

/* like internet message format. rfc5322. */

EXPORT VOID timfparser_clear(timfparser_t *timf)
{
	//taditerator_initialize(&timf->iterator);
	tctokenchecker_clear(&timf->headerchecker);
	timf->state = TIMFPARSER_STATE_SEARCH_HEADER;
	timf->ishankaku = False;
}

LOCAL Bool is_chratio_fusen(LTADSEG *seg, RATIO *w_ratio)
{
	UB *data;
	UB subid;

	if ((seg->id & 0xFF) != TS_TFONT) {
		return False;
	}
	if (seg->len != 6) {
		return False;
	}
	data = ((UB*)seg) + 4;
	subid = *(UH*)data >> 8;
	if (subid != 3) {
		return False;
	}
	*w_ratio = *(RATIO*)(data + 4);
	return True;
}

EXPORT W timfparser_next(timfparser_t *timf, W *headername, UB **bin, W *len)
{
	W segsize, ret, val, ratio_a, ratio_b;
	TC ch;
	UB *data;
	LTADSEG *seg;
	TADITERATOR_RESULT_T result;
	Bool is_chratio;
	RATIO w_ratio;

	for (;;) {
		result = taditerator_next(&timf->iterator, NULL, &ch, &seg, &segsize, &data);
		if (result == TADITERATOR_RESULT_END) {
			return TIMFPARSER_RESULT_END;
		}
		if (result == TADITERATOR_RESULT_CHARCTOR) {
			switch (timf->state) {
			case TIMFPARSER_STATE_SEARCH_HEADER:
				if (ch == TK_NL) {
					timf->state = TIMFPARSER_STATE_READ_VALUE_MESSAGE;
					if (timf->ishankaku == True) {
						*bin = (UB*)timf->chratio;
						*len = 10;
						return TIMFPARSER_RESULT_BODY;
					}
					break;
				}
				tctokenchecker_clear(&timf->headerchecker);
				timf->state = TIMFPARSER_STATE_READ_HEADER_FIELDNAME;
				ret = tctokenchecker_inputchar(&timf->headerchecker, ch, &val);
				if (ret == TCTOKENCHECKER_DETERMINE) {
					timf->state = TIMFPARSER_STATE_READ_HEADER_FIELDVALUE;
					timf->headertype = val;
				} else if (ret != TCTOKENCHECKER_CONTINUE) {
					timf->state = TIMFPARSER_STATE_SKIP_HEADER;
				} else {
					timf->state = TIMFPARSER_STATE_READ_HEADER_FIELDNAME;
				}
				break;
			case TIMFPARSER_STATE_SKIP_HEADER:
				if (ch == TK_NL) {
					timf->state = TIMFPARSER_STATE_SEARCH_HEADER;
				}
				break;
			case TIMFPARSER_STATE_READ_HEADER_FIELDNAME:
				if (ch == TK_NL) {
					timf->state = TIMFPARSER_STATE_SEARCH_HEADER;
					break;
				}
				ret = tctokenchecker_inputchar(&timf->headerchecker, ch, &val);
				if (ret == TCTOKENCHECKER_DETERMINE) {
					timf->state = TIMFPARSER_STATE_SEARCH_HEADER_FIELDVALUE;
					timf->headertype = val;
				} else if (ret != TCTOKENCHECKER_CONTINUE) {
					timf->state = TIMFPARSER_STATE_SKIP_HEADER;
				}
				break;
			case TIMFPARSER_STATE_SEARCH_HEADER_FIELDVALUE:
				if (ch == TK_KSP) {
					break;
				}
				if (ch == TK_NL) {
					timf->state = TIMFPARSER_STATE_SEARCH_HEADER;
					break;
				}
				timf->state = TIMFPARSER_STATE_READ_HEADER_FIELDVALUE;
				*headername = timf->headertype;
				if (timf->ishankaku == True) {
					memcpy(timf->buf, timf->chratio, 10);
					timf->buf[5] = ch;
					*bin = (UB*)timf->buf;
					*len = 12;
				} else {
					timf->buf[0] = ch;
					*bin = (UB*)timf->buf;
					*len = 2;
				}
				return TIMFPARSER_RESULT_HEADERVALUE;
			case TIMFPARSER_STATE_READ_HEADER_FIELDVALUE:
				if (ch == TK_NL) {
					timf->state = TIMFPARSER_STATE_SEARCH_HEADER;
					break;
				}
				timf->buf[0] = ch;
				*headername = timf->headertype;
				*bin = (UB*)timf->buf;
				*len = 2;
				return TIMFPARSER_RESULT_HEADERVALUE;
			case TIMFPARSER_STATE_READ_VALUE_MESSAGE:
				timf->buf[0] = ch;
				*bin = (UB*)timf->buf;
				*len = 2;
				return TIMFPARSER_RESULT_BODY;
			}
		} else if (result == TADITERATOR_RESULT_SEGMENT) {
			is_chratio = is_chratio_fusen(seg, &w_ratio);
			if (is_chratio == False) {
				continue;
			}

			switch (timf->state) {
			case TIMFPARSER_STATE_SEARCH_HEADER:
			case TIMFPARSER_STATE_SKIP_HEADER:
			case TIMFPARSER_STATE_READ_HEADER_FIELDNAME:
			case TIMFPARSER_STATE_SEARCH_HEADER_FIELDVALUE:
				memcpy(timf->chratio, seg, 10);
				ratio_a = w_ratio >> 8;
				ratio_b = w_ratio & 0xFF;
				if ((ratio_a * 2 > ratio_b)||(ratio_b == 0)) {
					timf->ishankaku = False;
				} else {
					timf->ishankaku = True;
				}
				break;
			case TIMFPARSER_STATE_READ_HEADER_FIELDVALUE:
				memcpy(timf->chratio, seg, 10);
				ratio_a = w_ratio >> 8;
				ratio_b = w_ratio & 0xFF;
				if ((ratio_a * 2 > ratio_b)||(ratio_b == 0)) {
					timf->ishankaku = False;
				} else {
					timf->ishankaku = True;
				}
				*headername = timf->headertype;
				*bin = (UB*)timf->chratio;
				*len = 10;
				return TIMFPARSER_RESULT_HEADERVALUE;
			case TIMFPARSER_STATE_READ_VALUE_MESSAGE:
				*bin = (UB*)timf->chratio;
				*len = 10;
				return TIMFPARSER_RESULT_BODY;
			}
		}
	}

	return TIMFPARSER_RESULT_END;
}

LOCAL TC timfparser_headerend[] = {TK_COLN, TNULL};

EXPORT VOID timfparser_initialize(timfparser_t *timf, tctokenchecker_valuetuple_t *val, W val_len, TC *bin, W strlen)
{
	taditerator_initialize(&timf->iterator, bin, strlen);
	tctokenchecker_initialize(&timf->headerchecker, val, val_len, timfparser_headerend);
	timf->state = TIMFPARSER_STATE_SEARCH_HEADER;
	timf->ishankaku = False;
}

EXPORT VOID timfparser_finalize(timfparser_t *timf)
{
	tctokenchecker_finalize(&timf->headerchecker);
	taditerator_finalize(&timf->iterator);
}
