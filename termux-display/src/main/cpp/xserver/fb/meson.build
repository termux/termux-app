srcs_fb = [
	'fballpriv.c',
	'fbarc.c',
	'fbbits.c',
	'fbblt.c',
	'fbbltone.c',
	'fbcmap_mi.c',
	'fbcopy.c',
	'fbfill.c',
	'fbfillrect.c',
	'fbfillsp.c',
	'fbgc.c',
	'fbgetsp.c',
	'fbglyph.c',
	'fbimage.c',
	'fbline.c',
	'fboverlay.c',
	'fbpict.c',
	'fbpixmap.c',
	'fbpoint.c',
	'fbpush.c',
	'fbscreen.c',
	'fbseg.c',
	'fbsetsp.c',
	'fbsolid.c',
	'fbtrap.c',
	'fbutil.c',
	'fbwindow.c',
]

hdrs_fb = [
	'fb.h',
	'fboverlay.h',
	'fbpict.h',
	'fbrop.h',
	'wfbrename.h'
]

libxserver_fb = static_library('libxserver_fb',
	srcs_fb,
	include_directories: inc,
	dependencies: common_dep,
	pic: true,
)

wfb_args = '-DFB_ACCESS_WRAPPER'

libxserver_wfb = static_library('libxserver_wfb',
	srcs_fb,
	c_args: wfb_args,
	include_directories: inc,
	dependencies: common_dep,
	pic: true,
	build_by_default: false,
)

if build_xorg
    install_data(hdrs_fb, install_dir: xorgsdkdir)
endif
