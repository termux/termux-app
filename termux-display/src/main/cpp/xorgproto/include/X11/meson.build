fd_set_maybe_headers = [ 'sys/param.h',
                         'sys/types.h',
                         'sys/time.h',
                         'sys/select.h'
]
fd_set_headers = ''
foreach h : fd_set_maybe_headers
    if cc.has_header(h)
        fd_set_headers += '#include <' + h + '>\n'
    endif
endforeach

# generated headers. try not to make more of these, please.
if cc.has_member('fd_set', 'fds_bits', prefix: fd_set_headers)
    fds_bits = 'fds_bits'
elif cc.has_member('fd_set', '__fds_bits', prefix: fd_set_headers)
    fds_bits = '__fds_bits'
else
    error('Your fd_set is too weird.')
endif
xpoll_config = configuration_data()
xpoll_config.set('USE_FDS_BITS', fds_bits)
configure_file(input : 'Xpoll.h.in',
               output : 'Xpoll.h',
               install : true,
               install_dir : get_option('includedir') + '/X11',
               configuration: xpoll_config)

install_headers(
    'ap_keysym.h',
    'DECkeysym.h',
    'HPkeysym.h',
    'keysymdef.h',
    'keysym.h',
    'Sunkeysym.h',
    'Xalloca.h',
    'Xarch.h',
    'Xatom.h',
    'Xdefs.h',
    'XF86keysym.h',
    'Xfuncs.h',
    'Xfuncproto.h',
    'X.h',
    'Xmd.h',
    'Xosdefs.h',
    'Xos.h',
    'Xos_r.h',
    'Xproto.h',
    'Xprotostr.h',
    'Xthreads.h',
    'Xw32defs.h',
    'XWDFile.h',
    'Xwindows.h',
    'Xwinsock.h',
    subdir : 'X11')

subdir('dri')
subdir('extensions')
subdir('fonts')
if get_option('legacy') == true
    subdir('PM')
endif
