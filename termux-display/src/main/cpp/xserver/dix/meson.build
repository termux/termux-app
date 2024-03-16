srcs_dix = [
    'atom.c',
    'colormap.c',
    'cursor.c',
    'devices.c',
    'dispatch.c',
    'dixfonts.c',
    'main.c',
    'dixutils.c',
    'enterleave.c',
    'events.c',
    'eventconvert.c',
    'extension.c',
    'gc.c',
    'gestures.c',
    'getevents.c',
    'globals.c',
    'glyphcurs.c',
    'grabs.c',
    'initatoms.c',
    'inpututils.c',
    'pixmap.c',
    'privates.c',
    'property.c',
    'ptrveloc.c',
    'region.c',
    'registry.c',
    'resource.c',
    'selection.c',
    'swaprep.c',
    'swapreq.c',
    'tables.c',
    'touch.c',
    'window.c',
]

dtrace_src = []
if with_dtrace
    dtrace_object = generator(dtrace,
        output: '@BASENAME@.o',
        arguments: ['-G', '-s', '@INPUT@', '-o', '@OUTPUT@']
    )

    dtrace_src += dtrace_object.process(dtrace_tmpl)
endif

dtrace_dep = declare_dependency(sources: [dtrace_src, dtrace_hdr])

libxserver_dix = static_library('libxserver_dix',
    srcs_dix,
    include_directories: inc,
    dependencies: [ dtrace_dep, common_dep, ]
)

libxserver_main = static_library('libxserver_main',
    'stubmain.c',
    include_directories: inc,
    dependencies: common_dep,
)

install_data(
    'protocol.txt',
    install_dir: serverconfigdir,
)
