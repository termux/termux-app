noinst_LTLIBRARIES = libstub.la

AM_CFLAGS = $(XORG_CFLAGS) $(DIX_CFLAGS)

AM_CPPFLAGS = $(XORG_INCS)

libstub_la_SOURCES = \
	$(srcdir)/../shared/VTsw_noop.c \
	$(srcdir)/../shared/agp_noop.c \
	$(srcdir)/../shared/ioperm_noop.c \
	$(srcdir)/../shared/kmod_noop.c \
	$(srcdir)/../shared/pm_noop.c \
	$(srcdir)/../shared/vidmem.c \
	$(srcdir)/../shared/posix_tty.c \
	$(srcdir)/../shared/sigio.c \
	stub_bell.c \
	stub_init.c \
	stub_video.c
