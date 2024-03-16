dnl  Copyright 2005 Red Hat, Inc.
dnl 
dnl  Permission to use, copy, modify, distribute, and sell this software and its
dnl  documentation for any purpose is hereby granted without fee, provided that
dnl  the above copyright notice appear in all copies and that both that
dnl  copyright notice and this permission notice appear in supporting
dnl  documentation, and that the name of Red Hat not be used in
dnl  advertising or publicity pertaining to distribution of the software without
dnl  specific, written prior permission.  Red Hat makes no
dnl  representations about the suitability of this software for any purpose.  It
dnl  is provided "as is" without express or implied warranty.
dnl 
dnl  RED HAT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
dnl  INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
dnl  EVENT SHALL RED HAT BE LIABLE FOR ANY SPECIAL, INDIRECT OR
dnl  CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
dnl  DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
dnl  TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
dnl  PERFORMANCE OF THIS SOFTWARE.
dnl
dnl Process this file with autoconf to create configure.

AC_PREREQ([2.57])

#   Pixman versioning scheme
#
#   - The version in git has an odd MICRO version number
#
#   - Released versions, both development and stable, have an
#     even MICRO version number
#
#   - Released development versions have an odd MINOR number
#
#   - Released stable versions have an even MINOR number
#
#   - Versions that break ABI must have a new MAJOR number
#
#   - If you break the ABI, then at least this must be done:
#
#        - increment MAJOR
#
#        - In the first development release where you break ABI, find
#          all instances of "pixman-n" and change them to pixman-(n+1)
#
#          This needs to be done at least in 
#                    configure.ac
#                    all Makefile.am's
#                    pixman-n.pc.in
#
#      This ensures that binary incompatible versions can be installed
#      in parallel.  See http://www106.pair.com/rhp/parallel.html for
#      more information
#

m4_define([pixman_major], 0)
m4_define([pixman_minor], 42)
m4_define([pixman_micro], 2)

m4_define([pixman_version],[pixman_major.pixman_minor.pixman_micro])

AC_INIT(pixman, pixman_version, [pixman@lists.freedesktop.org], pixman)
AM_INIT_AUTOMAKE([foreign dist-xz])

# Suppress verbose compile lines
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_CONFIG_HEADERS(config.h)

AC_CANONICAL_HOST

test_CFLAGS=${CFLAGS+set} # We may override autoconf default CFLAGS.

AC_PROG_CC
AM_PROG_AS
AC_PROG_LIBTOOL
AC_CHECK_FUNCS([getisax])
AC_C_BIGENDIAN
AC_C_INLINE

dnl PIXMAN_LINK_WITH_ENV(env-setup, program, true-action, false-action)
dnl
dnl Compiles and links the given program in the environment setup by env-setup
dnl and executes true-action on success and false-action on failure.
AC_DEFUN([PIXMAN_LINK_WITH_ENV],[dnl
	save_CFLAGS="$CFLAGS"
	save_LDFLAGS="$LDFLAGS"
	save_LIBS="$LIBS"
	CFLAGS=""
	LDFLAGS=""
	LIBS=""
	$1
	CFLAGS="$save_CFLAGS $CFLAGS"
	LDFLAGS="$save_LDFLAGS $LDFLAGS"
	LIBS="$save_LIBS $LIBS"
	AC_LINK_IFELSE(
		[AC_LANG_SOURCE([$2])],
		[pixman_cc_stderr=`test -f conftest.err && cat conftest.err`
		 pixman_cc_flag=yes],
		[pixman_cc_stderr=`test -f conftest.err && cat conftest.err`
		 pixman_cc_flag=no])

	if test "x$pixman_cc_stderr" != "x"; then
		pixman_cc_flag=no
	fi

	if test "x$pixman_cc_flag" = "xyes"; then
		ifelse([$3], , :, [$3])
	else
		ifelse([$4], , :, [$4])
	fi
	CFLAGS="$save_CFLAGS"
	LDFLAGS="$save_LDFLAGS"
	LIBS="$save_LIBS"
])

dnl Find a -Werror for catching warnings.
WERROR=
for w in -Werror -errwarn; do
    if test "z$WERROR" = "z"; then
        AC_MSG_CHECKING([whether the compiler supports $w])
        PIXMAN_LINK_WITH_ENV(
		[CFLAGS=$w],
		[int main(int c, char **v) { (void)c; (void)v; return 0; }],
		[WERROR=$w; yesno=yes], [yesno=no])
	AC_MSG_RESULT($yesno)
    fi
done

dnl PIXMAN_CHECK_CFLAG(flag, [program])
dnl  Adds flag to CFLAGS if the given program links without warnings or errors.
AC_DEFUN([PIXMAN_CHECK_CFLAG], [dnl
	AC_MSG_CHECKING([whether the compiler supports $1])
	PIXMAN_LINK_WITH_ENV(
		[CFLAGS="$WERROR $1"],
		[$2
		 int main(int c, char **v) { (void)c; (void)v; return 0; }
		],
		[_yesno=yes],
		[_yesno=no])
	if test "x$_yesno" = xyes; then
	   CFLAGS="$CFLAGS $1"
	fi
	AC_MSG_RESULT($_yesno)
])

AC_CHECK_SIZEOF(long)

# Checks for Sun Studio compilers
AC_CHECK_DECL([__SUNPRO_C], [SUNCC="yes"], [SUNCC="no"])
AC_CHECK_DECL([__amd64], [AMD64_ABI="yes"], [AMD64_ABI="no"])

# Default CFLAGS to -O -g rather than just the -g from AC_PROG_CC
# if we're using Sun Studio and neither the user nor a config.site
# has set CFLAGS.
if test $SUNCC = yes &&			\
   test "x$test_CFLAGS" = "x" &&	\
   test "$CFLAGS" = "-g"
