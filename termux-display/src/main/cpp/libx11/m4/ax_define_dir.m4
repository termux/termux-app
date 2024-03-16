# ===========================================================================
#       http://www.gnu.org/software/autoconf-archive/ax_define_dir.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_DEFINE_DIR(VARNAME, DIR [, DESCRIPTION])
#
# DESCRIPTION
#
#   This macro sets VARNAME to the expansion of the DIR variable, taking
#   care of fixing up ${prefix} and such.
#
#   VARNAME is then offered as both an output variable and a C preprocessor
#   symbol.
#
#   Example:
#
#     AX_DEFINE_DIR([DATADIR], [datadir], [Where data are placed to.])
#
# LICENSE
#
#   Copyright (c) 2008 Stepan Kasal <kasal@ucw.cz>
#   Copyright (c) 2008 Andreas Schwab <schwab@suse.de>
#   Copyright (c) 2008 Guido U. Draheim <guidod@gmx.de>
#   Copyright (c) 2008 Alexandre Oliva
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved. This file is offered as-is, without any
#   warranty.

#serial 6

AU_ALIAS([AC_DEFINE_DIR], [AX_DEFINE_DIR])
AC_DEFUN([AX_DEFINE_DIR], [
  prefix_NONE=
  exec_prefix_NONE=
  test "x$prefix" = xNONE && prefix_NONE=yes && prefix=$ac_default_prefix
  test "x$exec_prefix" = xNONE && exec_prefix_NONE=yes && exec_prefix=$prefix
dnl In Autoconf 2.60, ${datadir} refers to ${datarootdir}, which in turn
dnl refers to ${prefix}.  Thus we have to use `eval' twice.
  eval ax_define_dir="\"[$]$2\""
  eval ax_define_dir="\"$ax_define_dir\""
  AC_SUBST($1, "$ax_define_dir")
  AC_DEFINE_UNQUOTED($1, "$ax_define_dir", [$3])
  test "$prefix_NONE" && prefix=NONE
  test "$exec_prefix_NONE" && exec_prefix=NONE
])
