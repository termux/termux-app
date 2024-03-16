# mach interface
mig = find_program('mig')
mach_startup = custom_target('mach_startup',
    command: [mig,
              '-sheader', '@OUTPUT0@', '-header', '@OUTPUT2@',
              '-server', '@OUTPUT1@', '-user', '@OUTPUT3@',
              '@INPUT@'],
    input: 'mach_startup.defs',
    output: ['mach_startupServer.h',
             'mach_startupServer.c',
             'mach_startup.h',
             'mach_startupUser.c'],
)

mach_startup_dep = declare_dependency(
    sources: mach_startup[2] # mach_startup.h
)

# common defines
xquartz_defs = [
    bundle_id_def,
    '-DXSERVER_VERSION="@0@"'.format(meson.project_version()),
    '-DX11BINDIR="@0@"'.format(join_paths(get_option('prefix'), get_option('bindir'))),
]

# X11.bin
x11appdir = join_paths(bundle_root, 'Contents/MacOS')

x11_bin_deps = [
     meson.get_compiler('c').find_library('Xplugin'),
     dependency('Carbon', method: 'extraframework'),
     cocoa,
     dependency('CoreAudio', method: 'extraframework'),
     dependency('IOKit', method: 'extraframework')
]

if build_glx
    x11_bin_deps += [dependency('OpenGL', method: 'extraframework')]
endif

if build_sparkle
    x11_bin_deps += sparkle
endif

x11_bin_libs = [
    libXquartz,
    libXquartzXpr,
    libxpbproxy,
    libxserver_fb,
    libxserver,
    libxserver_xkb_stubs,
]

if build_glx
   x11_bin_libs += [libcglcore, libxserver_glx, libglxvnd]
endif

x11_bin = executable('X11.bin',
     [
      'bundle-main.c',
      mach_startup[1], # mach_startupServer.c
      mach_startup[3], # mach_startupUser.c
     ],
     link_with: x11_bin_libs,
     dependencies: [xproto_dep, x11_bin_deps, mach_startup_dep],
     include_directories: [inc, '..', top_dir_inc],
     c_args: xquartz_defs,
     link_args: ['-Objc'],
     install: true,
     install_dir: x11appdir,
)

# X11 (Bundle trampoline)
x11 = executable('X11',
     [
      'bundle_trampoline.c',
     ],
     c_args: xquartz_defs,
     link_args: ['-Objc'],
     install: true,
     install_dir: x11appdir,
)

# Xquartz
xquartz_deps = [
    dependency('CoreServices', method: 'extraframework'),
]

xquartz = executable('Xquartz',
    [
     'stub.c',
     'launchd_fd.c',
     mach_startup[3], # mach_startupUser.c
    ],
    include_directories: inc,
    c_args: xquartz_defs,
    dependencies: [xquartz_deps, mach_startup_dep],
    install: true,
)
