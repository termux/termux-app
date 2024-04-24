AUTOMAKE_OPTIONS=nostdinc

# Required for automake < 1.14
ACLOCAL_AMFLAGS = -I m4

if COMPOSITE
COMPOSITE_DIR=composite
endif

if GLX
GLX_DIR=glx
endif

if DBE
DBE_DIR=dbe
endif

if RECORD
RECORD_DIR=record
endif

if DRI3
DRI3_DIR=dri3
endif

if PRESENT
PRESENT_DIR=present
endif

if PSEUDORAMIX
PSEUDORAMIX_DIR=pseudoramiX
endif

if GLAMOR
GLAMOR_DIR=glamor
endif

SUBDIRS = \
	doc \
	man \
	include \
	dix  \
	fb \
	mi \
	Xext \
	miext \
	os \
	randr \
	render  \
	Xi \
	xkb \
	$(PSEUDORAMIX_DIR) \
	$(DBE_DIR) \
	$(RECORD_DIR) \
	xfixes \
	damageext \
	$(COMPOSITE_DIR) \
	$(GLX_DIR) \
	$(PRESENT_DIR) \
	$(DRI3_DIR) \
	exa \
	$(GLAMOR_DIR) \
	config \
	hw \
	test

if XORG
aclocaldir = $(datadir)/aclocal
aclocal_DATA = xorg-server.m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = xorg-server.pc
endif

EXTRA_DIST = xorg-server.pc.in xorg-server.m4 autogen.sh README.md

DISTCHECK_CONFIGURE_FLAGS=\
	--with-xkb-path=$(XKB_BASE_DIRECTORY) \
	--with-xkb-bin-directory=$(XKB_BIN_DIRECTORY) \
	--with-xkb-output='$${datadir}/X11/xkb/compiled'

.PHONY: ChangeLog INSTALL

INSTALL:
	$(INSTALL_CMD)

ChangeLog:
	$(CHANGELOG_CMD)

dist-hook: ChangeLog INSTALL

DIST_SUBDIRS = \
	doc \
	man \
	include \
	dix  \
	fb \
	mi \
	Xext \
	miext \
	os \
	pseudoramiX \
	randr \
	render  \
	Xi \
	xkb \
	dbe \
	record \
	xfixes \
	damageext \
	composite \
	glx \
	exa \
	glamor \
	config \
	dri3 \
	present \
	hw \
	test

# gross hack
relink: all
	$(AM_V_at)$(MAKE) -C hw relink

install-headers: Makefile
	+find . -name Makefile | while read m; do			\
		if grep -q install-sdkHEADERS $$m; then			\
			(cd `dirname "$$m"` && make install-sdkHEADERS)	\
		fi							\
	done

distcheck-hook:
	cd $(srcdir) && \
	meson setup _distcheck_build && \
	meson configure _distcheck_build && \
	ninja -C _distcheck_build && \
	rm -rf _distcheck_build && \
	cd -

EXTRA_DIST += \
	meson_options.txt \
	include/xorg-config.h.meson.in \
	include/xwin-config.h.meson.in \
	hw/xfree86/loader/symbol-test.c \
	composite/meson.build \
	config/meson.build \
	damageext/meson.build \
	dbe/meson.build \
	dix/meson.build \
	doc/dtrace/meson.build \
	doc/meson.build \
	dri3/meson.build \
	exa/meson.build \
	fb/meson.build \
	glamor/meson.build \
	glx/meson.build \
	hw/kdrive/ephyr/meson.build \
	hw/kdrive/meson.build \
	hw/kdrive/src/meson.build \
	hw/meson.build \
	hw/vfb/meson.build \
	hw/xfree86/common/meson.build \
	hw/xfree86/ddc/meson.build \
	hw/xfree86/dixmods/meson.build \
	hw/xfree86/doc/meson.build \
	hw/xfree86/dri2/meson.build \
	hw/xfree86/dri/meson.build \
	hw/xfree86/drivers/inputtest/meson.build \
	hw/xfree86/drivers/modesetting/meson.build \
	hw/xfree86/exa/meson.build \
	hw/xfree86/fbdevhw/meson.build \
	hw/xfree86/glamor_egl/meson.build \
	hw/xfree86/i2c/meson.build \
	hw/xfree86/int10/meson.build \
	hw/xfree86/loader/meson.build \
	hw/xfree86/meson.build \
	hw/xfree86/modes/meson.build \
	hw/xfree86/os-support/meson.build \
	hw/xfree86/parser/meson.build \
	hw/xfree86/ramdac/meson.build \
	hw/xfree86/shadowfb/meson.build \
	hw/xfree86/vgahw/meson.build \
	hw/xfree86/x86emu/meson.build \
	hw/xfree86/xkb/meson.build \
	hw/xnest/meson.build \
	hw/xquartz/bundle/meson.build \
	hw/xquartz/GL/meson.build \
	hw/xquartz/mach-startup/meson.build \
	hw/xquartz/meson.build \
	hw/xquartz/pbproxy/meson.build \
	hw/xquartz/xpr/meson.build \
	hw/xwin/dri/meson.build \
	hw/xwin/glx/meson.build \
	hw/xwin/meson.build \
	hw/xwin/winclipboard/meson.build \
	include/meson.build \
	meson.build \
	miext/damage/meson.build \
	miext/rootless/meson.build \
	miext/shadow/meson.build \
	miext/sync/meson.build \
	mi/meson.build \
	os/meson.build \
	present/meson.build \
	pseudoramiX/meson.build \
	randr/meson.build \
	record/meson.build \
	render/meson.build \
	test/bigreq/meson.build \
	test/bigreq/request-length.c \
	test/damage/meson.build \
	test/damage/primitives.c \
	test/meson.build \
	test/sync/meson.build \
	test/sync/sync.c \
	Xext/meson.build \
	xfixes/meson.build \
	Xi/meson.build \
	xkb/meson.build

DISTCLEANFILES = buildDateTime.h
