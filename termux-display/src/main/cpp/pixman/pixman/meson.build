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

config_h = configure_file(
  configuration : config,
  output : 'config.h'
)

version_h = configure_file(
  configuration : version_conf,
  input : 'pixman-version.h.in',
  output : 'pixman-version.h',
  install_dir : join_paths(get_option('prefix'), get_option('includedir'), 'pixman-1')
)

libpixman_extra_cargs = []
if cc.has_function_attribute('dllexport')
  libpixman_extra_cargs = ['-DPIXMAN_API=__declspec(dllexport)']
endif

pixman_simd_libs = []
simds = [
  # the mmx library can be compiled with mmx on x86/x86_64, iwmmxt on
  # some arm cores, or loongson mmi on loongson mips systems. The
  # libraries will all have the same name, "pixman-mmx", but there is
  # no chance of more than one version being built in the same build
  # because no system could have mmx, iwmmxt, and mmi, and it
  # simplifies the build logic to give them the same name.
  ['mmx', have_mmx, mmx_flags, []],
  ['mmx', have_loongson_mmi, loongson_mmi_flags, []],
  ['mmx', have_iwmmxt, iwmmxt_flags, []],

  ['sse2', have_sse2, sse2_flags, []],
  ['ssse3', have_ssse3, ssse3_flags, []],
  ['vmx', have_vmx, vmx_flags, []],
  ['arm-simd', have_armv6_simd, [],
   ['pixman-arm-simd-asm.S', 'pixman-arm-simd-asm-scaled.S']],
  ['arm-neon', have_neon, [],
   ['pixman-arm-neon-asm.S', 'pixman-arm-neon-asm-bilinear.S']],
  ['arm-neon', have_a64neon, [],
   ['pixman-arma64-neon-asm.S', 'pixman-arma64-neon-asm-bilinear.S']],
  ['mips-dspr2', have_mips_dspr2, mips_dspr2_flags,
   ['pixman-mips-dspr2-asm.S', 'pixman-mips-memcpy-asm.S']],
]

foreach simd : simds
  if simd[1]
    name = 'pixman-' + simd[0]
    pixman_simd_libs += static_library(
      name,
      [name + '.c', config_h, version_h, simd[3]],
      c_args : simd[2]
    )
  endif
endforeach

pixman_files = files(
  'pixman.c',
  'pixman-access.c',
  'pixman-access-accessors.c',
  'pixman-bits-image.c',
  'pixman-combine32.c',
  'pixman-combine-float.c',
  'pixman-conical-gradient.c',
  'pixman-filter.c',
  'pixman-x86.c',
  'pixman-mips.c',
  'pixman-arm.c',
  'pixman-ppc.c',
  'pixman-edge.c',
  'pixman-edge-accessors.c',
  'pixman-fast-path.c',
  'pixman-glyph.c',
  'pixman-general.c',
  'pixman-gradient-walker.c',
  'pixman-image.c',
  'pixman-implementation.c',
  'pixman-linear-gradient.c',
  'pixman-matrix.c',
  'pixman-noop.c',
  'pixman-radial-gradient.c',
  'pixman-region16.c',
  'pixman-region32.c',
  'pixman-solid-fill.c',
  'pixman-timer.c',
  'pixman-trap.c',
  'pixman-utils.c',
)

# Android cpu-features
cpu_features_path = get_option('cpu-features-path')
cpu_features_sources = []
cpu_features_inc = []
if cpu_features_path != ''
  message('Using cpu-features.[ch] from ' + cpu_features_path)
  cpu_features_sources = files(
    cpu_features_path / 'cpu-features.h',
    cpu_features_path / 'cpu-features.c',
  )
  cpu_features_inc = include_directories(cpu_features_path)
endif

libpixman = library(
  'pixman-1',
  [pixman_files, config_h, version_h, cpu_features_sources],
  link_with: pixman_simd_libs,
  c_args : libpixman_extra_cargs,
  dependencies : [dep_m, dep_threads],
  include_directories : cpu_features_inc,
  version : meson.project_version(),
  install : true,
)

inc_pixman = include_directories('.')

idep_pixman = declare_dependency(
  link_with: libpixman,
  include_directories : inc_pixman,
)

if meson.version().version_compare('>= 0.54.0')
  meson.override_dependency('pixman-1', idep_pixman)
endif

install_headers('pixman.h', subdir : 'pixman-1')
