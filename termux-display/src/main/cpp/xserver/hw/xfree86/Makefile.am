
if DRI
DRI_SUBDIR = dri
DRI_LIB = dri/libdri.la
endif

if DRI2
DRI2_SUBDIR = dri2
DRI2_LIB = dri2/libdri2.la
endif

if DRI3
DRI3_BUILDDIR = $(top_builddir)/dri3
DRI3_LIB = $(DRI3_BUILDDIR)/libdri3.la
endif

if GLX
GLXVND_LIB = $(top_builddir)/glx/libglxvnd.la
endif

if GLAMOR_EGL
GLAMOR_EGL_SUBDIR = glamor_egl
endif

if XF86UTILS
XF86UTILS_SUBDIR = utils
endif

if VGAHW
VGAHW_SUBDIR = vgahw
endif

if INT10MODULE
INT10_SUBDIR = int10
endif

SUBDIRS = common ddc x86emu $(INT10_SUBDIR) os-support parser \
	  ramdac $(VGAHW_SUBDIR) loader modes $(DRI_SUBDIR) \
	  $(DRI2_SUBDIR) . i2c dixmods xkb \
	  fbdevhw shadowfb exa $(XF86UTILS_SUBDIR) doc man \
	  $(GLAMOR_EGL_SUBDIR) drivers

DIST_SUBDIRS = common ddc i2c x86emu int10 fbdevhw os-support \
               parser ramdac shadowfb vgahw \
               loader dixmods xkb dri dri2 exa modes \
	       utils doc man glamor_egl drivers

bin_PROGRAMS = Xorg
nodist_Xorg_SOURCES = sdksyms.c

AM_CFLAGS = $(DIX_CFLAGS) @XORG_CFLAGS@
AM_CPPFLAGS = $(XORG_INCS) -I$(srcdir)/parser \
	-I$(srcdir)/ddc -I$(srcdir)/i2c -I$(srcdir)/modes -I$(srcdir)/ramdac \
	-I$(srcdir)/dri -I$(srcdir)/dri2 -I$(top_srcdir)/dri3

LOCAL_LIBS = \
            $(MAIN_LIB) \
            $(XSERVER_LIBS) \
            loader/libloader.la \
            common/libcommon.la \
            os-support/libxorgos.la \
            parser/libxf86config.la \
            dixmods/libdixmods.la \
            modes/libxf86modes.la \
            ramdac/libramdac.la \
            ddc/libddc.la \
            i2c/libi2c.la \
            $(XORG_LIBS) \
            xkb/libxorgxkb.la \
            $(DRI_LIB) \
            $(DRI2_LIB) \
	    $(DRI3_LIB) \
	    $(GLXVND_LIB) \
	    $(top_builddir)/fb/libfb.la \
	    $(top_builddir)/miext/sync/libsync.la \
            $(top_builddir)/mi/libmi.la \
            $(top_builddir)/os/libos.la \
	    $(top_builddir)/Xext/libXvidmode.la

Xorg_LDADD = \
            $(LOCAL_LIBS) \
            $(XORG_SYS_LIBS) \
            $(XSERVER_SYS_LIBS)
Xorg_DEPENDENCIES = $(LOCAL_LIBS)

Xorg_LDFLAGS = $(LD_EXPORT_SYMBOLS_FLAG)

if SUID_WRAPPER
wrapexecdir = $(SUID_WRAPPER_DIR)
wrapexec_PROGRAMS = Xorg.wrap
Xorg_wrap_SOURCES = xorg-wrapper.c
endif

BUILT_SOURCES = xorg.conf.example
DISTCLEANFILES = xorg.conf.example
EXTRA_DIST = xorgconf.cpp

# Without logdir, X will post an error on the terminal and will not start
install-data-local:
	$(AM_V_GEN)$(MKDIR_P) $(DESTDIR)$(logdir)
if CYGWIN
	$(INSTALL_DATA) libXorg.exe.a $(DESTDIR)$(libdir)/libXorg.exe.a
endif

install-exec-hook:
	(cd $(DESTDIR)$(bindir) && rm -f X && $(LN_S) Xorg$(EXEEXT) X)
if INSTALL_SETUID
	chown 0 $(DESTDIR)$(bindir)/Xorg
	chmod u+s $(DESTDIR)$(bindir)/Xorg
endif
if SUID_WRAPPER
	$(MKDIR_P) $(DESTDIR)$(SUID_WRAPPER_DIR)
	mv $(DESTDIR)$(bindir)/Xorg $(DESTDIR)$(SUID_WRAPPER_DIR)/Xorg
	${INSTALL} -m 755 Xorg.sh $(DESTDIR)$(bindir)/Xorg
	-chown 0 $(DESTDIR)$(SUID_WRAPPER_DIR)/Xorg.wrap && chmod u+s $(DESTDIR)$(SUID_WRAPPER_DIR)/Xorg.wrap
endif

uninstall-local:
if CYGWIN
	$(AM_V_at)rm -f $(DESTDIR)$(libdir)/libXorg.exe.a
endif

uninstall-hook:
	$(AM_V_at)rm -f $(DESTDIR)$(bindir)/X

# Use variables from XORG_MANPAGE_SECTIONS and X Server configuration
# Do not include manpages.am as values are not appropriate for rc files
CONF_SUBSTS =	-e 's|__filemansuffix__|$(FILE_MAN_SUFFIX)|g' \
		-e 's|MODULEPATH|$(DEFAULT_MODULE_PATH)|g' \
		-e 's|DEFAULTFONTPATH|$(COMPILEDDEFAULTFONTPATH)|g'

xorg.conf.example: xorgconf.cpp
	$(AM_V_GEN)$(SED) $(CONF_SUBSTS) < $(srcdir)/xorgconf.cpp > $@

relink:
	$(AM_V_at)rm -f Xorg$(EXEEXT) && $(MAKE) Xorg$(EXEEXT)

CLEANFILES = sdksyms.c sdksyms.dep Xorg.sh
EXTRA_DIST += sdksyms.sh

sdksyms.dep sdksyms.c: sdksyms.sh
	$(AM_V_GEN)CPP='$(CPP)' AWK='$(AWK)' $(SHELL) $(srcdir)/sdksyms.sh $(top_srcdir) $(CFLAGS) $(AM_CFLAGS) $(AM_CPPFLAGS)

SDKSYMS_DEP = sdksyms.dep
-include $(SDKSYMS_DEP)

i2c/libi2c.la:
	$(AM_V_at)cd i2c && $(MAKE) libi2c.la

dixmods/libdixmods.la:
	$(AM_V_at)cd dixmods && $(MAKE) libdixmods.la

xkb/libxorgxkb.la:
	$(AM_V_at)cd xkb && $(MAKE) libxorgxkb.la
