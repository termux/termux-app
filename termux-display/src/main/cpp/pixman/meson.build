# Copyright © 2018 Intel Corporation

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

project(
  'pixman',
  ['c'],
  version : '0.42.2',
  license : 'MIT',
  meson_version : '>= 0.52.0',
  default_options : ['c_std=gnu99', 'buildtype=debugoptimized'],
)

config = configuration_data()
cc = meson.get_compiler('c')
null_dep = dependency('', required : false)

add_project_arguments(
  cc.get_supported_arguments([
    '-Wdeclaration-after-statement',
    '-fno-strict-aliasing',
    '-fvisibility=hidden',
    '-Wundef',
    # -ftrapping-math is the default for gcc, but -fno-trapping-math is the
    # default for clang.  The FLOAT_IS_ZERO macro is used to guard against
    # floating-point exceptions, however with -fno-trapping-math, the compiler
    # can reorder floating-point operations so that they occur before the guard.
    # Note, this function is ignored in clang < 10.0.0.
    '-ftrapping-math'
  ]),
  language : ['c']
)

# GCC and Clang both ignore -Wno options that they don't recognize, so test for
# -W<opt>, then add -Wno-<opt> if it's ignored
foreach opt : ['unused-local-typedefs']
  if cc.has_argument('-W' + opt)
    add_project_arguments(['-Wno-' + opt], language : ['c'])
  endif
endforeach

use_loongson_mmi = get_option('loongson-mmi')
have_loongson_mmi = false
loongson_mmi_flags = ['-mloongson-mmi']
if not use_loongson_mmi.disabled()
  if host_machine.cpu_family() == 'mips64' and cc.compiles('''
      #ifndef __mips_loongson_vector_rev
      #error "Loongson Multimedia Instructions are only available on Loongson"
      #endif
      #if defined(__GNUC__) && (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 4))
      #error "Need GCC >= 4.4 for Loongson MMI compilation"
      #endif
      #include "pixman/loongson-mmintrin.h"
      int main () {
        union {
          __m64 v;
          char c[8];
        } a = { .c = {1, 2, 3, 4, 5, 6, 7, 8} };
        int b = 4;
        __m64 c = _mm_srli_pi16 (a.v, b);
        return 0;
      }''',
      args : loongson_mmi_flags,
      include_directories : include_directories('.'),
      name : 'Loongson MMI Intrinsic Support')
    have_loongson_mmi = true
  endif
endif

if have_loongson_mmi
  config.set10('USE_LOONGSON_MMI', true)
elif use_loongson_mmi.enabled()
  error('Loongson MMI Support unavailable, but required')
endif

use_mmx = get_option('mmx')
have_mmx = false
mmx_flags = []

if cc.get_id() == 'msvc'
  mmx_flags = ['/w14710', '/w14714', '/wd4244']
elif cc.get_id() == 'sun'
  mmx_flags = ['-xarch=sse']
else
  mmx_flags = ['-mmmx', '-Winline']
endif
if not use_mmx.disabled()
  if host_machine.cpu_family() == 'x86_64' or cc.get_id() == 'msvc'
    have_mmx = true
  elif host_machine.cpu_family() == 'x86' and cc.compiles('''
      #include <mmintrin.h>
      #include <stdint.h>

      /* Check support for block expressions */
      #define _mm_shuffle_pi16(A, N)                    \
        ({                                              \
        __m64 ret;                                      \
                                                        \
        /* Some versions of clang will choke on K */    \
        asm ("pshufw %2, %1, %0\n\t"                    \
             : "=y" (ret)                               \
             : "y" (A), "K" ((const int8_t)N)           \
        );                                              \
                                                        \
        ret;                                            \
        })

      int main () {
          __m64 v = _mm_cvtsi32_si64 (1);
          __m64 w;

          w = _mm_shuffle_pi16(v, 5);

          /* Some versions of clang will choke on this */
          asm ("pmulhuw %1, %0\n\t"
               : "+y" (w)
               : "y" (v)
          );

          return _mm_cvtsi64_si32 (v);
      }''',
      args : mmx_flags,
      name : 'MMX Intrinsic Support')
    have_mmx = true
  endif
endif

if have_mmx
  # Inline assembly do not work on X64 MSVC, so we use
  # compatibility intrinsics there
  if cc.get_id() != 'msvc' or host_machine.cpu_family() != 'x86_64'
    config.set10('USE_X86_MMX', true)
  endif
elif use_mmx.enabled()
  error('MMX Support unavailable, but required')
endif

use_sse2 = get_option('sse2')
have_sse2 = false
sse2_flags = []
if cc.get_id() == 'sun'
  sse2_flags = ['-xarch=sse2']
