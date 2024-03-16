srcs_kdrive = [
    'kcmap.c',
    'kdrive.c',
    'kinfo.c',
    'kinput.c',
    'kshadow.c',
    '../../../mi/miinitext.c',
    '../../../mi/miinitext.h',
]

if build_xv
    srcs_kdrive += 'kxv.c'
endif

kdrive = static_library('kdrive',
    srcs_kdrive,
    include_directories: inc,
    dependencies: common_dep,
    link_with: libxserver_miext_shadow,
)
