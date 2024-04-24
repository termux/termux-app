module_LTLIBRARIES = libvgahw.la
libvgahw_la_LDFLAGS = -avoid-version
libvgahw_la_LIBADD = $(PCIACCESS_LIBS)
libvgahw_la_SOURCES = vgaHW.c vgaHWmodule.c
AM_CPPFLAGS = $(XORG_INCS) -I$(srcdir)/../ddc -I$(srcdir)/../i2c
AM_CFLAGS = $(DIX_CFLAGS) $(XORG_CFLAGS)

sdk_HEADERS = vgaHW.h
