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

module_LTLIBRARIES = libglamoregl.la

libglamoregl_la_SOURCES = \
	$(top_srcdir)/glamor/glamor_egl.c \
	$(top_srcdir)/glamor/glamor_eglmodule.c \
	glamor_xf86_xv.c \
	$()

libglamoregl_la_LDFLAGS = \
	-avoid-version \
	$(GBM_LIBS) \
	$()

libglamoregl_la_LIBADD = \
	$(top_builddir)/glamor/libglamor.la \
	$()

AM_CPPFLAGS = $(XORG_INCS) \
	-I$(top_srcdir)/dri3 \
	-I$(top_srcdir)/glamor \
	$()

AM_CFLAGS = $(DIX_CFLAGS) $(XORG_CFLAGS) $(GLAMOR_CFLAGS) $(GBM_CFLAGS)
