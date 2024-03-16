srcs_xorg_parser = [
    'Device.c',
    'Files.c',
    'Flags.c',
    'Input.c',
    'InputClass.c',
    'OutputClass.c',
    'Layout.c',
    'Module.c',
    'Video.c',
    'Monitor.c',
    'Pointer.c',
    'Screen.c',
    'Vendor.c',
    'read.c',
    'scan.c',
    'write.c',
    'DRI.c',
    'Extensions.c',
]

xorg_parser = static_library('xorg_parser',
    srcs_xorg_parser,
    include_directories: [inc, xorg_inc],
    dependencies: common_dep,
    c_args: [
        xorg_c_args,
        '-DDATADIR="' + join_paths(get_option('prefix'), get_option('datadir')) + '"',
    ],
)

install_data(['xf86Parser.h', 'xf86Optrec.h'], install_dir: xorgsdkdir)
