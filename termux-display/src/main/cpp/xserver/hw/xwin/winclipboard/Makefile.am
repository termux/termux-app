noinst_LTLIBRARIES = libXWinclipboard.la

libXWinclipboard_la_SOURCES = \
	internal.h \
	winclipboard.h \
	textconv.c \
	thread.c \
	wndproc.c \
	xevents.c

libXWinclipboard_la_CFLAGS = -DHAVE_XWIN_CONFIG_H \
			     $(DIX_CFLAGS) \
			     $(XWINMODULES_CFLAGS)

libXWinclipboard_la_LDFLAGS = -static -no-undefined

bin_PROGRAMS = xwinclip

xwinclip_SOURCES = xwinclip.c debug.c

xwinclip_CFLAGS = $(XWINMODULES_CFLAGS)

xwinclip_LDADD = libXWinclipboard.la $(XWINMODULES_LIBS) -lgdi32 -lpthread

include $(top_srcdir)/manpages.am
appman_PRE = xwinclip.man
