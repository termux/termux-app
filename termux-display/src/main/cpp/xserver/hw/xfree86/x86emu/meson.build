srcs_xorg_x86emu = [
    'debug.c',
    'decode.c',
    'fpu.c',
    'ops2.c',
    'ops.c',
    'prim_ops.c',
    'sys.c',
]

xorg_x86emu = static_library('x86emu',
    srcs_xorg_x86emu,
    include_directories: [inc, xorg_inc],
    dependencies: common_dep,
)
