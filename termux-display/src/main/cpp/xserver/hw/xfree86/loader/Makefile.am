noinst_LTLIBRARIES = libloader.la

AM_CPPFLAGS = $(XORG_INCS) -I$(srcdir)/../parser \
	   -I$(srcdir)/../ddc -I$(srcdir)/../i2c -I$(srcdir)/../modes \
	   -I$(srcdir)/../ramdac -I$(srcdir)/../dri -I$(srcdir)/../dri2

#AM_LDFLAGS = -r
AM_CFLAGS = $(DIX_CFLAGS) $(XORG_CFLAGS)

EXTRA_DIST = \
	loader.h \
	loaderProcs.h

libloader_la_SOURCES = \
	loader.c \
	loaderProcs.h \
        loadmod.c

libloader_la_LIBADD = $(DLOPEN_LIBS)
