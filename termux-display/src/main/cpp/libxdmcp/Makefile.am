SUBDIRS=doc . test

lib_LTLIBRARIES = libXdmcp.la

AM_CPPFLAGS = -I${top_srcdir}/include
AM_CFLAGS = \
	$(XDMCP_CFLAGS) \
	$(CWARNFLAGS)

libXdmcp_la_LDFLAGS = -version-number 6:0:0 -no-undefined
libXdmcp_la_LIBADD = $(XDMCP_LIBS)
libXdmcp_la_SOURCES = \
	Array.c \
	Fill.c \
	Flush.c \
	Key.c \
	Read.c \
	Unwrap.c \
	Wrap.c \
	Wrap.h \
	Write.c

if HASXDMAUTH
nodist_libXdmcp_la_SOURCES = Wraphelp.c
endif HASXDMAUTH

xdmcpincludedir=$(includedir)/X11

xdmcpinclude_HEADERS = \
	include/X11/Xdmcp.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = xdmcp.pc

MAINTAINERCLEANFILES = ChangeLog INSTALL
EXTRA_DIST = autogen.sh Wraphelp.c Wraphelp.README.crypto README.md

if LINT
ALL_LINT_FLAGS=$(LINT_FLAGS) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) \
		$(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS)

lint:
	$(LINT) $(ALL_LINT_FLAGS) $(libXdmcp_la_SOURCES) $(nodist_libXdmcp_la_SOURCES) $(LIBS) $(XDMCP_LIBS)
endif LINT

if MAKE_LINT_LIB
lintlibdir = $(libdir)

lintlib_DATA = $(LINTLIB)

$(LINTLIB): $(libXdmcp_la_SOURCES) $(nodist_libXdmcp_la_SOURCES)
	$(LINT) -y -oXdmcp -x $(ALL_LINT_FLAGS) $(libXdmcp_la_SOURCES) $(nodist_libXdmcp_la_SOURCES)
endif MAKE_LINT_LIB


.PHONY: ChangeLog INSTALL

INSTALL:
	$(INSTALL_CMD)

ChangeLog:
	$(CHANGELOG_CMD)

dist-hook: ChangeLog INSTALL

CLEANFILES=$(lintlib_DATA)
