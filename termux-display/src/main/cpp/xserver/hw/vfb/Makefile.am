SUBDIRS = man

bin_PROGRAMS = Xvfb

AM_CFLAGS = -DHAVE_DIX_CONFIG_H \
            $(XVFBMODULES_CFLAGS) \
	    $(DIX_CFLAGS)

SRCS =	InitInput.c \
	InitOutput.c \
	$(top_srcdir)/mi/miinitext.c \
	$(top_srcdir)/mi/miinitext.h

Xvfb_SOURCES = $(SRCS)

XVFB_LIBS = \
        @XVFB_LIBS@ \
	$(MAIN_LIB) \
	$(XSERVER_LIBS) \
	$(top_builddir)/Xi/libXistubs.la

Xvfb_LDADD = $(XVFB_LIBS) $(XVFB_SYS_LIBS) $(XSERVER_SYS_LIBS)
Xvfb_DEPENDENCIES = $(XVFB_LIBS)
Xvfb_LDFLAGS = $(LD_EXPORT_SYMBOLS_FLAG)

relink:
	$(AM_V_at)rm -f Xvfb$(EXEEXT) && $(MAKE) Xvfb$(EXEEXT)