then
  CFLAGS="-O -g"
fi

# 
# We ignore pixman_major in the version here because the major version should
# always be encoded in the actual library name. Ie., the soname is:
#
#      pixman-$(pixman_major).0.minor.micro
#
m4_define([lt_current], [pixman_minor])
m4_define([lt_revision], [pixman_micro])
m4_define([lt_age], [pixman_minor])

LT_VERSION_INFO="lt_current:lt_revision:lt_age"

PIXMAN_VERSION_MAJOR=pixman_major()
AC_SUBST(PIXMAN_VERSION_MAJOR)
PIXMAN_VERSION_MINOR=pixman_minor()
AC_SUBST(PIXMAN_VERSION_MINOR)
PIXMAN_VERSION_MICRO=pixman_micro()
AC_SUBST(PIXMAN_VERSION_MICRO)

AC_SUBST(LT_VERSION_INFO)

# Check for dependencies

PIXMAN_CHECK_CFLAG([-Wall])
PIXMAN_CHECK_CFLAG([-Wdeclaration-after-statement])
PIXMAN_CHECK_CFLAG([-Wno-unused-local-typedefs])
PIXMAN_CHECK_CFLAG([-fno-strict-aliasing])

dnl =========================================================================
dnl OpenMP for the test suite?
dnl

# Check for OpenMP support only when autoconf support that (require autoconf >=2.62)
OPENMP_CFLAGS=
m4_ifdef([AC_OPENMP], [AC_OPENMP])

if test "x$enable_openmp" = "xyes" && test "x$ac_cv_prog_c_openmp" = "xunsupported" ; then
  AC_MSG_WARN([OpenMP support requested but found unsupported])
fi

dnl May not fail to link without -Wall -Werror added
dnl So try to link only when openmp is supported
dnl ac_cv_prog_c_openmp is not defined when --disable-openmp is used
if test "x$ac_cv_prog_c_openmp" != "xunsupported" && test "x$ac_cv_prog_c_openmp" != "x"; then
  m4_define([openmp_test_program],[dnl
  #include <stdio.h>

  extern unsigned int lcg_seed;
  #pragma omp threadprivate(lcg_seed)
  unsigned int lcg_seed;

  unsigned function(unsigned a, unsigned b)
  {
	lcg_seed ^= b;
	return ((a + b) ^ a ) + lcg_seed;
  }

  int main(int argc, char **argv)
  {
	int i;
	int n1 = 0, n2 = argc;
	unsigned checksum = 0;
	int verbose = argv != NULL;
	unsigned (*test_function)(unsigned, unsigned);
	test_function = function;
	#pragma omp parallel for reduction(+:checksum) default(none) \
					shared(n1, n2, test_function, verbose)
	for (i = n1; i < n2; i++)
	{
		unsigned crc = test_function (i, 0);
		if (verbose)
			printf ("%d: %08X\n", i, crc);
		checksum += crc;
	}
	printf("%u\n", checksum);
	return 0;
  }
  ])

  PIXMAN_LINK_WITH_ENV(
	[CFLAGS="$OPENMP_CFLAGS" LDFLAGS="$OPENMP_CFLAGS"],
	[openmp_test_program],
	[have_openmp=yes],
	[have_openmp=no])
  if test "x$have_openmp" = "xyes" ; then
    AC_DEFINE(USE_OPENMP, 1, [use OpenMP in the test suite])
  fi
fi
AC_SUBST(OPENMP_CFLAGS)

dnl =========================================================================
dnl -fvisibility stuff

PIXMAN_CHECK_CFLAG([-fvisibility=hidden], [dnl
#if defined(__GNUC__) && (__GNUC__ >= 4)
#ifdef _WIN32
#error Have -fvisibility but it is ignored and generates a warning
#endif
#else
#error Need GCC 4.0 for visibility
#endif
])

PIXMAN_CHECK_CFLAG([-xldscope=hidden], [dnl
#if defined(__SUNPRO_C) && (__SUNPRO_C >= 0x550)
#else
#error Need Sun Studio 8 for visibility
#endif
])

dnl ===========================================================================
dnl Check for Loongson Multimedia Instructions

if test "x$LS_CFLAGS" = "x" ; then
    LS_CFLAGS="-mloongson-mmi"
fi

have_loongson_mmi=no
AC_MSG_CHECKING(whether to use Loongson MMI assembler)

xserver_save_CFLAGS=$CFLAGS
CFLAGS=" $LS_CFLAGS $CFLAGS -I$srcdir"
AC_LINK_IFELSE([AC_LANG_SOURCE([[
#ifndef __mips_loongson_vector_rev
#error "Loongson Multimedia Instructions are only available on Loongson"
#endif
#if defined(__GNUC__) && (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 4))
#error "Need GCC >= 4.4 for Loongson MMI compilation"
#endif
#include "pixman/loongson-mmintrin.h"
int main () {
    union {
        __m64 v;
        char c[8];
    } a = { .c = {1, 2, 3, 4, 5, 6, 7, 8} };
    int b = 4;
    __m64 c = _mm_srli_pi16 (a.v, b);
    return 0;
}]])], have_loongson_mmi=yes)
CFLAGS=$xserver_save_CFLAGS

AC_ARG_ENABLE(loongson-mmi,
   [AC_HELP_STRING([--disable-loongson-mmi],
                   [disable Loongson MMI fast paths])],
   [enable_loongson_mmi=$enableval], [enable_loongson_mmi=auto])

if test $enable_loongson_mmi = no ; then
   have_loongson_mmi=disabled
fi

if test $have_loongson_mmi = yes ; then
   AC_DEFINE(USE_LOONGSON_MMI, 1, [use Loongson Multimedia Instructions])
