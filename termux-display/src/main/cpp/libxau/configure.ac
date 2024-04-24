#
#  Copyright © 2003 Keith Packard, Noah Levitt
#
#  Permission to use, copy, modify, distribute, and sell this software and its
#  documentation for any purpose is hereby granted without fee, provided that
#  the above copyright notice appear in all copies and that both that
#  copyright notice and this permission notice appear in supporting
#  documentation, and that the name of Keith Packard not be used in
#  advertising or publicity pertaining to distribution of the software without
#  specific, written prior permission.  Keith Packard makes no
#  representations about the suitability of this software for any purpose.  It
#  is provided "as is" without express or implied warranty.
#
#  KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
#  INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
#  EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
#  CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
#  DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
#  TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
#  PERFORMANCE OF THIS SOFTWARE.
#

# Initialize Autoconf
AC_PREREQ([2.60])
AC_INIT([libXau], [1.0.11],
	[https://gitlab.freedesktop.org/xorg/lib/libxau/-/issues], [libXau])
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_HEADERS([config.h])
AC_USE_SYSTEM_EXTENSIONS

# Initialize Automake
AM_INIT_AUTOMAKE([foreign dist-xz])

# Initialize libtool
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL

# Require X.Org macros 1.8 or later for MAN_SUBSTS set by XORG_MANPAGE_SECTIONS
m4_ifndef([XORG_MACROS_VERSION],
          [m4_fatal([must install xorg-macros 1.8 or later before running autoconf/autogen])])
XORG_MACROS_VERSION(1.8)
XORG_DEFAULT_OPTIONS

# Checks for programs.
AC_PROG_LN_S

# Checks for library functions.
AC_CHECK_FUNCS([explicit_bzero pathconf])

# Obtain compiler/linker options for dependencies
PKG_CHECK_MODULES(XAU, xproto)

# Checks for typedefs, structures, and compiler characteristics.
AC_SYS_LARGEFILE

AC_ARG_ENABLE(xthreads,
              AS_HELP_STRING([--disable-xthreads],
                [Disable libXau support for Multithreading]),
              [xthreads=$enableval],[xthreads=yes])

if test "x$xthreads" = "xyes" ; then
    AC_DEFINE(XTHREADS,1,[Whether libXau is compiled with thread support])
    AC_CHECK_LIB(c, gethostbyname_r, [mtsafe=yes])
    if test "x$mtsafe" = "x" ; then
	AC_CHECK_LIB(nsl, gethostbyname_r, [mtsafe=yes])
    fi
    if test "x$mtsafe" = "xyes" ; then
	AC_DEFINE(XUSE_MTSAFE_API, 1,
		  [Whether libXau needs to use MT safe API's])
    fi

# XXX incomplete, please fill this in
    case $host_os in
    solaris*)
	XTHREAD_CFLAGS="-D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS" ;;
    esac
    XAU_CFLAGS="$XAU_CFLAGS $XTHREAD_CFLAGS"
fi

# Allow checking code with lint, sparse, etc.
XORG_WITH_LINT
XORG_LINT_LIBRARY([Xau])
LINT_FLAGS="${LINT_FLAGS} ${XAU_CFLAGS}"

AC_CONFIG_FILES([Makefile
		man/Makefile
		xau.pc])
AC_OUTPUT
