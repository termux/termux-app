xcb_dep = dependency('xcb', required: false)
xcb_damage_dep = dependency('xcb-damage', required: false)

if get_option('xvfb')
    if xcb_dep.found() and xcb_damage_dep.found()
        damage_primitives = executable('damage-primitives', 'primitives.c', dependencies: [xcb_dep, xcb_damage_dep])
        test('damage-primitives', simple_xinit, args: [damage_primitives, '--', xvfb_server])
    endif
endif
