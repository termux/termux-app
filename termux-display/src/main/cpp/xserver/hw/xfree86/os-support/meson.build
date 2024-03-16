srcs_xorg_os_support = [
    'bus/nobus.c',
    'shared/posix_tty.c',
    'shared/sigio.c',
    'shared/vidmem.c',
]

hdrs_xorg_os_support = [
    'bus/xf86Pci.h',
    'xf86_OSlib.h',
    'xf86_OSproc.h'
]

os_dep = []
os_c_args = []

if get_option('pciaccess')
    srcs_xorg_os_support += 'bus/Pci.c'
    if host_machine.system() != 'linux'
        srcs_xorg_os_support += 'bus/bsd_pci.c'
    endif
    if host_machine.cpu() == 'sparc' or host_machine.cpu() == 'sparc64'
        srcs_xorg_os_support += 'bus/Sbus.c'
        install_data('bus/xf86Sbus.h', install_dir: xorgsdkdir)
    endif
endif

if get_option('agp') == 'auto'
    build_agp = cc.has_header('sys/agpio.h') or cc.has_header('sys/agpgart.h') or cc.has_header('linux/agpgart.h')
else
    build_agp = get_option('agp') == 'true'
endif

if host_machine.system() == 'linux'
    srcs_xorg_os_support += [
        'linux/lnx_bell.c',
        'linux/lnx_init.c',
        'linux/lnx_kmod.c',
        'linux/lnx_platform.c',
        'linux/lnx_video.c',
        'misc/SlowBcopy.c',
        'shared/VTsw_usl.c',
    ]
    if build_agp
        srcs_xorg_os_support += 'linux/lnx_agp.c'
    else
        srcs_xorg_os_support += 'shared/agp_noop.c'
    endif
    if build_systemd_logind
        srcs_xorg_os_support += 'linux/systemd-logind.c'
    endif

    # this is ugly because the code is also
    if build_apm or build_acpi
        srcs_xorg_os_support += 'linux/lnx_apm.c'
        if build_acpi
            srcs_xorg_os_support += 'linux/lnx_acpi.c'
        endif
    else
        srcs_xorg_os_support += 'shared/pm_noop.c'
    endif

elif host_machine.system() == 'sunos'
    srcs_xorg_os_support += [
        'solaris/sun_apm.c',
        'solaris/sun_bell.c',
        'solaris/sun_init.c',
        'solaris/sun_vid.c',
        'shared/kmod_noop.c',
    ]

    if cc.has_header('sys/vt.h')
        srcs_xorg_os_support += 'solaris/sun_VTsw.c'
    else
        srcs_xorg_os_support += 'shared/VTsw_noop.c'
    endif

    if build_agp
        srcs_xorg_os_support += 'solaris/sun_agp.c'
    else
        srcs_xorg_os_support += 'shared/agp_noop.c'
    endif

    if host_machine.cpu_family() == 'sparc'
        srcs_xorg_os_support += 'solaris/solaris-sparcv8plus.S'
    elif host_machine.cpu_family() == 'x86_64'
        srcs_xorg_os_support += 'solaris/solaris-amd64.S'
    elif host_machine.cpu_family() == 'x86'
        srcs_xorg_os_support += 'solaris/solaris-ia32.S'
    else
        error('Unknown CPU family for Solaris build')
    endif

elif host_machine.system().endswith('bsd')
    srcs_xorg_os_support += [
        'bsd/bsd_VTsw.c',
        'bsd/bsd_bell.c',
        'bsd/bsd_init.c',
	'shared/pm_noop.c'
    ]

    if host_machine.cpu_family() == 'x86_64'
        srcs_xorg_os_support += 'bsd/i386_video.c'
        if host_machine.system() == 'netbsd'
            os_dep += cc.find_library('x86_64')
        elif host_machine.system() == 'openbsd'
            os_dep += cc.find_library('amd64')
        endif
    elif host_machine.cpu_family() == 'x86'
        srcs_xorg_os_support += 'bsd/i386_video.c'
        if host_machine.system() == 'netbsd' or host_machine.system() == 'openbsd'
            os_dep += cc.find_library('i386')
        endif
    elif host_machine.cpu_family() == 'arm'
        srcs_xorg_os_support += 'bsd/arm_video.c'
    elif host_machine.cpu_family() == 'powerpc'
        srcs_xorg_os_support += 'bsd/ppc_video.c'
    elif host_machine.cpu_family() == 'sparc64'
        srcs_xorg_os_support += 'bsd/sparc64_video.c'
        srcs_xorg_os_support += 'shared/ioperm_noop.c'
    elif host_machine.cpu_family() == 'alpha'
        srcs_xorg_os_support += 'bsd/alpha_video.c'
        if host_machine.system() == 'freebsd'
            os_dep += cc.find_library('io')
        elif host_machine.system() == 'netbsd'
            os_c_args += '-DUSE_ALPHA_PIO'
        endif
    endif

    if host_machine.system() == 'freebsd'
        srcs_xorg_os_support += 'bsd/bsd_kmod.c'
    else
        srcs_xorg_os_support += 'shared/kmod_noop.c'
    endif

    if build_agp
        srcs_xorg_os_support += 'linux/lnx_agp.c'
    else
        srcs_xorg_os_support += 'shared/agp_noop.c'
    endif
else
    # stub ossupport
    srcs_xorg_os_support += [
        'shared/VTsw_noop.c',
        'shared/agp_noop.c',
        'shared/ioperm_noop.c',
        'shared/kmod_noop.c',
        'shared/pm_noop.c',
        'shared/vidmem.c',
        'shared/posix_tty.c',
        'shared/sigio.c',
        'stub/stub_bell.c',
        'stub/stub_init.c',
        'stub/stub_video.c',
    ]
endif

xorg_os_support = static_library('xorg_os_support',
    srcs_xorg_os_support,
    include_directories: [inc, xorg_inc],
    dependencies: [
        common_dep,
        dbus_dep,
        libdrm_dep,
        os_dep,
    ],
    c_args: xorg_c_args + os_c_args,
)

install_data(hdrs_xorg_os_support, install_dir: xorgsdkdir)
