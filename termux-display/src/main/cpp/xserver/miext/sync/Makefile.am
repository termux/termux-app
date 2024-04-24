noinst_LTLIBRARIES = libsync.la

AM_CFLAGS = $(DIX_CFLAGS)

AM_CPPFLAGS = 

if XORG
sdk_HEADERS = misync.h misyncstr.h misyncshm.h misyncfd.h
endif

XSHMFENCE_SRCS = misyncshm.c

libsync_la_SOURCES =	\
	misync.c	\
	misync.h	\
	misyncfd.c	\
	misyncstr.h

if XSHMFENCE
libsync_la_SOURCES += $(XSHMFENCE_SRCS)
endif
