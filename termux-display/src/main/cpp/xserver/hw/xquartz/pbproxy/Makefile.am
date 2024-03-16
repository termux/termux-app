AM_CPPFLAGS=-I.. -DXQUARTZ_DATA_DIR=\"$(datadir)/X11/XQuartz\"

AM_CFLAGS=$(XPBPROXY_CFLAGS)
AM_OBJCFLAGS=$(XPBPROXY_CFLAGS)

noinst_LTLIBRARIES = libxpbproxy.la
libxpbproxy_la_SOURCES = \
	trick_autotools.c \
	main.m \
	x-input.m \
	x-selection.m

libxpbproxy_la_LDFLAGS=$(XPBPROXY_LIBS)

if STANDALONE_XPBPROXY

bin_PROGRAMS = xpbproxy
xpbproxy_SOURCES = app-main.m ../NSUserDefaults+XQuartzDefaults.m
xpbproxy_LDADD = libxpbproxy.la
xpbproxy_LDFLAGS = -Wl,-framework,Cocoa

AM_CPPFLAGS += -DSTANDALONE_XPBPROXY

endif

EXTRA_DIST = \
	pbproxy.h \
	x-selection.h
