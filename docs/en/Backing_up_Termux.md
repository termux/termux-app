This page shows an example of backing up your Termux installation.
Instructions listed there cover basic usage of archiving utility "tar"
as well as show which files should be archived. **It is highly
recommended to understand what the listed commands do before
copy-pasting them.** Misunderstanding the purpose of each step may
irrecoverably damage your data. If that happened to you - do not
complain.

# Backing up

In this example, a backup of both home and sysroot will be shown. The
resulting archive will be stored on your shared storage (`/sdcard`) and
compressed with `gzip`.

1\. Ensure that storage permission is granted:

`termux-setup-storage`

2\. Backing up files:

`tar -zcf /sdcard/termux-backup.tar.gz -C /data/data/com.termux/files ./home ./usr`

Backup should be finished without any error. There shouldn't be any
permission denials unless the user abused root permissions. If you got
some warnings about socket files, ignore them.

**Warning**: never store your backups in Termux private directories.
Their paths may look like:

`/data/data/com.termux                       - private Termux directory on internal storage`
`/sdcard/Android/data/com.termux             - private Termux directory on shared storage`
`/storage/XXXX-XXXX/Android/data/com.termux  - private Termux directory on external storage, XXXX-XXXX is the UUID of your micro-sd card.`
`${HOME}/storage/external-1                  - alias for Termux private directory on your micro-sd.`

Once you clear Termux data from settings, these directories are erased
too.

# Restoring

Here will be assumed that you have backed up both home and usr directory
into same archive. Please note that all files would be overwritten
during the process.

1\. Ensure that storage permission is granted:

`termux-setup-storage`

2\. Extract home and usr with overwriting everything. Pass
`--recursive-unlink` to remove any junk and orphaned files. Pass
`--preserve-permissions` to set file permissions as in archive, ignoring
the umask value. By combining these extra options you will get
installation state exactly as was in archive.

`tar -zxf /sdcard/termux-backup.tar.gz -C /data/data/com.termux/files --recursive-unlink --preserve-permissions`

Now close Termux with the "exit" button from notification and open it
again.

# Using supplied scripts

The latest version of package "termux-tools" contains basic scripts for
backup and restore of Termux installation. They are working in a way
similar to tar commands mentioned above.

**These scripts backup and restore scripts will not backup, restore or
in any other way touch your home directory.** Check out the
[notice](https://github.com/termux/termux-packages/blob/06499edb90a16fa471cff739ffee634f42a73913/packages/termux-tools/termux-restore#L6-L34)
if have questions why so. Termux developers are not responsible with
what you are doing with your files. If you managed to lost data, that
would be your own problem.

## Using termux-backup

Simple backup with auto compression:

`termux-backup /sdcard/backup.tar.xz`

Compression format is determined by file extension, which is typically
.tar.gz (gzip), .tar.xz (xz) or .tar (no compression).

It is possible to stream backup contents to standard output, for example
to encrypt it with GnuPG utility or send to remote storage. Set file
name as "-" to enable streaming to stdout:

`termux-backup - | gpg --symmetric --output /sdcard/backup.tar.gpg`

Content written to stdout is not compressed.

## Using termux-restore

**Warning**: restore procedure will destroy any previous data stored in
\$PREFIX. Script will perform a complete rollback to state state exactly
as in backup archive.

Restoring backup is also simple:

`termux-restore /sdcard/backup.tar.xz`

Once finished, restart Termux application.

The utility `termux-restore` is able to read backup data from standard
input. You can use this for reading content supplied by other tool.
Backup contents supplied to stdin must not be compressed. See below
example for restoring from encrypted, compressed backup:

`export GPG_TTY=$(tty)`
`gpg --decrypt /sdcard/backup.tar.gz.gpg | gunzip | termux-restore -`

Please note that if restore procedure will be terminated before finish,
your environment will be corrupted.