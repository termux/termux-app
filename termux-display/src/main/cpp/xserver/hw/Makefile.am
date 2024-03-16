if XORG
XORG_SUBDIRS = xfree86
endif

if XVFB
XVFB_SUBDIRS = vfb
endif

if XNEST
XNEST_SUBDIRS = xnest
endif

if XWIN
XWIN_SUBDIRS = xwin
endif

if KDRIVE
KDRIVE_SUBDIRS = kdrive
endif

if XQUARTZ
XQUARTZ_SUBDIRS = xquartz
endif

SUBDIRS =			\
	$(XORG_SUBDIRS)		\
	$(XWIN_SUBDIRS)		\
	$(XVFB_SUBDIRS)		\
	$(XNEST_SUBDIRS)	\
	$(KDRIVE_SUBDIRS)	\
	$(XQUARTZ_SUBDIRS)

DIST_SUBDIRS = xfree86 vfb xnest xwin xquartz kdrive

relink:
	$(AM_V_at)for i in $(SUBDIRS) ; do $(MAKE) -C $$i relink || exit 1 ; done
