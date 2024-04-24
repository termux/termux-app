project('libxcvt', 'c',
        version: '0.1.2',
        meson_version: '>= 0.40.0',
        default_options: ['warning_level=1',
                          'buildtype=debugoptimized'])

libcvt_version = meson.project_version().split('.')

cc = meson.get_compiler('c')
mdep = cc.find_library('m', required : false)

prefix  = get_option('prefix')
inc = include_directories('include')
man = join_paths(prefix, get_option('mandir'))

subdir('include')
subdir('lib')
subdir('cvt')
subdir('man')

pkg_mod = import('pkgconfig')
pkg_mod.generate(libraries : libxcvt,
                 version : meson.project_version(),
                 name : 'libxcvt',
                 description : 'A Library to generate VESA CVT standard timing modelines.')

libxcvt_dep = declare_dependency(link_with: libxcvt,
                                 include_directories: inc)
