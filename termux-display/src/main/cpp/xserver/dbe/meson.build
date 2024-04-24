srcs_dbe = [
	'dbe.c',
	'midbe.c',
]

hdrs_dbe = [
	'dbestruct.h',
]

libxserver_dbe = static_library('libxserver_dbe',
	srcs_dbe,
	include_directories: inc,
	dependencies: common_dep,
)

if build_xorg
    install_data(hdrs_dbe, install_dir: xorgsdkdir)
endif
