X Server
--------

The X server accepts requests from client applications to create windows,
which are (normally rectangular) "virtual screens" that the client program
can draw into.

Windows are then composed on the actual screen by the X server
(or by a separate composite manager) as directed by the window manager,
which usually communicates with the user via graphical controls such as buttons
and draggable titlebars and borders.

For a comprehensive overview of X Server and X Window System, consult the
following article:
https://en.wikipedia.org/wiki/X_server

All questions regarding this software should be directed at the
Xorg mailing list:

  https://lists.freedesktop.org/mailman/listinfo/xorg

The primary development code repository can be found at:

  https://gitlab.freedesktop.org/xorg/xserver

For patch submission instructions, see:

  https://www.x.org/wiki/Development/Documentation/SubmittingPatches

As with other projects hosted on freedesktop.org, X.Org follows its
Code of Conduct, based on the Contributor Covenant. Please conduct
yourself in a respectful and civilized manner when using the above
mailing lists, bug trackers, etc:

  https://www.freedesktop.org/wiki/CodeOfConduct
