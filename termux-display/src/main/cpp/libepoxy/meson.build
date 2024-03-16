project('libepoxy', 'c', version: '1.5.10',
        default_options: [
          'buildtype=debugoptimized',
          'c_std=gnu99',
          'warning_level=1',
        ],
        license: 'MIT',
        meson_version: '>= 0.54.0')

epoxy_version = meson.project_version().split('.')
epoxy_major_version = epoxy_version[0].to_int()
epoxy_minor_version = epoxy_version[1].to_int()
epoxy_micro_version = epoxy_version[2].to_int()

epoxy_prefix = get_option('prefix')
epoxy_libdir = join_paths(epoxy_prefix, get_option('libdir'))
epoxy_datadir = join_paths(epoxy_prefix, get_option('datadir'))
epoxy_includedir = join_paths(epoxy_prefix, get_option('includedir'))

cc = meson.get_compiler('c')
host_system = host_machine.system()

conf = configuration_data()
conf.set_quoted('PACKAGE_NAME', meson.project_name())
conf.set_quoted('PACKAGE_VERSION', meson.project_version())
conf.set_quoted('PACKAGE_STRING', '@0@-@1@'.format(meson.project_name(), meson.project_version()))
conf.set_quoted('PACKAGE_DATADIR', join_paths(get_option('prefix'), get_option('datadir')))
conf.set_quoted('PACKAGE_LIBDIR', join_paths(get_option('prefix'), get_option('libdir')))
conf.set_quoted('PACKAGE_LOCALEDIR', join_paths(get_option('prefix'), get_option('datadir'), 'locale'))
conf.set_quoted('PACKAGE_LIBEXECDIR', join_paths(get_option('prefix'), get_option('libexecdir')))
conf.set('HAVE_KHRPLATFORM_H', cc.has_header('KHR/khrplatform.h'))

# GLX can be used on different platforms, so we expose a
# configure time switch to enable or disable it; in case
# the "auto" default value is set, we only enable GLX
# support on Linux and Unix
enable_glx = get_option('glx')
if enable_glx == 'auto'
  build_glx = not ['windows', 'darwin', 'android', 'haiku'].contains(host_system)
else
  build_glx = enable_glx == 'yes'
endif

enable_egl = get_option('egl')
if enable_egl == 'auto'
  build_egl = not ['windows', 'darwin'].contains(host_system)
else
  build_egl = enable_egl == 'yes'
endif

enable_x11 = get_option('x11')
if not enable_x11
  if enable_glx == 'yes'
    error('GLX support is explicitly enabled, but X11 was disabled')
  endif
  build_glx = false
endif

# The remaining platform specific API for GL/GLES are enabled
# depending on the platform we're building for
if host_system == 'windows'
  build_wgl = true
  has_znow = true
elif host_system == 'darwin'
  build_wgl = false
  has_znow = false
else
  build_wgl = false
  has_znow = true
endif

conf.set10('ENABLE_GLX', build_glx)
conf.set10('ENABLE_EGL', build_egl)
conf.set10('ENABLE_X11', enable_x11)

# Compiler flags, taken from the Xorg macros
if cc.get_id() == 'msvc'
  # Compiler options taken from msvc_recommended_pragmas.h
  # in GLib, based on _Win32_Programming_ by Rector and Newcomer
  test_cflags = [
    '-we4002', # too many actual parameters for macro
    '-we4003', # not enough actual parameters for macro
    '-w14010', # single-line comment contains line-continuation character
    '-we4013', # 'function' undefined; assuming extern returning int
    '-w14016', # no function return type; using int as default
    '-we4020', # too many actual parameters
    '-we4021', # too few actual parameters
    '-we4027', # function declared without formal parameter list
    '-we4029', # declared formal parameter list different from definition
    '-we4033', # 'function' must return a value
    '-we4035', # 'function' : no return value
    '-we4045', # array bounds overflow
    '-we4047', # different levels of indirection
    '-we4049', # terminating line number emission
    '-we4053', # an expression of type void was used as an operand
    '-we4071', # no function prototype given
    '-we4819', # the file contains a character that cannot be represented in the current code page
    '/utf-8', # Set the input and exec encoding to utf-8, like is the default with GCC
  ]
