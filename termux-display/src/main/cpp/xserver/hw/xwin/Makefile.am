bin_PROGRAMS = XWin

SRCS_CLIPBOARD = \
	winclipboardinit.c \
	winclipboardwrappers.c
CLIPBOARD_LIBS = $(top_builddir)/hw/xwin/winclipboard/libXWinclipboard.la

if XWIN_GLX_WINDOWS
GLX_DIR =
DEFS_GLX_WINDOWS =
XWIN_GLX_LIBS =
if XWIN_WINDOWS_DRI
GLX_DIR += dri
DEFS_GLX_WINDOWS += -DXWIN_WINDOWS_DRI
XWIN_GLX_LIBS += $(top_builddir)/hw/xwin/dri/libWindowsDRI.la
endif
GLX_DIR += glx
DEFS_GLX_WINDOWS += -DXWIN_GLX_WINDOWS
XWIN_GLX_LIBS += $(top_builddir)/hw/xwin/glx/libXwinGLX.la
XWIN_GLX_SYS_LIBS = -lopengl32
endif

SRCS_MULTIWINDOW = \
	winmultiwindowshape.c \
	winmultiwindowwindow.c \
	winmultiwindowwm.c \
	winmultiwindowwndproc.c \
	propertystore.h \
	winSetAppUserModelID.c
MULTIWINDOW_SYS_LIBS = -lshlwapi -lole32 -ldwmapi

SRCS_RANDR = \
	winrandr.c

SRCS =	InitInput.c \
	InitOutput.c \
	winallpriv.c \
	winauth.c \
	winblock.c \
	wincmap.c \
	winconfig.c \
	wincreatewnd.c \
	wincursor.c \
	windialogs.c \
	winengine.c \
	winerror.c \
	winglobals.c \
	winkeybd.c \
	winkeyhook.c \
	winmisc.c \
	winmonitors.c \
	winmouse.c \
	winmsg.c \
	winmsgwindow.c \
	winmultiwindowclass.c \
	winmultiwindowicons.c \
	winos.c \
	winprefs.c \
	winprefsyacc.y \
	winprefslex.l \
	winprocarg.c \
	winscrinit.c \
	winshadddnl.c \
	winshadgdi.c \
	wintaskbar.c \
	wintrayicon.c \
	winvalargs.c \
	winwakeup.c \
	winwindow.c \
	winwndproc.c \
	ddraw.h \
	winconfig.h \
	win.h \
	winglobals.h \
	winkeybd.h \
	winkeynames.h \
	winlayouts.h \
	winmessages.h \
	winmonitors.h \
	winmsg.h \
	winms.h \
	winmultiwindowclass.h \
	winmultiwindowicons.h \
	winprefs.h \
	winresource.h \
	winwindow.h \
	windisplay.c \
	windisplay.h \
	XWin.rc \
	$(top_srcdir)/mi/miinitext.c \
	$(top_srcdir)/mi/miinitext.h \
	$(SRCS_CLIPBOARD) \
	$(SRCS_MULTIWINDOW) \
	$(SRCS_RANDR)

DEFS = \
	$(DEFS_GLX_WINDOWS) \
	$(DEFS_MULTIWINDOWEXTWM)

XWin_SOURCES = $(SRCS)

XWIN_SYS_LIBS += -ldxguid

XWIN_LIBS += \
	$(top_builddir)/pseudoramiX/libPseudoramiX.la \
	$(top_builddir)/Xi/libXistubs.la

XWin_DEPENDENCIES = \
	$(XWIN_GLX_LIBS) \
	$(XWIN_LIBS) \
	$(CLIPBOARD_LIBS) \
	$(XSERVER_LIBS)


XWin_LDADD = \
	$(XWIN_GLX_LIBS) \
	$(XWIN_LIBS) \
	$(CLIPBOARD_LIBS) \
	$(XSERVER_LIBS) \
	$(XWIN_GLX_SYS_LIBS) \
	$(XSERVER_SYS_LIBS) \
	$(XWIN_SYS_LIBS) \
	$(MULTIWINDOW_SYS_LIBS)

XWin_LDFLAGS = -mwindows -Wl,--disable-stdcall-fixup $(LD_EXPORT_SYMBOLS_FLAG)


.rc.o:
	$(AM_V_GEN)$(WINDRES) --use-temp-file -i $< --input-format=rc -o $@ -O coff -I $(top_builddir)/include

XWin.o: XWin.rc XWin.exe.manifest X.ico

winprefsyacc.h: winprefsyacc.c
winprefslex.c: winprefslex.l winprefsyacc.c winprefsyacc.h

BUILT_SOURCES = winprefsyacc.h winprefsyacc.c winprefslex.c
CLEANFILES = $(BUILT_SOURCES)

AM_YFLAGS = -d
AM_LFLAGS = -i
AM_CFLAGS = -DHAVE_XWIN_CONFIG_H $(DIX_CFLAGS) \
            $(XWINMODULES_CFLAGS) \
            -I$(top_srcdir) \
            -Wno-bad-function-cast

xwinconfigdir = $(sysconfdir)/X11
xwinconfig_DATA = system.XWinrc

install-exec-hook:
	(cd $(DESTDIR)$(bindir) && rm -f X && $(LN_S) XWin$(EXEEXT) X)

EXTRA_DIST = \
	$(xwinconfig_DATA) \
	X.ico \
	XWin.rc \
	XWin.exe.manifest

relink:
	$(AM_V_at)rm -f XWin$(EXEEXT) && $(MAKE) XWin$(EXEEXT)

SUBDIRS = man $(GLX_DIR) winclipboard .
DIST_SUBDIRS = man dri glx winclipboard .
