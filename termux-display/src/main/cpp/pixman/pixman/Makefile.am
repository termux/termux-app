include $(top_srcdir)/pixman/Makefile.sources

lib_LTLIBRARIES = libpixman-1.la

libpixman_1_la_LDFLAGS = -version-info $(LT_VERSION_INFO) -no-undefined @PTHREAD_LDFLAGS@ 
libpixman_1_la_LIBADD = @PTHREAD_LIBS@ -lm
libpixman_1_la_SOURCES = $(libpixman_sources) $(libpixman_headers)

libpixmanincludedir = $(includedir)/pixman-1
libpixmaninclude_HEADERS = pixman.h pixman-version.h
noinst_LTLIBRARIES = 

EXTRA_DIST =				\
	Makefile.win32			\
	dither/make-blue-noise.c	\
	pixman-region.c			\
	solaris-hwcap.mapfile		\
	meson.build			\
	$(NULL)

# mmx code
if USE_X86_MMX
noinst_LTLIBRARIES += libpixman-mmx.la
libpixman_mmx_la_SOURCES = \
	pixman-mmx.c
libpixman_mmx_la_CFLAGS = $(MMX_CFLAGS)
libpixman_1_la_LDFLAGS += $(MMX_LDFLAGS)
libpixman_1_la_LIBADD += libpixman-mmx.la

ASM_CFLAGS_mmx=$(MMX_CFLAGS)
endif

# vmx code
if USE_VMX
noinst_LTLIBRARIES += libpixman-vmx.la
libpixman_vmx_la_SOURCES = \
	pixman-vmx.c \
	pixman-combine32.h
libpixman_vmx_la_CFLAGS = $(VMX_CFLAGS)
libpixman_1_la_LIBADD += libpixman-vmx.la

ASM_CFLAGS_vmx=$(VMX_CFLAGS)
endif

# sse2 code
if USE_SSE2
noinst_LTLIBRARIES += libpixman-sse2.la
libpixman_sse2_la_SOURCES = \
	pixman-sse2.c
libpixman_sse2_la_CFLAGS = $(SSE2_CFLAGS)
libpixman_1_la_LDFLAGS += $(SSE2_LDFLAGS)
libpixman_1_la_LIBADD += libpixman-sse2.la

ASM_CFLAGS_sse2=$(SSE2_CFLAGS)
endif

# ssse3 code
if USE_SSSE3
noinst_LTLIBRARIES += libpixman-ssse3.la
libpixman_ssse3_la_SOURCES = \
	pixman-ssse3.c
libpixman_ssse3_la_CFLAGS = $(SSSE3_CFLAGS)
libpixman_1_la_LDFLAGS += $(SSSE3_LDFLAGS)
libpixman_1_la_LIBADD += libpixman-ssse3.la

ASM_CFLAGS_ssse3=$(SSSE3_CFLAGS)
endif

# arm simd code
if USE_ARM_SIMD
noinst_LTLIBRARIES += libpixman-arm-simd.la
libpixman_arm_simd_la_SOURCES = \
	pixman-arm-simd.c	\
	pixman-arm-common.h	\
	pixman-arm-simd-asm.S   \
	pixman-arm-simd-asm-scaled.S \
	pixman-arm-asm.h	\
	pixman-arm-simd-asm.h
libpixman_1_la_LIBADD += libpixman-arm-simd.la

ASM_CFLAGS_arm_simd=
endif

# arm neon code
if USE_ARM_NEON
noinst_LTLIBRARIES += libpixman-arm-neon.la
libpixman_arm_neon_la_SOURCES = \
        pixman-arm-neon.c	\
        pixman-arm-common.h	\
        pixman-arm-neon-asm.S	\
		pixman-arm-neon-asm-bilinear.S \
        pixman-arm-asm.h	\
        pixman-arm-neon-asm.h
libpixman_1_la_LIBADD += libpixman-arm-neon.la

ASM_CFLAGS_arm_neon=
endif