elif cc.get_id() != 'msvc'
  sse2_flags = ['-msse2', '-Winline']
endif
if not use_sse2.disabled()
  if host_machine.cpu_family() == 'x86'
    if cc.compiles('''
        #if defined(__GNUC__) && (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 2))
        #   if !defined(__amd64__) && !defined(__x86_64__)
        #      error "Need GCC >= 4.2 for SSE2 intrinsics on x86"
        #   endif
        #endif
        #include <mmintrin.h>
        #include <xmmintrin.h>
        #include <emmintrin.h>
        int param;
        int main () {
          __m128i a = _mm_set1_epi32 (param), b = _mm_set1_epi32 (param + 1), c;
          c = _mm_xor_si128 (a, b);
          return _mm_cvtsi128_si32(c);
        }''',
        args : sse2_flags,
        name : 'SSE2 Intrinsic Support')
      have_sse2 = true
    endif
  elif host_machine.cpu_family() == 'x86_64'
    have_sse2 = true
  endif
endif

if have_sse2
  config.set10('USE_SSE2', true)
elif use_sse2.enabled()
  error('sse2 Support unavailable, but required')
endif

use_ssse3 = get_option('ssse3')
have_ssse3 = false
ssse3_flags = []
if cc.get_id() != 'msvc'
  ssse3_flags = ['-mssse3', '-Winline']
endif

# x64 pre-2010 MSVC compilers crashes when building the ssse3 code
if not use_ssse3.disabled() and not (cc.get_id() == 'msvc' and cc.version().version_compare('<16') and host_machine.cpu_family() == 'x86_64')
  if host_machine.cpu_family().startswith('x86')
    if cc.compiles('''
        #include <mmintrin.h>
        #include <xmmintrin.h>
        #include <emmintrin.h>
        int param;
        int main () {
          __m128i a = _mm_set1_epi32 (param), b = _mm_set1_epi32 (param + 1), c;
          c = _mm_xor_si128 (a, b);
          return _mm_cvtsi128_si32(c);
        }''',
        args : ssse3_flags,
        name : 'SSSE3 Intrinsic Support')
      have_ssse3 = true
    endif
  endif
endif

if have_ssse3
  config.set10('USE_SSSE3', true)
elif use_ssse3.enabled()
  error('ssse3 Support unavailable, but required')
endif

use_vmx = get_option('vmx')
have_vmx = false
vmx_flags = ['-maltivec', '-mabi=altivec']
if not use_vmx.disabled()
  if host_machine.cpu_family().startswith('ppc')
    if cc.compiles('''
        #include <altivec.h>
        int main () {
            vector unsigned int v = vec_splat_u32 (1);
            v = vec_sub (v, v);
            return 0;
        }''',
        args : vmx_flags,
        name : 'VMX/Altivec Intrinsic Support')
      have_vmx = true
    endif
  endif
endif

if have_vmx
  config.set10('USE_VMX', true)
elif use_vmx.enabled()
  error('vmx Support unavailable, but required')
endif

use_armv6_simd = get_option('arm-simd')
have_armv6_simd = false
if not use_armv6_simd.disabled()
  if host_machine.cpu_family() == 'arm'
    if cc.compiles(files('arm-simd-test.S'), name : 'ARMv6 SIMD Intrinsic Support')
      have_armv6_simd = true
    endif
  endif
endif

if have_armv6_simd
  config.set10('USE_ARM_SIMD', true)
elif use_armv6_simd.enabled()
  error('ARMv6 SIMD Support unavailable, but required')
endif

use_neon = get_option('neon')
have_neon = false
if not use_neon.disabled()
  if host_machine.cpu_family() == 'arm'
    if cc.compiles(files('neon-test.S'), name : 'NEON Intrinsic Support')
      have_neon = true
    endif
  endif
endif

if have_neon
  config.set10('USE_ARM_NEON', true)
elif use_neon.enabled()
  error('NEON Support unavailable, but required')
endif

use_a64neon = get_option('a64-neon')
have_a64neon = false
if not use_a64neon.disabled()
  if host_machine.cpu_family() == 'aarch64'
    if cc.compiles(files('a64-neon-test.S'), name : 'NEON A64 Intrinsic Support')
      have_a64neon = true
    endif
  endif
endif

if have_a64neon
  config.set10('USE_ARM_A64_NEON', true)
elif use_a64neon.enabled()
  error('A64 NEON Support unavailable, but required')
endif

