#!/bin/bash

set -e
set -o xtrace

# Packages which are needed by this script, but not for the xserver build
EPHEMERAL="
	libcairo2-dev
	libevdev-dev
	libexpat-dev
	libgles2-mesa-dev
	libinput-dev
	libxkbcommon-dev
	x11-utils
	x11-xserver-utils
	xauth
	xvfb
	"

apt-get install -y \
	$EPHEMERAL \
	autoconf \
	automake \
	bison \
	build-essential \
	ca-certificates \
	ccache \
	dpkg-dev \
	flex \
	gcc-mingw-w64-i686 \
	git \
	libaudit-dev \
	libbsd-dev \
	libcairo2 \
	libdbus-1-dev \
	libdmx-dev \
	libdrm-dev \
	libegl1-mesa-dev \
	libepoxy-dev \
	libevdev2 \
	libexpat1 \
	libffi-dev \
	libgbm-dev \
	libgcrypt-dev \
	libgl1-mesa-dev \
	libgles2 \
	libglx-mesa0 \
	libinput10 \
	libnvidia-egl-wayland-dev \
	libpciaccess-dev \
	libpixman-1-dev \
	libselinux1-dev \
	libsystemd-dev \
	libtool \
	libudev-dev \
	libunwind-dev \
	libx11-dev \
	libx11-xcb-dev \
	libxau-dev \
	libxaw7-dev \
	libxcb-glx0-dev \
	libxcb-icccm4-dev \
	libxcb-image0-dev \
	libxcb-keysyms1-dev \
	libxcb-randr0-dev \
	libxcb-render-util0-dev \
	libxcb-render0-dev \
	libxcb-shape0-dev \
	libxcb-shm0-dev \
	libxcb-util0-dev \
	libxcb-xf86dri0-dev \
	libxcb-xkb-dev \
	libxcb-xv0-dev \
	libxcb1-dev \
	libxdmcp-dev \
	libxext-dev \
	libxfixes-dev \
	libxfont-dev \
	libxi-dev \
	libxinerama-dev \
	libxkbcommon0 \
	libxkbfile-dev \
	libxmu-dev \
	libxmuu-dev \
	libxpm-dev \
	libxrender-dev \
	libxres-dev \
	libxshmfence-dev \
	libxt-dev \
	libxtst-dev \
	libxv-dev \
	libz-mingw-w64-dev \
	mesa-common-dev \
	meson \
	mingw-w64-tools \
	nettle-dev \
	pkg-config \
	python3-mako \
	python3-numpy \
	python3-six \
	xfonts-utils \
	xkb-data \
	xtrans-dev \
	xutils-dev

.gitlab-ci/cross-prereqs-build.sh i686-w64-mingw32

cd /root

# xserver requires libxcvt
git clone https://gitlab.freedesktop.org/xorg/lib//libxcvt.git --depth 1 --branch=libxcvt-0.1.0
cd libxcvt
meson _build
ninja -C _build -j${FDO_CI_CONCURRENT:-4} install
cd ..
rm -rf libxcvt

# xserver requires xorgproto >= 2021.4.99.2 for XI 2.3.99.1
git clone https://gitlab.freedesktop.org/xorg/proto/xorgproto.git --depth 1 --branch=xorgproto-2021.4.99.2
pushd xorgproto
./autogen.sh
make -j${FDO_CI_CONCURRENT:-4} install
popd
rm -rf xorgproto

# weston 9.0 requires libwayland >= 1.18
git clone https://gitlab.freedesktop.org/wayland/wayland.git --depth 1 --branch=1.18.0
cd wayland
meson _build -D{documentation,dtd_validation}=false
ninja -C _build -j${FDO_CI_CONCURRENT:-4} install
cd ..
rm -rf wayland

# Xwayland requires wayland-protocols >= 1.18, but Debian buster has 1.17 only
git clone https://gitlab.freedesktop.org/wayland/wayland-protocols.git --depth 1 --branch=1.18
cd wayland-protocols
./autogen.sh
make -j${FDO_CI_CONCURRENT:-4} install
cd ..
rm -rf wayland-protocols

# Xwayland requires weston > 5.0, but Debian buster has 5.0 only
git clone https://gitlab.freedesktop.org/wayland/weston.git --depth 1 --branch=9.0
cd weston
meson _build -Dbackend-{drm,drm-screencast-vaapi,fbdev,rdp,wayland,x11}=false \
      -Dbackend-default=headless -Dcolor-management-{colord,lcms}=false \
      -Ddemo-clients=false -Dimage-{jpeg,webp}=false \
      -D{pipewire,remoting,screenshare,test-junit-xml,wcap-decode,weston-launch,xwayland}=false \
      -Dshell-{fullscreen,ivi,kiosk}=false -Dsimple-clients=
ninja -C _build -j${FDO_CI_CONCURRENT:-4} install
cd ..
rm -rf weston

git clone https://gitlab.freedesktop.org/mesa/piglit.git --depth 1

git clone https://gitlab.freedesktop.org/xorg/test/xts --depth 1
cd xts
./autogen.sh
xvfb-run make -j${FDO_CI_CONCURRENT:-4}
cd ..

git clone https://gitlab.freedesktop.org/xorg/test/rendercheck --depth 1
cd rendercheck
meson build
ninja -j${FDO_CI_CONCURRENT:-4} -C build install
cd ..

rm -rf piglit/.git xts/.git piglit/tests/spec/ rendercheck/

echo '[xts]' > piglit/piglit.conf
echo 'path=/root/xts' >> piglit/piglit.conf

find -name \*.a -o -name \*.o -o -name \*.c -o -name \*.h -o -name \*.la\* | xargs rm
strip xts/xts5/*/.libs/*

# Running meson dist requires xkbcomp 1.4.1 or newer, but Debian buster has 1.4.0 only
git clone https://gitlab.freedesktop.org/xorg/app/xkbcomp.git --depth 1 --branch=xkbcomp-1.4.1
cd xkbcomp
./autogen.sh --datarootdir=/usr/share
make -j${FDO_CI_CONCURRENT:-4} install
cd ..
rm -rf xkbcomp

apt-get purge -y \
	$EPHEMERAL

apt-get autoremove -y --purge
