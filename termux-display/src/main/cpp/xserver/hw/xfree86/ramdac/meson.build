srcs_xorg_ramdac = [
    'xf86CursorRD.c',
    'xf86HWCurs.c',
]

xorg_ramdac = static_library('xorg_ramdac',
    srcs_xorg_ramdac,
    include_directories: [inc, xorg_inc],
    dependencies: common_dep,
    c_args: xorg_c_args,
)

install_data(
    [
        'xf86Cursor.h',
    ],
    install_dir: xorgsdkdir,
)
