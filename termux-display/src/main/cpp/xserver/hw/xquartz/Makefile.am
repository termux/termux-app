noinst_LTLIBRARIES = libXquartz.la
AM_CFLAGS = $(DIX_CFLAGS)
AM_OBJCFLAGS = $(DIX_CFLAGS)
AM_CPPFLAGS = \
	-DXQUARTZ_DATA_DIR=\"$(datadir)/X11/XQuartz\" \
	-DXSERVER_VERSION=\"$(VERSION)\" \
	-DBUNDLE_ID_PREFIX=\"$(BUNDLE_ID_PREFIX)\" \
	-DUSE_NEW_CLUT \
	-DXFree86Server \
	-I$(top_srcdir)/miext/rootless \
	-I$(top_builddir)/pseudoramiX \
	-I$(top_builddir)

if GLX
GL_DIR = GL
endif

SUBDIRS = bundle . $(GL_DIR) xpr pbproxy mach-startup man

DIST_SUBDIRS = bundle . GL xpr pbproxy mach-startup man

libXquartz_la_SOURCES = \
	$(top_srcdir)/mi/miinitext.c \
	$(top_srcdir)/mi/miinitext.h \
	NSUserDefaults+XQuartzDefaults.m \
	X11Application.m \
	X11Controller.m \
	applewm.c \
	darwin.c \
	darwinEvents.c \
	darwinXinput.c \
	keysym2ucs.c \
	quartz.c \
	quartzKeyboard.c \
	quartzStartup.c \
	quartzRandR.c

libXquartz_la_LIBADD = $(top_builddir)/pseudoramiX/libPseudoramiX.la

EXTRA_DIST = \
	NSUserDefaults+XQuartzDefaults.h \
	X11Application.h \
	X11Controller.h \
	applewmExt.h \
	darwin.h \
	darwinfb.h \
	darwinEvents.h \
	defaults.plist \
	keysym2ucs.h \
	quartz.h \
	quartzKeyboard.h \
	quartzRandR.h \
	sanitizedCarbon.h \
	sanitizedCocoa.h
