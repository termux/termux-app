noinst_LTLIBRARIES = libxf86config.la

INTERNAL_SOURCES= \
	Device.c \
	Files.c \
	Flags.c \
	Input.c \
	InputClass.c \
	OutputClass.c \
	Layout.c \
	Module.c \
	Video.c \
	Monitor.c \
	Pointer.c \
	Screen.c \
	Vendor.c \
	read.c \
	scan.c \
	write.c \
	DRI.c \
	Extensions.c

libxf86config_la_SOURCES = \
	$(INTERNAL_SOURCES)

AM_CFLAGS = $(DIX_CFLAGS) $(XORG_CFLAGS) \
	-DDATADIR=\"$(datadir)\"

EXTRA_DIST = \
	Configint.h \
	configProcs.h \
	xf86Optrec.h \
	xf86Parser.h \
	xf86tokens.h

sdk_HEADERS = \
	xf86Parser.h \
	xf86Optrec.h

AM_CPPFLAGS = -I$(srcdir)/../common
