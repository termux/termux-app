SUBDIRS=pci_ids

noinst_LTLIBRARIES = libdri2.la
AM_CFLAGS = \
	-DHAVE_XORG_CONFIG_H \
	@DIX_CFLAGS@ @XORG_CFLAGS@ \
	-I$(top_srcdir)/hw/xfree86/common \
	-I$(top_srcdir)/hw/xfree86/os-support/bus

libdri2_la_SOURCES = \
	dri2.c \
	dri2.h \
	dri2ext.c \
	dri2int.h

sdk_HEADERS = dri2.h
