srcs_os = [
    'WaitFor.c',
    'access.c',
    'auth.c',
    'backtrace.c',
    'client.c',
    'connection.c',
    'inputthread.c',
    'io.c',
    'mitauth.c',
    'oscolor.c',
    'osinit.c',
    'ospoll.c',
    'utils.c',
    'xdmauth.c',
    'xsha1.c',
    'xstrans.c',
    'xprintf.c',
    'log.c',
]

# Wrapper code for missing C library functions. Note that conf_data contains either '1' or false.
srcs_libc = []
if conf_data.get('HAVE_REALLOCARRAY').to_int() == 0
    srcs_libc += 'reallocarray.c'
endif
if conf_data.get('HAVE_STRCASECMP').to_int() == 0
    srcs_libc += 'strcasecmp.c'
endif
if conf_data.get('HAVE_STRCASESTR').to_int() == 0
    srcs_libc += 'strcasestr.c'
endif
if conf_data.get('HAVE_STRLCAT').to_int() == 0
    srcs_libc += 'strlcat.c'
endif
if conf_data.get('HAVE_STRLCPY').to_int() == 0
    srcs_libc += 'strlcpy.c'
endif
if conf_data.get('HAVE_STRNDUP').to_int() == 0
    srcs_libc += 'strndup.c'
endif
if conf_data.get('HAVE_TIMINGSAFE_MEMCMP').to_int() == 0
    srcs_libc += 'timingsafe_memcmp.c'
endif
if conf_data.get('HAVE_POLL').to_int() == 0
    srcs_os += 'xserver_poll.c'
endif

if conf_data.get('BUSFAULT').to_int() != 0
    srcs_os += 'busfault.c'
endif

if get_option('xdmcp')
    srcs_os += 'xdmcp.c'
endif

rpc_dep = []
if get_option('secure-rpc')
    # prefer libtirpc (if available), otherwise ensure RPC functions are
    # provided by libc.
    rpc_dep = dependency('libtirpc', required: false)
    if not (rpc_dep.found() or cc.has_header('rpc/rpc.h'))
        error('secure-rpc requested, but neither libtirpc or libc RPC support were found')
    endif
    # XXX: also check if RPC library provides xdr_opaque_auth, authdes_(sec)create ???
    srcs_os += 'rpcauth.c'
endif

os_dep = []
os_c_args = []
if get_option('xres')
    # Only the XRes extension cares about the client ID.
    os_c_args += '-DCLIENTIDS'
    if host_machine.system() == 'openbsd'
        os_dep += cc.find_library('kvm')
    endif
endif

libxlibc = []
if srcs_libc.length() > 0
    libxlibc = static_library('libxlibc',
        srcs_libc,
        include_directories: inc,
        dependencies: [
            xproto_dep,
        ],
    )
endif

if enable_input_thread
    os_dep += cc.find_library('pthread')
endif

libxserver_os = static_library('libxserver_os',
    srcs_os,
    include_directories: inc,
    dependencies: [
        dtrace_dep,
        common_dep,
        dl_dep,
        sha1_dep,
        rpc_dep,
        os_dep,
        dependency('xau')
    ],
    c_args: os_c_args,
    link_with: libxlibc,
)