else
   LS_CFLAGS=
fi

AC_MSG_RESULT($have_loongson_mmi)
if test $enable_loongson_mmi = yes && test $have_loongson_mmi = no ; then
   AC_MSG_ERROR([Loongson MMI not detected])
fi

AM_CONDITIONAL(USE_LOONGSON_MMI, test $have_loongson_mmi = yes)

dnl ===========================================================================
dnl Check for MMX

if test "x$MMX_CFLAGS" = "x" ; then
   if test "x$SUNCC" = "xyes"; then
      # Sun Studio doesn't have an -xarch=mmx flag, so we have to use sse
      # but if we're building 64-bit, mmx & sse support is on by default and
      # -xarch=sse throws an error instead
      if test "$AMD64_ABI" = "no" ; then
         MMX_CFLAGS="-xarch=sse"
      fi
   else
      MMX_CFLAGS="-mmmx -Winline"
   fi
fi

have_mmx_intrinsics=no
AC_MSG_CHECKING(whether to use MMX intrinsics)
xserver_save_CFLAGS=$CFLAGS
CFLAGS="$MMX_CFLAGS $CFLAGS"
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#if defined(__GNUC__) && (__GNUC__ < 3 || (__GNUC__ == 3 && __GNUC_MINOR__ < 4))
#error "Need GCC >= 3.4 for MMX intrinsics"
#endif
#include <mmintrin.h>
#include <stdint.h>

/* Check support for block expressions */
#define _mm_shuffle_pi16(A, N)						\
    ({									\
	__m64 ret;							\
									\
	/* Some versions of clang will choke on K */ 			\
	asm ("pshufw %2, %1, %0\n\t"					\
	     : "=y" (ret)						\
	     : "y" (A), "K" ((const int8_t)N)				\
	);								\
									\
	ret;								\
    })

int main () {
    __m64 v = _mm_cvtsi32_si64 (1);
    __m64 w;

    w = _mm_shuffle_pi16(v, 5);

    /* Some versions of clang will choke on this */
    asm ("pmulhuw %1, %0\n\t"
	: "+y" (w)
	: "y" (v)
    );

    return _mm_cvtsi64_si32 (v);
}]])], have_mmx_intrinsics=yes)
CFLAGS=$xserver_save_CFLAGS

AC_ARG_ENABLE(mmx,
   [AC_HELP_STRING([--disable-mmx],
                   [disable x86 MMX fast paths])],
   [enable_mmx=$enableval], [enable_mmx=auto])

if test $enable_mmx = no ; then
   have_mmx_intrinsics=disabled
fi

if test $have_mmx_intrinsics = yes ; then
   AC_DEFINE(USE_X86_MMX, 1, [use x86 MMX compiler intrinsics])
else
   MMX_CFLAGS=
fi

AC_MSG_RESULT($have_mmx_intrinsics)
if test $enable_mmx = yes && test $have_mmx_intrinsics = no ; then
   AC_MSG_ERROR([x86 MMX intrinsics not detected])
fi

AM_CONDITIONAL(USE_X86_MMX, test $have_mmx_intrinsics = yes)

dnl ===========================================================================
dnl Check for SSE2

if test "x$SSE2_CFLAGS" = "x" ; then
   if test "x$SUNCC" = "xyes"; then
      # SSE2 is enabled by default in the Sun Studio 64-bit environment
      if test "$AMD64_ABI" = "no" ; then
         SSE2_CFLAGS="-xarch=sse2"
      fi
   else
      SSE2_CFLAGS="-msse2 -Winline"
   fi
fi

have_sse2_intrinsics=no
AC_MSG_CHECKING(whether to use SSE2 intrinsics)
xserver_save_CFLAGS=$CFLAGS
CFLAGS="$SSE2_CFLAGS $CFLAGS"

AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#if defined(__GNUC__) && (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 2))
#   if !defined(__amd64__) && !defined(__x86_64__)
#      error "Need GCC >= 4.2 for SSE2 intrinsics on x86"
#   endif
#endif
#include <mmintrin.h>
#include <xmmintrin.h>
#include <emmintrin.h>
int param;
int main () {
    __m128i a = _mm_set1_epi32 (param), b = _mm_set1_epi32 (param + 1), c;
	c = _mm_xor_si128 (a, b);
    return _mm_cvtsi128_si32(c);
}]])], have_sse2_intrinsics=yes)
CFLAGS=$xserver_save_CFLAGS

AC_ARG_ENABLE(sse2,
   [AC_HELP_STRING([--disable-sse2],
                   [disable SSE2 fast paths])],
   [enable_sse2=$enableval], [enable_sse2=auto])

if test $enable_sse2 = no ; then
   have_sse2_intrinsics=disabled
fi

if test $have_sse2_intrinsics = yes ; then
   AC_DEFINE(USE_SSE2, 1, [use SSE2 compiler intrinsics])
fi

AC_MSG_RESULT($have_sse2_intrinsics)
if test $enable_sse2 = yes && test $have_sse2_intrinsics = no ; then
   AC_MSG_ERROR([SSE2 intrinsics not detected])
fi

AM_CONDITIONAL(USE_SSE2, test $have_sse2_intrinsics = yes)

dnl ===========================================================================
dnl Check for SSSE3

if test "x$SSSE3_CFLAGS" = "x" ; then
    SSSE3_CFLAGS="-mssse3 -Winline"
fi

have_ssse3_intrinsics=no
AC_MSG_CHECKING(whether to use SSSE3 intrinsics)
xserver_save_CFLAGS=$CFLAGS
CFLAGS="$SSSE3_CFLAGS $CFLAGS"

AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#include <mmintrin.h>
#include <xmmintrin.h>
#include <emmintrin.h>
#include <tmmintrin.h>
int param;
int main () {
    __m128i a = _mm_set1_epi32 (param), b = _mm_set1_epi32 (param + 1), c;
    c = _mm_maddubs_epi16 (a, b);
    return _mm_cvtsi128_si32(c);
}]])], have_ssse3_intrinsics=yes)
CFLAGS=$xserver_save_CFLAGS

AC_ARG_ENABLE(ssse3,
   [AC_HELP_STRING([--disable-ssse3],
                   [disable SSSE3 fast paths])],
   [enable_ssse3=$enableval], [enable_ssse3=auto])

if test $enable_ssse3 = no ; then
   have_ssse3_intrinsics=disabled
fi

if test $have_ssse3_intrinsics = yes ; then
   AC_DEFINE(USE_SSSE3, 1, [use SSSE3 compiler intrinsics])
fi

AC_MSG_RESULT($have_ssse3_intrinsics)
if test $enable_ssse3 = yes && test $have_ssse3_intrinsics = no ; then
   AC_MSG_ERROR([SSSE3 intrinsics not detected])
fi

AM_CONDITIONAL(USE_SSSE3, test $have_ssse3_intrinsics = yes)

dnl ===========================================================================
dnl Other special flags needed when building code using x86 ISA extensions
case $host_os in
   solaris*)
      # When building Solaris binaries, apply a mapfile to ensure that the
      # binaries aren't flagged as only able to run on MMX/SSE/SSSE3 capable
      # CPUs since they check at runtime before using those instructions.
      # Not all linkers grok the mapfile format so we check for that first.
      if test "$host_cpu" = "i386" -o "$host_cpu" = "x86_64"; then
	 use_hwcap_mapfile=no
	 AC_MSG_CHECKING(whether to use a hardware capability map file)
	 hwcap_save_LDFLAGS="$LDFLAGS"
	 HWCAP_LDFLAGS='-Wl,-M,$(srcdir)/solaris-hwcap.mapfile'
	 LDFLAGS="$LDFLAGS -Wl,-M,${srcdir}/pixman/solaris-hwcap.mapfile"
	 AC_LINK_IFELSE([AC_LANG_SOURCE([[int main() { return 0; }]])],
			use_hwcap_mapfile=yes,
			HWCAP_LDFLAGS="")
	 LDFLAGS="$hwcap_save_LDFLAGS"
	 AC_MSG_RESULT($use_hwcap_mapfile)
      fi
      if test "x$MMX_LDFLAGS" = "x" ; then
         MMX_LDFLAGS="$HWCAP_LDFLAGS"
      fi
      if test "x$SSE2_LDFLAGS" = "x" ; then
	 SSE2_LDFLAGS="$HWCAP_LDFLAGS"
      fi
      if test "x$SSSE3_LDFLAGS" = "x" ; then
	 SSSE3_LDFLAGS="$HWCAP_LDFLAGS"
      fi
      ;;
esac

AC_SUBST(LS_CFLAGS)
AC_SUBST(IWMMXT_CFLAGS)
AC_SUBST(MMX_CFLAGS)
AC_SUBST(MMX_LDFLAGS)
AC_SUBST(SSE2_CFLAGS)
AC_SUBST(SSE2_LDFLAGS)
AC_SUBST(SSSE3_CFLAGS)
AC_SUBST(SSSE3_LDFLAGS)

dnl ===========================================================================
dnl Check for VMX/Altivec
if test -n "`$CC -v 2>&1 | grep version | grep Apple`"; then
    VMX_CFLAGS="-faltivec"
else
    VMX_CFLAGS="-maltivec -mabi=altivec"
fi

have_vmx_intrinsics=no
AC_MSG_CHECKING(whether to use VMX/Altivec intrinsics)
xserver_save_CFLAGS=$CFLAGS
CFLAGS="$VMX_CFLAGS $CFLAGS"
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#if defined(__GNUC__) && (__GNUC__ < 3 || (__GNUC__ == 3 && __GNUC_MINOR__ < 4))
#error "Need GCC >= 3.4 for sane altivec support"
#endif
#include <altivec.h>
int main () {
    vector unsigned int v = vec_splat_u32 (1);
    v = vec_sub (v, v);
    return 0;
}]])], have_vmx_intrinsics=yes)
CFLAGS=$xserver_save_CFLAGS

AC_ARG_ENABLE(vmx,
   [AC_HELP_STRING([--disable-vmx],
                   [disable VMX fast paths])],
   [enable_vmx=$enableval], [enable_vmx=auto])

if test $enable_vmx = no ; then
   have_vmx_intrinsics=disabled
fi

if test $have_vmx_intrinsics = yes ; then
   AC_DEFINE(USE_VMX, 1, [use VMX compiler intrinsics])
else
   VMX_CFLAGS=
fi

AC_MSG_RESULT($have_vmx_intrinsics)
if test $enable_vmx = yes && test $have_vmx_intrinsics = no ; then
   AC_MSG_ERROR([VMX intrinsics not detected])
fi

AC_SUBST(VMX_CFLAGS)

AM_CONDITIONAL(USE_VMX, test $have_vmx_intrinsics = yes)

dnl ==========================================================================
dnl Check if assembler is gas compatible and supports ARM SIMD instructions
have_arm_simd=no
AC_MSG_CHECKING(whether to use ARM SIMD assembler)
xserver_save_CFLAGS=$CFLAGS
CFLAGS="-x assembler-with-cpp $CFLAGS"
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
.text
.arch armv6
.object_arch armv4
.arm
.altmacro
#ifndef __ARM_EABI__
#error EABI is required (to be sure that calling conventions are compatible)
#endif
pld [r0]
uqadd8 r0, r0, r0]])], have_arm_simd=yes)
CFLAGS=$xserver_save_CFLAGS

