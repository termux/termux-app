glamoregl_src = [
    '../../../glamor/glamor_egl.c',
    '../../../glamor/glamor_eglmodule.c',
    'glamor_xf86_xv.c',
]

shared_module(
    'glamoregl',
    glamoregl_src,

    include_directories: [inc, xorg_inc],
    c_args: xorg_c_args,
    dependencies: [
        common_dep,
        dependency('libdrm', version: '>= 2.4.46'),
        gbm_dep,
    ],
    link_with: glamor,

    install: true,
    install_dir: module_dir,
)
