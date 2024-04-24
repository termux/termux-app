AM_CFLAGS = $(DIX_CFLAGS)

noinst_LTLIBRARIES = libconfig.la
libconfig_la_SOURCES = config.c config-backends.h
libconfig_la_LIBADD =

if NEED_DBUS
AM_CFLAGS += $(DBUS_CFLAGS)
libconfig_la_SOURCES += dbus-core.c
libconfig_la_LIBADD += $(DBUS_LIBS)
endif

if CONFIG_UDEV

AM_CFLAGS += $(UDEV_CFLAGS)
libconfig_la_SOURCES += udev.c
libconfig_la_LIBADD += $(UDEV_LIBS)

if XORG
xorgconfddir = $(datadir)/X11/$(XF86CONFIGDIR)
xorgconfd_DATA = 10-quirks.conf
endif

else

if CONFIG_HAL
AM_CFLAGS += $(HAL_CFLAGS)
libconfig_la_SOURCES += hal.c
libconfig_la_LIBADD += $(HAL_LIBS)

else

if CONFIG_WSCONS
libconfig_la_SOURCES += wscons.c
endif # CONFIG_WSCONS

endif # !CONFIG_HAL

endif # !CONFIG_UDEV

EXTRA_DIST = x11-input.fdi fdi2iclass.py 10-quirks.conf
