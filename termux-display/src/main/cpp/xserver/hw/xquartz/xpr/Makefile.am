noinst_LTLIBRARIES = libXquartzXpr.la

AM_CFLAGS =  $(DIX_CFLAGS)
AM_CPPFLAGS = \
	-I$(srcdir) -I$(srcdir)/.. \
	-I$(top_builddir)/pseudoramiX \
	-I$(top_srcdir)/miext \
	-I$(top_srcdir)/miext/rootless

libXquartzXpr_la_SOURCES = \
	appledri.c \
	dri.c \
	driWrap.c \
	xprAppleWM.c \
	xprCursor.c \
	xprEvent.c \
	xprFrame.c \
	xprScreen.c \
	x-hash.c \
	x-hook.c \
	x-list.c

EXTRA_DIST = \
	dri.h \
	driWrap.h \
	dristruct.h \
	appledri.h \
	appledristr.h \
	x-hash.h \
	x-hook.h \
	x-list.h \
	xpr.h \
	xprEvent.h
