SED = sed

XORGRELSTRING = @PACKAGE_STRING@
  XORGMANNAME = X Version 11

MAN_SUBSTS = \
        -e 's|__vendorversion__|"$(XORGRELSTRING)" "$(XORGMANNAME)"|' \
        -e 's|__xorgversion__|"$(XORGRELSTRING)" "$(XORGMANNAME)"|' \
        -e 's|__datadir__|$(datadir)|g' \
        -e 's|__libdir__|$(libdir)|g' \
        -e 's|__sysconfdir__|$(sysconfdir)|g' \
        -e 's|__appmansuffix__|$(APP_MAN_SUFFIX)|g' \
        -e 's|__libmansuffix__|$(LIB_MAN_SUFFIX)|g' \
        -e 's|__miscmansuffix__|$(MISC_MAN_SUFFIX)|g'

SUFFIXES = .$(MISC_MAN_SUFFIX) .man

.man.$(MISC_MAN_SUFFIX): man
	$(MKDIR_P) man
	$(AM_V_GEN)$(SED) $(MAN_SUBSTS) < $< > $@

dist_doc_DATA =

applewmdir = $(includedir)/X11/extensions
applewm_HEADERS = \
        include/X11/extensions/applewmconst.h \
	include/X11/extensions/applewmproto.h

applewmprotopkgconfigdir = $(datadir)/pkgconfig
applewmprotopkgconfig_DATA = applewmproto.pc

bigreqsdir = $(includedir)/X11/extensions
bigreqs_HEADERS = \
	include/X11/extensions/bigreqsproto.h \
	include/X11/extensions/bigreqstr.h

bigreqsprotopkgconfigdir = $(datadir)/pkgconfig
bigreqsprotopkgconfig_DATA = bigreqsproto.pc

compositedir = $(includedir)/X11/extensions
composite_HEADERS = \
	include/X11/extensions/composite.h \
	include/X11/extensions/compositeproto.h

compositeprotopkgconfigdir = $(datadir)/pkgconfig
compositeprotopkgconfig_DATA = compositeproto.pc

dist_doc_DATA += compositeproto.txt

damagedir = $(includedir)/X11/extensions
damage_HEADERS = \
	include/X11/extensions/damageproto.h \
	include/X11/extensions/damagewire.h

damageprotopkgconfigdir = $(datadir)/pkgconfig
damageprotopkgconfig_DATA = damageproto.pc

dist_doc_DATA += damageproto.txt

dmxdir = $(includedir)/X11/extensions
dmx_HEADERS = \
	include/X11/extensions/dmx.h \
	include/X11/extensions/dmxproto.h

dmxprotopkgconfigdir = $(datadir)/pkgconfig
dmxprotopkgconfig_DATA = dmxproto.pc

dpmsprotopkgconfigdir = $(datadir)/pkgconfig
dpmsprotopkgconfig_DATA = dpmsproto.pc

dri2dir = $(includedir)/X11/extensions
dri2_HEADERS = \
	include/X11/extensions/dri2proto.h \
	include/X11/extensions/dri2tokens.h

dri2protopkgconfigdir = $(datadir)/pkgconfig
dri2protopkgconfig_DATA = dri2proto.pc

dist_doc_DATA += dri2proto.txt

dri3dir = $(includedir)/X11/extensions
dri3_HEADERS = \
	include/X11/extensions/dri3proto.h

dri3protopkgconfigdir = $(datadir)/pkgconfig
dri3protopkgconfig_DATA = dri3proto.pc

dist_doc_DATA += dri3proto.txt

fixesdir = $(includedir)/X11/extensions
fixes_HEADERS = \
	include/X11/extensions/xfixesproto.h \
	include/X11/extensions/xfixeswire.h

fixesprotopkgconfigdir = $(datadir)/pkgconfig
fixesprotopkgconfig_DATA = fixesproto.pc

dist_doc_DATA += fixesproto.txt

