libxcvt
=======

`libxcvt` is a library providing a standalone version of the X server
implementation of the VESA CVT standard timing modelines generator.

`libxcvt` also provides a standalone version of the command line tool
`cvt` copied from the Xorg implementation and is meant to be a direct
replacement to the version provided by the `Xorg` server.

An example output is:

```
$ $ cvt --verbose 1920 1200 75
# 1920x1200 74.93 Hz (CVT 2.30MA) hsync: 94.04 kHz; pclk: 245.25 MHz
Modeline "1920x1200_75.00"  245.25  1920 2064 2264 2608  1200 1203 1209 1255 -hsync +vsync
```

Building
========

`libxcvt` is built using [Meson](https://mesonbuild.com/)

	$ git clone https://gitlab.freedesktop.org/xorg/lib/libxcvt.git
	$ cd libxcvt
	$ meson build/ --prefix=...
	$ ninja -C build/ install
	$ cd ..

Credit
======

The code base of `libxcvt` is identical to `xf86CVTMode()` therefore
all credits for `libxcvt` go to the author (Luc Verhaegen) and
contributors of `xf86CVTMode()` and the `cvt` utility as found in the
[xserver](https://gitlab.freedesktop.org/xorg/xserver/) repository.
