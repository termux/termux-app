if DRI2
GLXDRI_LIBRARY = libglxdri.la
endif

noinst_LTLIBRARIES = libglx.la $(GLXDRI_LIBRARY) libglxvnd.la

AM_CFLAGS = \
	@DIX_CFLAGS@ \
	@GL_CFLAGS@ \
	@XLIB_CFLAGS@ \
	@LIBDRM_CFLAGS@ \
	@GLX_DEFINES@ \
	@GLX_ARCH_DEFINES@

sdk_HEADERS = vndserver.h

AM_CPPFLAGS = \
	-I$(top_srcdir)/hw/xfree86/os-support \
	-I$(top_srcdir)/hw/xfree86/os-support/bus \
	-I$(top_srcdir)/hw/xfree86/common \
	-I$(top_srcdir)/hw/xfree86/dri \
	-I$(top_srcdir)/hw/xfree86/dri2 \
	-I$(top_srcdir)/mi \
	-I$(top_srcdir)/present

indirect_sources =				\
	indirect_dispatch.c			\
	indirect_dispatch.h			\
	indirect_dispatch_swap.c		\
	indirect_reqsize.c			\
	indirect_reqsize.h			\
	indirect_size.h				\
	indirect_size_get.c			\
	indirect_size_get.h			\
	indirect_table.c

libglxdri_la_SOURCES =
if DRI2
libglxdri_la_SOURCES += glxdri2.c
endif

libglxdri_la_LIBADD = $(DLOPEN_LIBS)

libglx_la_SOURCES = \
	$(indirect_sources) \
	clientinfo.c \
	createcontext.c \
	extension_string.c \
	extension_string.h \
	indirect_util.c \
	indirect_util.h \
	indirect_program.c \
	indirect_table.h \
	indirect_texture_compression.c \
        glxbyteorder.h \
        glxcmds.c \
        glxcmdsswap.c \
        glxcontext.h \
        glxdrawable.h \
        glxext.c \
        glxext.h \
	glxdriswrast.c \
	glxdricommon.c \
	glxdricommon.h \
        glxscreens.c \
        glxscreens.h \
        glxserver.h \
        glxutil.h \
        render2.c \
        render2swap.c \
        renderpix.c \
        renderpixswap.c \
        rensize.c \
        single2.c \
        single2swap.c \
        singlepix.c \
        singlepixswap.c \
        singlesize.c \
        singlesize.h \
        swap_interval.c \
        unpack.h \
        xfont.c

libglx_la_LIBADD = $(DLOPEN_LIBS) $(top_builddir)/Xext/libhashtable.la

libglxvnd_la_SOURCES = \
	vndcmds.c \
	vndext.c \
	vndservermapping.c \
	vndservervendor.h \
	vndservervendor.c

libglxvnd_la_LIBADD = $(top_builddir)/Xext/libhashtable.la

EXTRA_DIST = vnd_dispatch_stubs.c
