lib_LTLIBRARIES = libxkbfile.la

libxkbfile_la_SOURCES = \
         cout.c \
         maprules.c \
         srvmisc.c \
         xkbatom.c \
         xkbbells.c \
         xkbconfig.c \
         xkbdraw.c \
         xkberrs.c \
         XKBfileInt.h \
         xkbmisc.c \
         xkbout.c \
         xkbtext.c \
         xkmout.c \
         xkmread.c

AM_CPPFLAGS = -I$(top_srcdir)/include/ -I$(top_srcdir)/include/X11/extensions/
AM_CFLAGS = $(CWARNFLAGS) @XKBFILE_CFLAGS@

libxkbfile_la_LIBADD = @XKBFILE_LIBS@
libxkbfile_la_LDFLAGS = -version-number 1:0:2 -no-undefined

libxkbfileincludedir = $(includedir)/X11/extensions
libxkbfileinclude_HEADERS = \
    $(top_srcdir)/include/X11/extensions/XKM.h\
    $(top_srcdir)/include/X11/extensions/XKBrules.h\
    $(top_srcdir)/include/X11/extensions/XKBbells.h\
    $(top_srcdir)/include/X11/extensions/XKBconfig.h\
    $(top_srcdir)/include/X11/extensions/XKMformat.h\
    $(top_srcdir)/include/X11/extensions/XKBfile.h

EXTRA_DIST = magic meson.build
