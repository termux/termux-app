noinst_LTLIBRARIES = libxf86modes.la

if DGA
DGA_SRCS = xf86DiDGA.c
endif

libxf86modes_la_SOURCES = \
	xf86Crtc.c \
	xf86Crtc.h \
	xf86Cursors.c \
	xf86gtf.c \
	xf86EdidModes.c \
	xf86Modes.c \
	xf86Modes.h \
	xf86RandR12.c \
	xf86RandR12.h \
	xf86Rotate.c \
        $(DGA_SRCS)

AM_CPPFLAGS = $(XORG_INCS) -I$(srcdir)/../ddc -I$(srcdir)/../i2c \
	   -I$(srcdir)/../loader -I$(srcdir)/../rac -I$(srcdir)/../parser \
	   -I$(srcdir)/../vbe -I$(srcdir)/../int10 \
	   -I$(srcdir)/../vgahw -I$(srcdir)/../ramdac \
	   -I$(srcdir)/../dixmods/extmod

sdk_HEADERS = \
	xf86Crtc.h \
	xf86Modes.h \
	xf86RandR12.h

AM_CFLAGS = $(DIX_CFLAGS) $(XORG_CFLAGS)
