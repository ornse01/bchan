----
-- bchan.d
--
-- Copyright (c) 2009 project bchan
--
-- This software is provided 'as-is', without any express or implied
-- warranty. In no event will the authors be held liable for any damages
-- arising from the use of this software.
--
-- Permission is granted to anyone to use this software for any purpose,
-- including commercial applications, and to alter it and redistribute it
-- freely, subject to the following restrictions:
--
-- 1. The origin of this software must not be misrepresented; you must not
--    claim that you wrote the original software. If you use this software
--    in a product, an acknowledgment in the product documentation would be
--    appreciated but is not required.
--
-- 2. Altered source versions must be plainly marked as such, and must not be
--    misrepresented as being the original software.
--
-- 3. This notice may not be removed or altered from any source
--    distribution.
--
----

.BASE = 0L

#include	<stddef.d>

.BASE = 0H

.MENU_TEST	=	20
.TEXT_MLIST0	=	21
.TEXT_MLIST1	=	22
.TEXT_MLIST2	=	23
.MSGTEXT_RETRIEVING	=	24
.MSGTEXT_NOTMODIFIED	=	25

---------
-- data type = PARTS_DATA
---------

---------
-- data type = TEXT_DATA
---------
	{% 6 0}		-- datatype TEXT_DATA
	{# TEXT_MLIST0 0 0}	-- data number
	MC_STR+MC_STRKEY1 "Ｅ終了\0"

	{# TEXT_MLIST1 0 0}	-- data number
	MC_STR "表示" MC_STR "再表示\0"

	{# TEXT_MLIST2 0 0}     -- data number
	MC_STR "操作"
	MC_STR "スレッド取得\0"

	{# MSGTEXT_RETRIEVING 0 0}	-- data number
	"スレッド取得中\0"

	{# MSGTEXT_NOTMODIFIED 0 0}	-- data number
	"更新されていません\0"

---------
-- data type = MENU_DATA
---------
	{% 8 0}			-- datatype MENU_DATA
	{# MENU_TEST 0 0} 	-- data number
	0L 0L 0L TEXT_MLIST0:L 0L	-- mlist0
	0L 0L 0L TEXT_MLIST1:L 0L	-- mlist1
	0L 0L 0L TEXT_MLIST2:L 0L	-- mlist2
	0L 0L 0L 0L 0L	-- [ウィンドウ]
	0L 0L 0L 0L 0L	-- [小物]
