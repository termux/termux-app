noinst_LTLIBRARIES = libfb.la libwfb.la

AM_CFLAGS = $(DIX_CFLAGS)

if XORG
sdk_HEADERS = fb.h fbrop.h fboverlay.h wfbrename.h fbpict.h
endif

libfb_la_CFLAGS = $(AM_CFLAGS)
libfb_la_LIBADD = $(PIXMAN_LIBS)

libwfb_la_CFLAGS = $(AM_CFLAGS) -DFB_ACCESS_WRAPPER
libwfb_la_LIBADD = $(PIXMAN_LIBS)

libfb_la_SOURCES = 	\
	fb.h		\
	fballpriv.c	\
	fbarc.c		\
	fbbits.c	\
	fbbits.h	\
	fbblt.c		\
	fbbltone.c	\
	fbcmap_mi.c     \
	fbcopy.c	\
	fbfill.c	\
	fbfillrect.c	\
	fbfillsp.c	\
	fbgc.c		\
	fbgetsp.c	\
	fbglyph.c	\
	fbimage.c	\
	fbline.c	\
	fboverlay.c	\
	fboverlay.h	\
	fbpict.c	\
	fbpict.h	\
	fbpixmap.c	\
	fbpoint.c	\
	fbpush.c	\
	fbrop.h		\
	fbscreen.c	\
	fbseg.c		\
	fbsetsp.c	\
	fbsolid.c	\
	fbtrap.c	\
	fbutil.c	\
	fbwindow.c

libwfb_la_SOURCES = $(libfb_la_SOURCES)