fontsdir = $(includedir)/X11/fonts
fonts_HEADERS = \
	include/X11/fonts/font.h \
	include/X11/fonts/fontproto.h \
	include/X11/fonts/fontstruct.h \
	include/X11/fonts/FS.h \
	include/X11/fonts/fsmasks.h \
	include/X11/fonts/FSproto.h

fontsprotopkgconfigdir = $(datadir)/pkgconfig
fontsprotopkgconfig_DATA = fontsproto.pc

gldir = $(includedir)/GL
gl_HEADERS = \
	include/GL/glxint.h \
	include/GL/glxmd.h \
	include/GL/glxproto.h \
	include/GL/glxtokens.h

glinternaldir = $(includedir)/GL/internal
glinternal_HEADERS = \
	include/GL/internal/glcore.h

glprotopkgconfigdir = $(datadir)/pkgconfig
glprotopkgconfig_DATA = glproto.pc

inputdir = $(includedir)/X11/extensions
input_HEADERS = \
	include/X11/extensions/XI.h \
	include/X11/extensions/XIproto.h \
	include/X11/extensions/XI2.h \
	include/X11/extensions/XI2proto.h

inputprotopkgconfigdir = $(datadir)/pkgconfig
inputprotopkgconfig_DATA = inputproto.pc

kbdir = $(includedir)/X11/extensions
kb_HEADERS = \
	include/X11/extensions/XKB.h \
	include/X11/extensions/XKBproto.h \
	include/X11/extensions/XKBsrv.h \
	include/X11/extensions/XKBstr.h
if LEGACY
kb_HEADERS += include/X11/extensions/XKBgeom.h
endif

kbprotopkgconfigdir = $(datadir)/pkgconfig
kbprotopkgconfig_DATA = kbproto.pc

presentdir = $(includedir)/X11/extensions
present_HEADERS = \
	include/X11/extensions/presentproto.h \
	include/X11/extensions/presenttokens.h

presentprotopkgconfigdir = $(datadir)/pkgconfig
presentprotopkgconfig_DATA = presentproto.pc

dist_doc_DATA += presentproto.txt

randrdir = $(includedir)/X11/extensions
randr_HEADERS = \
	include/X11/extensions/randr.h \
	include/X11/extensions/randrproto.h

randrprotopkgconfigdir = $(datadir)/pkgconfig
randrprotopkgconfig_DATA = randrproto.pc

dist_doc_DATA += randrproto.txt

recorddir = $(includedir)/X11/extensions
record_HEADERS = \
	include/X11/extensions/recordconst.h \
	include/X11/extensions/recordproto.h \
	include/X11/extensions/recordstr.h

recordprotopkgconfigdir = $(datadir)/pkgconfig
recordprotopkgconfig_DATA = recordproto.pc

renderdir = $(includedir)/X11/extensions
render_HEADERS = \
	include/X11/extensions/render.h \
	include/X11/extensions/renderproto.h

renderprotopkgconfigdir = $(datadir)/pkgconfig
renderprotopkgconfig_DATA = renderproto.pc

dist_doc_DATA += renderproto.txt

resourcedir = $(includedir)/X11/extensions
resource_HEADERS = \
	include/X11/extensions/XResproto.h

resourceprotopkgconfigdir = $(datadir)/pkgconfig
resourceprotopkgconfig_DATA = resourceproto.pc

dist_doc_DATA += resproto.txt

scrnsaverdir = $(includedir)/X11/extensions
scrnsaver_HEADERS = \
	include/X11/extensions/saver.h \
	include/X11/extensions/saverproto.h

scrnsaverprotopkgconfigdir = $(datadir)/pkgconfig
scrnsaverprotopkgconfig_DATA = scrnsaverproto.pc

videodir = $(includedir)/X11/extensions
video_HEADERS = \
	include/X11/extensions/Xv.h \
	include/X11/extensions/XvMC.h \
	include/X11/extensions/XvMCproto.h \
	include/X11/extensions/Xvproto.h
