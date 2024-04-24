noinst_LTLIBRARIES = librender.la

AM_CFLAGS = $(DIX_CFLAGS)

librender_la_SOURCES =	\
	animcur.c	\
	filter.c	\
	glyph.c		\
	matrix.c	\
	miindex.c	\
	mipict.c	\
	mirect.c	\
	mitrap.c	\
	mitri.c		\
	picture.c	\
	render.c

if XORG
sdk_HEADERS = picture.h mipict.h glyphstr.h picturestr.h
endif
