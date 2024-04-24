srcs_windows_dri = [
    'windowsdri.c',
    'windowsdri.h',
]

xwin_windowsdri = static_library(
    'WindowsDRI',
    srcs_windows_dri,
    include_directories: [ inc, include_directories('../') ],
    dependencies: [
        windowsdri_dep,
        pixman_dep,
    ],
)