if LEGACY
video_HEADERS += include/X11/extensions/vldXvMC.h
endif

videoprotopkgconfigdir = $(datadir)/pkgconfig
videoprotopkgconfig_DATA = videoproto.pc

dist_doc_DATA += xv-protocol-v2.txt

xprotodir = $(includedir)/X11
xproto_HEADERS = \
	include/X11/ap_keysym.h \
	include/X11/DECkeysym.h \
	include/X11/HPkeysym.h \
	include/X11/keysymdef.h \
	include/X11/keysym.h \
	include/X11/Sunkeysym.h \
	include/X11/Xalloca.h \
	include/X11/Xarch.h \
	include/X11/Xatom.h \
	include/X11/Xdefs.h \
	include/X11/XF86keysym.h \
	include/X11/Xfuncproto.h \
	include/X11/Xfuncs.h \
	include/X11/X.h \
	include/X11/Xmd.h \
	include/X11/Xosdefs.h \
	include/X11/Xos.h \
	include/X11/Xos_r.h \
	include/X11/Xproto.h \
	include/X11/Xprotostr.h \
	include/X11/Xthreads.h \
	include/X11/Xw32defs.h \
	include/X11/XWDFile.h \
	include/X11/Xwindows.h \
	include/X11/Xwinsock.h

nodist_xproto_HEADERS = \
	include/X11/Xpoll.h

x11protopkgconfigdir = $(datadir)/pkgconfig
x11protopkgconfig_DATA = xproto.pc

xcmiscdir = $(includedir)/X11/extensions
xcmisc_HEADERS = \
	include/X11/extensions/xcmiscproto.h \
	include/X11/extensions/xcmiscstr.h

xcmiscprotopkgconfigdir = $(datadir)/pkgconfig
xcmiscprotopkgconfig_DATA = xcmiscproto.pc

xextdir = $(includedir)/X11/extensions
xext_HEADERS = \
	include/X11/extensions/dpmsconst.h \
	include/X11/extensions/dpmsproto.h \
        include/X11/extensions/ge.h \
        include/X11/extensions/geproto.h \
	include/X11/extensions/lbx.h \
	include/X11/extensions/lbxproto.h \
	include/X11/extensions/mitmiscconst.h \
	include/X11/extensions/mitmiscproto.h \
	include/X11/extensions/multibufconst.h \
	include/X11/extensions/multibufproto.h \
	include/X11/extensions/secur.h \
	include/X11/extensions/securproto.h \
	include/X11/extensions/shapeconst.h \
	include/X11/extensions/shapeproto.h \
	include/X11/extensions/shm.h \
	include/X11/extensions/shmproto.h \
	include/X11/extensions/syncconst.h \
	include/X11/extensions/syncproto.h \
	include/X11/extensions/ag.h \
	include/X11/extensions/agproto.h \
	include/X11/extensions/cup.h \
	include/X11/extensions/cupproto.h \
	include/X11/extensions/dbe.h \
	include/X11/extensions/dbeproto.h \
	include/X11/extensions/EVI.h \
	include/X11/extensions/EVIproto.h \
	include/X11/extensions/xtestext1proto.h \
	include/X11/extensions/xtestext1const.h \
	include/X11/extensions/xtestconst.h \
	include/X11/extensions/xtestproto.h

compatdir = $(xextdir)
compat_HEADERS = \
	include/X11/extensions/shapestr.h \
	include/X11/extensions/shmstr.h \
	include/X11/extensions/syncstr.h

xextprotopkgconfigdir = $(datadir)/pkgconfig
xextprotopkgconfig_DATA = xextproto.pc

xf86bigfontdir = $(includedir)/X11/extensions
xf86bigfont_HEADERS = \
	include/X11/extensions/xf86bigfont.h \
	include/X11/extensions/xf86bigfproto.h \
	include/X11/extensions/xf86bigfstr.h

xf86bigfontprotopkgconfigdir = $(datadir)/pkgconfig
xf86bigfontprotopkgconfig_DATA = xf86bigfontproto.pc

