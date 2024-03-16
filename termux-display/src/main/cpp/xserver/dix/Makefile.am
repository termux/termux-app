noinst_LTLIBRARIES = libdix.la libmain.la

AM_CPPFLAGS = -I$(top_srcdir)/include
AM_CFLAGS = $(DIX_CFLAGS)

libmain_la_SOURCES =    \
	stubmain.c

libdix_la_SOURCES = 	\
	atom.c		\
	colormap.c	\
	cursor.c	\
	devices.c	\
	dispatch.c	\
	dispatch.h	\
	dixfonts.c	\
	main.c		\
	dixutils.c	\
	enterleave.c	\
	enterleave.h	\
	events.c	\
	eventconvert.c  \
	extension.c	\
	gc.c		\
	getevents.c	\
	gestures.c	\
	globals.c	\
	glyphcurs.c	\
	grabs.c		\
	initatoms.c	\
	inpututils.c	\
	pixmap.c	\
	privates.c	\
	property.c	\
	ptrveloc.c	\
	region.c	\
	registry.c	\
	resource.c	\
	selection.c	\
	swaprep.c	\
	swapreq.c	\
	tables.c	\
	touch.c		\
	window.c

EXTRA_DIST = buildatoms BuiltInAtoms

# Install list of protocol names
miscconfigdir = $(SERVER_MISC_CONFIG_PATH)
dist_miscconfig_DATA = protocol.txt

if SPECIAL_DTRACE_OBJECTS
# Generate dtrace object code for probes in libdix
dtrace-dix.o: $(top_srcdir)/include/Xserver.d libdix.la
	$(AM_V_GEN)$(DTRACE) -G -C -o $@ -s $(top_srcdir)/include/Xserver.d $(am_libdix_la_OBJECTS:%.lo=.libs/%.o)

noinst_PROGRAMS = dix.O

dix_O_SOURCES =
dix.O: dtrace-dix.o libdix.la
	$(AM_V_GEN)ld -r -o $@ $(am_libdix_la_OBJECTS:%.lo=.libs/%.o)
endif
