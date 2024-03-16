# Copyright © 2018 Intel Corporation

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# keep version in sync with configure.ac
project('xorgproto', 'c', license : 'MIT', version : '2023.2',
        meson_version : '>=0.56.0')
cc = meson.get_compiler('c')

pcs = [
        'applewmproto',
        'bigreqsproto',
        'compositeproto',
        'damageproto',
        'dmxproto',
        'dpmsproto',
        'dri2proto',
        'dri3proto',
        'fixesproto',
        'fontsproto',
        'glproto',
        'inputproto',
        'kbproto',
        'presentproto',
        'randrproto',
        'recordproto',
        'renderproto',
        'resourceproto',
        'scrnsaverproto',
        'videoproto',
        'xcmiscproto',
        'xextproto',
        'xf86bigfontproto',
        'xf86dgaproto',
        'xf86driproto',
        'xf86vidmodeproto',
        'xineramaproto',
        'xproto',
        'xwaylandproto',
]

pc_data = configuration_data()
pc_data.set('prefix', get_option('prefix'))
# meson does not allow installing the includedir outside of the prefix
pc_data.set('includedir', '${prefix}/' + get_option('includedir'))

foreach pc : pcs
    configure_file(
        input : pc + '.pc.in',
        output : pc + '.pc',
        install_dir : get_option('datadir') + '/pkgconfig',
        configuration : pc_data,
    )
endforeach


docs = files('compositeproto.txt',
             'damageproto.txt',
             'dri2proto.txt',
             'dri3proto.txt',
             'fixesproto.txt',
             'presentproto.txt',
             'randrproto.txt',
             'renderproto.txt',
             'resproto.txt',
             'xv-protocol-v2.txt',
             'xwaylandproto.txt')
install_data(docs,
             install_dir: get_option('datadir') / 'doc' / 'xorgproto')


if get_option('legacy') == true
    legacy_pcs = [
        'evieproto',
        'fontcacheproto',
        'lg3dproto',
        'printproto',
        'trapproto',
        'windowswmproto',
        'xcalibrateproto',
        'xf86miscproto',
        'xf86rushproto',
        'xproxymngproto',
    ]
    foreach pc : legacy_pcs
        configure_file(
            input : pc + '.pc.in',
            output : pc + '.pc',
            install_dir : get_option('datadir') + '/pkgconfig',
            configuration : pc_data,
        )
    endforeach
    legacy_docs = files('PM_spec')
    install_data(legacy_docs,
                 install_dir: get_option('datadir') / 'doc' / 'xorgproto')
endif

ext_xorgproto = declare_dependency(
    include_directories : [
        include_directories('include'),
        include_directories('include' / 'GL'),
        include_directories('include' / 'GL' / 'internal'),
        include_directories('include' / 'X11'),
        include_directories('include' / 'X11' / 'dri'),
        include_directories('include' / 'X11' / 'extensions'),
        include_directories('include' / 'X11' / 'fonts'),
        include_directories('include' / 'X11' / 'PM'),
    ]
)

subdir('include')

keysymfile = meson.project_source_root() / 'include' / 'X11' / 'XF86keysym.h'
test('evdev-keysym-check',
     find_program('scripts/keysym-generator.py'),
     args: ['-v', '--header', keysymfile, 'verify'])
