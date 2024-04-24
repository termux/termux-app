xorg_inc = include_directories(
    'common',
    'ddc',
    'dri2',
    'i2c',
    'int10',
    'loader',
    'modes',
    'os-support',
    'os-support/bus',
    'parser',
    'ramdac',
    'vgahw',
)

xorg_c_args = []
xorg_c_args += '-DHAVE_XORG_CONFIG_H'
xorg_c_args += '-DXORG_NO_SDKSYMS'

pciaccess_dep = []
if get_option('pciaccess')
    pciaccess_dep = dependency('pciaccess', version: '>= 0.12.901')
endif

# subdirs for convenience libraries statically linked into Xorg
subdir('common')
subdir('ddc')
if build_dri1
    subdir('dri')
endif
if build_dri2
    subdir('dri2')
endif
subdir('i2c')
subdir('loader')
subdir('modes')
subdir('os-support')
subdir('parser')
subdir('ramdac')
subdir('xkb')

srcs_xorg = [
    '../../mi/miinitext.c',
    '../../mi/miinitext.h',
]

# Extract all the objects so that all symbols get brought into the
# server.  This prevents us from needing a global table of all symbols
# that should be exported to Xorg modules, at the expense of all
# symbols being included and public

xorg_link = [
    libxserver,
    libglxvnd,
    xorg_common,
    xorg_loader,
    xorg_ddc,
    xorg_xkb,
    xorg_i2c,
    xorg_modes,
    xorg_os_support,
    xorg_parser,
    xorg_ramdac,
    libxserver_fb,
    libxserver_xext_vidmode,
    libxserver_main,
    libxserver_config,
]
if build_dri1
    xorg_link += xorg_dri
endif
if build_dri2
    xorg_link += xorg_dri2
endif

if host_machine.system() == 'cygwin' or host_machine.system() == 'windows'
    linker_export_flags = '-Wl,--export-all-symbols'
elif host_machine.system() == 'sunos' or host_machine.system() == 'darwin'
    linker_export_flags = []
else
    linker_export_flags = '-Wl,--export-dynamic'
endif

xorg_deps = [
    pixman_dep,
    m_dep,
    dl_dep,
    pciaccess_dep,
    sha1_dep,
    dependency('xau'),
    xdmcp_dep,
    xfont2_dep,
    xshmfence_dep,
    config_dep,
    libdrm_dep,
]

if get_option('suid_wrapper')
    xorg_install_dir = get_option('libexecdir')
else
    xorg_install_dir = get_option('bindir')
endif

e = executable(
    'Xorg',
    srcs_xorg,
    include_directories: [inc, xorg_inc],
    link_whole: xorg_link,
    dependencies: xorg_deps,
    link_args: linker_export_flags,
    c_args: xorg_c_args,
    install: true,
    install_dir: xorg_install_dir,
    implib: true,
)

# subdirs for modules loadable by Xorg
subdir('dixmods')
subdir('exa')
subdir('fbdevhw')
if gbm_dep.found()
    subdir('glamor_egl')
endif
if int10 != 'false'
    if int10 == 'x86emu'
        subdir('x86emu')
    endif
    subdir('int10')
endif
subdir('shadowfb')
if build_vgahw
    subdir('vgahw')
endif
if build_modesetting
   subdir('drivers/modesetting')
endif
if get_option('xf86-input-inputtest')
    subdir('drivers/inputtest')
endif

meson.add_install_script(
    'sh', '-c',
    'ln -fs Xorg @0@@1@'.format(
        '${DESTDIR}',
        join_paths(get_option('prefix'), get_option('bindir'), 'X')))

if get_option('suid_wrapper')
    executable('Xorg.wrap',
        'xorg-wrapper.c',
        include_directories: [inc, xorg_inc],
        dependencies: xorg_deps,
        c_args: xorg_c_args,
        install: true,
        install_dir: get_option('libexecdir'),
        install_mode: ['r-sr-xr-x', 0, 0],
    )

    # meson gets confused when there are two targets of the same name
    # within the same directory, so we use a different intermediate name.
    xorg_sh = configure_file(
        input: 'Xorg.sh.in',
        output: 'Xorg.sh',
        configuration: conf_data,
    )

    install_data(
        xorg_sh,
        install_mode: 'rwxr-xr-x',
        install_dir: join_paths(get_option('prefix'), get_option('bindir')),
        rename: ['Xorg']
    )
endif

executable('gtf',
    'utils/gtf/gtf.c',
    include_directories: [inc, xorg_inc],
    dependencies: xorg_deps,
    c_args: xorg_c_args,
    install: true,
)

# For symbol presence testing only
xorgserver_lib = shared_library(
    'xorgserver',
    srcs_xorg,
    include_directories: [inc, xorg_inc],
    link_whole: xorg_link,
    dependencies: xorg_deps,
    link_args: linker_export_flags,
    c_args: xorg_c_args,
    install: false,
)

xorgserver_dep = declare_dependency(link_with: xorgserver_lib)

install_man(configure_file(
    input: 'man/Xorg.man',
    output: 'Xorg.1',
    configuration: manpage_config,
))

if get_option('suid_wrapper')
    install_man(configure_file(
        input: 'man/Xorg.wrap.man',
        output: 'Xorg.wrap.1',
        configuration: manpage_config,
    ))

    install_man(configure_file(
        input: 'man/Xwrapper.config.man',
        output: 'Xwrapper.config.5',
        configuration: manpage_config,
    ))
endif

install_man(configure_file(
    input: 'man/xorg.conf.man',
    output: 'xorg.conf.5',
    configuration: manpage_config,
))

install_man(configure_file(
    input: 'man/xorg.conf.d.man',
    output: 'xorg.conf.d.5',
    configuration: manpage_config,
))

install_man(configure_file(
    input: 'utils/man/gtf.man',
    output: 'gtf.1',
    configuration: manpage_config,
))

subdir('doc')
