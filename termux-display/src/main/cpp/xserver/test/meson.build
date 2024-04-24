simple_xinit = executable(
    'simple-xinit',
    'simple-xinit.c',
    include_directories: inc,
)

piglit_env = environment()
piglit_env.set('XSERVER_DIR', meson.source_root())
piglit_env.set('XSERVER_BUILDDIR', meson.build_root())

some_ops = ' -o clear,src,dst,over,xor,disjointover'
rendercheck_tests = [
    ['blend/All/a8r8g8b8', '-t blend -f a8r8g8b8'],
    ['blend/All/x8r8g8b8', '-t blend -f a8r8g8b8,x8r8g8b8'],
    ['blend/All/a2r10g10b10', '-t blend -f a8r8g8b8,a2r10g10b10'],
    ['blend/Clear', '-t blend -o clear'],
    ['blend/Src', '-t blend -o src'],
    ['blend/Over', '-t blend -o over'],
    ['composite/Some/a8r8g8b8', '-t composite -f a8r8g8b8' + some_ops],
    ['composite/Some/x8r8g8b8', '-t composite -f a8r8g8b8,x8r8g8b8' + some_ops],
    ['composite/Some/a2r10g10b10', '-t composite -f a8r8g8b8,a2r10g10b10' + some_ops],
    ['ca composite/Some/a8r8g8b8', '-t cacomposite -f a8r8g8b8' + some_ops],
    ['ca composite/Some/a8', '-t cacomposite -f a8r8g8b8,a8' + some_ops],
    ['ca composite/Some/x8r8g8b8', '-t cacomposite -f a8r8g8b8,x8r8g8b8' + some_ops],
    ['ca composite/Some/a2r10g10b10', '-t cacomposite -f a8r8g8b8,a2r10g10b10' + some_ops],
    ['fill', '-t fill'],
    ['bug7366', '-t bug7366'],
    ['destination coordinates', '-t dcoords'],
    ['source coordinates', '-t scoords'],
    ['mask coordinates', '-t mcoords'],
    ['translated source coordinates', '-t tscoords'],
    ['translated mask coordinates', '-t tmcoords'],
    ['triangles', '-t triangles'],
    ['LibreOffice xRGB', '-t libreoffice_xrgb'],
    ['GTK ARGB vs xBGR', '-t gtk_argb_xbgr'],
]

rendercheck = find_program('rendercheck', required:false)

if get_option('xvfb')
    xvfb_args = [
        xvfb_server.full_path(),
        '-screen',
        'scrn',
        '1280x1024x24',
    ]

    test('XTS',
        find_program('scripts/xvfb-piglit.sh'),
        env: piglit_env,
        timeout: 1200,
        suite: 'xvfb'
    )

    if rendercheck.found()
        foreach rctest: rendercheck_tests
            test(rctest[0],
                 simple_xinit,
                 # Use full_path so people can copy and paste the
                 # command line from testlog.txt easily.
                 args: [
                     rendercheck.path(),
                     rctest[1].split(' '),
                     '--',
                     xvfb_args,
                 ],
                 suite: 'xvfb'
                )
        endforeach
    endif

    if get_option('xephyr') and build_glamor
        test('XTS',
            find_program('scripts/xephyr-glamor-piglit.sh'),
            env: piglit_env,
            timeout: 1200,
            suite: 'xephr-glamor',
        )

        if rendercheck.found()
            foreach rctest: rendercheck_tests
                test(rctest[0],
                     simple_xinit,
                     args: [simple_xinit.full_path(),
                            rendercheck.path(),
                            rctest[1].split(' '),
                            '----',
                            xephyr_server.full_path(),
                            '-glamor',
                            '-glamor-skip-present',
                            '-schedMax', '2000',
                            '--',
                            xvfb_args,
                           ],
                     suite: 'xephr-glamor',
                     timeout: 300,
                    )
            endforeach
        endif
    endif
endif

subdir('bigreq')
subdir('damage')
subdir('sync')

if build_xorg
# Tests that require at least some DDX functions in order to fully link
# For now, requires xf86 ddx, could be adjusted to use another
    unit_sources = [
     '../mi/miinitext.c',
     '../mi/miinitext.h',
     '../mi/micmap.c',
     '../mi/micmap.h',
     'fixes.c',
     'input.c',
     'list.c',
     'misc.c',
     'signal-logging.c',
     'string.c',
     'test_xkb.c',
     'tests-common.c',
     'tests.c',
     'touch.c',
     'xfree86.c',
     'xtest.c',
    ]
    unit_defines = ['-DXORG_TESTS']
    unit_includes = [inc, xorg_inc]

    if build_res
        unit_sources += ['hashtabletest.c']
        unit_defines += ['-DRES_TESTS']
    endif

    unit_c_args = unit_defines
    if meson.get_compiler('c').has_link_argument('-Wl,-wrap')
       # LTO breaks with -Wl,-wrap on certain configurations
       unit_c_args += ['-fno-lto']
       unit_sources += [
        'xi1/protocol-xchangedevicecontrol.c',
        'xi2/protocol-common.c',
        'xi2/protocol-xiqueryversion.c',
        'xi2/protocol-xiquerydevice.c',
        'xi2/protocol-xiselectevents.c',
        'xi2/protocol-xigetselectedevents.c',
        'xi2/protocol-xisetclientpointer.c',
        'xi2/protocol-xigetclientpointer.c',
        'xi2/protocol-xiquerypointer.c',
        'xi2/protocol-xipassivegrabdevice.c',
        'xi2/protocol-xiwarppointer.c',
        'xi2/protocol-eventconvert.c',
        'xi2/xi2.c',
       ]
       unit_defines += ['-DLDWRAP_TESTS']
       unit_includes += [include_directories('xi1', 'xi2')]
       ldwraps = [
        '-Wl,-wrap,dixLookupWindow',
        '-Wl,-wrap,dixLookupClient',
        '-Wl,-wrap,WriteToClient',
        '-Wl,-wrap,dixLookupWindow',
        '-Wl,-wrap,XISetEventMask',
        '-Wl,-wrap,AddResource',
        '-Wl,-wrap,GrabButton',
       ]
    else
       ldwraps = []
       message('ld -wrap required for xi1 & xi2 unit tests, skipping')
    endif

    unit = executable('tests',
         unit_sources,
         c_args: unit_c_args,
         dependencies: [pixman_dep, randrproto_dep, inputproto_dep, libxcvt_dep],
         include_directories: unit_includes,
         link_args: ldwraps,
         link_with: xorg_link,
    )

    test('unit', unit)
endif
