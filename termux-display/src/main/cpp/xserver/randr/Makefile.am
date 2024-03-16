noinst_LTLIBRARIES = librandr.la

AM_CFLAGS = $(DIX_CFLAGS)

XINERAMA_SRCS = rrxinerama.c

if XORG
sdk_HEADERS = randrstr.h rrtransform.h
endif

librandr_la_SOURCES =	\
	randr.c		\
	randrstr.h	\
	rrcrtc.c	\
	rrdispatch.c	\
	rrinfo.c	\
	rrlease.c	\
	rrmode.c	\
	rrmonitor.c	\
	rroutput.c	\
	rrpointer.c	\
	rrproperty.c	\
	rrprovider.c    \
	rrproviderproperty.c    \
	rrscreen.c	\
	rrsdispatch.c	\
	rrtransform.h	\
	rrtransform.c

if XINERAMA
librandr_la_SOURCES += ${XINERAMA_SRCS}
endif

