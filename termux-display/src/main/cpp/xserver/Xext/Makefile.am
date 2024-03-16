noinst_LTLIBRARIES = libXext.la libXvidmode.la libhashtable.la

AM_CFLAGS = $(DIX_CFLAGS)

if XORG
sdk_HEADERS = xvdix.h xvmcext.h geext.h geint.h shmint.h syncsdk.h
endif

# Sources always included in libXextbuiltin.la, libXext.la
BUILTIN_SRCS =			\
	bigreq.c		\
        geext.c			\
	shape.c			\
	sleepuntil.c		\
	sleepuntil.h		\
	sync.c			\
	syncsdk.h		\
	syncsrv.h		\
	xcmisc.c		\
	xtest.c
BUILTIN_LIBS =

# Optional sources included if extension enabled by configure.ac rules

# MIT Shared Memory extension
MITSHM_SRCS = shm.c shmint.h
if MITSHM
BUILTIN_SRCS += $(MITSHM_SRCS)
endif

# XVideo extension
XV_SRCS = xvmain.c xvdisp.c xvmc.c xvdix.h xvmcext.h xvdisp.h
if XV
BUILTIN_SRCS  += $(XV_SRCS)
endif

# XResource extension: lets clients get data about per-client resource usage
RES_SRCS = xres.c
if RES
BUILTIN_SRCS  += $(RES_SRCS)
endif

# MIT ScreenSaver extension
SCREENSAVER_SRCS = saver.c
if SCREENSAVER
BUILTIN_SRCS  += $(SCREENSAVER_SRCS)
endif

# Xinerama extension: making multiple video devices act as one virtual screen
XINERAMA_SRCS = panoramiX.c panoramiX.h panoramiXh.h panoramiXsrv.h panoramiXprocs.c panoramiXSwap.c 
if XINERAMA
BUILTIN_SRCS += $(XINERAMA_SRCS)
if XORG
sdk_HEADERS += panoramiXsrv.h panoramiX.h
endif
endif

# X-ACE extension: provides hooks for building security policy extensions
# like XC-Security, X-SELinux & XTSol
XACE_SRCS = xace.c xace.h xacestr.h
if XACE
BUILTIN_SRCS += $(XACE_SRCS)
if XORG
sdk_HEADERS += xace.h xacestr.h
endif
endif

# SELinux extension: provides SELinux policy support for X objects
# requires X-ACE extension
XSELINUX_SRCS = xselinux_ext.c xselinux_hooks.c xselinux_label.c xselinux.h xselinuxint.h
if XSELINUX
BUILTIN_SRCS += $(XSELINUX_SRCS)
BUILTIN_LIBS += $(SELINUX_LIBS)
endif

# Security extension: multi-level security to protect clients from each other
XCSECURITY_SRCS = security.c securitysrv.h
if XCSECURITY   
BUILTIN_SRCS += $(XCSECURITY_SRCS)
endif

# XF86 Big Font extension
BIGFONT_SRCS = xf86bigfont.c xf86bigfontsrv.h
if XF86BIGFONT
BUILTIN_SRCS += $(BIGFONT_SRCS)
endif

# DPMS extension
DPMS_SRCS = dpms.c dpmsproc.h
if DPMSExtension
BUILTIN_SRCS += $(DPMS_SRCS)
endif

# Now take all of the above, mix well, bake for 10 minutes and get libXext*.la

libXext_la_SOURCES =		$(BUILTIN_SRCS)
libXext_la_LIBADD =		$(BUILTIN_LIBS)
if RES
libXext_la_LIBADD += libhashtable.la
endif

# XVidMode extension
libXvidmode_la_SOURCES = vidmode.c

#Hashtable
libhashtable_la_SOURCES = hashtable.c hashtable.h

EXTRA_DIST = \
	$(MITSHM_SRCS) \
	$(XV_SRCS) \
	$(RES_SRCS) \
	$(SCREENSAVER_SRCS) \
	$(XACE_SRCS) \
	$(XCSECURITY_SRCS) \
	$(XSELINUX_SRCS) \
	$(XINERAMA_SRCS) \
	$(BIGFONT_SRCS) \
	$(DPMS_SRCS) \
        $(GE_SRCS)