# arm a64 neon code
if USE_ARM_A64_NEON
noinst_LTLIBRARIES += libpixman-arma64-neon.la
libpixman_arma64_neon_la_SOURCES = \
        pixman-arm-neon.c        \
        pixman-arm-common.h      \
        pixman-arma64-neon-asm.S \
        pixman-arma64-neon-asm-bilinear.S \
        pixman-arm-asm.h         \
        pixman-arma64-neon-asm.h
libpixman_1_la_LIBADD += libpixman-arma64-neon.la

ASM_CFLAGS_arm_neon=
endif

# iwmmxt code
if USE_ARM_IWMMXT
libpixman_iwmmxt_la_SOURCES = pixman-mmx.c
noinst_LTLIBRARIES += libpixman-iwmmxt.la
libpixman_1_la_LIBADD += libpixman-iwmmxt.la

libpixman_iwmmxt_la-pixman-mmx.lo: pixman-mmx.c
	$(AM_V_CC)$(LIBTOOL) $(AM_V_lt) --tag=CC $(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=compile $(CC) $(DEFS) $(DEFAULT_INCLUDES) $(AM_CPPFLAGS) $(AM_CPPFLAGS) $(CPPFLAGS) $(CFLAGS) $(IWMMXT_CFLAGS) -MT libpixman_iwmmxt_la-pixman-mmx.lo -MD -MP -MF $(DEPDIR)/libpixman_iwmmxt_la-pixman-mmx.Tpo -c -o libpixman_iwmmxt_la-pixman-mmx.lo `test -f 'pixman-mmx.c' || echo '$(srcdir)/'`pixman-mmx.c
	$(AM_V_at)$(am__mv) $(DEPDIR)/libpixman_iwmmxt_la-pixman-mmx.Tpo $(DEPDIR)/libpixman_iwmmxt_la-pixman-mmx.Plo

libpixman_iwmmxt_la_DEPENDENCIES = $(am__DEPENDENCIES_1)
libpixman_iwmmxt_la_LINK = $(LIBTOOL) $(AM_V_lt) --tag=CC \
        $(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=link $(CCLD) \
	$(CFLAGS) $(IWMMXT_CFLAGS) $(AM_LDFLAGS) \
	$(LDFLAGS) -o $@

libpixman-iwmmxt.la: libpixman_iwmmxt_la-pixman-mmx.lo $(libpixman_iwmmxt_la_DEPENDENCIES) 
	$(AM_V_CCLD)$(libpixman_iwmmxt_la_LINK) libpixman_iwmmxt_la-pixman-mmx.lo $(libpixman_iwmmxt_la_LIBADD) $(LIBS)
endif

# mips dspr2 code
if USE_MIPS_DSPR2
noinst_LTLIBRARIES += libpixman-mips-dspr2.la
libpixman_mips_dspr2_la_SOURCES = \
        pixman-mips-dspr2.c \
        pixman-mips-dspr2.h \
        pixman-mips-dspr2-asm.S \
        pixman-mips-dspr2-asm.h \
        pixman-mips-memcpy-asm.S
libpixman_1_la_LIBADD += libpixman-mips-dspr2.la

ASM_CFLAGS_mips_dspr2=
endif

# loongson code
if USE_LOONGSON_MMI
noinst_LTLIBRARIES += libpixman-loongson-mmi.la
libpixman_loongson_mmi_la_SOURCES = pixman-mmx.c loongson-mmintrin.h
libpixman_loongson_mmi_la_CFLAGS = $(LS_CFLAGS)
libpixman_1_la_LDFLAGS += $(LS_LDFLAGS)
libpixman_1_la_LIBADD += libpixman-loongson-mmi.la
endif

.c.s : $(libpixmaninclude_HEADERS)
	$(CC) $(CFLAGS) $(ASM_CFLAGS_$(@:pixman-%.s=%)) $(ASM_CFLAGS_$(@:pixman-arm-%.s=arm_%)) -DHAVE_CONFIG_H -I$(srcdir) -I$(builddir) -I$(top_builddir) -S -o $@ $<
