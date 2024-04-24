libXfont - X font handling library for server & utilities
---------------------------------------------------------

libXfont provides the core of the legacy X11 font system, handling the index
files (fonts.dir, fonts.alias, fonts.scale), the various font file formats,
and rasterizing them.  It is used by the X display servers (Xorg, Xvfb, etc.)
and the X Font Server (xfs), but should not be used by normal X11 clients.  X11
clients access fonts via either the new APIs in libXft, or the legacy APIs in
libX11.

libXfont supports a number of compression and font formats, and the
configure script takes various options to enable or disable them:

- Compression types:

  * gzip - always enabled, no option to disable, requires libz

  * bzip2 - disabled by default, enable via --with-bzip2, requires libbz2

- Font formats:

  * builtins - copies of the "fixed" & "cursor" fonts required by the
    X protocol are built into the library so the X server always
    has the fonts it requires to start up.   Accessed via the
    special 'built-ins' entry in the X server font path.  
    Enabled by default, disable via --disable-builtins.

  * freetype - handles scalable font formats including OpenType, FreeType,
    and PostScript formats.  Requires FreeType2 library.
    Can also be used to handle bdf & bitmap pcf font formats.  
    Enabled by default, disable via --disable-freetype.

  * bdf bitmap fonts - text file format for distributing fonts, described
    in https://www.x.org/docs/BDF/bdf.pdf specification.  Normally
    not used by the X server at runtime, as the fonts distributed
    by X.Org in bdf format are compiled with bdftopcf when
    installing/packaging them.  
    Enabled by default, disable via --disable-bdfformat.

  * pcf bitmap fonts - standard bitmap font format since X11R5 in 1991,
    used for all bitmap fonts installed from X.Org packages.
    Compiled format is architecture independent.
    As noted above, usually produced by bdftopcf.  
    Enabled by default, disable via --disable-pcfformat.

  * snf bitmap fonts - standard bitmap font format prior to X11R5 in 1991,
    remains only for backwards compatibility.  Unlike pcf, snf files
    are architecture specific, and contain less font information
    than pcf files.  snf fonts are deprecated and support for them
    may be removed in future libXfont releases.  
    Disabled by default, enable via --disable-snfformat.

- Font services:

  * xfs font servers - allows retrieving fonts as a client of an xfs server.
    Enabled by default, disable via --disable-fc (font client).

    If enabled, you can also use the standard libxtrans flags to
    configure which transports can be used to connect to xfs:
    
        --enable-unix-transport  Enable UNIX domain socket transport
        --enable-tcp-transport   Enable TCP socket transport (IPv4)
        --enable-ipv6            Enable IPv6 support for tcp-transport
        --enable-local-transport Enable os-specific local transport

    (Change --enable to --disable to force disabling support.)  
    The default setting is to enable all of the transports the
    configure script can find OS support for.

--------------------------------------------------------------------------

All questions regarding this software should be directed at the
Xorg mailing list:

  https://lists.x.org/mailman/listinfo/xorg

The primary development code repository can be found at:

  https://gitlab.freedesktop.org/xorg/lib/libXfont

Please submit bug reports and requests to merge patches there.

For patch submission instructions, see:

  https://www.x.org/wiki/Development/Documentation/SubmittingPatches
