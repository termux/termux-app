Files stored in the home directory in Termux is not accessible to other
applications by default. This is a limitation of Android itself.

As a workaround, you can use `termux-open` available in termux-tools
package to share files with read access.

`$ termux-open -h`
`Usage: termux-open [options] path-or-url`
`Open a file or URL in an external app.`
`  --send               if the file should be shared for sending`
`  --view               if the file should be shared for viewing (default)`
`  --chooser            if an app chooser should always be shown`
`  --content-type type  specify the content type to use`
`$ termux-open hello.c`

For compatibility with standard Linux programs, `xdg-open` is symlinked
to `termux-open`.

Sometimes, read access is not enough, you need to be able to modify
files. This can be achieved by storing the required files on an sdcard.
Android Lollipop should work right out of the box, but, Marshmallow and
above require the Termux app to request permissions to Read/Write
External Data.

Run `termux-setup-storage` and grant permissions when the dialog pops
up. This will create a `$HOME/storage` directory with symlinks to
respective paths of sdcard(s).