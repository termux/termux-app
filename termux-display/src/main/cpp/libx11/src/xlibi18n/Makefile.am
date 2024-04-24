AM_CPPFLAGS= \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/include/X11 \
	-I$(top_builddir)/include \
	-I$(top_builddir)/include/X11 \
	-I$(top_srcdir)/src/xcms \
	-I$(top_srcdir)/src/xkb \
	-I$(top_srcdir)/src/xlibi18n \
	-I$(top_srcdir)/src \
	-D_BSD_SOURCE

AM_CFLAGS= \
	$(X11_CFLAGS) \
	$(BIGFONT_CFLAGS) \
	$(XMALLOC_ZERO_CFLAGS) \
	$(CWARNFLAGS)

noinst_LTLIBRARIES = libi18n.la


#
# Dynamic loading code for i18n modules
#
if XLIB_LOADABLE_I18N
XI18N_DL_SOURCES = \
                  XlcDL.c \
                  XlcSL.c
else
#
# Static interfaces to input/output methods
#
IM_LIBS = \
	${top_builddir}/modules/im/ximcp/libximcp.la

LC_LIBS = \
	${top_builddir}/modules/lc/def/libxlcDef.la \
	${top_builddir}/modules/lc/gen/libxlibi18n.la \
	${top_builddir}/modules/lc/Utf8/libxlcUTF8Load.la

OM_LIBS = \
	${top_builddir}/modules/om/generic/libxomGeneric.la
endif

libi18n_la_LIBADD = \
	 $(IM_LIBS) $(LC_LIBS) $(OM_LIBS)

libi18n_la_SOURCES = \
	$(XI18N_DL_SOURCES) \
	XDefaultIMIF.c \
	XDefaultOMIF.c \
	xim_trans.c\
	ICWrap.c\
	IMWrap.c\
	imKStoUCS.c\
	lcCT.c\
	lcCharSet.c\
	lcConv.c\
	lcDB.c\
	lcDynamic.c\
	lcFile.c\
	lcGeneric.c\
	lcInit.c\
	lcPrTxt.c\
	lcPubWrap.c\
	lcPublic.c\
	lcRM.c\
	lcStd.c\
	lcTxtPr.c\
	lcUTF8.c\
	lcUtil.c\
	lcWrap.c\
	mbWMProps.c\
	mbWrap.c\
	utf8WMProps.c\
	utf8Wrap.c\
	wcWrap.c\
	Xaixlcint.h\
	XimImSw.h\
	XimProto.h\
	XimThai.h\
	XimTrInt.h\
	XimTrX.h\
	XimTrans.h\
	Ximint.h\
	XimintL.h\
	XimintP.h\
	XlcGeneric.h\
	XlcPubI.h\
	XlcPublic.h\
	Xlcint.h\
	lcUniConv/armscii_8.h\
	lcUniConv/ascii.h\
	lcUniConv/big5.h\
	lcUniConv/big5_emacs.h\
	lcUniConv/big5hkscs.h\
	lcUniConv/cp1133.h\
	lcUniConv/cp1251.h\
	lcUniConv/cp1255.h\
	lcUniConv/cp1256.h\
	lcUniConv/gb2312.h\
	lcUniConv/gbk.h\
	lcUniConv/georgian_academy.h\
	lcUniConv/georgian_ps.h\
	lcUniConv/iso8859_1.h\
	lcUniConv/iso8859_10.h\
	lcUniConv/iso8859_11.h\
	lcUniConv/iso8859_13.h\
	lcUniConv/iso8859_14.h\
	lcUniConv/iso8859_15.h\
	lcUniConv/iso8859_16.h\
	lcUniConv/iso8859_2.h\
	lcUniConv/iso8859_3.h\
	lcUniConv/iso8859_4.h\
	lcUniConv/iso8859_5.h\
	lcUniConv/iso8859_6.h\
	lcUniConv/iso8859_7.h\
	lcUniConv/iso8859_8.h\
	lcUniConv/iso8859_9.h\
	lcUniConv/iso8859_9e.h\
	lcUniConv/jisx0201.h\
	lcUniConv/jisx0208.h\
	lcUniConv/jisx0212.h\
	lcUniConv/koi8_c.h\
	lcUniConv/koi8_r.h\
	lcUniConv/koi8_u.h\
	lcUniConv/ksc5601.h\
	lcUniConv/mulelao.h\
	lcUniConv/tatar_cyr.h\
	lcUniConv/tcvn.h\
	lcUniConv/tis620.h\
	lcUniConv/ucs2be.h\
	lcUniConv/utf8.h\
	lcUniConv/viscii.h

EXTRA_DIST = \
	lcUniConv/README \
	lcUniConv/COPYRIGHT \
	lcUniConv/8bit_tab_to_h.c \
	lcUniConv/cjk_tab_to_h.c

if LINT
# Check source code with tools like lint & sparse

ALL_LINT_FLAGS=$(LINT_FLAGS) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) \
		$(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS)

lint:
	$(LINT) $(ALL_LINT_FLAGS) $(libi18n_la_SOURCES)

endif LINT
