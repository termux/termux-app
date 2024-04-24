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

tests = [
  'oob-test',
  'infinite-loop',
  'trap-crasher',
  'fence-image-self-test',
  'region-translate-test',
  'fetch-test',
  'a1-trap-test',
  'prng-test',
  'radial-invalid',
  'pdf-op-test',
  'region-test',
  'combiner-test',
  'scaling-crash-test',
  'alpha-loop',
  'scaling-helpers-test',
  'rotate-test',
  'alphamap',
  'gradient-crash-test',
  'pixel-test',
  'matrix-test',
  'filter-reduction-test',
  'composite-traps-test',
  'region-contains-test',
  'glyph-test',
  'solid-test',
  'stress-test',
  'cover-test',
  'blitters-test',
  'affine-test',
  'scaling-test',
  'composite',
  'tolerance-test',
]

# Remove/update this once thread-test.c supports threading methods
# other than PThreads and Windows threads
if pthreads_found or host_machine.system() == 'windows'
  tests += 'thread-test'
endif

progs = [
  'lowlevel-blt-bench',
  'radial-perf-test',
  'check-formats',
  'scaling-bench',
  'affine-bench',
]

foreach t : tests
  test(
    t,
    executable(
      t,
      [t + '.c', config_h],
      dependencies : [idep_pixman, libtestutils_dep, dep_threads, dep_openmp, dep_png],
    ),
    timeout : 120,
    is_parallel : true,
  )
endforeach

foreach p : progs
  executable(
    p,
    p + '.c',
    dependencies : [idep_pixman, libtestutils_dep, dep_openmp],
  )
endforeach

