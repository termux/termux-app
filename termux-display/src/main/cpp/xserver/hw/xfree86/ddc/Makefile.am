sdk_HEADERS = edid.h xf86DDC.h

noinst_LTLIBRARIES = libddc.la

libddc_la_SOURCES = ddc.c interpret_edid.c print_edid.c ddcProperty.c

AM_CPPFLAGS = $(XORG_INCS) -I$(srcdir)/../i2c

AM_CFLAGS = $(DIX_CFLAGS) $(XORG_CFLAGS)

EXTRA_DIST = DDC.HOWTO
