SUBDIRS=man

lib_LTLIBRARIES = libXau.la

AM_CFLAGS = $(XAU_CFLAGS) $(CWARNFLAGS)
AM_CPPFLAGS = -I${top_srcdir}/include

libXau_la_LDFLAGS = -version-number 6:0:0 -no-undefined

libXau_la_LIBADD = $(XAU_LIBS)

libXau_la_SOURCES = \
	AuDispose.c \
	AuFileName.c \
	AuGetAddr.c \
	AuGetBest.c \
	AuLock.c \
	AuRead.c \
	AuUnlock.c \
	AuWrite.c

xauincludedir=$(includedir)/X11

xauinclude_HEADERS = include/X11/Xauth.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = xau.pc

check_PROGRAMS=Autest
TESTS=Autest

Autest_SOURCES=Autest.c
Autest_LDADD=libXau.la

if LINT
ALL_LINT_FLAGS=$(LINT_FLAGS) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) \
		$(AM_CPPFLAGS) $(CPPFLAGS)

lint:
	$(LINT) $(ALL_LINT_FLAGS) $(libXau_la_SOURCES)
endif LINT

if MAKE_LINT_LIB
lintlibdir = $(libdir)

lintlib_DATA = $(LINTLIB)

$(LINTLIB): $(libXau_la_SOURCES)
	$(LINT) -y -oXau -x $(ALL_LINT_FLAGS) $(libXau_la_SOURCES)
endif MAKE_LINT_LIB

CLEANFILES = $(lintlib_DATA)
MAINTAINERCLEANFILES = ChangeLog INSTALL

.PHONY: ChangeLog INSTALL

INSTALL:
	$(INSTALL_CMD)

ChangeLog:
	$(CHANGELOG_CMD)

dist-hook: ChangeLog INSTALL
