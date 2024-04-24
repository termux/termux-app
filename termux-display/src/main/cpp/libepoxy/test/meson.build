dl_dep = cc.find_library('dl', required: false)
has_dlvsym = cc.has_function('dlvsym', dependencies: dl_dep)

has_gles1 = gles1_dep.found()
has_gles2 = gles2_dep.found()
build_x11_tests = enable_x11 and x11_dep.found()

test_cflags = common_cflags
if not has_dlvsym
test_cflags += [
  '-D_XOPEN_SOURCE',
  '-D_POSIX_C_SOURCE=200809L',
]
endif

# Unconditionally built tests
test('header_guards',
     executable('header guards', 'headerguards.c',
                c_args: common_cflags,
                dependencies: libepoxy_dep,
                include_directories: libepoxy_inc))
test('misc_defines',
     executable('misc defines', 'miscdefines.c',
                c_args: common_cflags,
                dependencies: libepoxy_dep,
                include_directories: libepoxy_inc))
test('khronos_typedefs',
     executable('khronos typedefs', [
                  'khronos_typedefs.c',
                  'khronos_typedefs.h',
                  'khronos_typedefs_nonepoxy.c',
                ],
                c_args: common_cflags,
                dependencies: libepoxy_dep,
                include_directories: libepoxy_inc))
test('gl_version',
     executable('gl_version',
                'gl_version.c',
                c_args: common_cflags,
                dependencies: libepoxy_dep,
                include_directories: libepoxy_inc))

if build_egl and build_x11_tests
  egl_common_sources = [ 'egl_common.h', 'egl_common.c', ]
  egl_common_lib = static_library('egl_common',
                                  sources: egl_common_sources,
                                  dependencies: libepoxy_dep,
                                  include_directories: libepoxy_inc,
                                  c_args: common_cflags,
                                  install: false)

  egl_tests = [
    [ 'egl_has_extension_nocontext', [], [ 'egl_has_extension_nocontext.c' ], true, ],
    [ 'egl_epoxy_api', [], [ 'egl_epoxy_api.c' ], true ],
    [ 'egl_gles1_without_glx', [ '-DGLES_VERSION=1', ], [ 'egl_without_glx.c' ], has_gles1, ],
    [ 'egl_gles2_without_glx', [ '-DGLES_VERSION=2', ], [ 'egl_without_glx.c' ], has_gles2, ],
  ]

  if build_glx
    egl_tests += [
      [ 'egl_gl', [], [ 'egl_gl.c' ], true, ],
    ]
  endif

  foreach test: egl_tests
    test_name = test[0]
    test_source = test[2]
    test_args = test[1]
    test_run = test[3]

    if test_run
      test_bin = executable(test_name, test_source,
                            c_args: test_cflags + test_args,
                            include_directories: libepoxy_inc,
                            dependencies: [ libepoxy_dep, x11_dep, egl_dep, dl_dep ],
                            link_with: egl_common_lib,
                            link_args: '-rdynamic')
      test(test_name, test_bin)
    endif
  endforeach
endif

if build_glx and build_x11_tests
  glx_common_sources = [ 'glx_common.h', 'glx_common.c', ]
  glx_common_lib = static_library('glx_common',
                                  sources: glx_common_sources,
                                  dependencies: libepoxy_dep,
                                  include_directories: libepoxy_inc,
                                  c_args: common_cflags,
                                  install: false)

  # glx_beginend links directly with the GL library, so we need to check it
  # separately
  test('glx_beginend', executable('glx_beginend', 'glx_beginend.c',
                                  c_args: test_cflags,
                                  include_directories: libepoxy_inc,
                                  dependencies: [ libepoxy_dep, x11_dep, gl_dep, dl_dep ],
                                  link_with: glx_common_lib))

  glx_tests = [
    [ 'glx_public_api', [ 'glx_public_api.c' ], [], [], true ],
    [ 'glx_public_api_core', [ 'glx_public_api_core.c' ], [], [], true ],
    [ 'glx_glxgetprocaddress_nocontext', [ 'glx_glxgetprocaddress_nocontext.c' ], [], [], true ],
    [ 'glx_has_extension_nocontext', [ 'glx_has_extension_nocontext.c' ], [], [], true ],
    [ 'glx_static', [ 'glx_static.c' ], [ '-DNEEDS_TO_BE_STATIC'], [ '-static' ], libtype == 'static' ],
    [ 'glx_shared_znow', [ 'glx_static.c', ], [], [ '-Wl,-z,now' ], has_znow ],
    [ 'glx_alias_prefer_same_name', [ 'glx_alias_prefer_same_name.c', 'dlwrap.c', 'dlwrap.h' ], [], [ '-rdynamic' ], has_dlvsym ],
    [ 'glx_gles2', [ 'glx_gles2.c', 'dlwrap.c', 'dlwrap.h' ], [], [ '-rdynamic' ], has_dlvsym ],
  ]

  foreach test: glx_tests
    test_name = test[0]
    test_source = test[1]
    test_c_args = test[2]
    test_link_args = test[3]
    test_run = test[4]

    if test_run
      test_bin = executable(test_name, test_source,
                            c_args: test_cflags + test_c_args,
                            include_directories: libepoxy_inc,
                            dependencies: [ libepoxy_dep, x11_dep, dl_dep ],
                            link_with: glx_common_lib,
                            link_args: test_link_args)
      test(test_name, test_bin)
    endif
  endforeach
endif

# WGL
if build_wgl
  wgl_common_sources = [ 'wgl_common.h', 'wgl_common.c', ]
  wgl_common_lib = static_library('wgl_common',
                                  sources: wgl_common_sources,
                                  dependencies: libepoxy_dep,
                                  include_directories: libepoxy_inc,
                                  c_args: common_cflags,
                                  install: false)

  wgl_tests = [
    [ 'wgl_core_and_exts', [ 'wgl_core_and_exts.c' ], [], ],
    [ 'wgl_per_context_funcptrs', [ 'wgl_per_context_funcptrs.c' ], [], ],
    [ 'wgl_usefontbitmaps', [ 'wgl_usefontbitmaps.c'], [], ],
    [ 'wgl_usefontbitmaps_unicode', [ 'wgl_usefontbitmaps.c' ], [ '-DUNICODE' ], ],
  ]

  foreach test: wgl_tests
    test_name = test[0]
    test_source = test[1]
    test_c_args = test[2]

    test_bin = executable(test_name, test_source,
                          c_args: test_cflags + test_c_args,
                          include_directories: libepoxy_inc,
                          dependencies: [ libepoxy_dep ],
                          link_with: wgl_common_lib)

    test(test_name, test_bin)
  endforeach
endif

# Apple
if host_machine.system().contains('darwin')
  opengl_dep = dependency('appleframeworks', modules: ['OpenGL', 'Carbon'], required: true)

  cgl_tests = [
    [ 'cgl_core', [ 'cgl_core.c' ] ],
    [ 'cgl_epoxy_api', [ 'cgl_epoxy_api.c' ] ],
  ]

  foreach t: cgl_tests
    test_name = t[0]
    test_sources = t[1]

    test(test_name,
      executable(
        test_name, test_sources,
        c_args: test_cflags,
        include_directories: libepoxy_inc,
        dependencies: [ libepoxy_dep, opengl_dep ],
      ),
    )
  endforeach
endif
