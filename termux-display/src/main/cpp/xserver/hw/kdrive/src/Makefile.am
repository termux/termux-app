AM_CPPFLAGS = 					\
	@KDRIVE_INCS@				\
	@KDRIVE_CFLAGS@

AM_CFLAGS =	-DHAVE_DIX_CONFIG_H

noinst_LTLIBRARIES = libkdrive.la

if XV
KDRIVE_XV_SOURCES =	\
	kxv.c		\
	kxv.h
endif

libkdrive_la_SOURCES =	\
	kcmap.c		\
	kdrive.c	\
	kdrive.h	\
	kinfo.c		\
	kinput.c	\
	kshadow.c	\
	$(KDRIVE_XV_SOURCES) \
	$(top_srcdir)/mi/miinitext.c	\
	$(top_srcdir)/mi/miinitext.h

if CONFIG_UDEV
libkdrive_la_LIBADD = $(top_builddir)/config/libconfig.la
else
if CONFIG_HAL
libkdrive_la_LIBADD = $(top_builddir)/config/libconfig.la
endif
endif
