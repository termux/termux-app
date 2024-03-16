SUBDIRS = pixman demos test

pkgconfigdir=$(libdir)/pkgconfig
pkgconfig_DATA=pixman-1.pc

$(pkgconfig_DATA): pixman-1.pc.in

snapshot:
	distdir="$(distdir)-`date '+%Y%m%d'`"; \
	test -d "$(srcdir)/.git" && distdir=$$distdir-`cd "$(srcdir)" && git rev-parse HEAD | cut -c 1-6`; \
	$(MAKE) $(AM_MAKEFLAGS) distdir="$$distdir" dist

GPGKEY=3892336E
USERNAME=$$USER
RELEASE_OR_SNAPSHOT = $$(if test "x$(PIXMAN_VERSION_MINOR)" = "x$$(echo "$(PIXMAN_VERSION_MINOR)/2*2" | bc)" ; then echo release; else echo snapshot; fi)
RELEASE_CAIRO_HOST =	$(USERNAME)@cairographics.org
RELEASE_CAIRO_DIR =	/srv/cairo.freedesktop.org/www/$(RELEASE_OR_SNAPSHOT)s
RELEASE_CAIRO_URL = 	https://cairographics.org/$(RELEASE_OR_SNAPSHOT)s
RELEASE_XORG_URL =	https://www.x.org/releases/individual/lib
RELEASE_XORG_HOST =	$(USERNAME)@xorg.freedesktop.org
RELEASE_XORG_DIR =	/srv/xorg.freedesktop.org/archive/individual/lib
RELEASE_ANNOUNCE_LIST = cairo-announce@cairographics.org, xorg-announce@lists.freedesktop.org, pixman@lists.freedesktop.org

EXTRA_DIST =				\
	Makefile.win32			\
	Makefile.win32.common		\
	meson.build			\
	meson_options.txt		\
	neon-test.S			\
	a64-neon-test.S			\
	arm-simd-test.S			\
	$(NULL)

tar_gz = $(PACKAGE)-$(VERSION).tar.gz
tar_xz = $(PACKAGE)-$(VERSION).tar.xz

sha512_tgz = $(tar_gz).sha512
sha256_tgz = $(tar_gz).sha256

sha512_txz = $(tar_xz).sha512
sha256_txz = $(tar_xz).sha256

gpg_file = $(sha512_tgz).asc

$(sha512_tgz): $(tar_gz)
	sha512sum $^ > $@

$(sha256_tgz): $(tar_gz)
	sha256sum $^ > $@

$(sha512_txz): $(tar_xz)
	sha512sum $^ > $@

$(sha256_txz): $(tar_xz)
	sha256sum $^ > $@

$(gpg_file): $(sha512_tgz)
	@echo "Please enter your GPG password to sign the checksum."
	gpg --armor --sign $^ 

HASHFILES = $(sha512_tgz) $(sha512_txz) $(sha256_tgz) $(sha256_txz)

release-verify-newer:
	@echo -n "Checking that no $(VERSION) release already exists at $(RELEASE_XORG_HOST)..."
	@ssh $(RELEASE_XORG_HOST) test ! -e $(RELEASE_XORG_DIR)/$(tar_gz) \
		|| (echo "Ouch." && echo "Found: $(RELEASE_XORG_HOST):$(RELEASE_XORG_DIR)/$(tar_gz)" \
		&& echo "Refusing to try to generate a new release of the same name." \
		&& false)
	@ssh $(RELEASE_CAIRO_HOST) test ! -e $(RELEASE_CAIRO_DIR)/$(tar_gz) \
		|| (echo "Ouch." && echo "Found: $(RELEASE_CAIRO_HOST):$(RELEASE_CAIRO_DIR)/$(tar_gz)" \
		&& echo "Refusing to try to generate a new release of the same name." \
		&& false)
	@echo "Good."

