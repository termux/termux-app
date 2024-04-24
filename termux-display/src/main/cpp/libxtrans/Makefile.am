SUBDIRS=doc

Xtransincludedir = $(includedir)/X11/Xtrans
Xtransinclude_HEADERS = \
	Xtrans.h \
	Xtrans.c \
	Xtransint.h \
	Xtranslcl.c \
	Xtranssock.c \
	Xtransutil.c \
	transport.c

aclocaldir = $(datadir)/aclocal
aclocal_DATA = xtrans.m4

pkgconfigdir = $(datadir)/pkgconfig
pkgconfig_DATA = xtrans.pc

MAINTAINERCLEANFILES = ChangeLog INSTALL
EXTRA_DIST = ${aclocal_DATA} README.md

.PHONY: ChangeLog INSTALL

INSTALL:
	$(INSTALL_CMD)

ChangeLog:
	$(CHANGELOG_CMD)

dist-hook: ChangeLog INSTALL
