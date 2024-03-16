add_languages('objc')

apple_applications_dir = get_option('apple-applications-dir')
apple_application_name = get_option('apple-application-name')
bundle_id_prefix = get_option('bundle-id-prefix')
xquartz_sparkle_feed_url = get_option('sparkle-feed-url')
xquartz_sparkle_public_edkey = get_option('sparkle-public-edkey')

bundle_version_string = get_option('bundle-version-string') # CFBundleShortVersionString
if bundle_version_string == 'auto'
    bundle_version_string = meson.project_version()
endif

bundle_version = get_option('bundle-version') # CFBundleVersion
if bundle_version == 'auto'
    version_arr = bundle_version_string.split('.')
    version_major = version_arr[0].to_int()
    version_minor = version_arr[1].to_int()
    version_tiny = version_arr[2].to_int()
    bundle_version = '@0@.@1@.@2@'.format(version_major, version_minor, version_tiny)
endif

bundle_id_def = '-DBUNDLE_ID_PREFIX="@0@"'.format(bundle_id_prefix)

bundle_root = join_paths(apple_applications_dir, apple_application_name + '.app')
xquartz_data_dir = join_paths(get_option('prefix'), get_option('datadir'),'X11', 'XQuartz')

# using sparkle update framework?
build_sparkle = xquartz_sparkle_feed_url != '' and xquartz_sparkle_public_edkey != ''
if build_sparkle
    sparkle = dependency('Sparkle', method: 'extraframework')

    sparkle_defs = [
        '-DXQUARTZ_SPARKLE',
        '-DXQUARTZ_SPARKLE_FEED_URL=@0@'.format(xquartz_sparkle_feed_url),
        '-DXQUARTZ_SPARKLE_PUBLIC_EDKEY=@0@'.format(xquartz_sparkle_public_edkey),
    ]
else
    sparkle_defs = []
endif

# libxquartz
srcs_libxquartz = [
    'NSUserDefaults+XQuartzDefaults.m',
    'X11Application.m',
    'X11Controller.m',
    'applewm.c',
    'darwin.c',
    'darwinEvents.c',
    'darwinXinput.c',
    'keysym2ucs.c',
    'quartz.c',
    'quartzKeyboard.c',
    'quartzStartup.c',
    'quartzRandR.c',
    '../../mi/miinitext.c',
    '../../mi/miinitext.h',
]

libxquartz_defs = [
    '-DUSE_NEW_CLUT',
    '-DXFree86Server',
    '-DXQUARTZ',
    '-DXQUARTZ_DATA_DIR="@0@"'.format(xquartz_data_dir),
    '-DXSERVER_VERSION="@0@"'.format(meson.project_version()),
    bundle_id_def,
    sparkle_defs,
]

if cc.has_function('dispatch_async')
    libxquartz_defs += '-DHAVE_LIBDISPATCH'
endif

libXquartz = static_library('Xquartz',
    srcs_libxquartz,
    c_args: libxquartz_defs,
    objc_args: libxquartz_defs,
    include_directories: [inc, '../../pseudoramiX', '../../miext/rootless', top_dir_inc],
    dependencies: [xproto_dep, pixman_dep],
    link_with: [libxserver_pseudoramix, libxserver_miext_rootless],
)

subdir('bundle')
if build_glx
    subdir('GL')
endif
subdir('pbproxy')
subdir('xpr')
subdir('mach-startup')

# manpage
xquartz_man = configure_file(
    input: 'man/Xquartz.man',
    output: 'Xquartz.1',
    configuration: manpage_config,
)
install_man(xquartz_man)

install_data('defaults.plist',
    install_dir: xquartz_data_dir,
    install_mode: 'rw-r--r--')
