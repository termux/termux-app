# ===========================================================================
#      https://www.gnu.org/software/autoconf-archive/ax_gcc_builtin.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_GCC_BUILTIN(BUILTIN)
#
# DESCRIPTION
#
#   This macro checks if the compiler supports one of GCC's built-in
#   functions; many other compilers also provide those same built-ins.
#
#   The BUILTIN parameter is the name of the built-in function.
#
#   If BUILTIN is supported define HAVE_<BUILTIN>. Keep in mind that since
#   builtins usually start with two underscores they will be copied over
#   into the HAVE_<BUILTIN> definition (e.g. HAVE___BUILTIN_EXPECT for
#   __builtin_expect()).
#
#   The macro caches its result in the ax_cv_have_<BUILTIN> variable (e.g.
#   ax_cv_have___builtin_expect).
#
#   The macro currently supports the following built-in functions:
#
#    __builtin_assume_aligned
#    __builtin_bswap16
#    __builtin_bswap32
#    __builtin_bswap64
#    __builtin_choose_expr
#    __builtin___clear_cache
#    __builtin_clrsb
#    __builtin_clrsbl
#    __builtin_clrsbll
#    __builtin_clz
#    __builtin_clzl
#    __builtin_clzll
#    __builtin_complex
#    __builtin_constant_p
#    __builtin_cpu_init
#    __builtin_cpu_is
#    __builtin_cpu_supports
#    __builtin_ctz
#    __builtin_ctzl
#    __builtin_ctzll
#    __builtin_expect
#    __builtin_ffs
#    __builtin_ffsl
#    __builtin_ffsll
#    __builtin_fpclassify
#    __builtin_huge_val
#    __builtin_huge_valf
#    __builtin_huge_vall
#    __builtin_inf
#    __builtin_infd128
#    __builtin_infd32
#    __builtin_infd64
#    __builtin_inff
#    __builtin_infl
#    __builtin_isinf_sign
#    __builtin_nan
#    __builtin_nand128
#    __builtin_nand32
#    __builtin_nand64
#    __builtin_nanf
#    __builtin_nanl
#    __builtin_nans
#    __builtin_nansf
#    __builtin_nansl
#    __builtin_object_size
#    __builtin_parity
#    __builtin_parityl
#    __builtin_parityll
#    __builtin_popcount
#    __builtin_popcountl
#    __builtin_popcountll
#    __builtin_powi
#    __builtin_powif
#    __builtin_powil
#    __builtin_prefetch
#    __builtin_trap
#    __builtin_types_compatible_p
#    __builtin_unreachable
#
#   Unsupported built-ins will be tested with an empty parameter set and the
#   result of the check might be wrong or meaningless so use with care.
#
# LICENSE
#
#   Copyright (c) 2013 Gabriele Svelto <gabriele.svelto@gmail.com>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved.  This file is offered as-is, without any
#   warranty.

#serial 7

AC_DEFUN([AX_GCC_BUILTIN], [
    AS_VAR_PUSHDEF([ac_var], [ax_cv_have_$1])

    AC_CACHE_CHECK([for $1], [ac_var], [
        AC_LINK_IFELSE([AC_LANG_PROGRAM([], [
            m4_case([$1],
                [__builtin_assume_aligned], [$1("", 0)],
                [__builtin_bswap16], [$1(0)],
                [__builtin_bswap32], [$1(0)],
                [__builtin_bswap64], [$1(0)],
                [__builtin_choose_expr], [$1(0, 0, 0)],
                [__builtin___clear_cache], [$1("", "")],
                [__builtin_clrsb], [$1(0)],
                [__builtin_clrsbl], [$1(0)],
                [__builtin_clrsbll], [$1(0)],
                [__builtin_clz], [$1(0)],
                [__builtin_clzl], [$1(0)],
                [__builtin_clzll], [$1(0)],
                [__builtin_complex], [$1(0.0, 0.0)],
                [__builtin_constant_p], [$1(0)],
                [__builtin_cpu_init], [$1()],
                [__builtin_cpu_is], [$1("intel")],
                [__builtin_cpu_supports], [$1("sse")],
                [__builtin_ctz], [$1(0)],
                [__builtin_ctzl], [$1(0)],
                [__builtin_ctzll], [$1(0)],
                [__builtin_expect], [$1(0, 0)],
                [__builtin_ffs], [$1(0)],
                [__builtin_ffsl], [$1(0)],
                [__builtin_ffsll], [$1(0)],
                [__builtin_fpclassify], [$1(0, 1, 2, 3, 4, 0.0)],
                [__builtin_huge_val], [$1()],
                [__builtin_huge_valf], [$1()],
                [__builtin_huge_vall], [$1()],
                [__builtin_inf], [$1()],
                [__builtin_infd128], [$1()],
                [__builtin_infd32], [$1()],
                [__builtin_infd64], [$1()],
                [__builtin_inff], [$1()],
                [__builtin_infl], [$1()],
                [__builtin_isinf_sign], [$1(0.0)],
                [__builtin_nan], [$1("")],
                [__builtin_nand128], [$1("")],
                [__builtin_nand32], [$1("")],
                [__builtin_nand64], [$1("")],
                [__builtin_nanf], [$1("")],
                [__builtin_nanl], [$1("")],
                [__builtin_nans], [$1("")],
                [__builtin_nansf], [$1("")],
                [__builtin_nansl], [$1("")],
                [__builtin_object_size], [$1("", 0)],
                [__builtin_parity], [$1(0)],
                [__builtin_parityl], [$1(0)],
                [__builtin_parityll], [$1(0)],
                [__builtin_popcount], [$1(0)],
                [__builtin_popcountl], [$1(0)],
                [__builtin_popcountll], [$1(0)],
                [__builtin_powi], [$1(0, 0)],
                [__builtin_powif], [$1(0, 0)],
                [__builtin_powil], [$1(0, 0)],
                [__builtin_prefetch], [$1("")],
                [__builtin_trap], [$1()],
                [__builtin_types_compatible_p], [$1(int, int)],
                [__builtin_unreachable], [$1()],
                [m4_warn([syntax], [Unsupported built-in $1, the test may fail])
                 $1()]
            )
            ])],
            [AS_VAR_SET([ac_var], [yes])],
            [AS_VAR_SET([ac_var], [no])])
    ])

    AS_IF([test yes = AS_VAR_GET([ac_var])],
        [AC_DEFINE_UNQUOTED(AS_TR_CPP(HAVE_$1), 1,
            [Define to 1 if the system has the `$1' built-in function])], [])

    AS_VAR_POPDEF([ac_var])
])
