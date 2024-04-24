srcs_xorg_loader = [
    'loader.c',
    'loadmod.c',
]

xorg_loader = static_library('xorg_loader',
    srcs_xorg_loader,
    include_directories: [inc, xorg_inc],
    dependencies: [common_dep, dl_dep],
    c_args: xorg_c_args,
)

xorg_symbol_test = executable('xorg_symbol_test',
    'symbol-test.c',
    dependencies: dl_dep,
)
