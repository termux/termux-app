srcs_miext_damage = [
	'damage.c',
]

hdrs_miext_damage = [
	'damage.h',
	'damagestr.h',
]

libxserver_miext_damage = static_library('libxserver_miext_damage',
	srcs_miext_damage,
	include_directories: inc,
	dependencies: common_dep,
)

if build_xorg
    install_data(hdrs_miext_damage, install_dir: xorgsdkdir)
endif
