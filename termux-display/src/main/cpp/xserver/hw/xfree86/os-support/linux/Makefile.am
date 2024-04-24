noinst_LTLIBRARIES = liblinux.la

if LNXACPI
ACPI_SRCS = lnx_acpi.c
if !LNXAPM
ACPI_SRCS += lnx_apm.c
endif
XORG_CFLAGS += -DHAVE_ACPI
endif

if LNXAPM
APM_SRCS = lnx_apm.c
XORG_CFLAGS += -DHAVE_APM
endif

if !LNXAPM
if !LNXACPI
ACPI_SRCS = $(srcdir)/../shared/pm_noop.c
endif
endif

if SYSTEMD_LOGIND
LOGIND_SRCS = systemd-logind.c
XORG_CFLAGS += $(DBUS_CFLAGS)
endif

if AGP
AGP_SRCS = lnx_agp.c
endif

liblinux_la_SOURCES = linux.h lnx_init.c lnx_video.c \
                      lnx_kmod.c lnx_bell.c lnx_platform.c \
		     $(srcdir)/../shared/VTsw_usl.c \
		     $(srcdir)/../shared/posix_tty.c \
                     $(srcdir)/../shared/vidmem.c \
		     $(srcdir)/../shared/sigio.c \
                     $(ACPI_SRCS) \
                     $(APM_SRCS) \
                     $(LOGIND_SRCS) \
		     $(AGP_SRCS)

AM_CFLAGS = -DHAVE_SYSV_IPC $(DIX_CFLAGS) $(XORG_CFLAGS) $(PLATFORM_DEFINES)

AM_CPPFLAGS = $(XORG_INCS) $(PLATFORM_INCLUDES) $(LIBDRM_CFLAGS)
