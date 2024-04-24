X Window System Unified Protocol
--------------------------------

This package provides the headers and specification documents defining
the core protocol and (many) extensions for the X Window System. The
extensions are those common among servers descended from X11R7. It
also includes a number of headers that aren't purely protocol related,
but are depended upon by many other X Window System packages to provide
common definitions and porting layer.

Though the protocol specifications herein are authoritative, the
content of the headers is bound by compatibility constraints with older
versions of the X11 suite. If you are looking for a machine-readable
protocol description suitable for code generation or use in new
projects, please refer to the XCB project:

  https://xcb.freedesktop.org/
  https://gitlab.freedesktop.org/xorg/proto/xcbproto

All questions regarding this software should be directed at the
Xorg mailing list:

  https://lists.x.org/mailman/listinfo/xorg

The primary development code repository can be found at:

  https://gitlab.freedesktop.org/xorg/proto/xorgproto

Please submit bug reports and requests to merge patches there.

For patch submission instructions, see:

  https://www.x.org/wiki/Development/Documentation/SubmittingPatches


Updating for new Linux kernel releases
--------------------------------------

The XF86keysym.h header file needs updating whenever the Linux kernel
adds a new keycode to linux/input-event-codes.h. See the comment in
include/X11/XF86keysym.h for details on the format.

The steps to update the file are:

- if the kernel release did not add new `KEY_FOO` defines, no work is
  required
- ensure that libevdev has been updated to the new kernel headers. This may
  require installing libevdev from git.
- run `scripts/keysym-generator.py` to add new keysyms. See the `--help`
  output for the correct invocation.
- verify that the format for any keys added by this script is correct and
  that the keys need to be mapped. Where a key code should not get a new
  define or is already defined otherwise, comment the line.
- file a merge request with the new changes
- notify the xkeyboard-config maintainers that updates are needed

Note that any #define added immediately becomes API. Due diligence is
recommended.