use_iwmmxt = get_option('iwmmxt')
have_iwmmxt = false
iwmmxt_flags = ['-flax-vector-conversions', '-Winline']
if not use_iwmmxt.disabled()
  if get_option('iwmmxt2')
    iwmmxt_flags += '-march=iwmmxt2'
  else
    iwmmxt_flags += '-march=iwmmxt'
  endif

  if host_machine.cpu_family() == 'arm'
    if cc.compiles('''
        #ifndef __IWMMXT__
        #error "IWMMXT not enabled (with -march=iwmmxt)"
        #endif
        #if defined(__GNUC__) && (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 8))
        #error "Need GCC >= 4.8 for IWMMXT intrinsics"
        #endif
        #include <mmintrin.h>
        int main () {
          union {
            __m64 v;
            char c[8];
          } a = { .c = {1, 2, 3, 4, 5, 6, 7, 8} };
          int b = 4;
          __m64 c = _mm_srli_si64 (a.v, b);
        }
        ''',
        args : iwmmxt_flags,
        name : 'IWMMXT Intrinsic Support')
      have_iwmmxt = true
    endif
  endif
endif

if have_iwmmxt
  config.set10('USE_ARM_IWMMXT', true)
elif use_iwmmxt.enabled()
  error('IWMMXT Support unavailable, but required')
endif

use_mips_dspr2 = get_option('mips-dspr2')
have_mips_dspr2 = false
mips_dspr2_flags = ['-mdspr2']
if not use_mips_dspr2.disabled()
  if host_machine.cpu_family() == 'mips32'
    if cc.compiles('''
        #if !(defined(__mips__) &&  __mips_isa_rev >= 2)
        #error MIPS DSPr2 is currently only available on MIPS32r2 platforms.
        #endif
        int
        main ()
        {
            int c = 0, a = 0, b = 0;
            __asm__ __volatile__ (
                "precr.qb.ph %[c], %[a], %[b]          \n\t"
                : [c] "=r" (c)
                : [a] "r" (a), [b] "r" (b)
            );
            return c;
        }''',
        args : mipds_dspr2_flags,
        name : 'DSPr2 Intrinsic Support')
      have_mips_dspr2 = true
    endif
  endif
endif

if have_mips_dspr2
  config.set10('USE_MIPS_DSPR2', true)
elif use_mips_dspr2.enabled()
  error('MIPS DSPr2 Support unavailable, but required')
endif

use_gnu_asm = get_option('gnu-inline-asm')
if not use_gnu_asm.disabled()
  if cc.compiles('''
      int main () {
        /* Most modern architectures have a NOP instruction, so this is a fairly generic test. */
        asm volatile ( "\tnop\n" : : : "cc", "memory" );
        return 0;
      }
      ''',
      name : 'GNU Inline ASM support.')
    config.set10('USE_GCC_INLINE_ASM', true)
  elif use_gnu_asm.enabled()
    error('GNU inline assembly support missing but required.')
  endif
endif

if get_option('timers')
  config.set('PIXMAN_TIMERS', 1)
endif
if get_option('gnuplot')
  config.set('PIXMAN_GNUPLOT', 1)
endif

if cc.get_id() != 'msvc'
  dep_openmp = dependency('openmp', required : get_option('openmp'))
  if dep_openmp.found()
    config.set10('USE_OPENMP', true)
  elif meson.version().version_compare('<0.51.0')
  # In versions of meson before 0.51 the openmp dependency can still
  # inject arguments in the the auto case when it is not found, the
  # detection does work correctly in that case however, so we just
  # replace dep_openmp with null_dep to work around this.
    dep_openmp = null_dep
  endif
else
  # the MSVC implementation of openmp is not compliant enough for our
  # uses here, so we disable it here.
  # Please see: https://stackoverflow.com/questions/12560243/using-threadprivate-directive-in-visual-studio
  dep_openmp = null_dep
endif

dep_gtk = dependency('gtk+-3.0', required : get_option('gtk'))
dep_glib = dependency('glib-2.0', required : get_option('gtk'))

dep_png = null_dep
if not get_option('libpng').disabled()
  dep_png = dependency('libpng', required : false)

  # We need to look for the right library to link to for libpng,
  # when looking for libpng manually
  foreach png_ver : [ '16', '15', '14', '13', '12', '10' ]
    if not dep_png.found()
      dep_png = cc.find_library('libpng@0@'.format(png_ver), has_headers : ['png.h'], required : false)
    endif
  endforeach

  if get_option('libpng').enabled() and not dep_png.found()
    error('libpng support requested but libpng library not found')
  endif
endif

if dep_png.found()
  config.set('HAVE_LIBPNG', 1)
endif
dep_m = cc.find_library('m', required : false)
dep_threads = dependency('threads')

