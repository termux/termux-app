if XORG
sdk_HEADERS =		\
	XIstubs.h	\
	Xprintf.h	\
	callback.h	\
	client.h	\
	closestr.h	\
	closure.h	\
	colormap.h	\
	colormapst.h	\
	hotplug.h	\
	cursor.h	\
	cursorstr.h	\
	dix.h		\
	dixaccess.h	\
	dixevents.h	\
	dixfont.h	\
	dixfontstr.h	\
	dixgrabs.h	\
	dixstruct.h	\
	events.h	\
	exevents.h	\
	extension.h	\
	extinit.h	\
	extnsionst.h	\
	fourcc.h	\
	gc.h		\
	gcstruct.h	\
	globals.h	\
	glx_extinit.h	\
	glxvndabi.h	\
	input.h		\
	inputstr.h	\
	list.h		\
	misc.h		\
	miscstruct.h	\
	opaque.h	\
	nonsdk_extinit.h	\
	optionstr.h	\
	os.h		\
	pixmap.h	\
	pixmapstr.h	\
	privates.h	\
	property.h	\
	propertyst.h	\
	ptrveloc.h  \
	region.h	\
	regionstr.h	\
	registry.h	\
	resource.h	\
	rgb.h		\
	screenint.h	\
	scrnintstr.h	\
	selection.h	\
	servermd.h	\
	validate.h	\
	displaymode.h    \
	window.h	\
	windowstr.h	\
	xkbfile.h	\
	xkbsrv.h	\
	xkbstr.h        \
	xkbrules.h      \
	xserver_poll.h	\
	xserver-properties.h

nodist_sdk_HEADERS = xorg-server.h
endif

AM_CFLAGS = $(DIX_CFLAGS)

EXTRA_DIST = 	\
	busfault.h dbus-core.h \
	dix-config-apple-verbatim.h \
	eventconvert.h eventstr.h inpututils.h \
	probes.h \
	protocol-versions.h \
	swaprep.h \
	swapreq.h \
	systemd-logind.h \
        vidmodestr.h \
	xorg-config.h.meson.in \
	xorg-server.h.meson.in \
	xwin-config.h.meson.in \
	xsha1.h \
	Xserver.d

if XSERVER_DTRACE
# Generate dtrace header file for C sources to include
BUILT_SOURCES = Xserver-dtrace.h

Xserver-dtrace.h: $(srcdir)/Xserver.d
	$(AM_V_GEN)$(DTRACE) -C -h -o $@ -s $(srcdir)/Xserver.d
endif

CLEANFILES = Xserver-dtrace.h
