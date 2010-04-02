/*
 * test_sjisstring.c
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

#include    <btron/btron.h>
#include    <bstdio.h>

#include    "test.h"

#include    "sjisstring.h"

/* from rfc1738. 2.2. URL Character Encoding Issues */
LOCAL Bool test_sjistring_isurlusablecharacter_testdata(UB ch)
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

LOCAL TEST_RESULT test_isurlusablecharacter_1()
{
	W ch;
	Bool usable, usable2;
	TEST_RESULT result = TEST_RESULT_PASS;

	for (ch = 0; ch < 0x100; ch++) {
		usable = sjstring_isurlusablecharacter(ch & 0xFF);
		usable2 = test_sjistring_isurlusablecharacter_testdata(ch);
		if (usable != usable2) {
			result = TEST_RESULT_FAIL;
			printf("nomatch character %c[%02x]\n", ch, ch);
		}
	}

	return result;
}

LOCAL VOID test_sjisstring_printresult(TEST_RESULT (*proc)(), B *test_name)
{
	TEST_RESULT result;

	printf("test_sjisstring: %s\n", test_name);
	printf("---------------------------------------------\n");
	result = proc();
	if (result == TEST_RESULT_PASS) {
		printf("--pass---------------------------------------\n");
	} else {
		printf("--fail---------------------------------------\n");
	}
	printf("---------------------------------------------\n");
}

EXPORT VOID test_sjistring_main()
{
	test_sjisstring_printresult(test_isurlusablecharacter_1, "test_isurlusablecharacter_1");
}
