----
-- bchan.d
--
-- Copyright (c) 2009-2011 project bchan
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
.MSGTEXT_POSTSUCCEED	=	26
.MSGTEXT_POSTDENIED	=	27
.MSGTEXT_POSTERROR	=	28
.MS_CONFIRM_POST	=	29
.MS_CONFIRM_CANCEL	=	30
.TEXT_CONFIRM_TITLE	=	31
.MSGTEXT_NONAUTHORITATIVE	=	32
.MSGTEXT_NETWORKERROR	=	33
.FFUSEN_BBB		=	34
.FFUSEN_TEXEDIT		=	35
.FFUSEN_VIEWER		=	36
.MSGTEXT_NOTFOUND	=	37
.TEXT_MLIST3	=	38
.MS_PANEL_OK	=	39
.TEXT_SERVERNAME	=	40
.TEXT_BOARDNAME	=	41
.TEXT_THREADNUMBER	=	42
.TEXT_THREADINFO	=	43
.MSGTEXT_CANTRETRIEVE	=	44
.GMENU_RESNUMBER	=	45
.MLIST_RESNUMBER	=	46
.GMENU_RESID	=	47
.MLIST_RESID	=	48
.SS_NGWORD_LIST	=	49
.MS_NGWORD_DELETE	=	50
.TB_NGWORD_APPEND	=	51
.MS_NGWORD_APPEND	=	52

---  for NG word window layout values.

.NGWORD_MARGIN = 8
.NGWORD_TEXT_HEIGHT = 24
.NGWORD_TEXT_WIDTH = 360
.NGWORD_LIST_WIDTH = NGWORD_TEXT_WIDTH
.NGWORD_LIST_HEIGHT = 144
.NGWORD_BUTTON_WIDTH = 80
.NGWORD_BUTTON_HEIGHT = NGWORD_TEXT_HEIGHT
.NGWORD_APDTB_L = NGWORD_MARGIN
.NGWORD_APDTB_T = NGWORD_MARGIN
.NGWORD_APDTB_R = NGWORD_APDTB_L+NGWORD_TEXT_WIDTH
.NGWORD_APDTB_B = NGWORD_APDTB_T+NGWORD_TEXT_HEIGHT
.NGWORD_APDMS_L = NGWORD_APDTB_R+NGWORD_MARGIN
.NGWORD_APDMS_T = NGWORD_MARGIN
.NGWORD_APDMS_R = NGWORD_APDMS_L+NGWORD_BUTTON_WIDTH
.NGWORD_APDMS_B = NGWORD_APDMS_T+NGWORD_BUTTON_HEIGHT
.NGWORD_LIST_L = NGWORD_MARGIN
.NGWORD_LIST_T = NGWORD_APDTB_B+NGWORD_MARGIN+NGWORD_MARGIN
.NGWORD_LIST_R = NGWORD_LIST_L+NGWORD_LIST_WIDTH
.NGWORD_LIST_B = NGWORD_LIST_T+NGWORD_LIST_HEIGHT
.NGWORD_DELMS_L = NGWORD_LIST_R+NGWORD_MARGIN
.NGWORD_DELMS_T = NGWORD_LIST_T
.NGWORD_DELMS_R = NGWORD_DELMS_L+NGWORD_BUTTON_WIDTH
.NGWORD_DELMS_B = NGWORD_DELMS_T+NGWORD_BUTTON_HEIGHT