AC_ARG_ENABLE(arm-simd,
   [AC_HELP_STRING([--disable-arm-simd],
                   [disable ARM SIMD fast paths])],
   [enable_arm_simd=$enableval], [enable_arm_simd=auto])

if test $enable_arm_simd = no ; then
   have_arm_simd=disabled
fi

if test $have_arm_simd = yes ; then
   AC_DEFINE(USE_ARM_SIMD, 1, [use ARM SIMD assembly optimizations])
fi

AM_CONDITIONAL(USE_ARM_SIMD, test $have_arm_simd = yes)

AC_MSG_RESULT($have_arm_simd)
if test $enable_arm_simd = yes && test $have_arm_simd = no ; then
   AC_MSG_ERROR([ARM SIMD intrinsics not detected])
fi

dnl ==========================================================================
dnl Check if assembler is gas compatible and supports NEON instructions
have_arm_neon=no
AC_MSG_CHECKING(whether to use ARM NEON assembler)
xserver_save_CFLAGS=$CFLAGS
CFLAGS="-x assembler-with-cpp $CFLAGS"
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
.text
.fpu neon
.arch armv7a
.object_arch armv4
.eabi_attribute 10, 0
.arm
.altmacro
#ifndef __ARM_EABI__
#error EABI is required (to be sure that calling conventions are compatible)
#endif
pld [r0]
vmovn.u16 d0, q0]])], have_arm_neon=yes)
CFLAGS=$xserver_save_CFLAGS

AC_ARG_ENABLE(arm-neon,
   [AC_HELP_STRING([--disable-arm-neon],
                   [disable ARM NEON fast paths])],
   [enable_arm_neon=$enableval], [enable_arm_neon=auto])

if test $enable_arm_neon = no ; then
   have_arm_neon=disabled
fi

if test $have_arm_neon = yes ; then
   AC_DEFINE(USE_ARM_NEON, 1, [use ARM NEON assembly optimizations])
fi

AM_CONDITIONAL(USE_ARM_NEON, test $have_arm_neon = yes)

AC_MSG_RESULT($have_arm_neon)
if test $enable_arm_neon = yes && test $have_arm_neon = no ; then
   AC_MSG_ERROR([ARM NEON intrinsics not detected])
fi

dnl ==========================================================================
dnl Check if assembler is gas compatible and supports ARM-a64 NEON instructions
have_arm_a64_neon=no
AC_MSG_CHECKING(whether to use ARM A64 NEON assembler)
xserver_save_CFLAGS=$CFLAGS
CFLAGS="-x assembler-with-cpp $CFLAGS"
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
.text
.arch armv8-a
.altmacro
prfm pldl2strm, [x0]
xtn v0.8b, v0.8h]])], have_arm_a64_neon=yes)
CFLAGS=$xserver_save_CFLAGS

AC_ARG_ENABLE(arm-a64-neon,
   [AC_HELP_STRING([--disable-arm-a64-neon],
                   [disable ARM A64 NEON fast paths])],
   [enable_arm_a64_neon=$enableval], [enable_arm_a64_neon=auto])

if test $enable_arm_a64_neon = no ; then
   have_arm_a64_neon=disabled
fi

if test $have_arm_a64_neon = yes ; then
   AC_DEFINE(USE_ARM_A64_NEON, 1, [use ARM A64_NEON assembly optimizations])
fi

AM_CONDITIONAL(USE_ARM_A64_NEON, test $have_arm_a64_neon = yes)

AC_MSG_RESULT($have_arm_a64_neon)
if test $enable_arm_a64_neon = yes && test $have_arm_a64_neon4 = no ; then
   AC_MSG_ERROR([ARM A64 NEON intrinsics not detected])
fi

dnl ===========================================================================
dnl Check for IWMMXT

AC_ARG_ENABLE(arm-iwmmxt,
   [AC_HELP_STRING([--disable-arm-iwmmxt],
                   [disable ARM IWMMXT fast paths])],
   [enable_iwmmxt=$enableval], [enable_iwmmxt=auto])

AC_ARG_ENABLE(arm-iwmmxt2,
   [AC_HELP_STRING([--disable-arm-iwmmxt2],
                   [build ARM IWMMXT fast paths with -march=iwmmxt instead of -march=iwmmxt2])],
   [enable_iwmmxt2=$enableval], [enable_iwmmxt2=auto])

if test "x$IWMMXT_CFLAGS" = "x" ; then
   IWMMXT_CFLAGS="-flax-vector-conversions -Winline -march=iwmmxt"
   if test $enable_iwmmxt2 != no ; then
      IWMMXT_CFLAGS="${IWMMXT_CFLAGS}2"
   fi
fi

have_iwmmxt_intrinsics=no
AC_MSG_CHECKING(whether to use ARM IWMMXT intrinsics)
xserver_save_CFLAGS=$CFLAGS
CFLAGS="$CFLAGS $IWMMXT_CFLAGS"
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#ifndef __arm__
#error "IWMMXT is only available on ARM"
#endif
#ifndef __IWMMXT__
#error "IWMMXT not enabled (with -march=iwmmxt)"
#endif
#if defined(__GNUC__) && (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 8))
#error "Need GCC >= 4.8 for IWMMXT intrinsics"
#endif
#include <mmintrin.h>
int main () {
	union {
		__m64 v;
		char c[8];
	} a = { .c = {1, 2, 3, 4, 5, 6, 7, 8} };
	int b = 4;
	__m64 c = _mm_srli_si64 (a.v, b);
}]])], have_iwmmxt_intrinsics=yes)
CFLAGS=$xserver_save_CFLAGS

