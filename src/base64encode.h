/*
 * base64encode.h
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

#ifndef __BASE64ENCODE_H__
#define __BASE64ENCODE_H__

struct base64encode_t_ {
	enum {
		BASE64ENCODE_STATE_FIRSTBYTE,
		BASE64ENCODE_STATE_SECONDBYTE,
		BASE64ENCODE_STATE_THIRDBYTE,
	} state;
	UB buffer[3];
	UB save;
};
typedef struct base64encode_t_ base64encode_t;

IMPORT W base64encode_initialize(base64encode_t *encode);
IMPORT VOID base64encode_finalize(base64encode_t *encode);
IMPORT VOID base64encode_inputchar(base64encode_t *encode, UB ch, UB **result, W *result_len);
IMPORT VOID base64encode_endinput(base64encode_t *encode, UB **result, W *result_len);
IMPORT VOID base64encode_clear(base64encode_t *encode);

#endif
