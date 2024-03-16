EXTRA_DIST =		\
     parrot.c		\
     parrot.jpg		\
     scale.ui		\
     dither.ui		\
     meson.build	\
     $(NULL)

if HAVE_GTK

AM_CFLAGS = $(OPENMP_CFLAGS)
AM_LDFLAGS = $(OPENMP_CFLAGS)

LDADD = $(top_builddir)/pixman/libpixman-1.la -lm $(GTK_LIBS) $(PNG_LIBS)
AM_CPPFLAGS = -I$(top_srcdir)/pixman -I$(top_builddir)/pixman $(GTK_CFLAGS) $(PNG_CFLAGS)

GTK_UTILS = gtk-utils.c gtk-utils.h ../test/utils.c ../test/utils.h \
            ../test/utils-prng.c ../test/utils-prng.h

DEMOS =				\
	clip-test		\
	clip-in			\
	composite-test		\
	gradient-test		\
	radial-test		\
	linear-gradient		\
	conical-test		\
	alpha-test		\
	screen-test		\
	convolution-test	\
	trap-test		\
	tri-test		\
	quad2quad		\
	checkerboard		\
	srgb-trap-test		\
	srgb-test		\
	scale			\
	dither

gradient_test_SOURCES = gradient-test.c $(GTK_UTILS)
alpha_test_SOURCES = alpha-test.c $(GTK_UTILS)
composite_test_SOURCES = composite-test.c $(GTK_UTILS)
clip_test_SOURCES = clip-test.c $(GTK_UTILS)
clip_in_SOURCES = clip-in.c $(GTK_UTILS)
trap_test_SOURCES = trap-test.c $(GTK_UTILS)
screen_test_SOURCES = screen-test.c $(GTK_UTILS)
convolution_test_SOURCES = convolution-test.c $(GTK_UTILS)
radial_test_SOURCES = radial-test.c $(GTK_UTILS)
linear_gradient_SOURCES = linear-gradient.c $(GTK_UTILS)
conical_test_SOURCES = conical-test.c $(GTK_UTILS)
tri_test_SOURCES = tri-test.c $(GTK_UTILS)
checkerboard_SOURCES = checkerboard.c $(GTK_UTILS)
srgb_test_SOURCES = srgb-test.c $(GTK_UTILS)
srgb_trap_test_SOURCES = srgb-trap-test.c $(GTK_UTILS)
scale_SOURCES = scale.c $(GTK_UTILS)
dither_SOURCES = dither.c $(GTK_UTILS)

noinst_PROGRAMS = $(DEMOS)

endif
