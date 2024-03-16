AM_CPPFLAGS = \
	-I$(srcdir)/.. \
	-I$(top_builddir) \
	-DXSERVER_VERSION=\"$(VERSION)\" \
	-DX11BINDIR=\"$(bindir)\"

AM_CFLAGS = $(DIX_CFLAGS)

x11appdir = $(APPLE_APPLICATIONS_DIR)/$(APPLE_APPLICATION_NAME).app/Contents/MacOS
x11app_PROGRAMS = X11.bin X11

dist_X11_bin_SOURCES = \
	bundle-main.c

dist_X11_SOURCES = \
	bundle_trampoline.c

nodist_X11_bin_SOURCES = \
	mach_startupServer.c \
	mach_startupUser.c

X11_bin_DEPENDENCIES = \
	$(top_builddir)/hw/xquartz/libXquartz.la \
	$(top_builddir)/hw/xquartz/xpr/libXquartzXpr.la \
	$(top_builddir)/dix/dixfonts.lo \
	$(top_builddir)/miext/rootless/librootless.la \
	$(top_builddir)/hw/xquartz/pbproxy/libxpbproxy.la \
	$(XQUARTZ_LIBS) $(XSERVER_LIBS)

# $(XSERVER_SYS_LIBS) is placed here in order to set command line ordering
# to work around build issues on Tiger.
X11_bin_LDADD = $(X11_bin_DEPENDENCIES) $(XSERVER_SYS_LIBS)

X11_bin_LDFLAGS =  \
	-lXplugin \
	-XCClinker -Objc \
	-Wl,-u,_miDCInitialize \
	-Wl,-framework,Carbon \
	-Wl,-framework,Cocoa \
	-Wl,-framework,CoreAudio \
	-Wl,-framework,IOKit

if GLX
X11_bin_DEPENDENCIES += \
	$(top_builddir)/hw/xquartz/GL/libCGLCore.la \
	$(top_builddir)/glx/libglxvnd.la \
	$(top_builddir)/glx/libglx.la

X11_bin_LDFLAGS += \
	-Wl,-framework,OpenGL
endif

if XQUARTZ_SPARKLE
X11_bin_LDFLAGS += \
	-Wl,-framework,Sparkle
endif

if RECORD
X11_bin_DEPENDENCIES += \
	$(top_builddir)/record/librecord.la
endif

bin_PROGRAMS = Xquartz

dist_Xquartz_SOURCES = \
	stub.c \
	launchd_fd.c

nodist_Xquartz_SOURCES = \
	mach_startupUser.c

Xquartz_LDFLAGS =  \
	-Wl,-framework,CoreServices

if XQUARTZ

BUILT_SOURCES = \
	mach_startupServer.c \
	mach_startupUser.c \
	mach_startupServer.h \
	mach_startup.h

CLEANFILES = \
	$(BUILT_SOURCES)

$(BUILT_SOURCES): $(srcdir)/mach_startup.defs
	mig -sheader mach_startupServer.h $(srcdir)/mach_startup.defs

endif

EXTRA_DIST = \
	launchd_fd.h \
	mach_startup.defs \
	mach_startup_types.h
