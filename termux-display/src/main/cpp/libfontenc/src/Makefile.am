lib_LTLIBRARIES = libfontenc.la

libfontenc_la_SOURCES = \
         encparse.c \
         fontenc.c \
         fontencI.h \
         reallocarray.h

AM_CFLAGS = \
	$(FONTENC_CFLAGS) \
	$(CWARNFLAGS)

FONTENCDIR=@ENCODINGSDIR@
FONTENCDEFS = -DFONT_ENCODINGS_DIRECTORY=\"$(FONTENCDIR)/encodings.dir\"

AM_CPPFLAGS = -I$(top_srcdir)/include $(FONTENCDEFS)


libfontenc_la_LIBADD = $(LTLIBOBJS) @FONTENC_LIBS@

libfontenc_la_LDFLAGS = -version-number 1:0:0 -no-undefined

libfontencincludedir = $(includedir)/X11/fonts
libfontencinclude_HEADERS = \
    $(top_srcdir)/include/X11/fonts/fontenc.h

if LINT
ALL_LINT_FLAGS=$(LINT_FLAGS) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) \
		$(AM_CPPFLAGS) $(CPPFLAGS)

lint:
	$(LINT) $(ALL_LINT_FLAGS) $(libfontenc_la_SOURCES) $(libfontenc_la_LIBADD) $(LIBS)
endif LINT