if test $enable_iwmmxt = no ; then
   have_iwmmxt_intrinsics=disabled
fi

if test $have_iwmmxt_intrinsics = yes ; then
   AC_DEFINE(USE_ARM_IWMMXT, 1, [use ARM IWMMXT compiler intrinsics])
else
   IWMMXT_CFLAGS=
fi

AC_MSG_RESULT($have_iwmmxt_intrinsics)
if test $enable_iwmmxt = yes && test $have_iwmmxt_intrinsics = no ; then
   AC_MSG_ERROR([IWMMXT intrinsics not detected])
fi

AM_CONDITIONAL(USE_ARM_IWMMXT, test $have_iwmmxt_intrinsics = yes)

dnl ==========================================================================
dnl Check if assembler is gas compatible and supports MIPS DSPr2 instructions

have_mips_dspr2=no
AC_MSG_CHECKING(whether to use MIPS DSPr2 assembler)
xserver_save_CFLAGS=$CFLAGS
CFLAGS="-mdspr2 $CFLAGS"

AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#if !(defined(__mips__) &&  __mips_isa_rev >= 2)
#error MIPS DSPr2 is currently only available on MIPS32r2 platforms.
#endif
int
main ()
{
    int c = 0, a = 0, b = 0;
    __asm__ __volatile__ (
        "precr.qb.ph %[c], %[a], %[b]          \n\t"
        : [c] "=r" (c)
        : [a] "r" (a), [b] "r" (b)
    );
    return c;
}]])], have_mips_dspr2=yes)
CFLAGS=$xserver_save_CFLAGS

AC_ARG_ENABLE(mips-dspr2,
   [AC_HELP_STRING([--disable-mips-dspr2],
                   [disable MIPS DSPr2 fast paths])],
   [enable_mips_dspr2=$enableval], [enable_mips_dspr2=auto])

if test $enable_mips_dspr2 = no ; then
   have_mips_dspr2=disabled
fi

if test $have_mips_dspr2 = yes ; then
   AC_DEFINE(USE_MIPS_DSPR2, 1, [use MIPS DSPr2 assembly optimizations])
fi

AM_CONDITIONAL(USE_MIPS_DSPR2, test $have_mips_dspr2 = yes)

AC_MSG_RESULT($have_mips_dspr2)
if test $enable_mips_dspr2 = yes && test $have_mips_dspr2 = no ; then
   AC_MSG_ERROR([MIPS DSPr2 instructions not detected])
fi

dnl =========================================================================================
dnl Check for GNU-style inline assembly support

have_gcc_inline_asm=no
AC_MSG_CHECKING(whether to use GNU-style inline assembler)
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
int main () {
    /* Most modern architectures have a NOP instruction, so this is a fairly generic test. */
	asm volatile ( "\tnop\n" : : : "cc", "memory" );
    return 0;
}]])], have_gcc_inline_asm=yes)

AC_ARG_ENABLE(gcc-inline-asm,
   [AC_HELP_STRING([--disable-gcc-inline-asm],
                   [disable GNU-style inline assembler])],
   [enable_gcc_inline_asm=$enableval], [enable_gcc_inline_asm=auto])

if test $enable_gcc_inline_asm = no ; then
   have_gcc_inline_asm=disabled
fi

if test $have_gcc_inline_asm = yes ; then
   AC_DEFINE(USE_GCC_INLINE_ASM, 1, [use GNU-style inline assembler])
fi

AC_MSG_RESULT($have_gcc_inline_asm)
if test $enable_gcc_inline_asm = yes && test $have_gcc_inline_asm = no ; then
   AC_MSG_ERROR([GNU-style inline assembler not detected])
fi

AM_CONDITIONAL(USE_GCC_INLINE_ASM, test $have_gcc_inline_asm = yes)

dnl ==============================================
dnl Static test programs

AC_ARG_ENABLE(static-testprogs,
   [AC_HELP_STRING([--enable-static-testprogs],
		   [build test programs as static binaries [default=no]])],
   [enable_static_testprogs=$enableval], [enable_static_testprogs=no])

TESTPROGS_EXTRA_LDFLAGS=
if test "x$enable_static_testprogs" = "xyes" ; then
   TESTPROGS_EXTRA_LDFLAGS="-all-static"
fi
AC_SUBST(TESTPROGS_EXTRA_LDFLAGS)

dnl ==============================================
dnl Timers

AC_ARG_ENABLE(timers,
   [AC_HELP_STRING([--enable-timers],
		   [enable TIMER_BEGIN and TIMER_END macros [default=no]])],
   [enable_timers=$enableval], [enable_timers=no])

if test $enable_timers = yes ; then 
   AC_DEFINE(PIXMAN_TIMERS, 1, [enable TIMER_BEGIN/TIMER_END macros])
fi
AC_SUBST(PIXMAN_TIMERS)

dnl ===================================
dnl gnuplot

AC_ARG_ENABLE(gnuplot,
   [AC_HELP_STRING([--enable-gnuplot],
                   [enable output of filters that can be piped to gnuplot [default=no]])],
   [enable_gnuplot=$enableval], [enable_gnuplot=no])

if test $enable_gnuplot = yes ; then
   AC_DEFINE(PIXMAN_GNUPLOT, 1, [enable output that can be piped to gnuplot])
fi
AC_SUBST(PIXMAN_GNUPLOT)

dnl ===================================
dnl GTK+

AC_ARG_ENABLE(gtk,
   [AC_HELP_STRING([--enable-gtk],
                   [enable tests using GTK+ [default=auto]])],
   [enable_gtk=$enableval], [enable_gtk=auto])

PKG_PROG_PKG_CONFIG

