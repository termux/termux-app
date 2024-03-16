SUBDIRS = man

module_LTLIBRARIES = libexa.la

libexa_la_LDFLAGS = -module -avoid-version $(LD_NO_UNDEFINED_FLAG)

AM_CPPFLAGS = \
	$(XORG_INCS) \
	-I$(srcdir)/../../../exa

AM_CFLAGS = $(DIX_CFLAGS) $(XORG_CFLAGS)

libexa_la_SOURCES = \
	examodule.c

libexa_la_LIBADD = \
	../../../exa/libexa.la $(PIXMAN_LIBS)
