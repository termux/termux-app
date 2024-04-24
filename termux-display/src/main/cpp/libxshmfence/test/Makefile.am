check_PROGRAMS = xshmfence_test

TESTS=$(check_PROGRAMS)

xshmfence_test_SOURCES = xshmfence_test.c

xshmfence_test_CFLAGS = -I$(top_srcdir)/src
xshmfence_test_LDADD = $(top_builddir)/src/libxshmfence.la @PTHREAD_LIBS@