if test $enable_gtk = yes ; then
   AC_CHECK_LIB([pixman-1], [pixman_version_string])
   PKG_CHECK_MODULES(GTK, [gtk+-3.0 pixman-1])
fi

if test $enable_gtk = auto ; then
   AC_CHECK_LIB([pixman-1], [pixman_version_string], [enable_gtk=auto], [enable_gtk=no])
fi

if test $enable_gtk = auto ; then
   PKG_CHECK_MODULES(GTK, [gtk+-3.0 pixman-1], [enable_gtk=yes], [enable_gtk=no])
fi

AM_CONDITIONAL(HAVE_GTK, [test "x$enable_gtk" = xyes])

AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

dnl =====================================
dnl posix_memalign, sigaction, alarm, gettimeofday

AC_CHECK_FUNC(posix_memalign, have_posix_memalign=yes, have_posix_memalign=no)
if test x$have_posix_memalign = xyes; then
   AC_DEFINE(HAVE_POSIX_MEMALIGN, 1, [Whether we have posix_memalign()])
fi

AC_CHECK_FUNC(sigaction, have_sigaction=yes, have_sigaction=no)
if test x$have_sigaction = xyes; then
   AC_DEFINE(HAVE_SIGACTION, 1, [Whether we have sigaction()])
fi

AC_CHECK_FUNC(alarm, have_alarm=yes, have_alarm=no)
if test x$have_alarm = xyes; then
   AC_DEFINE(HAVE_ALARM, 1, [Whether we have alarm()])
fi

AC_CHECK_HEADER([sys/mman.h],
   [AC_DEFINE(HAVE_SYS_MMAN_H, [1], [Define to 1 if we have <sys/mman.h>])])

AC_CHECK_FUNC(mmap, have_mmap=yes, have_mmap=no)
if test x$have_mmap = xyes; then
   AC_DEFINE(HAVE_MMAP, 1, [Whether we have mmap()])
fi

AC_CHECK_FUNC(mprotect, have_mprotect=yes, have_mprotect=no)
if test x$have_mprotect = xyes; then
   AC_DEFINE(HAVE_MPROTECT, 1, [Whether we have mprotect()])
fi

AC_CHECK_FUNC(getpagesize, have_getpagesize=yes, have_getpagesize=no)
if test x$have_getpagesize = xyes; then
   AC_DEFINE(HAVE_GETPAGESIZE, 1, [Whether we have getpagesize()])
fi

AC_CHECK_HEADER([fenv.h],
   [AC_DEFINE(HAVE_FENV_H, [1], [Define to 1 if we have <fenv.h>])])

AC_CHECK_LIB(m, feenableexcept, have_feenableexcept=yes, have_feenableexcept=no)
if test x$have_feenableexcept = xyes; then
   AC_DEFINE(HAVE_FEENABLEEXCEPT, 1, [Whether we have feenableexcept()])
fi

