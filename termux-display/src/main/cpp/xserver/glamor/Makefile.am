noinst_LTLIBRARIES = libglamor.la libglamor_egl_stubs.la

libglamor_la_LIBADD = $(GLAMOR_LIBS)

AM_CFLAGS = $(CWARNFLAGS) $(DIX_CFLAGS) $(GLAMOR_CFLAGS)

libglamor_la_SOURCES = \
	glamor.c \
	glamor_context.h \
	glamor_copy.c \
	glamor_core.c \
	glamor_dash.c \
	glamor_debug.h \
	glamor_font.c \
	glamor_font.h \
	glamor_glx.c \
	glamor_composite_glyphs.c \
	glamor_image.c \
	glamor_lines.c \
	glamor_segs.c \
	glamor_render.c \
	glamor_gradient.c \
	glamor_prepare.c \
	glamor_prepare.h \
	glamor_program.c \
	glamor_program.h \
	glamor_rects.c \
	glamor_spans.c \
	glamor_text.c \
	glamor_transfer.c \
	glamor_transfer.h \
	glamor_transform.c \
	glamor_transform.h \
	glamor_trapezoid.c \
	glamor_triangles.c\
	glamor_addtraps.c\
	glamor_glyphblt.c\
	glamor_points.c\
	glamor_priv.h\
	glamor_pixmap.c\
	glamor_largepixmap.c\
	glamor_picture.c\
	glamor_vbo.c \
	glamor_window.c\
	glamor_fbo.c\
	glamor_compositerects.c\
	glamor_utils.c\
	glamor_utils.h\
	glamor_sync.c \
	glamor.h

if XV
libglamor_la_SOURCES += \
	glamor_xv.c
endif

libglamor_egl_stubs_la_SOURCES = \
	glamor_egl_stubs.c \
	glamor_egl_ext.h \
	glamor_egl.h

sdk_HEADERS = glamor.h
