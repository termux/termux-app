dnl
dnl Copyright © 2013 Keith Packard
dnl
dnl Permission to use, copy, modify, distribute, and sell this software and its
dnl documentation for any purpose is hereby granted without fee, provided that
dnl the above copyright notice appear in all copies and that both that copyright
dnl notice and this permission notice appear in supporting documentation, and
dnl that the name of the copyright holders not be used in advertising or
dnl publicity pertaining to distribution of the software without specific,
dnl written prior permission.  The copyright holders make no representations
dnl about the suitability of this software for any purpose.  It is provided "as
dnl is" without express or implied warranty.
dnl
dnl THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
dnl INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
dnl EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
dnl CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
dnl DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
dnl TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
dnl OF THIS SOFTWARE.
dnl
dnl
dnl Process this file with autoconf to create configure.

AC_PREREQ([2.60])
AC_INIT([libxshmfence], [1.3.2],
        [https://gitlab.freedesktop.org/xorg/lib/libxshmfence/-/issues],
        [libxshmfence])
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_HEADERS([config.h])

AC_USE_SYSTEM_EXTENSIONS

# Initialize Automake
AM_INIT_AUTOMAKE([foreign dist-xz])

# Initialize libtool
AC_PROG_LIBTOOL

# Require xorg-macros: XORG_DEFAULT_OPTIONS, XORG_WITH_LINT
m4_ifndef([XORG_MACROS_VERSION],
          [m4_fatal([must install xorg-macros 1.3 or later before running autoconf/autogen])])
XORG_MACROS_VERSION(1.3)
XORG_DEFAULT_OPTIONS

dnl Allow checking code with lint, sparse, etc.
XORG_WITH_LINT
LINT_FLAGS="${LINT_FLAGS} ${FONTENC_CFLAGS}"

# Checks for typedefs, structures, and compiler characteristics.
AC_SYS_LARGEFILE

dnl
dnl Locate a suitable tmp file system for creating shared memory files
dnl

AC_ARG_ENABLE(futex,	AS_HELP_STRING([--enable-futex], [Enable futexes (default: auto)]),
		[FUTEX=$enableval], [FUTEX=auto])

if test "x$FUTEX" = "xauto"; then
	AC_CHECK_HEADER([linux/futex.h], [FUTEX=yes])
fi

if test "x$FUTEX" = "xauto"; then
	AC_CHECK_HEADER([sys/umtx.h], [FUTEX=yes], [FUTEX=no],
	    [#include <errno.h>
	     #include <sys/types.h>])
	if test "x$FUTEX" = "xyes"; then
		AC_DEFINE(HAVE_UMTX, 1, [Use umtx])
	fi
fi

if test "x$FUTEX" = "xyes"; then
	PTHREAD=no
	AC_DEFINE(HAVE_FUTEX,1,[Use futexes])
else
	PTHREAD=yes
	AC_DEFINE(HAVE_PTHREAD,1,[Use pthread primitives])
fi

PTHREAD_LIBS=
if test "x$PTHREAD" = "xyes"; then
	AC_CHECK_LIB(pthread,pthread_create,[PTHREAD_LIBS=-lpthread],[PTHREAD_LIBS=])
fi

AC_SUBST([PTHREAD_LIBS])

AM_CONDITIONAL([FUTEX], [test x"$FUTEX" = xyes])
AM_CONDITIONAL([PTHREAD], [test x"$PTHREAD" = xyes])

PKG_CHECK_MODULES(XPROTO, xproto)

AC_SUBST([XPROTO_CFLAGS])

CFLAGS="$CFLAGS $XPROTO_CFLAGS"

AC_CHECK_FUNCS(memfd_create mkostemp)

AC_CHECK_DECLS([__NR_memfd_create], [], [], [[#include <asm/unistd.h>]])

AC_CHECK_HEADERS([sys/memfd.h], [AC_DEFINE([HAVE_MEMFD_H], 1, [Has sys/memfd.h header])])

AC_ARG_ENABLE(visibility,     AC_HELP_STRING([--enable-visibility], [Enable symbol visibility (default: auto)]),
				[SYMBOL_VISIBILITY=$enableval],
				[SYMBOL_VISIBILITY=auto])

dnl ==================================================================
dnl symbol visibility
symbol_visibility=
have_visibility=disabled
if test x$SYMBOL_VISIBILITY != xno; then
    AC_MSG_CHECKING(for symbol visibility support)
    if test x$GCC = xyes; then
	VISIBILITY_CFLAGS="-fvisibility=hidden"
    else
	if test x$SUNCC = xyes; then
	    VISIBILITY_CFLAGS="-xldscope=hidden"
	else
	    have_visibility=no
	fi
    fi
    if test x$have_visibility != xno; then
	AC_TRY_COMPILE(
	    [#include <X11/Xfuncproto.h>
	     extern _X_HIDDEN int hidden_int;
	     extern _X_EXPORT int public_int;
	     extern _X_HIDDEN int hidden_int_func(void);
	     extern _X_EXPORT int public_int_func(void);],
	    [],
	    have_visibility=yes,
	    have_visibility=no)
    fi
    AC_MSG_RESULT([$have_visibility])
    if test x$have_visibility != xno; then
	symbol_visibility=$VISIBILITY_CFLAGS
	CFLAGS="$CFLAGS $VISIBILITY_CFLAGS"
    fi
fi

AC_ARG_WITH(shared-memory-dir, AS_HELP_STRING([--with-shared-memory-dir=PATH], [Path to directory in a world-writable temporary directory for anonymous shared memory (default: auto)]),
[],
[with_shared_memory_dir=yes])

shmdirs="/run/shm /dev/shm /var/tmp /tmp"

case x"$with_shared_memory_dir" in
xyes)
	for dir in $shmdirs; do
		case x"$with_shared_memory_dir" in
		xyes)
			echo Checking temp dir "$dir"
			if test -d "$dir"; then
				with_shared_memory_dir="$dir"
			fi
			;;
		esac
	done
	;;
x/*)
	;;
xno)
	;;
*)
	AC_MSG_ERROR([Invalid directory specified for --with-shared-memory-dir: $with_shared_memory_dir])
	;;
esac

case x"$with_shared_memory_dir" in
xyes)
	AC_MSG_ERROR([No directory found for shared memory temp files.])
	;;
xno)
	;;
*)
	AC_DEFINE_UNQUOTED(SHMDIR, ["$with_shared_memory_dir"], [Directory for shared memory temp files])
	;;
esac

AC_CONFIG_FILES([Makefile
                 src/Makefile
		 test/Makefile
                 xshmfence.pc])
AC_OUTPUT
