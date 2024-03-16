if ENABLE_UNIT_TESTS
SUBDIRS= .
AM_CFLAGS = $(DIX_CFLAGS) @XORG_CFLAGS@
AM_CPPFLAGS = $(XORG_INCS)

tests_CPPFLAGS=
CLEANFILES=

tests_SOURCES = \
        tests-common.c \
	tests-common.h \
        list.c \
        string.c \
        tests.c \
	tests.h

noinst_PROGRAMS = simple-xinit tests

if XVFB
XVFB_TESTS = scripts/xvfb-piglit.sh
if XEPHYR
if GLAMOR
XEPHYR_GLAMOR_TESTS = scripts/xephyr-glamor-piglit.sh
endif
endif
endif

SCRIPT_TESTS = \
	$(XVFB_TESTS) \
	$(XEPHYR_GLAMOR_TESTS) \
	$(NULL)

TESTS = tests \
	$(SCRIPT_TESTS) \
	$(NULL)

TESTS_ENVIRONMENT = \
	XSERVER_DIR=$(abs_top_srcdir) \
	XSERVER_BUILDDIR=$(abs_top_builddir) \
	$(XORG_MALLOC_DEBUG_ENV) \
	$(NULL)

tests_LDADD =

if XORG

# Tests that require at least some DDX functions in order to fully link
# For now, requires xf86 ddx, could be adjusted to use another

AM_CPPFLAGS += \
	-I$(srcdir)/xi1 \
	-I$(srcdir)/xi2 \
	-I$(top_srcdir)/hw/xfree86/common \
	-I$(top_srcdir)/hw/xfree86/parser \
	-I$(top_srcdir)/hw/xfree86/ddc \
	-I$(top_srcdir)/hw/xfree86/i2c -I$(top_srcdir)/hw/xfree86/modes \
	-I$(top_srcdir)/hw/xfree86/ramdac -I$(top_srcdir)/hw/xfree86/dri \
	-I$(top_srcdir)/hw/xfree86/dri2 -I$(top_srcdir)/dri3
tests_CPPFLAGS += $(AM_CPPFLAGS)

tests_SOURCES += \
        fixes.c \
        input.c \
        misc.c \
        signal-logging.c \
        touch.c \
        xfree86.c \
        test_xkb.c \
        xtest.c
tests_CPPFLAGS += -DXORG_TESTS

if RES
tests_SOURCES += hashtabletest.c
tests_CPPFLAGS += -DRES_TESTS
endif

endif XORG

if HAVE_LD_WRAP

tests_CPPFLAGS += -DLDWRAP_TESTS

if XORG
tests_SOURCES += \
	xi1/protocol-xchangedevicecontrol.c \
	xi2/protocol-common.c \
	xi2/protocol-xiqueryversion.c \
	xi2/protocol-xiquerydevice.c \
	xi2/protocol-xiselectevents.c \
	xi2/protocol-xigetselectedevents.c \
	xi2/protocol-xisetclientpointer.c \
	xi2/protocol-xigetclientpointer.c \
	xi2/protocol-xiquerypointer.c \
	xi2/protocol-xipassivegrabdevice.c \
	xi2/protocol-xiwarppointer.c \
	xi2/protocol-eventconvert.c \
	xi2/xi2.c \
	xi2/protocol-common.h

tests_LDFLAGS = \
	-Wl,-wrap,dixLookupWindow \
	-Wl,-wrap,dixLookupClient \
	-Wl,-wrap,WriteToClient \
	-Wl,-wrap,dixLookupWindow \
	-Wl,-wrap,XISetEventMask \
	-Wl,-wrap,AddResource \
	-Wl,-wrap,GrabButton \
	$()
endif XORG

else !HAVE_LD_WRAP

# Print that xi1-tests were skipped (exit code 77 for automake test harness)
TESTS += xi1-tests
CLEANFILES += xi1-tests

xi1-tests:
	@echo 'echo "ld -wrap support required for xi1 unit tests, skipping"' > $@
	@echo 'exit 77' >> $@
	$(AM_V_GEN)chmod +x $@

# Print that xi2-tests were skipped (exit code 77 for automake test harness)
TESTS += xi2-tests
CLEANFILES += xi2-tests

xi2-tests:
	@echo 'echo "ld -wrap support required for xi2 unit tests, skipping"' > $@
	@echo 'exit 77' >> $@
	$(AM_V_GEN)chmod +x $@

endif !HAVE_LD_WRAP

if XORG

nodist_tests_SOURCES = sdksyms.c

tests_LDADD += \
            $(top_builddir)/hw/xfree86/loader/libloader.la \
            $(top_builddir)/hw/xfree86/common/libcommon.la \
            $(top_builddir)/hw/xfree86/os-support/libxorgos.la \
            $(top_builddir)/hw/xfree86/parser/libxf86config.la \
            $(top_builddir)/hw/xfree86/dixmods/libdixmods.la \
            $(top_builddir)/hw/xfree86/modes/libxf86modes.la \
            $(top_builddir)/hw/xfree86/ramdac/libramdac.la \
            $(top_builddir)/hw/xfree86/ddc/libddc.la \
            $(top_builddir)/hw/xfree86/i2c/libi2c.la \
            $(top_builddir)/hw/xfree86/xkb/libxorgxkb.la \
            $(top_builddir)/Xext/libXvidmode.la \
            $(top_builddir)/fb/libfb.la \
            $(XSERVER_LIBS) \
            $(XORG_LIBS)

if !SPECIAL_DTRACE_OBJECTS
tests_LDADD += $(top_builddir)/os/libos.la
endif

if GLX
tests_LDADD += $(top_builddir)/glx/libglxvnd.la
endif

BUILT_SOURCES = sdksyms.c
CLEANFILES += sdksyms.c

sdksyms.c: $(top_builddir)/hw/xfree86/sdksyms.c
	$(AM_V_GEN)$(LN_S) $(top_builddir)/hw/xfree86/sdksyms.c

if DRI
tests_LDADD += $(top_builddir)/hw/xfree86/dri/libdri.la
endif

if DRI2
tests_LDADD += $(top_builddir)/hw/xfree86/dri2/libdri2.la
endif

if DRI3
tests_LDADD += $(top_builddir)/dri3/libdri3.la
endif

endif XORG

# GNU LD scans only in one direction, add the following dependencies at the end
# so as they get picked up by the previously-linked libraries
tests_LDADD += $(XORG_SYS_LIBS) $(XSERVER_SYS_LIBS) $(GLX_SYS_LIBS)

endif ENABLE_UNIT_TESTS

EXTRA_DIST = \
	scripts/xvfb-piglit.sh \
	scripts/xephyr-glamor-piglit.sh \
	scripts/xinit-piglit-session.sh \
	scripts/run-piglit.sh \
	$(NULL)

