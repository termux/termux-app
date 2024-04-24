AC_INIT(libtirpc, 1.2.6)
AM_INIT_AUTOMAKE([silent-rules])
AM_SILENT_RULES([yes])
AC_CONFIG_SRCDIR([src/auth_des.c])
AC_CONFIG_MACRO_DIR([m4])
AC_PROG_CC

# LT_VERSION_INFO="current:revision:age"
#
# From the libtool manual:
#
# 1.  Start with version information of 0:0:0 for each libtool library.
# 2.  Update the version information only immediately before a public
#     release of your software. More frequent updates are unnecessary,
#     and only guarantee that the current interface number gets larger faster.
# 3.  If the library source code has changed at all since the last update,
#     then increment revision (c:r:a becomes c:r+1:a).
# 4.  If any interfaces have been added, removed, or changed since the last
#     update, increment current, and set revision to 0.
# 5.  If any interfaces have been added since the last public release,
#     then increment age.
# 6.  If any interfaces have been removed since the last public release,
#     then set age to 0.
#
# _Never_ try to set the interface numbers so that they correspond to the
# release number of your package. This is an abuse that only fosters
# misunderstanding of the purpose of library versions.
#
# In addition to these rules, symbol versioning is now in effect. soname
# changes should be avoided.
#
LT_VERSION_INFO="3:0:0"
AC_SUBST([LT_VERSION_INFO])

AC_CHECK_HEADER([gssapi/gssapi.h], [HAVE_GSSAPI_H=yes], [HAVE_GSSAPI_H=no])

AC_ARG_ENABLE(gssapi,
	[AC_HELP_STRING([--disable-gssapi], [Disable GSSAPI support @<:@default=no@:>@])],
      [],[enable_gssapi=yes])
AM_CONDITIONAL(GSS, test "x$enable_gssapi" = xyes)

if test "x$enable_gssapi" = xyes; then
	if test "x$HAVE_GSSAPI_H" = xno; then
		AC_MSG_ERROR([gssapi.h not found. Use --disable-gssapi, or install GSS-API.])
	fi
	AC_CHECK_TOOL([KRB5_CONFIG], [krb5-config], [no])
	if test "x$KRB5_CONFIG" = xno; then
		AC_MSG_ERROR([krb5-config tool not found. Use --disable-gssapi, or install Kerberos.])
	fi
	GSSAPI_CFLAGS=`${KRB5_CONFIG} --cflags gssapi`
	GSSAPI_LIBS=`${KRB5_CONFIG} --libs gssapi`
	AC_SUBST([GSSAPI_CFLAGS])
	AC_SUBST([GSSAPI_LIBS])
fi

AC_ARG_ENABLE(authdes,
	[AC_HELP_STRING([--enable-authdes], [Enable AUTH_DES support @<:@default=no@:>@])],
      [],[enable_authdes=no])
AM_CONDITIONAL(AUTHDES, test "x$enable_authdes" = xyes)
if test "x$enable_authdes" != xno; then
	AC_DEFINE(AUTHDES_SUPPORT, 1, [Define if AUTH_DES is support])
fi

AC_ARG_ENABLE(ipv6,
	[AC_HELP_STRING([--disable-ipv6], [Disable IPv6 support @<:@default=no@:>@])],
	[],[enable_ipv6=yes])
AM_CONDITIONAL(INET6, test "x$disable_ipv6" != xno)
if test "x$enable_ipv6" != xno; then
	AC_DEFINE(INET6, 1, [Define to 1 if IPv6 is available])
fi

AC_ARG_ENABLE(symvers,
	[AC_HELP_STRING([--disable-symvers], [Disable symbol versioning @<:@default=no@:>@])],
      [],[enable_symvers=yes])
AM_CONDITIONAL(SYMVERS, test "x$enable_symvers" = xyes)

AC_CANONICAL_BUILD
# Check for which host we are on and setup a few things
# specifically based on the host
case $build_os in
     linux*)
         # Do something specific for linux
         LDFLAG_NOUNDEFINED="-Wl,--no-undefined"
         AC_SUBST(LDFLAG_NOUNDEFINED)
         ;;
     *)
         #Default Case
         ;;
esac


AC_CONFIG_HEADERS([config.h])
AC_PROG_LIBTOOL
AC_HEADER_DIRENT
AC_PREFIX_DEFAULT(/usr)
AC_CHECK_HEADERS([arpa/inet.h fcntl.h libintl.h limits.h locale.h netdb.h netinet/in.h stddef.h stdint.h stdlib.h string.h sys/ioctl.h sys/param.h sys/socket.h sys/time.h syslog.h unistd.h features.h gssapi/gssapi_ext.h])
AC_CHECK_LIB([pthread], [pthread_create])
AC_CHECK_FUNCS([getrpcbyname getrpcbynumber setrpcent endrpcent getrpcent])

AC_CONFIG_FILES([Makefile src/Makefile man/Makefile doc/Makefile])
AC_OUTPUT(libtirpc.pc)


