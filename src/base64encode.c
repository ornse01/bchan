/*
 * base64encode.c
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

#include    "base64encode.h"

#include	<basic.h>
#include	<bstdio.h>

#ifdef BCHAN_CONFIG_DEBUG
# define DP(arg) printf arg
# define DP_ER(msg, err) printf("%s (%d/%x)\n", msg, err>>16, err)
#else
# define DP(arg) /**/
# define DP_ER(msg, err) /**/
#endif

LOCAL UB encode_table[] = {"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"};

EXPORT VOID base64encode_inputchar(base64encode_t *encode, UB ch, UB **result, W *result_len)
{
	switch (encode->state) {
	case BASE64ENCODE_STATE_FIRSTBYTE:
		encode->buffer[0] = encode_table[ch>>2];
		encode->save = (ch & 0x3) << 4;
		*result = encode->buffer;
		*result_len = 1;
		encode->state = BASE64ENCODE_STATE_SECONDBYTE;
		break;
	case BASE64ENCODE_STATE_SECONDBYTE:
		encode->buffer[0] = encode_table[encode->save | (ch>>4)];
		encode->save = (ch & 0xF) << 2;
		*result = encode->buffer;
		*result_len = 1;
		encode->state = BASE64ENCODE_STATE_THIRDBYTE;
		break;
	case BASE64ENCODE_STATE_THIRDBYTE:
		encode->buffer[0] = encode_table[encode->save | (ch>>6)];
		encode->buffer[1] = encode_table[ch & 0x3F];
		*result = encode->buffer;
		*result_len = 2;
		encode->state = BASE64ENCODE_STATE_FIRSTBYTE;
		break;
	}
}

EXPORT VOID base64encode_endinput(base64encode_t *encode, UB **result, W *result_len)
{
	switch (encode->state) {
	case BASE64ENCODE_STATE_FIRSTBYTE:
		*result = NULL;
		*result_len = 0;
		break;
	case BASE64ENCODE_STATE_SECONDBYTE:
		encode->buffer[0] = encode_table[encode->save];
		encode->buffer[1] = '=';
		encode->buffer[2] = '=';
		*result = encode->buffer;
		*result_len = 3;
		break;
	case BASE64ENCODE_STATE_THIRDBYTE:
		encode->buffer[0] = encode_table[encode->save];
		encode->buffer[1] = '=';
		*result = encode->buffer;
		*result_len = 2;
		break;
	}
}

EXPORT VOID base64encode_clear(base64encode_t *encode)
{
	encode->state = BASE64ENCODE_STATE_FIRSTBYTE;
}

EXPORT W base64encode_initialize(base64encode_t *encode)
{
	encode->state = BASE64ENCODE_STATE_FIRSTBYTE;
	return 0;
}

EXPORT VOID base64encode_finalize(base64encode_t *encode)
{
}
