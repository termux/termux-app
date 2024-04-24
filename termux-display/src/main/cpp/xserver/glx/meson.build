srcs_glx = [
    'indirect_dispatch.c',
    'indirect_dispatch_swap.c',
    'indirect_reqsize.c',
    'indirect_size_get.c',
    'indirect_table.c',
    'clientinfo.c',
    'createcontext.c',
    'extension_string.c',
    'indirect_util.c',
    'indirect_program.c',
    'indirect_texture_compression.c',
    'glxcmds.c',
    'glxcmdsswap.c',
    'glxext.c',
    'glxdriswrast.c',
    'glxdricommon.c',
    'glxscreens.c',
    'render2.c',
    'render2swap.c',
    'renderpix.c',
    'renderpixswap.c',
    'rensize.c',
    'single2.c',
    'single2swap.c',
    'singlepix.c',
    'singlepixswap.c',
    'singlesize.c',
    'swap_interval.c',
    'xfont.c',
]

libxserver_glx = []
if build_glx
    libxserver_glx = static_library('libxserver_glx',
        srcs_glx,
        include_directories: inc,
        dependencies: [
            common_dep,
            dl_dep,
            dri_dep,
            dependency('glproto', version: '>= 1.4.17'),
            dependency('gl', version: '>= 1.2'),
        ],
        c_args: [
            glx_align64,
            # XXX: generated code includes an unused function
            '-Wno-unused-function',
        ]
    )
endif

srcs_glxdri2 = []
if build_dri2 or build_dri3
    srcs_glxdri2 = files('glxdri2.c')
endif

srcs_vnd = [
    'vndcmds.c',
    'vndext.c',
    'vndservermapping.c',
    'vndservervendor.c',
]

hdrs_vnd = [
    'vndserver.h',
]

libglxvnd = []
if build_glx
    libglxvnd = static_library('libglxvnd',
    srcs_vnd,
    include_directories: inc,
        dependencies: [
            common_dep,
            dl_dep,
            dependency('glproto', version: '>= 1.4.17'),
            dependency('gl', version: '>= 1.2'),
        ],
    )

    if build_xorg
        install_data(hdrs_vnd, install_dir : xorgsdkdir)
    endif
endif
