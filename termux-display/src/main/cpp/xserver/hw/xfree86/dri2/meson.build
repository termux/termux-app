srcs_xorg_dri2 = [
    'dri2.c',
    'dri2ext.c',
]

xorg_dri2 = static_library('xorg_dri2',
    srcs_xorg_dri2,
    include_directories: [inc, xorg_inc],
    dependencies: [
        common_dep,
        libdrm_dep,
    ],
    c_args: xorg_c_args,
)

install_data('dri2.h', install_dir: xorgsdkdir)
