noinst_LTLIBRARIES = libXwinGLX.la
lib_LTLIBRARIES = libnativeGLthunk.la

libXwinGLX_la_SOURCES = \
	winpriv.c \
	winpriv.h \
	glwindows.h \
	glshim.c \
	indirect.c \
	indirect.h \
	wgl_ext_api.c \
	wgl_ext_api.h

if XWIN_WINDOWS_DRI
libXwinGLX_la_SOURCES += \
	dri_helpers.c \
	dri_helpers.h
endif

libnativeGLthunk_la_SOURCES = \
	glthunk.c

if XWIN_GLX_WINDOWS
DEFS_GLX_WINDOWS = -DXWIN_GLX_WINDOWS
endif

DEFS = \
	$(DEFS_GLX_WINDOWS)

AM_CFLAGS = -DHAVE_XWIN_CONFIG_H $(DIX_CFLAGS) \
            $(XWINMODULES_CFLAGS) \
            -I$(top_srcdir) \
            -I$(top_srcdir)/hw/xwin/

libnativeGLthunk_la_CFLAGS = $(AM_CFLAGS) -Wno-unused-function -Wno-missing-prototypes -Wno-missing-declarations
libnativeGLthunk_la_LDFLAGS = -shared -no-undefined -avoid-version -lopengl32 -export-symbols generated_gl_thunks.def
EXTRA_libnativeGLthunk_la_DEPENDENCIES = generated_gl_thunks.def

if XWIN_GLX_WINDOWS

if DEBUG
GENGLWRAPPERSOPTS=""
else
GENGLWRAPPERSOPTS="-nodebug"
endif

generated_wgl_wrappers.ic: $(srcdir)/gen_gl_wrappers.py $(KHRONOS_SPEC_DIR)/wgl.xml $(KHRONOS_SPEC_DIR)/reg.py
	$(AM_V_GEN)PYTHONPATH=$(KHRONOS_SPEC_DIR) $(PYTHON3) $(srcdir)/gen_gl_wrappers.py -registry $(KHRONOS_SPEC_DIR)/wgl.xml -prefix wgl -wrapper -preresolve $(GENGLWRAPPERSOPTS) -outfile $@

generated_gl_shim.ic: $(srcdir)/gen_gl_wrappers.py $(KHRONOS_SPEC_DIR)/gl.xml $(KHRONOS_SPEC_DIR)/reg.py
	$(AM_V_GEN)PYTHONPATH=$(KHRONOS_SPEC_DIR) $(PYTHON3) $(srcdir)/gen_gl_wrappers.py -registry $(KHRONOS_SPEC_DIR)/gl.xml -shim $(GENGLWRAPPERSOPTS) -outfile $@

generated_gl_thunks.ic: $(srcdir)/gen_gl_wrappers.py $(KHRONOS_SPEC_DIR)/gl.xml $(KHRONOS_SPEC_DIR)/reg.py
	$(AM_V_GEN)PYTHONPATH=$(KHRONOS_SPEC_DIR) $(PYTHON3) $(srcdir)/gen_gl_wrappers.py -registry $(KHRONOS_SPEC_DIR)/gl.xml -thunk $(GENGLWRAPPERSOPTS) -outfile $@

generated_gl_thunks.def: $(srcdir)/gen_gl_wrappers.py $(KHRONOS_SPEC_DIR)/gl.xml $(KHRONOS_SPEC_DIR)/reg.py
	$(AM_V_GEN)PYTHONPATH=$(KHRONOS_SPEC_DIR) $(PYTHON3) $(srcdir)/gen_gl_wrappers.py -registry $(KHRONOS_SPEC_DIR)/gl.xml -thunkdefs $(GENGLWRAPPERSOPTS) -outfile $@

BUILT_SOURCES = generated_gl_shim.ic generated_gl_thunks.ic generated_gl_thunks.def generated_wgl_wrappers.ic
CLEANFILES = $(BUILT_SOURCES)

endif

EXTRA_DIST = gen_gl_wrappers.py
