/*
 * sjisstring.h
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

#ifndef __SJISSTRING_H__
#define __SJISSTRING_H__

IMPORT W sjstring_appendasciistring(UB **dest, W *dest_len, UB *str, W len);
IMPORT W sjstring_appendUWstring(UB **dest, W *dlen, UW n);
IMPORT W sjstring_appendurlencodestring(UB **dest, W *dest_len, UB *str, W len);

IMPORT W sjstring_appendformpoststring(UB **dest, W *dest_len, UB *str, W len);

IMPORT UB* sjstring_searchchar(UB *str, W len, UB ch);

#ifdef BCHAN_CONFIG_DEBUG
IMPORT VOID SJSTRING_DP(UB *str, W len);
#else
#define SJSTRING_DP(str, len) /**/
#endif

#endif
