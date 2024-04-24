if XLIB_LOADABLE_I18N
ORDER=src modules
else
ORDER=modules src
endif
# Order: nls before specs
SUBDIRS=include $(ORDER) nls man specs

ACLOCAL_AMFLAGS = -I m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = x11.pc x11-xcb.pc

MAINTAINERCLEANFILES = ChangeLog INSTALL

.PHONY: ChangeLog INSTALL

INSTALL:
	$(INSTALL_CMD)

ChangeLog:
	$(CHANGELOG_CMD)

dist-hook: ChangeLog INSTALL

if LINT
# Check source code with tools like lint & sparse
LINT_SUBDIRS=src # modules

lint:
	@for subdir in $(LINT_SUBDIRS) ; do \
		echo "Making $@ in $$subdir"; \
		(cd $$subdir && $(MAKE) $(AM_MAKEFLAGS) lint) ; \
	done
endif LINT

EXTRA_DIST = README.md
