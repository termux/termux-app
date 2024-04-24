noinst_LTLIBRARIES = libCGLCore.la
AM_CFLAGS = $(DIX_CFLAGS) $(GL_CFLAGS)
AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/glx \
	-I$(top_srcdir)/hw/xquartz \
	-I$(top_srcdir)/hw/xquartz/xpr \
	-I$(top_srcdir)/miext/damage

libCGLCore_la_SOURCES = \
	indirect.c \
	capabilities.c \
	visualConfigs.c

EXTRA_DIST = \
	capabilities.h \
	visualConfigs.h
