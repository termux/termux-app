AM_CPPFLAGS= \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/include/X11 \
	-I$(top_builddir)/include \
	-I$(top_builddir)/include/X11 \
	-I$(top_srcdir)/src/xcms \
	-I$(top_srcdir)/src/xkb \
	-I$(top_srcdir)/src/xlibi18n \
	-I$(top_srcdir)/src \
	-D_BSD_SOURCE -DXIM_t -DTRANS_CLIENT

AM_CFLAGS= \
	$(X11_CFLAGS) \
	$(BIGFONT_CFLAGS) \
	$(MALLOC_ZERO_CFLAGS) \
	$(CWARNFLAGS)

if XLIB_LOADABLE_I18N

commonlibdir = $(X11_LOCALELIBDIR)/common
commonlib_LTLIBRARIES=ximcp.la
ximcp_la_LDFLAGS = -module -version-number 2:0:0
ximcp_la_LIBADD = $(I18N_MODULE_LIBS)

else

noinst_LTLIBRARIES=libximcp.la

endif


ximcp_la_SOURCES = \
	imCallbk.c \
	imDefFlt.c \
	imDefIc.c \
	imDefIm.c \
	imDefLkup.c \
	imDispch.c \
	imEvToWire.c \
	imExten.c \
	imImSw.c \
	imInsClbk.c \
	imInt.c \
	imLcFlt.c \
	imLcGIc.c \
	imLcIc.c \
	imLcIm.c \
	imLcLkup.c \
	imLcPrs.c \
	imLcSIc.c \
	imRmAttr.c \
	imRm.c \
	imThaiFlt.c \
	imThaiIc.c \
	imThaiIm.c \
	imTrans.c \
	imTransR.c \
	imTrX.c

libximcp_la_SOURCES = $(ximcp_la_SOURCES)
