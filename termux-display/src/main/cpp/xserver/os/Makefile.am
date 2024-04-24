noinst_LTLIBRARIES = libos.la

AM_CFLAGS = $(DIX_CFLAGS) $(SHA1_CFLAGS)

SECURERPC_SRCS = rpcauth.c
XDMCP_SRCS = xdmcp.c
XORG_SRCS = log.c
BUSFAULT_SRCS = busfault.c

XSERVER_POLL_SRCS=xserver_poll.c

libos_la_SOURCES = 	\
	WaitFor.c	\
	access.c	\
	auth.c		\
	backtrace.c	\
	client.c	\
	connection.c	\
	inputthread.c	\
	io.c		\
	mitauth.c	\
	oscolor.c	\
	osdep.h		\
	osinit.c	\
	ospoll.c	\
	ospoll.h	\
	utils.c		\
	xdmauth.c	\
	xsha1.c		\
	xstrans.c	\
	xprintf.c	\
	$(XORG_SRCS)
libos_la_LIBADD = @SHA1_LIBS@ $(DLOPEN_LIBS) $(LTLIBOBJS)

if SECURE_RPC
libos_la_SOURCES += $(SECURERPC_SRCS)
endif

if POLL
else
libos_la_SOURCES += $(XSERVER_POLL_SRCS)
endif

if XDMCP
libos_la_SOURCES += $(XDMCP_SRCS)
endif

if HAVE_LIBUNWIND
AM_CFLAGS += $(LIBUNWIND_CFLAGS)
libos_la_LIBADD += $(LIBUNWIND_LIBS)
endif

if BUSFAULT
libos_la_SOURCES += $(BUSFAULT_SRCS)
endif

EXTRA_DIST = $(SECURERPC_SRCS) $(XDMCP_SRCS)

if SPECIAL_DTRACE_OBJECTS
# Generate dtrace object code for probes in libos & libdix
dtrace.o: $(top_srcdir)/include/Xserver.d libos.la
	$(AM_V_GEN)$(DTRACE) -G -C -o $@ -s $(top_srcdir)/include/Xserver.d .libs/*.o ../dix/.libs/*.o

noinst_PROGRAMS = os.O

os_O_SOURCES =
os.O: dtrace.o libos.la
	$(AM_V_GEN)ld -r -o $@ dtrace.o .libs/*.o
endif
