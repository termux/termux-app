#  Copyright © 2003 Keith Packard
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
AC_INIT([libXfont2], [2.0.6],
	[https://gitlab.freedesktop.org/xorg/lib/libxfont/-/issues], [libXfont2])
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_HEADERS([config.h])

# Initialize Automake
AM_INIT_AUTOMAKE([foreign dist-xz subdir-objects])

# Set common system defines for POSIX extensions, such as _GNU_SOURCE
# Must be called before any macros that run the compiler (like
# AC_PROG_LIBTOOL) to avoid autoconf errors.
AC_USE_SYSTEM_EXTENSIONS

# Initialize libtool
AC_PROG_LIBTOOL

# Require xorg-macros minimum of 1.10 for DocBook XML documentation
m4_ifndef([XORG_MACROS_VERSION],
          [m4_fatal([must install xorg-macros 1.10 or later before running autoconf/autogen])])
XORG_MACROS_VERSION(1.10)
XORG_DEFAULT_OPTIONS
XORG_ENABLE_DEVEL_DOCS
XORG_WITH_XMLTO(0.0.22)
XORG_WITH_FOP
XORG_CHECK_SGML_DOCTOOLS(1.7)

# Checks for header files.
AC_CHECK_HEADERS([endian.h poll.h sys/poll.h])

# Checks for library functions.
AC_CHECK_FUNCS([poll readlink])
AC_SEARCH_LIBS([strlcat], [bsd])
if test "$ac_cv_search_strlcat" = "-lbsd"; then
  AC_DEFINE(HAVE_LIBBSD,1,[Has libbsd])
fi
AC_CONFIG_LIBOBJ_DIR([src/util])
AC_REPLACE_FUNCS([reallocarray realpath strlcat strlcpy])

# Check for BSDish err.h
AC_CHECK_HEADERS([err.h])

# If the first PKG_CHECK_MODULES appears inside a conditional, pkg-config
# must first be located explicitly.
PKG_PROG_PKG_CONFIG

#
# select libraries to include
#

XFONT_FONTFILE=no

#
# FreeType for all scalable (OpenType, TrueType, PostScript) and
# some bitmap formats (BDF & PCF)
#
AC_ARG_ENABLE(freetype,
	AS_HELP_STRING([--disable-freetype],
		[Build freetype backend (default: enabled)]),
	[XFONT_FREETYPE=$enableval],[XFONT_FREETYPE=yes])
AM_CONDITIONAL(XFONT_FREETYPE, [test x$XFONT_FREETYPE = xyes])
if test x$XFONT_FREETYPE = xyes; then
	AC_DEFINE(XFONT_FREETYPE,1,[Support FreeType rasterizer for nearly all font file formats])
	AC_ARG_WITH(freetype-config,
		AS_HELP_STRING([--with-freetype-config=PROG],
			[Use FreeType configuration program PROG]),
		[freetype_config=$withval], [freetype_config=auto])

	if test "$freetype_config" = "auto" ; then
		PKG_CHECK_MODULES(FREETYPE, freetype2,
			freetype_config=no, freetype_config=yes)
	fi

	if test "$freetype_config" = "yes"; then
		AC_PATH_PROG(ft_config,freetype-config,no)
		if test "$ft_config" = "no"; then
			AC_MSG_ERROR([You must have freetype installed; see http://www.freetype.org/])
		fi
	else
		ft_config="$freetype_config"
	fi

	if test "$freetype_config" != "no"; then
		FREETYPE_CFLAGS="`$ft_config --cflags`"
		FREETYPE_LIBS="`$ft_config --libs`"
	fi
	FREETYPE_REQUIRES="freetype2"
	XFONT_FONTFILE=yes
else
	FREETYPE_CFLAGS=""
	FREETYPE_LIBS=""
	FREETYPE_REQUIRES=""
fi
AC_SUBST(FREETYPE_LIBS)
AC_SUBST(FREETYPE_CFLAGS)
AC_SUBST(FREETYPE_REQUIRES)

#
# Bitmap fonts in files:
#	Builtins (gzipped pcf format in .text segment)
#	PCF
#	BDF
#	SNF
#

AC_DEFINE(X_GZIP_FONT_COMPRESSION,1,[Support gzip for bitmap fonts])
X_GZIP_FONT_COMPRESSION=1
AC_SUBST(X_GZIP_FONT_COMPRESSION)
PKG_CHECK_MODULES(ZLIB, [zlib], [Z_REQUIRES=zlib], [Z_REQUIRES=""])
if test "x$Z_REQUIRES" = "x"; then
	AC_CHECK_LIB(z, gzopen, [Z_LIBS=-lz],
		AC_MSG_ERROR([*** zlib is required]))
fi
AC_SUBST(Z_REQUIRES)

AC_ARG_WITH(bzip2,
	AS_HELP_STRING([--with-bzip2],
	  [Use libbz2 to support bzip2 compressed bitmap fonts (default: no)]),
	[], [with_bzip2=no])
if test "x$with_bzip2" = xyes; then
	AC_CHECK_LIB(bz2, BZ2_bzopen, [Z_LIBS="$Z_LIBS -lbz2"],
		AC_MSG_ERROR([*** libbz2 is required for bzip2 support]))
	AC_DEFINE(X_BZIP2_FONT_COMPRESSION,1,[Support bzip2 for bitmap fonts])
fi
AM_CONDITIONAL(X_BZIP2_FONT_COMPRESSION, [test "x$with_bzip2" = xyes ])
AC_SUBST(Z_LIBS)

AC_ARG_ENABLE(builtins,
	AS_HELP_STRING([--disable-builtins],
		[Support builtin fonts (default: enabled)]),
	[XFONT_BUILTINS=$enableval], [XFONT_BUILTINS=yes])
AM_CONDITIONAL(XFONT_BUILTINS, [test "x$XFONT_BUILTINS" = xyes ])
if test "x$XFONT_BUILTINS" = xyes; then
	AC_DEFINE(XFONT_BUILTINS,1,[Support built-in fonts])
	XFONT_BITMAP=yes
fi

AC_ARG_ENABLE(pcfformat,
	AS_HELP_STRING([--disable-pcfformat],
		[Support PCF format bitmap fonts (default: enabled)]),
	[XFONT_PCFFORMAT=$enableval], [XFONT_PCFFORMAT=yes])
AM_CONDITIONAL(XFONT_PCFFORMAT, [test "x$XFONT_PCFFORMAT" = xyes])
if test "x$XFONT_PCFFORMAT" = xyes; then
	AC_DEFINE(XFONT_PCFFORMAT,1,[Support pcf format bitmap font files])
	XFONT_BITMAP=yes
fi

AM_CONDITIONAL(XFONT_PCF_OR_BUILTIN,
	[test "x$XFONT_PCFFORMAT" = xyes -o "x$XFONT_BUILTINS" = xyes])

AC_ARG_ENABLE(bdfformat,
	AS_HELP_STRING([--disable-bdfformat],
		[Support BDF format bitmap fonts (default: enabled)]),
	[XFONT_BDFFORMAT=$enableval], [XFONT_BDFFORMAT=yes])
AM_CONDITIONAL(XFONT_BDFFORMAT, [test "x$XFONT_BDFFORMAT" = xyes])
if test "x$XFONT_BDFFORMAT" = xyes; then
	AC_DEFINE(XFONT_BDFFORMAT,1,[Support bdf format bitmap font files])
	XFONT_BITMAP=yes
fi

AC_ARG_ENABLE(snfformat,
	AS_HELP_STRING([--enable-snfformat],
		[Support SNF format bitmap fonts (default: disabled)]),
	[XFONT_SNFFORMAT=$enableval], [XFONT_SNFFORMAT=no])
AM_CONDITIONAL(XFONT_SNFFORMAT, [test "x$XFONT_SNFFORMAT" = xyes])
if test "x$XFONT_SNFFORMAT" = xyes; then
	AC_DEFINE(XFONT_SNFFORMAT,1,[Support snf format bitmap font files])
	XFONT_BITMAP=yes
fi

AM_CONDITIONAL(XFONT_BITMAP, [test "x$XFONT_BITMAP" = xyes ])
if test "x$XFONT_BITMAP" = xyes; then
	AC_DEFINE(XFONT_BITMAP,1,[Support bitmap font files])
	XFONT_FONTFILE=yes
fi

AM_CONDITIONAL(XFONT_FONTFILE, [test "x$XFONT_FONTFILE" = xyes])
if test "x$XFONT_FONTFILE" = xyes; then
	AC_DEFINE(XFONT_FONTFILE,1,[Support fonts in files])
fi

#
# Support connection to font servers?
#

AC_ARG_ENABLE(fc,
	AS_HELP_STRING([--disable-fc],
		[Support connections to xfs servers (default: enabled)]),
	[XFONT_FC=$enableval],[XFONT_FC=yes])
AM_CONDITIONAL(XFONT_FC, [test "x$XFONT_FC" = xyes])
if test "x$XFONT_FC" = xyes; then
	AC_DEFINE(XFONT_FC,1,[Support the X Font Services Protocol])
fi


AC_CHECK_LIB(m, hypot, [MATH_LIBS=-lm
AC_SUBST(MATH_LIBS)], AC_MSG_ERROR([*** libm is required]))

PKG_CHECK_MODULES(XFONT, [xproto xtrans fontsproto >= 2.1.3 fontenc])

# Transport selection macro from xtrans.m4
XTRANS_CONNECTION_FLAGS

case $host_os in
    linux*)
        OS_CFLAGS="-D_XOPEN_SOURCE"
        ;;
    *)
        OS_CFLAGS=
        ;;
esac

save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -fvisibility=hidden"
CFLAGS_VISIBILITY=
AC_COMPILE_IFELSE(
  [AC_LANG_SOURCE([[
    #if defined(__CYGWIN__) || defined(__MINGW32__)
    #error No visibility support
    #endif
    extern __attribute__((__visibility__("default"))) int x;
  ]])],
  [CFLAGS_VISIBILITY=-fvisibility=hidden],
  []
)
CFLAGS="$save_CFLAGS"
OS_CFLAGS="$OS_CFLAGS $CFLAGS_VISIBILITY"

AC_SUBST([OS_CFLAGS])

AC_CONFIG_FILES([Makefile
		doc/Makefile
		xfont2.pc])
AC_OUTPUT
