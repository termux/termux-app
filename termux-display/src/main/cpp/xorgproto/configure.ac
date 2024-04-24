dnl
dnl  Copyright © 2017 Keith Packard
dnl
dnl  Permission to use, copy, modify, distribute, and sell this software and its
dnl  documentation for any purpose is hereby granted without fee, provided that
dnl  the above copyright notice appear in all copies and that both that
dnl  copyright notice and this permission notice appear in supporting
dnl  documentation, and that the name of Keith Packard not be used in
dnl  advertising or publicity pertaining to distribution of the software without
dnl  specific, written prior permission.  Keith Packard makes no
dnl  representations about the suitability of this software for any purpose.  It
dnl  is provided "as is" without express or implied warranty.
dnl
dnl  KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
dnl  INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
dnl  EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
dnl  CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
dnl  DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
dnl  TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
dnl  PERFORMANCE OF THIS SOFTWARE.
dnl
dnl Process this file with autoconf to create configure.

AC_PREREQ([2.60])
# keep version in sync with meson.build
AC_INIT([xorgproto], [2023.2],
        [https://gitlab.freedesktop.org/xorg/proto/xorgproto/issues])
AM_INIT_AUTOMAKE([foreign dist-xz])

# Require xorg-macros: XORG_DEFAULT_OPTIONS
m4_ifndef([XORG_MACROS_VERSION],
          [m4_fatal([must install xorg-macros 1.12 or later before running autoconf/autogen])])
XORG_MACROS_VERSION(1.12)
XORG_DEFAULT_OPTIONS
XORG_ENABLE_SPECS
XORG_WITH_XMLTO(0.0.22)
XORG_WITH_FOP
XORG_WITH_XSLTPROC
XORG_CHECK_SGML_DOCTOOLS(1.8)

AM_PATH_PYTHON([3.6], , [:])
AM_CONDITIONAL([HAVE_PYTHON], [test "$PYTHON" != :])

AC_CANONICAL_HOST

AC_ARG_ENABLE(legacy,
	      AS_HELP_STRING([--enable-legacy],
			     [Install legacy protocol headers (default: false)]),
			     [LEGACY=$enableval], [LEGACY=no])
AM_CONDITIONAL(LEGACY, [test "x$LEGACY" = "xyes"])

AC_CHECK_HEADERS([sys/select.h sys/param.h sys/types.h sys/time.h])

# Handle Xpoll.h.in
# Avoid determining fds_bits on WIN32 hosts (not including cygwin)
case $host_os in
	mingw*)		fds_bits_found=true;;
	*)		fds_bits_found=false;;
esac

if test "x$fds_bits_found" = xfalse ; then
   AC_CHECK_MEMBER(fd_set.fds_bits,
			   [
			       fds_bits_found=plain
			       USE_FDS_BITS="fds_bits"
			   ],,
			   [
                               #ifdef HAVE_SYS_PARAM_H
                               #include <sys/param.h>
                               #endif
                               #ifdef HAVE_SYS_TYPES_H
                               #include <sys/types.h>
                               #endif
                               #ifdef HAVE_SYS_TIME_H
                               #include <sys/time.h>
                               #endif
                               #ifdef HAVE_SYS_SELECT_H
			       #include <sys/select.h>
                               #endif
			   ])
fi

if test "x$fds_bits_found" = xfalse ; then
   AC_CHECK_MEMBER(fd_set.__fds_bits,
			[
			    fds_bits_found=underscores
			    USE_FDS_BITS="__fds_bits"
			],,
			[
                               #ifdef HAVE_SYS_PARAM_H
                               #include <sys/param.h>
                               #endif
                               #ifdef HAVE_SYS_TYPES_H
                               #include <sys/types.h>
                               #endif
                               #ifdef HAVE_SYS_TIME_H
                               #include <sys/time.h>
                               #endif
                               #ifdef HAVE_SYS_SELECT_H
			       #include <sys/select.h>
                               #endif
			])
fi

if test "x$fds_bits_found" = xfalse ; then
    AC_MSG_ERROR([Could not determine how to access the fds_bits or equivalent
                  structure in fd_set on your platform.])
fi

AC_SUBST([USE_FDS_BITS])

AC_CONFIG_FILES([applewmproto.pc
		 bigreqsproto.pc
		 compositeproto.pc
		 damageproto.pc
		 dmxproto.pc
		 dpmsproto.pc
		 dri2proto.pc
		 dri3proto.pc
		 evieproto.pc
		 fixesproto.pc
		 fontcacheproto.pc
		 fontsproto.pc
		 glproto.pc
		 include/X11/Xpoll.h
		 inputproto.pc
		 kbproto.pc
		 lg3dproto.pc
		 Makefile
		 presentproto.pc
		 printproto.pc
		 randrproto.pc
		 recordproto.pc
		 renderproto.pc
		 resourceproto.pc
		 scrnsaverproto.pc
		 specs/Makefile
		 specs/bigreqsproto/Makefile
		 specs/fontsproto/Makefile
		 specs/kbproto/Makefile
		 specs/recordproto/Makefile
		 specs/scrnsaverproto/Makefile
		 specs/xcmiscproto/Makefile
		 specs/xextproto/Makefile
		 specs/xproto/Makefile
		 trapproto.pc
		 videoproto.pc
		 windowswmproto.pc
		 xcalibrateproto.pc
		 xcmiscproto.pc
		 xextproto.pc
		 xf86bigfontproto.pc
		 xf86dgaproto.pc
		 xf86driproto.pc
		 xf86miscproto.pc
		 xf86rushproto.pc
		 xf86vidmodeproto.pc
		 xineramaproto.pc
		 xproto.pc
		 xproxymngproto.pc
		 xwaylandproto.pc])

AC_OUTPUT
