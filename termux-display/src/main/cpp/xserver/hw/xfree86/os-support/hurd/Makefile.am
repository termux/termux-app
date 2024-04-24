noinst_LTLIBRARIES = libhurd.la

libhurd_la_SOURCES = hurd_bell.c hurd_init.c \
		hurd_video.c \
		$(srcdir)/../shared/VTsw_noop.c \
		$(srcdir)/../shared/posix_tty.c \
		$(srcdir)/../shared/vidmem.c \
		$(srcdir)/../shared/sigiostubs.c \
		$(srcdir)/../shared/pm_noop.c \
		$(srcdir)/../shared/kmod_noop.c \
		$(srcdir)/../shared/agp_noop.c

AM_CFLAGS = -DHAVE_SYSV_IPC $(XORG_CFLAGS) $(DIX_CFLAGS)

AM_CPPFLAGS = $(XORG_INCS)
