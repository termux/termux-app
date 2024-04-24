noinst_LTLIBRARIES = libdri.la
AM_CFLAGS = -I$(top_srcdir)/hw/xfree86/common \
            -I$(top_srcdir)/hw/xfree86/os-support \
            -I$(top_srcdir)/hw/xfree86/modes \
            -I$(top_srcdir)/hw/xfree86/ddc \
            -I$(top_srcdir)/hw/xfree86/i2c \
            -I$(top_srcdir)/hw/xfree86/parser \
            -I$(top_srcdir)/hw/xfree86/ramdac \
            -I$(top_srcdir)/hw/xfree86/os-support/bus \
            -I$(top_srcdir)/glx \
            -DHAVE_XORG_CONFIG_H \
            @DIX_CFLAGS@ @XORG_CFLAGS@
libdri_la_SOURCES = \
	dri.c \
	dri.h \
	dristruct.h \
	sarea.h \
	xf86dri.c

sdk_HEADERS = dri.h sarea.h dristruct.h
