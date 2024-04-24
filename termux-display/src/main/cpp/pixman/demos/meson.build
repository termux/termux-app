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

extra_demo_cflags = []
if cc.get_argument_syntax() == 'msvc'
  extra_demo_cflags = ['-D_USE_MATH_DEFINES']
endif

demos = [
  'gradient-test',
  'alpha-test',
  'composite-test',
  'clip-test',
  'trap-test',
  'screen-test',
  'convolution-test',
  'radial-test',
  'linear-gradient',
  'conical-test',
  'tri-test',
  'checkerboard',
  'srgb-test',
  'srgb-trap-test',
  'scale',
  'dither',
]

if dep_gtk.found()

  libdemo = static_library(
    'demo',
    ['gtk-utils.c', config_h, version_h],
    dependencies : [libtestutils_dep, dep_gtk, dep_glib, dep_png, dep_m, dep_openmp],
    include_directories : inc_pixman,
  )

  if dep_gtk.found()
    foreach d : demos
      executable(
        d,
        [d + '.c', config_h, version_h],
        c_args : extra_demo_cflags,
        link_with : [libdemo],
        dependencies : [idep_pixman, libtestutils_dep, dep_glib, dep_gtk, dep_openmp, dep_png],
      )
    endforeach
  endif

endif
