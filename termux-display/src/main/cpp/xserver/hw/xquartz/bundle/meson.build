# generate Info.plist
# https://github.com/mesonbuild/meson/issues/8434
#cpp = find_program('cpp')
cpp = '/usr/bin/cpp'
cpp_defs = [
    '-DAPPLE_APPLICATION_NAME=@0@'.format(apple_application_name),
    '-DBUNDLE_ID_PREFIX=@0@'.format(bundle_id_prefix),
    '-DBUNDLE_VERSION=@0@'.format(bundle_version),
    '-DBUNDLE_VERSION_STRING=@0@'.format(bundle_version_string),
]

if build_sparkle
cpp_defs += sparkle_defs
endif

# bundle data
localities = [
    'Dutch', 'English', 'French', 'German', 'Italian', 'Japanese', 'Spanish',
    'ar','ca','cs','da','el','fi','he','hr','hu','ko','no','pl','pt','pt_PT',
    'ro','ru','sk','sv','th','tr','uk','zh_CN','zh_TW'
]
foreach lang : localities
    install_data(join_paths('Resources', lang + '.lproj', 'Localizable.strings'),
        install_dir: join_paths(bundle_root, 'Contents/Resources', lang + '.lproj'),
        install_mode: 'rw-r--r--')

    install_data(join_paths('Resources', lang + '.lproj', 'main.nib/keyedobjects.nib'),
        install_dir: join_paths(bundle_root, 'Contents/Resources', lang + '.lproj', 'main.nib'),
        install_mode: 'rw-r--r--')
endforeach

install_data('Resources/English.lproj/main.nib/designable.nib',
    install_dir: join_paths(bundle_root, 'Contents/Resources/English.lproj/main.nib'),
    install_mode: 'rw-r--r--')

install_data('Resources/X11.icns',
    install_dir: join_paths(bundle_root, 'Contents/Resources'),
    install_mode: 'rw-r--r--')

custom_target('Info.plist',
    command: [cpp, '-P', cpp_defs, '@INPUT@'],
    capture: true,
    input: 'Info.plist.cpp',
    output: 'Info.plist',
    install: true,
    install_dir: join_paths(bundle_root, 'Contents'),
    install_mode: 'rw-r--r--',
    build_by_default: true,
)

install_data('PkgInfo',
    install_dir: join_paths(bundle_root, 'Contents'),
    install_mode: 'rw-r--r--')

install_data('X11.sh',
    install_dir: join_paths(bundle_root, 'Contents/MacOS'),
    install_mode: 'rwxr-xr-x')

meson.add_install_script('chown-bundle.sh', bundle_root)
