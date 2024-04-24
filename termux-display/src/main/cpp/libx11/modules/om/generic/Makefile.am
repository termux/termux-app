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
	$(MALLOC_ZERO_CFLAGS) \
	$(CWARNFLAGS)

if XLIB_LOADABLE_I18N

commonlibdir = $(X11_LOCALELIBDIR)/common
commonlib_LTLIBRARIES=xomGeneric.la
xomGeneric_la_LDFLAGS = -module -version-number 2:0:0
xomGeneric_la_LIBADD = $(I18N_MODULE_LIBS)

else

noinst_LTLIBRARIES = libxomGeneric.la

endif


xomGeneric_la_SOURCES = \
	omDefault.c \
	omGeneric.c \
	omImText.c \
	omText.c \
	omTextEsc.c \
	omTextExt.c \
	omTextPer.c \
	omXChar.c

libxomGeneric_la_SOURCES = $(xomGeneric_la_SOURCES)
