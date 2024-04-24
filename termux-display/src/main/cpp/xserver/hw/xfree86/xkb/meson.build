srcs_xorg_xkb = [
    'xkbVT.c',
    'xkbPrivate.c',
    'xkbKillSrv.c',
]

xorg_xkb = static_library('xorg_xkb',
    srcs_xorg_xkb,
    include_directories: [inc, xorg_inc],
    dependencies: common_dep,
    c_args: xorg_c_args,
)
