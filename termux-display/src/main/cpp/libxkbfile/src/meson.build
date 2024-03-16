libxkbfile_sources = [
    'cout.c',
    'maprules.c',
    'srvmisc.c',
    'xkbatom.c',
    'xkbbells.c',
    'xkbconfig.c',
    'xkbdraw.c',
    'xkberrs.c',
    'XKBfileInt.h',
    'xkbmisc.c',
    'xkbout.c',
    'xkbtext.c',
    'xkmout.c',
    'xkmread.c'
]

deps = [
    dependency('kbproto'),
    dependency('x11')
]

includes = include_directories(
    '../include/',
    '../include/X11/extensions/'
)

library('xkbfile',
        dependencies: deps,
        include_directories: includes,
        install: true,
        sources: libxkbfile_sources,
        version: '1.0.2'
)

libxkbfile_headers = [
    '../include/X11/extensions/XKM.h',
    '../include/X11/extensions/XKBrules.h',
    '../include/X11/extensions/XKBbells.h',
    '../include/X11/extensions/XKBconfig.h',
    '../include/X11/extensions/XKMformat.h',
    '../include/X11/extensions/XKBfile.h'
]
install_headers(libxkbfile_headers,
                subdir: 'X11' / 'extensions')

