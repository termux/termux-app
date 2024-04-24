xcb_dep = dependency('xcb', required: false)
xcb_xinput_dep = dependency('xcb-xinput', required: false)

if get_option('xvfb')
    if xcb_dep.found() and xcb_xinput_dep.found()
        requestlength = executable('request-length', 'request-length.c',
                                   dependencies: [xcb_dep, xcb_xinput_dep])
        test('request-length', simple_xinit, args: [requestlength, '--', xvfb_server])
    endif
endif
