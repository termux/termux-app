#!/bin/bash

set -e
set -o xtrace

HOST=$1

# Debian's cross-pkg-config wrappers are broken for MinGW targets, since
# dpkg-architecture doesn't know about MinGW target triplets.
# https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=930492
cat >/usr/local/bin/${HOST}-pkg-config <<EOF
#!/bin/sh

PKG_CONFIG_SYSROOT_DIR=/usr/${HOST} PKG_CONFIG_LIBDIR=/usr/${HOST}/lib/pkgconfig:/usr/share/pkgconfig pkg-config \$@
EOF
chmod +x /usr/local/bin/${HOST}-pkg-config

# when cross-compiling, some autoconf tests cannot be run:

# --enable-malloc0returnsnull
export xorg_cv_malloc0_returns_null=yes

build() {
    url=$1
    commit=$2
    config=$3

    name=$(basename ${url} .git)

    if [[ $commit =~ ^[[:xdigit:]]{1,}$ ]]
    then
        git clone ${url} ${name}
        git -C ${name} checkout ${commit}
    else
        git clone --depth 1 --branch ${commit:-master} --recurse-submodules -c advice.detachedHead=false ${url} ${name}
    fi

    pushd ${name}
    NOCONFIGURE=1 ./autogen.sh || ./.bootstrap
    ./configure ${config} --host=${HOST} --prefix= --with-sysroot=/usr/${HOST}/
    make -j$(nproc)
    DESTDIR=/usr/${HOST} make install

    popd
    rm -rf ${OLDPWD}
}

build 'https://gitlab.freedesktop.org/pixman/pixman.git' 'pixman-0.38.4'
build 'https://gitlab.freedesktop.org/xorg/lib/pthread-stubs.git' '0.4'
# we can't use the xorgproto pkgconfig files from /usr/share/pkgconfig, because
# these would add -I/usr/include to CFLAGS, which breaks cross-compilation
build 'https://gitlab.freedesktop.org/xorg/proto/xorgproto.git' 'xorgproto-2021.4.99.2' '--datadir=/lib'
build 'https://gitlab.freedesktop.org/xorg/lib/libXau.git' 'libXau-1.0.9'
build 'https://gitlab.freedesktop.org/xorg/proto/xcbproto.git' 'xcb-proto-1.14'
build 'https://gitlab.freedesktop.org/xorg/lib/libxcb.git' 'libxcb-1.14'
build 'https://gitlab.freedesktop.org/xorg/lib/libxtrans.git' 'xtrans-1.4.0'
# the default value of keysymdefdir is taken from the includedir variable for
# xproto, which isn't adjusted by pkg-config for the sysroot
build 'https://gitlab.freedesktop.org/xorg/lib/libX11.git' 'libX11-1.6.9' "--with-keysymdefdir=/usr/${HOST}/include/X11"
build 'https://gitlab.freedesktop.org/xorg/lib/libxkbfile.git' 'libxkbfile-1.1.0'
# freetype needs an explicit --build to know it's cross-compiling
# disable png as freetype tries to use libpng-config, even when cross-compiling
build 'git://git.savannah.gnu.org/freetype/freetype2.git' 'VER-2-10-1' "--build=$(cc -dumpmachine) --with-png=no"
build 'https://gitlab.freedesktop.org/xorg//font/util.git' 'font-util-1.3.2'
build 'https://gitlab.freedesktop.org/xorg/lib/libfontenc.git' 'libfontenc-1.1.4'
build 'https://gitlab.freedesktop.org/xorg/lib/libXfont.git'  'libXfont2-2.0.3'
build 'https://gitlab.freedesktop.org/xorg/lib/libXdmcp.git' 'libXdmcp-1.1.3'
build 'https://gitlab.freedesktop.org/xorg/lib/libXfixes.git' 'libXfixes-5.0.3'
build 'https://gitlab.freedesktop.org/xorg/lib/libxcb-util.git' '0.4.0'
build 'https://gitlab.freedesktop.org/xorg/lib/libxcb-image.git' '0.4.0'
build 'https://gitlab.freedesktop.org/xorg/lib/libxcb-wm.git' '0.4.1'

# workaround xcb_windefs.h leaking all Windows API types into X server build
# (some of which clash which types defined by Xmd.h) XXX: This is a bit of a
# hack, as it makes this header depend on xorgproto. Maybe an upstreamable
# fix would involve a macro defined in the X server (XFree86Server?
# XCB_NO_WINAPI?), which makes xcb_windefs.h wrap things like XWinsock.h
# does???
sed -i s#winsock2#X11/Xwinsock# /usr/${HOST}/include/xcb/xcb_windefs.h