xf86dgadir = $(includedir)/X11/extensions
xf86dga_HEADERS = \
	include/X11/extensions/xf86dga.h \
	include/X11/extensions/xf86dga1const.h \
	include/X11/extensions/xf86dga1str.h \
	include/X11/extensions/xf86dga1proto.h \
	include/X11/extensions/xf86dgaconst.h \
	include/X11/extensions/xf86dgaproto.h \
	include/X11/extensions/xf86dgastr.h

xf86dgaprotopkgconfigdir = $(datadir)/pkgconfig
xf86dgaprotopkgconfig_DATA = xf86dgaproto.pc

xf86dridir = $(includedir)/X11/dri
xf86dri_HEADERS = \
	include/X11/dri/xf86dri.h \
	include/X11/dri/xf86dristr.h \
	include/X11/dri/xf86driproto.h

xf86driprotopkgconfigdir = $(datadir)/pkgconfig
xf86driprotopkgconfig_DATA = xf86driproto.pc

xf86vidmodedir = $(includedir)/X11/extensions
xf86vidmode_HEADERS = \
	include/X11/extensions/xf86vm.h \
	include/X11/extensions/xf86vmproto.h \
	include/X11/extensions/xf86vmstr.h

xf86vidmodeprotopkgconfigdir = $(datadir)/pkgconfig
xf86vidmodeprotopkgconfig_DATA = xf86vidmodeproto.pc

xineramadir = $(includedir)/X11/extensions
xinerama_HEADERS = \
	include/X11/extensions/panoramiXproto.h

xineramaprotopkgconfigdir = $(datadir)/pkgconfig
xineramaprotopkgconfig_DATA = xineramaproto.pc

xwaylandprotodir = $(includedir)/X11/extensions
xwaylandproto_HEADERS = \
	include/X11/extensions/xwaylandproto.h

xwaylandprotopkgconfigdir = $(datadir)/pkgconfig
xwaylandprotopkgconfig_DATA = xwaylandproto.pc

dist_doc_DATA += xwaylandproto.txt

if LEGACY
XCalibrateincludedir = $(includedir)/X11/extensions
XCalibrateinclude_HEADERS = \
	include/X11/extensions/xcalibrateproto.h \
	include/X11/extensions/xcalibratewire.h

calibrateprotopkgconfigdir = $(datadir)/pkgconfig
calibrateprotopkgconfig_DATA = xcalibrateproto.pc

eviedir = $(includedir)/X11/extensions
evie_HEADERS = \
	include/X11/extensions/evieproto.h \
	include/X11/extensions/Xeviestr.h

evieprotopkgconfigdir = $(datadir)/pkgconfig
evieprotopkgconfig_DATA = evieproto.pc

fontcachedir = $(includedir)/X11/extensions
fontcache_HEADERS = \
	include/X11/extensions/fontcache.h \
	include/X11/extensions/fontcacheP.h \
	include/X11/extensions/fontcachstr.h

fontcacheprotopkgconfigdir = $(datadir)/pkgconfig
fontcacheprotopkgconfig_DATA = fontcacheproto.pc

lg3ddir = $(includedir)/X11/extensions
lg3d_HEADERS = \
	include/X11/extensions/lgewire.h

lg3dprotopkgconfigdir = $(datadir)/pkgconfig
lg3dprotopkgconfig_DATA = lg3dproto.pc

xproxymngdir = $(includedir)/X11/PM
xproxymng_HEADERS = \
	include/X11/PM/PM.h \
	include/X11/PM/PMproto.h

pmprotopkgconfigdir = $(datadir)/pkgconfig
pmprotopkgconfig_DATA = xproxymngproto.pc

dist_doc_DATA += PM_spec

printdir = $(includedir)/X11/extensions
print_HEADERS = \
	include/X11/extensions/Print.h \
	include/X11/extensions/Printstr.h

