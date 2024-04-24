shared_module('vgahw',
    [ 'vgaHW.c', 'vgaHWmodule.c'],
    include_directories: [ inc, xorg_inc ],
    dependencies: common_dep,
    c_args: xorg_c_args,
    install: true,
    install_dir: module_dir,
)

install_data('vgaHW.h', install_dir: xorgsdkdir)
