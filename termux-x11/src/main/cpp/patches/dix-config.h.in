#pragma once

#define _DIX_CONFIG_H_ 1
#define HAVE_TYPEOF 1
#define MONOTONIC_CLOCK 1
#undef XSERVER_DTRACE
#define X_BYTE_ORDER X_LITTLE_ENDIAN
#define _GNU_SOURCE 1
#undef HAS_APERTURE_DRV
#define INPUTTHREAD 1
#define HAVE_PTHREAD_SETNAME_NP_WITH_TID 1
#undef HAVE_PTHREAD_SETNAME_NP_WITHOUT_TID

#undef HAVE_LIBBSD
#undef HAVE_SYSTEMD_DAEMON
#undef CONFIG_UDEV
#undef CONFIG_UDEV_KMS
#undef HAVE_DBUS
#undef CONFIG_HAL
#undef SYSTEMD_LOGIND
#undef NEED_DBUS
#undef CONFIG_WSCONS

#define HAVE_XSHMFENCE 1
#undef WITH_LIBDRM
#undef GLAMOR_HAS_EGL_QUERY_DMABUF
#undef GLAMOR_HAS_EGL_QUERY_DRIVER
#define GLXEXT 1
#undef GLAMOR
#undef GLAMOR_HAS_GBM
#undef GLAMOR_HAS_GBM_LINEAR
#undef GBM_BO_WITH_MODIFIERS
#undef GBM_BO_FD_FOR_PLANE

#define SERVER_MISC_CONFIG_PATH "/usr/lib/xorg"
#define PROJECTROOT "/usr"
#define SYSCONFDIR "/usr/etc"
#define SUID_WRAPPER_DIR "/usr/libexec"
#define COMPILEDDEFAULTFONTPATH "/usr/share/fonts/X11/misc,/usr/share/fonts/X11/TTF,/usr/share/fonts/X11/OTF,/usr/share/fonts/X11/Type1,/usr/share/fonts/X11/100dpi,/usr/share/fonts/X11/75dpi"

#define HASXDMAUTH 1
#define SECURE_RPC 1

#define HAVE_DLFCN_H 1
#define HAVE_EXECINFO_H 1
#define HAVE_FNMATCH_H 1
#define HAVE_LINUX_AGPGART_H 1
#define HAVE_STRINGS_H 1
#define HAVE_SYS_AGPGART_H 1
#define HAVE_SYS_UN_H 1
#define HAVE_SYS_UTSNAME_H 1
#define HAVE_SYS_SYSMACROS_H 1

#define HAVE_ARC4RANDOM_BUF 1
#undef HAVE_BACKTRACE
#undef HAVE_LIBUNWIND
#undef HAVE_PSTACK
#define HAVE_CBRT 1
#define HAVE_EPOLL_CREATE1 1
#define HAVE_GETUID 1
#define HAVE_GETEUID 1
#undef HAVE_ISASTREAM
#undef HAVE_ISSETUGID
#define HAVE_GETIFADDRS 1
#undef HAVE_GETPEEREID
#undef HAVE_GETPEERUCRED
#define HAVE_GETPROGNAME 1
#undef HAVE_GETZONEID
#define HAVE_MEMFD_CREATE 1
#define HAVE_MKOSTEMP 1
#define HAVE_MMAP 1
#undef HAVE_OPEN_DEVICE
#define HAVE_POLL 1
#undef HAVE_POLLSET_CREATE
#define HAVE_POSIX_FALLOCATE 1
#undef HAVE_PORT_CREATE
#undef HAVE_REALLOCARRAY
#define HAVE_SETEUID 1
#define HAVE_SETITIMER 1
#undef HAVE_SHMCTL64
#define HAVE_SIGACTION 1
#define HAVE_SIGPROCMASK 1
#define HAVE_STRCASECMP 1
#define HAVE_STRCASESTR 1
#define HAVE_STRLCAT 1
#define HAVE_STRLCPY 1
#define HAVE_STRNCASECMP 1
#define HAVE_STRNDUP 1
#undef HAVE_TIMINGSAFE_MEMCMP
#define HAVE_VASPRINTF 1
#define HAVE_VSNPRINTF 1
#undef HAVE_WALKCONTEXT
#define BUSFAULT 1

#define _XTYPEDEF_POINTER 1
#define _XITYPEDEF_POINTER 1

#define LISTEN_UNIX
#define LISTEN_LOCAL

#define XTRANS_SEND_FDS 1

#define TCPCONN 1
#define UNIXCONN 1
#define IPv6 1

#define BIGREQS 1
#define COMPOSITE 1
#define DAMAGE 1
#define DBE 1
#define DGA 1
#define DPMSExtension 1
#define DRI2 1
#define DRI3 1
#define HAS_SHM 1
#define MITSHM 1
#define PANORAMIX 1
#define PRESENT 1
#define RANDR 1
#define RES 1
#define RENDER 1
#define SCREENSAVER 1
#define SHAPE 1
#define XACE 1
#define XCMISC 1
#undef XCSECURITY
#define XDMCP 1
#undef XF86BIGFONT
#define XF86DRI 1
#define XF86VIDMODE 1
#define XFIXES 1
#define XFreeXDGA 1
#define XINERAMA 1
#define XINPUT 1
#define XRECORD 1
#undef XSELINUX
#define XSYNC 1
#define XTEST 1
#define XV 1
#define XvExtension 1
#define XvMCExtension 1

/* Use libmd SHA1 functions */
#define HAVE_SHA1_IN_LIBMD 1

#define HAVE_APM 1
#define HAVE_ACPI 1

#undef DDXOSVERRORF
#undef DDXBEFORERESET
#undef DEBUG

#define XVENDORNAME "The X.Org Foundation"
#define XVENDORNAMESHORT "X.Org"
#define __VENDORDWEBSUPPORT__ "http://wiki.x.org"
#define BUILDERADDR "xorg@lists.freedesktop.org"
#define BUILDERSTRING ""

#undef SVR4
#define XKB_DFLT_RULES "evdev"
#undef XORGSERVER
#define XCONFIGFILE "xorg.conf"
#define __XSERVERNAME__ "Xorg"
#define WITH_VGAHW 1
#undef CSRG_BASED
#undef PCCONS_SUPPORT
#undef PCVT_SUPPORT
#undef SYSCONS_SUPPORT
#undef WSCONS_SUPPORT
#undef XSERVER_LIBPCIACCESS
#undef XSERVER_PLATFORM_BUS

#ifdef ANDROID
typedef long long quad_t;
typedef unsigned long long u_quad_t;

#include <unistd.h>

__attribute__((unused))
static int memfd_create(const char *name, unsigned int flags) {
#ifndef __NR_memfd_create
#if defined __i386__
#define __NR_memfd_create 356
#elif defined __x86_64__
#define __NR_memfd_create 319
#elif defined __arm__
#define __NR_memfd_create 385
#elif defined __aarch64__
#define __NR_memfd_create 279
#endif
#endif

#ifdef __NR_memfd_create
    return syscall(__NR_memfd_create, name, flags); // NOLINT(cppcoreguidelines-narrowing-conversions)
#else
    errno = ENOSYS;
	return -1;
#endif
}
#endif

#ifdef __ANDROID__
#define CHECK_EUID 0
extern int execl_xkbcomp(const char * path, const char * arg, ...);
#define setgid(a) 0
#define setuid(a) 0
#endif