printprotopkgconfigdir = $(datadir)/pkgconfig
printprotopkgconfig_DATA = printproto.pc

miscmandir = $(MISC_MAN_DIR)
miscman_DATA =	man/Xprint.$(MISC_MAN_SUFFIX)
CLEANFILES = $(miscman_DATA)

trapdir = $(includedir)/X11/extensions
trap_HEADERS = \
	include/X11/extensions/xtrapbits.h \
	include/X11/extensions/xtrapddmi.h \
	include/X11/extensions/xtrapdi.h \
	include/X11/extensions/xtrapemacros.h \
	include/X11/extensions/xtraplib.h \
	include/X11/extensions/xtraplibp.h \
	include/X11/extensions/xtrapproto.h

trapprotopkgconfigdir = $(datadir)/pkgconfig
trapprotopkgconfig_DATA = trapproto.pc

windowswmdir = $(includedir)/X11/extensions
windowswm_HEADERS = \
        include/X11/extensions/windowswm.h \
	include/X11/extensions/windowswmstr.h

windowswmprotopkgconfigdir = $(datadir)/pkgconfig
windowswmprotopkgconfig_DATA = windowswmproto.pc

xf86miscdir = $(includedir)/X11/extensions
xf86misc_HEADERS = \
	include/X11/extensions/xf86misc.h \
	include/X11/extensions/xf86mscstr.h

xf86miscprotopkgconfigdir = $(datadir)/pkgconfig
xf86miscprotopkgconfig_DATA = xf86miscproto.pc

xf86rushdir = $(includedir)/X11/extensions
xf86rush_HEADERS = \
	include/X11/extensions/xf86rush.h \
	include/X11/extensions/xf86rushstr.h

xf86rushprotopkgconfigdir = $(datadir)/pkgconfig
xf86rushprotopkgconfig_DATA = xf86rushproto.pc
endif

SUBDIRS=specs

EXTRA_DIST = \
    COPYING-applewmproto \
    COPYING-bigreqsproto \
    COPYING-compositeproto \
    COPYING-damageproto \
    COPYING-dmxproto \
    COPYING-dri2proto \
    COPYING-dri3proto \
    COPYING-evieproto \
    COPYING-fixesproto \
    COPYING-fontcacheproto \
    COPYING-fontsproto \
    COPYING-glproto \
    COPYING-inputproto \
    COPYING-kbproto \
    COPYING-lg3dproto \
    COPYING-pmproto \
    COPYING-presentproto \
    COPYING-printproto \
    COPYING-randrproto \
    COPYING-recordproto \
    COPYING-renderproto \
    COPYING-resourceproto \
    COPYING-scrnsaverproto \
    COPYING-trapproto \
    COPYING-videoproto \
    COPYING-windowswmproto \
    COPYING-x11proto \
    COPYING-xcmiscproto \
    COPYING-xextproto \
    COPYING-xf86bigfontproto \
    COPYING-xf86dgaproto \
    COPYING-xf86driproto \
    COPYING-xf86miscproto \
    COPYING-xf86rushproto \
    COPYING-xf86vidmodeproto \
    COPYING-xineramaproto \
    COPYING-xwaylandproto \
    README.md \
    autogen.sh \
    include/GL/internal/meson.build \
    include/GL/meson.build \
    include/meson.build \
    include/X11/dri/meson.build \
    include/X11/extensions/meson.build \
    include/X11/fonts/meson.build \
    include/X11/meson.build \
    include/X11/PM/meson.build \
    meson.build \
    meson_options.txt \
    man/Xprint.man \
    man/Xprint.sgml \
    scripts/keysym-generator.py


AM_DISTCHECK_CONFIGURE_FLAGS = --enable-legacy

if HAVE_PYTHON
AM_TESTS_ENVIRONMENT = \
       INCLUDESDIR=$(top_srcdir)/include
TESTS = scripts/keysym-generator.py
TEST_EXTENSIONS = .py
PY_LOG_COMPILER = $(PYTHON)
endif
