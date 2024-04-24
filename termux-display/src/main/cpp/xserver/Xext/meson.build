srcs_xext = [
    'bigreq.c',
    'geext.c',
    'shape.c',
    'sleepuntil.c',
    'sync.c',
    'xcmisc.c',
    'xtest.c',
]

hdrs_xext = [
    'geext.h',
    'geint.h',
    'syncsdk.h',
]

if build_dpms
    srcs_xext += 'dpms.c'
endif

if build_mitshm
    srcs_xext += 'shm.c'
    hdrs_xext += ['shmint.h']
endif

if build_hashtable
    srcs_xext += 'hashtable.c'
endif

if build_res
    srcs_xext += 'xres.c'
endif

if build_screensaver
    srcs_xext += 'saver.c'
endif

if build_xace
    srcs_xext += 'xace.c'
    hdrs_xext += ['xace.h', 'xacestr.h']
endif

if build_xf86bigfont
    srcs_xext += 'xf86bigfont.c'
endif

if build_xinerama
    srcs_xext += ['panoramiX.c', 'panoramiXprocs.c', 'panoramiXSwap.c']
    hdrs_xext += ['panoramiX.h', 'panoramiXsrv.h']
endif

if build_xsecurity
    srcs_xext += ['security.c']
endif

if build_xselinux
    srcs_xext += ['xselinux_ext.c', 'xselinux_hooks.c', 'xselinux_label.c']
endif

if build_xv
    srcs_xext += ['xvmain.c', 'xvdisp.c', 'xvmc.c']
    hdrs_xext += ['xvdix.h', 'xvmcext.h']
endif

libxserver_xext = static_library('libxserver_xext',
    srcs_xext,
    include_directories: inc,
    dependencies: common_dep,
)

libxserver_xext_vidmode = static_library('libxserver_xext_vidmode',
    'vidmode.c',
    include_directories: inc,
    dependencies: common_dep,
)

if build_xorg
    install_data(hdrs_xext, install_dir: xorgsdkdir)
endif
