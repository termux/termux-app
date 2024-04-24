noinst_LTLIBRARIES = libdamage.la

AM_CFLAGS = $(DIX_CFLAGS)

AM_CPPFLAGS = -I$(srcdir)/../cw

if XORG
sdk_HEADERS = damage.h damagestr.h
endif

libdamage_la_SOURCES =	\
	damage.c	\
	damage.h	\
	damagestr.h
