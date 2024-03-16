#
#  Copyright 2005  Red Hat, Inc.
#
#  Permission to use, copy, modify, distribute, and sell this software and its
#  documentation for any purpose is hereby granted without fee, provided that
#  the above copyright notice appear in all copies and that both that
#  copyright notice and this permission notice appear in supporting
#  documentation, and that the name of Red Hat not be used in
#  advertising or publicity pertaining to distribution of the software without
#  specific, written prior permission.  Red Hat makes no
#  representations about the suitability of this software for any purpose.  It
#  is provided "as is" without express or implied warranty.
#
#  RED HAT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
#  INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
#  EVENT SHALL RED HAT BE LIABLE FOR ANY SPECIAL, INDIRECT OR
#  CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
#  DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
#  TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
#  PERFORMANCE OF THIS SOFTWARE.

SUBDIRS = man
bin_PROGRAMS = xkbcomp

AM_CPPFLAGS = -DDFLT_XKB_CONFIG_ROOT='"$(XKBCONFIGROOT)"'
AM_CFLAGS = $(XKBCOMP_CFLAGS) $(CWARNFLAGS)
xkbcomp_LDADD = $(XKBCOMP_LIBS)

xkbcomp_SOURCES = \
        action.c \
        action.h \
        alias.c \
        alias.h \
        compat.c \
        compat.h \
        expr.c \
        expr.h \
        geometry.c \
        indicators.c \
        indicators.h \
        keycodes.c \
        keycodes.h \
        keymap.c \
        keytypes.c \
        listing.c \
        misc.c \
        misc.h \
        parseutils.c \
        parseutils.h \
        symbols.c \
        tokens.h \
        utils.c \
        utils.h \
        vmod.c \
        vmod.h \
        xkbcomp.c \
        xkbcomp.h \
        xkbparse.y \
        xkbpath.c \
        xkbpath.h \
        xkbscan.c

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = xkbcomp.pc

MAINTAINERCLEANFILES = ChangeLog INSTALL

.PHONY: ChangeLog INSTALL

INSTALL:
	$(INSTALL_CMD)

ChangeLog:
	$(CHANGELOG_CMD)

dist-hook: ChangeLog INSTALL