release-remove-old:
	$(RM) $(tar_gz) $(tar_xz) $(HASHFILES) $(gpg_file)

ensure-prev:
	@if [ "$(PREV)" = "" ]; then							\
		echo ""							          &&	\
		echo "You must set the PREV variable on the make command line to" &&	\
		echo "the last version."				  	  &&	\
		echo ""								  &&	\
		echo "For example:"						  &&	\
		echo "      make PREV=0.7.3"				  	  &&	\
		echo ""								  &&	\
		false;									\
	fi

release-check: ensure-prev release-verify-newer release-remove-old distcheck

release-tag:
	git tag -u $(GPGKEY) -m "$(PACKAGE) $(VERSION) release" $(PACKAGE)-$(VERSION)

release-upload: release-check $(tar_gz) $(tar_xz) $(sha512_tgz) $(sha512_txz) $(sha256_tgz) $(gpg_file)
	scp $(tar_gz) $(sha512_tgz) $(gpg_file) $(RELEASE_CAIRO_HOST):$(RELEASE_CAIRO_DIR)
	scp $(tar_gz) $(tar_xz) $(RELEASE_XORG_HOST):$(RELEASE_XORG_DIR)
	ssh $(RELEASE_CAIRO_HOST) "rm -f $(RELEASE_CAIRO_DIR)/LATEST-$(PACKAGE)-[0-9]* && ln -s $(tar_gz) $(RELEASE_CAIRO_DIR)/LATEST-$(PACKAGE)-$(VERSION)"

RELEASE_TYPE = $$(if test "x$(PIXMAN_VERSION_MINOR)" = "x$$(echo "$(PIXMAN_VERSION_MINOR)/2*2" | bc)" ; then echo "stable release in the" ; else echo "development snapshot leading up to a stable"; fi)

release-publish-message: $(HASHFILES) ensure-prev
	@echo "Please follow the instructions in RELEASING to push stuff out and"
	@echo "send out the announcement mails.  Here is the excerpt you need:"
	@echo ""
	@echo "Lists:  $(RELEASE_ANNOUNCE_LIST)"
	@echo "Subject: [ANNOUNCE] $(PACKAGE) release $(VERSION) now available"
	@echo "============================== CUT HERE =============================="
	@echo "A new $(PACKAGE) release $(VERSION) is now available. This is a $(RELEASE_TYPE)"
	@echo ""
	@echo "tar.gz:"
	@echo "	$(RELEASE_CAIRO_URL)/$(tar_gz)"
	@echo "	$(RELEASE_XORG_URL)/$(tar_gz)"
	@echo ""
	@echo "tar.xz:"
	@echo "	$(RELEASE_XORG_URL)/$(tar_xz)"
	@echo ""
	@echo "Hashes:"
	@echo -n "	SHA256: "
	@cat $(sha256_tgz)
	@echo -n "	SHA256: "
	@cat $(sha256_txz)
	@echo -n "	SHA512: "
	@cat $(sha512_tgz)
	@echo -n "	SHA512: "
	@cat $(sha512_txz)
	@echo ""
	@echo "GPG signature:"
	@echo "	$(RELEASE_CAIRO_URL)/$(gpg_file)"
	@echo "	(signed by`gpg --list-keys $(GPGKEY) | grep uid | cut -b4- | tr -s " "`)"
	@echo ""
	@echo "Git:"
	@echo "	https://gitlab.freedesktop.org/pixman/pixman.git"
	@echo "	tag: $(PACKAGE)-$(VERSION)"
	@echo ""
	@echo "Log:"
	@git log --no-merges "$(PACKAGE)-$(PREV)".."$(PACKAGE)-$(VERSION)" | git shortlog | awk '{ printf "\t"; print ; }' | cut -b1-80
	@echo "============================== CUT HERE =============================="
	@echo ""

release-publish: release-upload release-tag release-publish-message

.PHONY: release-upload release-publish release-publish-message release-tag