# MSVC-style compilers do not come with pthreads, so we must link
# to it explicitly, currently pthreads-win32 is supported
pthreads_found = false

if dep_threads.found() and cc.has_header('pthread.h')
  if cc.get_argument_syntax() == 'msvc'
    pthread_lib = null_dep
    foreach pthread_type : ['VC3', 'VSE3', 'VCE3', 'VC2', 'VSE2', 'VCE2']
      if not pthread_lib.found()
        pthread_lib = cc.find_library('pthread@0@'.format(pthread_type), required : false)
      endif
    endforeach
    if pthread_lib.found()
      pthreads_found = true
      dep_threads = pthread_lib
    endif
  else
    pthreads_found = true
  endif
endif

if pthreads_found
  config.set('HAVE_PTHREADS', 1)
endif

funcs = ['sigaction', 'alarm', 'mprotect', 'getpagesize', 'mmap', 'getisax', 'gettimeofday']
# mingw claimes to have posix_memalign, but it doesn't
if host_machine.system() != 'windows'
  funcs += 'posix_memalign'
endif

foreach f : funcs
  if cc.has_function(f)
    config.set('HAVE_@0@'.format(f.to_upper()), 1)
  endif
endforeach

# This is only used in one test, that defines _GNU_SOURCE
if cc.has_function('feenableexcept',
                   prefix : '#define _GNU_SOURCE\n#include <fenv.h>',
                   dependencies : dep_m)
  config.set('HAVE_FEENABLEEXCEPT', 1)
endif

if cc.has_header_symbol('fenv.h', 'FE_DIVBYZERO')
  config.set('HAVE_FEDIVBYZERO', 1)
endif

foreach h : ['sys/mman.h', 'fenv.h', 'unistd.h']
  if cc.check_header(h)
    config.set('HAVE_@0@'.format(h.underscorify().to_upper()), 1)
  endif
endforeach

use_tls = get_option('tls')
have_tls = ''
if not use_tls.disabled()
  # gcc on Windows only warns that __declspec(thread) isn't supported,
  # passing -Werror=attributes makes it fail.
  if (host_machine.system() == 'windows' and
      cc.compiles('int __declspec(thread) foo;',
                  args : cc.get_supported_arguments(['-Werror=attributes']),
                  name : 'TLS via __declspec(thread)'))
    have_tls = '__declspec(thread)'
  elif cc.compiles('int __thread foo;', name : 'TLS via __thread')
    have_tls = '__thread'
  endif
endif

if have_tls != ''
  config.set('TLS', have_tls)
elif use_tls.enabled()
  error('Compiler TLS Support unavailable, but required')
endif

if cc.links('''
    static int x = 1;
    static void __attribute__((constructor)) constructor_function () { x = 0; }
    int main (void) { return x; }
    ''',
    name : '__attribute__((constructor))')
  config.set('TOOLCHAIN_SUPPORTS_ATTRIBUTE_CONSTRUCTOR', 1)
endif

if cc.links(
    ' __float128 a = 1.0Q, b = 2.0Q; int main (void) { return a + b; }',
    name : 'Has float128 support')
  config.set('HAVE_FLOAT128', 1)
endif

if cc.has_function('clz')
  config.set('HAVE_BUILTIN_CLZ', 1)
endif

if cc.links('''
    unsigned int __attribute__ ((vector_size(16))) e, a, b;
    int main (void) { e = a - ((b << 27) + (b >> (32 - 27))) + 1; return e[0]; }
    ''',
    name : 'Support for GCC vector extensions')
  config.set('HAVE_GCC_VECTOR_EXTENSIONS', 1)
endif

if host_machine.endian() == 'big'
  config.set('WORDS_BIGENDIAN', 1)
endif

config.set('SIZEOF_LONG', cc.sizeof('long'))

# Required to make pixman-private.h
config.set('PACKAGE', 'foo')

version_conf = configuration_data()
split = meson.project_version().split('.')
version_conf.set('PIXMAN_VERSION_MAJOR', split[0])
version_conf.set('PIXMAN_VERSION_MINOR', split[1])
version_conf.set('PIXMAN_VERSION_MICRO', split[2])

add_project_arguments('-DHAVE_CONFIG_H', language : ['c'])

subdir('pixman')

if not get_option('tests').disabled()
  subdir('test')
  subdir('demos')
endif

pkg = import('pkgconfig')
pkg.generate(libpixman,
  name : 'Pixman',
  filebase : 'pixman-1',
  description : 'The pixman library (version 1)',
  subdirs: 'pixman-1',
  version : meson.project_version(),
)