AC_CHECK_DECL([FE_DIVBYZERO],
	[AC_DEFINE(HAVE_FEDIVBYZERO, 1, [Whether we have FE_DIVBYZERO])],
	[],
	[[#include <fenv.h>]])

AC_CHECK_FUNC(gettimeofday, have_gettimeofday=yes, have_gettimeofday=no)
AC_CHECK_HEADER(sys/time.h, have_sys_time_h=yes, have_sys_time_h=no)
if test x$have_gettimeofday = xyes && test x$have_sys_time_h = xyes; then
   AC_DEFINE(HAVE_GETTIMEOFDAY, 1, [Whether we have gettimeofday()])
fi

dnl =====================================
dnl Check for missing sqrtf() as, e.g., for Solaris 9

AC_SEARCH_LIBS([sqrtf], [m], [],
               [AC_DEFINE([sqrtf], [sqrt],
                          [Define to sqrt if you do not have the `sqrtf' function.])])

dnl =====================================
dnl Thread local storage

AC_MSG_CHECKING(for thread local storage (TLS) support)
AC_CACHE_VAL(ac_cv_tls, [
    ac_cv_tls=none
    keywords="__thread __declspec(thread)"
    for kw in $keywords ; do
        AC_TRY_COMPILE([
#if defined(__MINGW32__) && !(__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5))
#error This MinGW version has broken __thread support
#endif
#ifdef __OpenBSD__
#error OpenBSD has broken __thread support
#endif

int $kw test;], [], [ac_cv_tls=$kw; break])
    done
])
AC_MSG_RESULT($ac_cv_tls)

if test "$ac_cv_tls" != "none"; then
    AC_DEFINE_UNQUOTED([TLS], $ac_cv_tls, [The compiler supported TLS storage class])
fi

dnl
dnl posix tls
dnl

m4_define([pthread_test_program],AC_LANG_SOURCE([[dnl
#include <stdlib.h>
#include <pthread.h>

static pthread_once_t once_control = PTHREAD_ONCE_INIT;
static pthread_key_t key;

static void
make_key (void)
{
    pthread_key_create (&key, NULL);
}

int
main ()
{
    void *value = NULL;

    if (pthread_once (&once_control, make_key) != 0)
    {
	value = NULL;
    }
    else
    {
	value = pthread_getspecific (key);
	if (!value)
	{
	    value = malloc (100);
	    pthread_setspecific (key, value);
	}
    }
    return 0;
}
]]))

AC_DEFUN([PIXMAN_CHECK_PTHREAD],[dnl
    if test "z$support_for_pthreads" != "zyes"; then
	PIXMAN_LINK_WITH_ENV(
		[$1], [pthread_test_program],
		[PTHREAD_CFLAGS="$CFLAGS"
		 PTHREAD_LIBS="$LIBS"
		 PTHREAD_LDFLAGS="$LDFLAGS"
		 support_for_pthreads=yes])
    fi
])

support_for_pthreads=no

AC_MSG_CHECKING(for pthreads)

PIXMAN_CHECK_PTHREAD([CFLAGS="-pthread"; LDFLAGS="-pthread"])
PIXMAN_CHECK_PTHREAD([CFLAGS="-D_REENTRANT"; LIBS="-lpthread"])
PIXMAN_CHECK_PTHREAD([CFLAGS="-D_REENTRANT"; LDFLAGS="-lroot"])
    
if test $support_for_pthreads = yes; then
    AC_DEFINE([HAVE_PTHREADS], [], [Whether pthreads is supported])
    if test $ac_cv_tls = none ; then
        CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
    fi
fi

AC_MSG_RESULT($support_for_pthreads)

AC_SUBST(TOOLCHAIN_SUPPORTS__THREAD)
AC_SUBST(HAVE_PTHREADS)
AC_SUBST(PTHREAD_LDFLAGS)
AC_SUBST(PTHREAD_LIBS)
AC_SUBST(PTHREAD_CFLAGS)

dnl =====================================
dnl __attribute__((constructor))

support_for_attribute_constructor=no

AC_MSG_CHECKING(for __attribute__((constructor)))
AC_LINK_IFELSE([AC_LANG_SOURCE([[
#if defined(__GNUC__) && (__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7))
/* attribute 'constructor' is supported since gcc 2.7, but some compilers
 * may only pretend to be gcc, so let's try to actually use it
 */
static int x = 1;
static void __attribute__((constructor)) constructor_function () { x = 0; }
int main (void) { return x; }
#else
#error not gcc or gcc version is older than 2.7
#endif
]])], support_for_attribute_constructor=yes)

if test x$support_for_attribute_constructor = xyes; then
   AC_DEFINE([TOOLCHAIN_SUPPORTS_ATTRIBUTE_CONSTRUCTOR],
             [],[Whether the tool chain supports __attribute__((constructor))])
fi

AC_MSG_RESULT($support_for_attribute_constructor)
AC_SUBST(TOOLCHAIN_SUPPORTS_ATTRIBUTE_CONSTRUCTOR)

dnl =====================================
dnl __float128

support_for_float128=no

AC_MSG_CHECKING(for __float128)
AC_LINK_IFELSE([AC_LANG_SOURCE([[
__float128 a = 1.0Q, b = 2.0Q; int main (void) { return a + b; }
]])], support_for_float128=yes)

if test x$support_for_float128 = xyes; then
   AC_DEFINE([HAVE_FLOAT128], [], [Whether the tool chain supports __float128])
fi

AC_MSG_RESULT($support_for_float128)

dnl =====================================
dnl __builtin_clz

support_for_builtin_clz=no

AC_MSG_CHECKING(for __builtin_clz)
AC_LINK_IFELSE([AC_LANG_SOURCE([[
unsigned int x = 11; int main (void) { return __builtin_clz(x); }
]])], support_for_builtin_clz=yes)

if test x$support_for_builtin_clz = xyes; then
   AC_DEFINE([HAVE_BUILTIN_CLZ], [], [Whether the compiler supports __builtin_clz])
fi

AC_MSG_RESULT($support_for_builtin_clz)

dnl =====================================
dnl GCC vector extensions

support_for_gcc_vector_extensions=no

AC_MSG_CHECKING(for GCC vector extensions)
AC_LINK_IFELSE([AC_LANG_SOURCE([[
unsigned int __attribute__ ((vector_size(16))) e, a, b;
int main (void) { e = a - ((b << 27) + (b >> (32 - 27))) + 1; return e[0]; }
]])], support_for_gcc_vector_extensions=yes)

if test x$support_for_gcc_vector_extensions = xyes; then
   AC_DEFINE([HAVE_GCC_VECTOR_EXTENSIONS], [],
             [Whether the compiler supports GCC vector extensions])
fi

AC_MSG_RESULT($support_for_gcc_vector_extensions)

dnl ==================
dnl libpng

AC_ARG_ENABLE(libpng, AS_HELP_STRING([--enable-libpng], [Build support for libpng (default: auto)]),
                      [have_libpng=$enableval], [have_libpng=auto])

case x$have_libpng in
	xyes) PKG_CHECK_MODULES(PNG, [libpng]) ;;
	xno) ;;
	*) PKG_CHECK_MODULES(PNG, [libpng], have_libpng=yes, have_libpng=no) ;;
esac

if test x$have_libpng = xyes; then
    AC_DEFINE([HAVE_LIBPNG], [1], [Whether we have libpng])
fi

AC_SUBST(HAVE_LIBPNG)

AC_OUTPUT([pixman-1.pc
           pixman-1-uninstalled.pc
           Makefile
	   pixman/Makefile
	   pixman/pixman-version.h
	   demos/Makefile
	   test/Makefile])

m4_if(m4_eval(pixman_minor % 2), [1], [
   echo
   echo "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"
   echo
   echo "      Thanks for testing this development snapshot of pixman. Please"
   echo "      report any problems you find, either by sending email to "
   echo
   echo "          pixman@lists.freedesktop.org"
   echo
   echo "      or by filing a bug at "
   echo
   echo "          https://gitlab.freedesktop.org/pixman/pixman/-/issues/new "
   echo
   echo "      If you are looking for a stable release of pixman, please note "
   echo "      that stable releases have _even_ minor version numbers. Ie., "
   echo "      pixman-0.]m4_eval(pixman_minor & ~1)[.x are stable releases, whereas pixman-$PIXMAN_VERSION_MAJOR.$PIXMAN_VERSION_MINOR.$PIXMAN_VERSION_MICRO is a "
   echo "      development snapshot that may contain bugs and experimental "
   echo "      features. "
   echo 
   echo "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"
   echo
])
