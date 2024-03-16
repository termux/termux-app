if host_machine.system() == 'linux'
    srcs_fbdevhw = 'fbdevhw.c'
else
    srcs_fbdevhw = 'fbdevhwstub.c'
endif

shared_module('fbdevhw',
    srcs_fbdevhw,
    include_directories: [ inc, xorg_inc ],
    dependencies: common_dep,
    c_args: xorg_c_args,
    install: true,
    install_dir: module_dir,
    link_with: e,
)

install_data('fbdevhw.h', install_dir: xorgsdkdir)

install_man(configure_file(
    input: 'man/fbdevhw.man',
    output: 'fbdevhw.4',
    configuration: manpage_config,
))