elif cc.get_id() == 'gcc' or cc.get_id() == 'clang'
  test_cflags = [
    '-Wpointer-arith',
    '-Wmissing-declarations',
    '-Wformat=2',
    '-Wstrict-prototypes',
    '-Wmissing-prototypes',
    '-Wnested-externs',
    '-Wbad-function-cast',
    '-Wold-style-definition',
    '-Wdeclaration-after-statement',
    '-Wunused',
    '-Wuninitialized',
    '-Wshadow',
    '-Wmissing-noreturn',
    '-Wmissing-format-attribute',
    '-Wredundant-decls',
    '-Wlogical-op',
    '-Werror=implicit',
    '-Werror=nonnull',
    '-Werror=init-self',
    '-Werror=main',
    '-Werror=missing-braces',
    '-Werror=sequence-point',
    '-Werror=return-type',
    '-Werror=trigraphs',
    '-Werror=array-bounds',
    '-Werror=write-strings',
    '-Werror=address',
    '-Werror=int-to-pointer-cast',
    '-Werror=pointer-to-int-cast',
    '-fno-strict-aliasing',
    '-Wno-int-conversion',
  ]
else
  test_cflags = []
endif

common_cflags = cc.get_supported_arguments(test_cflags)

libtype = get_option('default_library')

# Visibility compiler flags; we only use this for shared libraries
visibility_cflags = []
if libtype == 'shared'
  if host_system == 'windows'
    conf.set('DLL_EXPORT', true)
    conf.set('EPOXY_PUBLIC', '__declspec(dllexport) extern')
    if cc.get_id() != 'msvc'
      visibility_cflags += [ '-fvisibility=hidden' ]
    endif
  else
    conf.set('EPOXY_PUBLIC', '__attribute__((visibility("default"))) extern')
    visibility_cflags += [ '-fvisibility=hidden' ]
  endif
endif

# The inline keyword is available only for C++ in MSVC.
# So we need to use Microsoft specific __inline.
if host_system == 'windows'
  if cc.get_id() == 'msvc'
    conf.set('inline', '__inline')
  endif
endif

# Dependencies
dl_dep = cc.find_library('dl', required: false)
gl_dep = dependency('gl', required: false)
egl_dep = dependency('egl', required: false)
elg_headers_dep = egl_dep.partial_dependency(compile_args: true, includes: true)

# Optional dependencies for tests
x11_dep = dependency('x11', required: false)
x11_headers_dep = x11_dep.partial_dependency(compile_args: true, includes: true)

# GLES v2 and v1 may have pkg-config files, courtesy of downstream
# packagers; let's check those first, and fall back to find_library()
# if we fail
gles2_dep = dependency('glesv2', required: false)
if not gles2_dep.found()
  gles2_dep = cc.find_library('libGLESv2', required: false)
endif

gles1_dep = dependency('glesv1_cm', required: false)
if not gles1_dep.found()
  gles1_dep = cc.find_library('libGLESv1_CM', required: false)
endif

# On windows, the DLL has to have all of its functions
# resolved at link time, so we have to link directly against
# opengl32.  But that's the only GL provider, anyway.
if host_system == 'windows'
  opengl32_dep = cc.find_library('opengl32', required: true)

  # When building against static libraries, we need to control
  # the order of the dependencies, and gdi32 provides symbols
  # needed when using opengl32, like SetPixelFormat and
  # ChoosePixelFormat. This is mostly a workaround for older
  # versions of Meson.
  gdi32_dep = cc.find_library('gdi32', required: true)
endif

# Generates the dispatch tables
gen_dispatch_py = find_program('src/gen_dispatch.py')

gl_registry = files('registry/gl.xml')
egl_registry = files('registry/egl.xml')
glx_registry = files('registry/glx.xml')
wgl_registry = files('registry/wgl.xml')

libepoxy_inc = [
  include_directories('include'),
  include_directories('src'),
]

subdir('include/epoxy')
subdir('src')

if get_option('tests')
  subdir('test')
endif

if get_option('docs')
  doxygen = find_program('doxygen', required: false)
  if doxygen.found()
    subdir('doc')
  else
    message('Documentation disabled without doxygen')
  endif
endif
