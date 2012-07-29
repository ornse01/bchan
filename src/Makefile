#
#	@(#)Makefile (bchan: BTRON 2channel thread viewer)
#		GNU make 用
#
#	make の方法
#	・make
#		リリース用の正式版を作成
#	・make mode=debug
#		デバッグ用に作成
#	・make clean
#		make で作成した全てのファイルを削除
#	・make install
#		規定の場所にインストール (実機用のみ)
#

# バージョン
version = 0x0301

# リリース用をデフォルトにする
mode =

# ソースの依存関係ファイル (自動生成)
DEPS = Dependencies
DEPENDENCIES_OUTPUT := $(DEPS)

# マルチタスク対応ライブラリを使用する
USE_MT = 1

# アプリケーション標準ルール
include ../../etc/makerules

#----------------------------------------------------------------------
# 作成対象
TARGET = bchan

# ソースファイルのサーチパス
S = ../src
VPATH = $(S)

# ヘッダファイルのディレクトリ追加
HEADER := $(S) $(HEADER) ../src/bchanf/src

# ソースファイル
SRC =	main.c cache.c parser.c layout.c retriever.c tadlib.c poptray.c postres.c http.c submit.c sjisstring.c parselib.c submitutil.c bchan_vobj.c tadurl.c bchan_panels.c bchan_menus.c residhash.c resindexhash.c tadimf.c layoutarray.c array.c render.c layoutstyle.c traydata.c hmi.c wordlist.c tadsearch.c httpdateparser.c setcookieheader.c psvlexer.c cookiedb.c

# データボックスソースファイル
DBSRC =

SRC += $(DBSRC)

WC_SRC = $(filter %.C, $(SRC))

# オブジェクトファイル
OBJ = $(addsuffix .o, $(basename $(SRC)))

# コンパイルオプション
ifneq ($(mode), debug)
  CFLAGS += -Wall
endif

CFLAGS += $(BUILDOPTIONS)

LOADLIBES += -limg

#----------------------------------------------------------------------
.PHONY: all clean install

INST = $(addprefix $(TARGET), .bz .map)
NOINST = $(addprefix $(TARGET), .out .fsn .dbx)

all: library $(INST)

library:
	@$(MAKE) -f Makefile.lib -C ./bchanf

$(TARGET).out: $(OBJ) bchanf/libbchanf.a
	$(LINK.o) $(LDOBJS) $^ $(LOADLIBES) $(LDLIBS) $(OUTPUT_OPTION)

clean:
	@$(MAKE) clean -f Makefile.lib -C ./bchanf
	$(RM) $(OBJ)
	$(RM) $(WC_SRC:%.C=%.c) $(DBSRC)
	$(RM) $(INST) $(NOINST) *.lst $(DEPS)
	$(RM) $(TARGET).btf _VerUpCmd.btf _t

install: $(INST:%=$(EXE_INSTALLDIR)/%)

$(EXE_INSTALLDIR)/$(TARGET).out: $(EXE_INSTALLDIR)/%: %
	$(EXE_INSTALL_STRIP)

# 圧縮アーカイブ

$(TARGET).bz: $(TARGET).btf _VerUpCmd.btf
	$(MKBTF) -o$@ -c -tbchan \
		@0.0.0.0.0._VerUpCmd.btf \
		@0.0.0.0.0.$(TARGET).btf

$(TARGET).btf: $(NOINST)
	$(CP) $(TARGET).out _t
	$(STRIP) _t
	$(MKBTF) -o$@ -a0x8002 -tbchan \
		9.0x163._t \
		10.0.$(TARGET).dbx \
		7.$(version).$(TARGET).fsn
	$(RM) _t

_VerUpCmd.btf: _VerUpCmd
	$(CP) $(S)/_VerUpCmd _t
	$(MKBTF) -o_VerUpCmd.btf -t_VerUpCmd 31.0._t
	$(RM) _t

# ソースの依存関係

$(WC_SRC:%.C=%.c):

$(DBSRC):

ifdef DEPENDENCIES_OUTPUT
$(DEPS):	; touch $(DEPS)
else
$(DEPS): $(SRC)	; $(MAKEDEPS) $@ $?
endif

include $(DEPS)