---------
-- data type = PARTS_DATA
---------
	{% 7 0}		-- datatype PARTS_DATA

	{# MS_CONFIRM_POST 0 0} 	-- data number
	MS_PARTS+P_DISP:L	-- type
	{0H 0H 350H 25H}	-- r
	0L			-- cv (unused)
	OFFSET:L+20		-- name ()
	{0L 0L -1L 0L}		-- PARTDISP
	MC_STR "上記全てを承諾して書き込む\0"

	{# MS_CONFIRM_CANCEL 0 0} 	-- data number
	MS_PARTS+P_DISP:L	-- type
	{0H 0H 100H 25H}	-- r
	0L			-- cv (unused)
	OFFSET:L+20		-- name ()
	{0L 0L -1L 0L}		-- PARTDISP
	MC_STR "取り消し\0"

	{# MS_PANEL_OK 0 0} 	-- data number
	MS_PARTS+P_DISP:L	-- type
	{0H 0H 80H 24H}	-- r
	0L			-- cv (unused)
	OFFSET:L+20		-- name ()
	{0L 0L -1L 0L}		-- PARTDISP
	MC_STR "確認\0"

	{# SS_NGWORD_LIST 0 0} 	-- data number
	SS_PARTS+P_DISP+P_NOSEL:L	-- type
	{NGWORD_LIST_L:H NGWORD_LIST_T:H NGWORD_LIST_R:H NGWORD_LIST_B:H}	 -- r
	0L			-- cv
	OFFSET:L+20		-- name
	{0L 0L -1L 0L}		-- PARTDISP
	MC_STR "\0"

	{# MS_NGWORD_DELETE 0 0} 	-- data number
	MS_PARTS+P_DISP:L	-- type
	{NGWORD_DELMS_L:H NGWORD_DELMS_T:H NGWORD_DELMS_R:H NGWORD_DELMS_B:H}	-- r
	0L			-- cv (unused)
	OFFSET:L+20		-- name ()
	{0L 0L -1L 0L}		-- PARTDISP
	MC_STR "削除\0"

	{# TB_NGWORD_APPEND 0 0} 	-- data number
	TB_PARTS+P_DISP:L	-- type
	{NGWORD_APDTB_L:H NGWORD_APDTB_T:H NGWORD_APDTB_R:H NGWORD_APDTB_B:H}	-- r
	128L			-- txsize
	0L			-- text
	{0L 0L -1L 0L}		-- PARTDISP

	{# MS_NGWORD_APPEND 0 0} 	-- data number
	MS_PARTS+P_DISP:L	-- type
	{NGWORD_APDMS_L:H NGWORD_APDMS_T:H NGWORD_APDMS_R:H NGWORD_APDMS_B:H}	-- r
	0L			-- cv (unused)
	OFFSET:L+20		-- name ()
	{0L 0L -1L 0L}		-- PARTDISP
	MC_STR "追加\0"

---------
-- data type = TEXT_DATA
---------
	{% 6 0}		-- datatype TEXT_DATA
	{# TEXT_MLIST0 0 0}	-- data number
	MC_STRKEY1 "Ｅ終了\0"

	{# TEXT_MLIST1 0 0}	-- data number
	MC_STR "表示"
	MC_STR "再表示"
	MC_STR "スレッド情報を表示\0"

	{# TEXT_MLIST2 0 0}     -- data number
	MC_STR "操作"
	MC_STR "スレッド取得\0"

	{# TEXT_MLIST3 0 0}     -- data number
	MC_STR "編集"
	MC_STR "スレタイをトレーに複写"
	MC_STR "スレッドＵＲＬをトレーに複写"
	MC_IND "ＮＧワード設定\0"

	{# MSGTEXT_RETRIEVING 0 0}	-- data number
	"スレッド取得中\0"

	{# MSGTEXT_NOTMODIFIED 0 0}	-- data number
	"更新されていません\0"

	{# MSGTEXT_POSTSUCCEED 0 0}	-- data number
	"書きこみました\0"

	{# MSGTEXT_POSTDENIED 0 0}	-- data number
	"書きこめませんでした\0"

	{# MSGTEXT_POSTERROR 0 0}	-- data number
	"エラーが発生しました\0"

	{# TEXT_CONFIRM_TITLE 0 0}	-- data number
	"書き込み確認\0"

	{# MSGTEXT_NONAUTHORITATIVE 0 0}	-- data number
	"ｄａｔ落ちしています\0"

	{# MSGTEXT_NETWORKERROR 0 0}	-- data number
	"ネットワークエラーが発生しました\0"

	{# MSGTEXT_NOTFOUND 0 0}	-- data number
	"スレッドが見つかりませんでした\0"

	{# TEXT_SERVERNAME 0 0}	-- data number
	"サーバー名：\0"

	{# TEXT_BOARDNAME 0 0}	-- data number
	"板名：\0"

	{# TEXT_THREADNUMBER 0 0}	-- data number
	"スレッド番号：\0"

	{# TEXT_THREADINFO 0 0}	-- data number
	"スレッド情報\0"

	{# MSGTEXT_CANTRETRIEVE 0 0}	-- data number
	"スレッド情報が存在しないためこの操作はできません\0"

	{# MLIST_RESNUMBER 0 0}     -- data number
	MC_STR+MC_IND "このレスのＮＧ指定"
	MC_STR "このレスをトレーに複写\0"

	{# MLIST_RESID 0 0}     -- data number
	MC_STR+MC_IND "このＩＤのＮＧ指定"
	MC_STR "このＩＤのレスをトレーに複写\0"

---------
-- data type = MENU_DATA
---------
	{% 8 0}			-- datatype MENU_DATA
	{# MENU_TEST 0 0} 	-- data number
	0L 0L 0L TEXT_MLIST0:L 0L	-- mlist0
	0L 0L 0L TEXT_MLIST1:L 0L	-- mlist1
	0L 0L 0L TEXT_MLIST3:L 0L	-- mlist3
	0L 0L 0L TEXT_MLIST2:L 0L	-- mlist2
	0L 0L 0L 0L 0L	-- [ウィンドウ]
	0L 0L 0L 0L 0L	-- [小物]

---------
-- data type = GMENU_DATA
---------
	{% 9 0}			-- datatype GMENU_DATA
	{# GMENU_RESNUMBER 0 0} 	-- data number
	1L	-- frame
	0L	-- bgpat
	0L	-- indpat
	-1L	-- chcol
	0H 0H 250H 40H	-- area
	0L	-- inact
	0L	-- select
	0L	-- desc
	MLIST_RESNUMBER:L	-- dnum
	0L	-- ptr
	2L	-- nitem
	26H 3H 240H 19H	-- r[0]
	10H 21H 240H 37H	-- r[1]

	{# GMENU_RESID 0 0} 	-- data number
	1L	-- frame
	0L	-- bgpat
	0L	-- indpat
	-1L	-- chcol
	0H 0H 250H 40H	-- area
	0L	-- inact
	0L	-- select
	0L	-- desc
	MLIST_RESID:L	-- dnum
	0L	-- ptr
	2L	-- nitem
	26H 3H 240H 19H	-- r[0]
	10H 21H 240H 37H	-- r[1]

---------
-- data type = USER_DATA
---------
	{% 64 0}		-- datatype USER_DATA
	{# FFUSEN_BBB 0 0} 	-- data number
	0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
	0x10Y 0x00Y 0x00Y 0x00Y 0x00Y 0x10Y 0x00Y 0x00Y
	0x00Y 0x10Y 0xffY 0xffY 0xffY 0x10Y 0x04Y 0x00Y
	0x0fY 0x80Y 0x00Y 0x00Y 0x00Y 0x80Y 0x70Y 0x34Y
	0x5cY 0x4bY 0x56Y 0x25Y 0x69Y 0x25Y 0x26Y 0x25Y
	0x36Y 0x25Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
	0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
	0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x55Y 0x23Y
	0x52Y 0x23Y 0x4cY 0x23Y 0x00Y 0x00Y 0x00Y 0x00Y
	0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
	0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
	0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x1aY 0x01Y
	0x07Y 0x00Y 0x44Y 0x00Y 0x5fY 0x02Y 0xfcY 0x01Y
	0x00Y 0x00Y 0xc6Y 0x00Y 0x03Y 0x00Y 0x00Y 0x00Y
	0x00Y 0x00Y 0x9aY 0x01Y 0x74Y 0x01Y 0x00Y 0x00Y
	0x00Y 0x01Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
	0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
	0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
	0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
	0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
	0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
	0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
	0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
	0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
	0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
	0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
	0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
	0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
	0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
	0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
	0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
	0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
	0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
	0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
	0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
	0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
	0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
	0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
	0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
	0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
	0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
	0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
	0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
	0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
	0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
	0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
	0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
	0x00Y 0x00Y

	{# FFUSEN_TEXEDIT 0 0} 	-- data number
        0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
        0x10Y 0x00Y 0x00Y 0x00Y 0x00Y 0x10Y 0x00Y 0x00Y
        0x00Y 0x10Y 0xffY 0xffY 0xffY 0x10Y 0x04Y 0x00Y
        0x00Y 0x80Y 0x03Y 0x00Y 0x00Y 0x80Y 0x70Y 0x34Y
        0x5cY 0x4bY 0x38Y 0x4aY 0x4fY 0x3eY 0x54Y 0x4aY
        0x38Y 0x3dY 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
        0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
        0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x38Y 0x4aY
        0x4fY 0x3eY 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
        0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
        0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
        0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x20Y 0x00Y
        0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y
        0x80Y 0x02Y 0x7cY 0x01Y 0x00Y 0x00Y 0x00Y 0x00Y
        0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0xffY 0xffY
        0xffY 0x10Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y 0x00Y

	{# FFUSEN_VIEWER 0 0} 	-- data number
	0 0 0 0					-- r
	16					-- chsz
	0x10000000L 0x10000000L 0x10FFFFFFL	-- colors
	4					-- pict
	0x8000	0xC053	0x8000			-- apl-id
	"ｂｃｈａｎ"16				-- name
	"２ｃｈスレ"16				-- type
	0					-- dlen
