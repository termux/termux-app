srcs_config = [
    'config.c',
]

config_dep = [common_dep]

if build_dbus
    srcs_config += 'dbus-core.c'
    config_dep += dbus_dep
endif

if build_hal
    srcs_config += 'hal.c'
    config_dep += hal_dep
endif

if build_udev
    srcs_config += 'udev.c'
    config_dep += udev_dep
endif

if host_machine.system() == 'openbsd'
    srcs_config += 'wscons.c'
endif

if build_xorg
    install_data('10-quirks.conf',
                 install_dir: join_paths(get_option('datadir'), 'X11/xorg.conf.d'))
endif

libxserver_config = static_library('libxserver_config',
    srcs_config,
    include_directories: inc,
    dependencies: config_dep,
)
