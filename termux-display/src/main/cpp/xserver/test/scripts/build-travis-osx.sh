#!/bin/sh

set -e
set -x

#
# based on instructions for building xorg-server in https://www.xquartz.org/Developer-Info.html
#

# install XQuartz for libraries, headers, etc.
XQUARTZ_VERSION="2.7.11"
wget -nv https://dl.bintray.com/xquartz/downloads/XQuartz-${XQUARTZ_VERSION}.dmg
hdiutil attach XQuartz-${XQUARTZ_VERSION}.dmg
sudo installer -pkg /Volumes/XQuartz-${XQUARTZ_VERSION}/XQuartz.pkg -target /
hdiutil detach /Volumes/XQuartz-${XQUARTZ_VERSION}

# build environment
export PATH="/opt/X11/bin:${PATH}"
export PKG_CONFIG_PATH="/opt/X11/share/pkgconfig:/opt/X11/lib/pkgconfig:${PKG_CONFIG_PATH}"
export ACLOCAL="aclocal -I /opt/X11/share/aclocal -I /usr/local/share/aclocal"
export CFLAGS="-Wall -O2 -ggdb3 -arch x86_64 -pipe -Wno-typedef-redefinition -Wno-deprecated-declarations"
export CXXFLAGS=$CFLAGS
export OBJCFLAGS=$CFLAGS
export LDFLAGS=$CFLAGS

# travis currently requires explicit ccache setup on OSX
export PATH="/usr/local/opt/ccache/libexec:$PATH"

# need newer xorgproto
pushd $HOME
git clone git://anongit.freedesktop.org/git/xorg/proto/xorgproto
cd xorgproto
if [[ "$1" == "autotools" ]]; then
    autoreconf -fvi
    ./configure --prefix=/opt/X11
    sudo make install
elif [[ "$1" == "meson" ]]; then
    meson _build/ -Dprefix=/opt/X11
    ninja -C _build/
    sudo ninja -C _build/ install
else
    echo "Unknown build tool $1"
    exit 1
fi
popd

# build
if [[ "$1" == "autotools" ]]; then
    autoreconf -fvi
    ./configure --prefix=/opt/X11 --disable-dependency-tracking --with-apple-application-name=XQuartz --with-bundle-id-prefix=org.macosforge.xquartz
    make
    make check
    make install DESTDIR=$(pwd)/staging
elif [[ "$1" == "meson" ]]; then
    meson _build/ -Dprefix=/opt/X11 -Dsecure-rpc=false
    DESTDIR=$(pwd)/staging ninja -C _build/ install
    ninja -C _build/ test
fi
