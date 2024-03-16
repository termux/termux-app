SUBDIRS = man

module_LTLIBRARIES = libfbdevhw.la

libfbdevhw_la_LDFLAGS = -module -avoid-version $(LD_NO_UNDEFINED_FLAG)

if FBDEVHW
libfbdevhw_la_SOURCES = fbdevhw.c
else
libfbdevhw_la_SOURCES = fbdevhwstub.c
endif

AM_CPPFLAGS = $(XORG_INCS) -I$(srcdir)/../i2c -I$(srcdir)/../modes -I$(srcdir)/../ddc -I$(srcdir)/../parser

AM_CFLAGS = $(DIX_CFLAGS) $(XORG_CFLAGS)

sdk_HEADERS = fbdevhw.h

EXTRA_DIST = fbpriv.h README
