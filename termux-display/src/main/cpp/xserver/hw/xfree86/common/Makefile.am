noinst_LTLIBRARIES = libcommon.la

if XORG_BUS_PCI
PCI_SOURCES = xf86pciBus.c xf86VGAarbiter.c xf86VGAarbiter.h \
              xf86VGAarbiterPriv.h
endif

if XORG_BUS_SPARC
SBUS_SOURCES = xf86sbusBus.c
endif

if XV
XVSOURCES = xf86xv.c xf86xvmc.c
XVSDKINCS = xf86xv.h xf86xvmc.h xf86xvpriv.h
endif

if DGA
DGASOURCES = xf86DGA.c
DGA_SDK = dgaproc.h
endif

if XORG_BUS_PLATFORM
PLATSOURCES = xf86platformBus.c
endif

RANDRSOURCES = xf86RandR.c

BUSSOURCES = xf86fbBus.c xf86noBus.c $(PCI_SOURCES) $(SBUS_SOURCES) $(PLATSOURCES)

MODEDEFSOURCES = $(srcdir)/vesamodes $(srcdir)/extramodes

xf86DefModeSet.c: $(srcdir)/modeline2c.awk $(MODEDEFSOURCES)
	$(AM_V_GEN)cat $(MODEDEFSOURCES) | LC_ALL=C $(AWK) -f $(srcdir)/modeline2c.awk > $@
	$(AM_V_GEN)echo >> $@

BUILT_SOURCES = xf86DefModeSet.c

AM_LDFLAGS = -r
libcommon_la_SOURCES = xf86Configure.c xf86Bus.c xf86Config.c \
                      xf86Cursor.c $(DGASOURCES) xf86DPMS.c \
                      xf86Events.c xf86Globals.c xf86AutoConfig.c \
                      xf86Option.c xf86Init.c \
                      xf86VidMode.c xf86fbman.c xf86cmap.c \
                      xf86Helper.c xf86PM.c xf86Xinput.c xisb.c \
                      xf86Mode.c xorgHelper.c xf86Extensions.h \
		      xf86Extensions.c $(XF86VMODESOURCES) \
                      $(XVSOURCES) $(BUSSOURCES) $(RANDRSOURCES)
nodist_libcommon_la_SOURCES = xf86DefModeSet.c
libcommon_la_LIBADD = $(top_builddir)/config/libconfig.la

AM_CPPFLAGS = $(XORG_INCS) -I$(srcdir)/../ddc -I$(srcdir)/../i2c \
	   -I$(srcdir)/../loader -I$(srcdir)/../parser \
           -I$(srcdir)/../vbe -I$(srcdir)/../int10 \
	   -I$(srcdir)/../vgahw -I$(srcdir)/../dixmods/extmod \
	   -I$(srcdir)/../modes -I$(srcdir)/../ramdac -I$(srcdir)/../dri2 \
	   -I$(top_builddir)

sdk_HEADERS = compiler.h xf86.h xf86Module.h xf86Opt.h \
              xf86PciInfo.h xf86Priv.h xf86Privstr.h \
              xf86cmap.h xf86fbman.h xf86str.h xf86Xinput.h xisb.h \
              $(XVSDKINCS) $(XF86VMODE_SDK) $(DGA_SDK) xorgVersion.h \
              xf86sbusBus.h xf86VGAarbiter.h xf86Optionstr.h \
	      xf86platformBus.h xf86MatchDrivers.h \
	      xaarop.h

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
	compiler.h \
	xf86.h \
	xf86Bus.h \
	xf86Config.h \
	xf86InPriv.h \
	xf86Module.h \
	xf86Opt.h \
	xf86PciInfo.h \
	xf86Priv.h \
	xf86Privstr.h \
	xf86Xinput.h \
	xf86cmap.h \
	xf86fbman.h \
	xf86pciBus.h \
	xf86str.h \
	xf86xv.h \
	xf86xvmc.h \
	xf86xvpriv.h \
	xisb.h \
	xorgVersion.h \
	$(MODEDEFSOURCES) \
	modeline2c.awk \
	xf86VGAarbiter.h \
	xf86VGAarbiterPriv.h \
        $(DISTKBDSOURCES) \
	xaarop.h

if LNXACPI
XORG_CFLAGS += -DHAVE_ACPI
endif

if NEED_DBUS
XORG_CFLAGS += $(DBUS_CFLAGS)
endif

AM_CFLAGS = $(DIX_CFLAGS) $(XORG_CFLAGS)
