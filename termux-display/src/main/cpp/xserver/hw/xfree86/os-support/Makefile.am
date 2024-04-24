SUBDIRS = bus @XORG_OS_SUBDIR@ misc $(DRI_SUBDIRS)
DIST_SUBDIRS = bsd bus misc linux solaris stub hurd

sdk_HEADERS = xf86_OSproc.h xf86_OSlib.h

EXTRA_DIST = int10Defines.h xf86OSpriv.h

# to get the grouping semantics right, you have to glom these three together
# as one library, otherwise libtool will actively defeat your attempts to
# list them multiple times on the link line.
noinst_LTLIBRARIES = libxorgos.la
libxorgos_la_SOURCES =
libxorgos_la_LIBADD = @XORG_OS_SUBDIR@/lib@XORG_OS_SUBDIR@.la \
                      bus/libbus.la \
                      misc/libmisc.la

AM_CFLAGS = $(DIX_CFLAGS) 

