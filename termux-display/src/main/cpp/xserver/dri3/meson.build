srcs_dri3 = [
	'dri3.c',
	'dri3_request.c',
	'dri3_screen.c',
]

hdrs_dri3 = [
	'dri3.h',
]

libxserver_dri3 = []
if build_dri3
    libxserver_dri3 = static_library('libxserver_dri3',
        srcs_dri3,
        include_directories: inc,
        dependencies: [ common_dep, libdrm_dep ],
        )
endif

if build_xorg
    install_data(hdrs_dri3, install_dir: xorgsdkdir)
endif
