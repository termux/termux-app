# Copyright © 2013 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice (including the next
# paragraph) shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.

SUBDIRS = man

AM_CPPFLAGS = 			\
	@KDRIVE_INCS@		\
	@KDRIVE_CFLAGS@		\
	@XEPHYR_INCS@		\
	@XEPHYR_CFLAGS@		\
	-DHAVE_DIX_CONFIG_H	\
	-I$(top_srcdir)		\
	-I$(top_srcdir)/glamor	\
	-I$(top_srcdir)/exa

if XV
XV_SRCS = ephyrvideo.c
endif

if GLAMOR
if XV
GLAMOR_XV_SRCS = ephyr_glamor_xv.c
endif

GLAMOR_SRCS = \
	ephyr_glamor_glx.c \
	ephyr_glamor_glx.h \
	$(GLAMOR_XV_SRCS)  \
	$()
endif

bin_PROGRAMS = Xephyr

Xephyr_SOURCES = \
	ephyr.c \
	ephyr.h \
	ephyrlog.h \
	ephyr_draw.c \
	ephyrinit.c \
	ephyrcursor.c \
	hostx.c \
	hostx.h \
	$(XV_SRCS) \
	$(GLAMOR_SRCS) \
	$()

if GLAMOR
AM_CPPFLAGS += $(XLIB_CFLAGS)
XEPHYR_GLAMOR_LIB = \
	$(top_builddir)/glamor/libglamor.la \
	$(top_builddir)/glamor/libglamor_egl_stubs.la \
	$()
endif

Xephyr_LDADD = 						\
	$(top_builddir)/exa/libexa.la			\
	$(XEPHYR_GLAMOR_LIB)				\
	@KDRIVE_LIBS@					\
	@XEPHYR_LIBS@

Xephyr_DEPENDENCIES = @KDRIVE_LOCAL_LIBS@ $(XEPHYR_GLAMOR_LIB)

Xephyr_LDFLAGS = $(LD_EXPORT_SYMBOLS_FLAG)

relink:
	$(AM_V_at)rm -f $(bin_PROGRAMS) && $(MAKE) $(bin_PROGRAMS)
