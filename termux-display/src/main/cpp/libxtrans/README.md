xtrans - X Network Transport layer shared code
----------------------------------------------

xtrans is a library of code that is shared among various X packages to
handle network protocol transport in a modular fashion, allowing a
single place to add new transport types.  It is used by the X server,
libX11, libICE, the X font server, and related components.

It is however, *NOT* a shared library, but code which each consumer
includes and builds it's own copy of with various #ifdef flags to make
each copy slightly different.  To support this in the modular build
system, this package simply installs the C source files into
$(prefix)/include/X11/Xtrans and installs a pkg-config file and an
autoconf m4 macro file with the flags needed to use it.

Documentation of the xtrans API can be found in the included xtrans.xml
file in DocBook XML format. If 'xmlto' is installed, you can generate text,
html, postscript or pdf versions of the documentation by configuring
the build with --enable-docs, which is the default.

 --------------------------------------------------------------------------

All questions regarding this software should be directed at the
Xorg mailing list:

  https://lists.x.org/mailman/listinfo/xorg

The master development code repository can be found at:

  https://gitlab.freedesktop.org/xorg/lib/libxtrans

Please submit bug reports and requests to merge patches there.

For patch submission instructions, see:

  https://www.x.org/wiki/Development/Documentation/SubmittingPatches
