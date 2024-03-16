srcs_exa = [
    'exa.c',
    'exa_classic.c',
    'exa_migration_classic.c',
    'exa_driver.c',
    'exa_mixed.c',
    'exa_migration_mixed.c',
    'exa_accel.c',
    'exa_glyphs.c',
    'exa_offscreen.c',
    'exa_render.c',
    'exa_unaccel.c',
]

libxserver_exa = static_library('libxserver_exa',
    srcs_exa,
    include_directories: inc,
    dependencies: common_dep,
    c_args: '-DHAVE_XORG_CONFIG_H'
)

if build_xorg
    install_data('exa.h', install_dir: xorgsdkdir)
endif
