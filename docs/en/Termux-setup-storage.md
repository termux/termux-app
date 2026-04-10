In order to have access to shared storage (/sdcard or
/storage/emulated/0), Termux needs a storage access permission. It is
not granted by default and is not requested on application startup since
it is not necessary for normal application functioning.

**Storage access permission will not enable write access to the external
sdcard and drives connected over USB.**

## How To

Open Termux application and do following steps:

1.  Execute command `termux-setup-storage`. If you already have executed
    this command previously or for some reason already have `storage`
    directory under \$HOME, utility will ask you to confirm wiping of
    `~/storage`. This is safe as will only rebuild symlink set.
2.  If permission request dialog was shown - grant permission.
3.  Verify that you can access shared storage `ls ~/storage/shared`

Termux uses utility `termux-setup-storage` to configure access to the
shared storage and setup these symlinks for quick access to various
kinds of storages:

- The root of the shared storage between all apps.

`~/storage/shared`

- The standard directory for downloads from e.g. the system browser.

`~/storage/downloads`

- The traditional location for pictures and videos when mounting the
  device as a camera.

`~/storage/dcim`

- Standard directory in which to place pictures that are available to
  the user.

`~/storage/pictures`

- Standard directory in which to place any audio files that should be in
  the regular list of music for the user.

`~/storage/music`

- Standard directory in which to place movies that are available to the
  user.

`~/storage/movies`

- Symlink to a Termux-private folder on external storage (only if
  external storage is available).

`~/storage/external-1`

### Android 11

You may get "Permission denied" error when trying to access shared
storage, even though the permission has been granted.

Workaround:

1.  Go to Android Settings --\> Applications --\> Termux --\>
    Permissions
2.  Revoke Storage permission
3.  Grant Storage permission again

This is a known issue, though this is not Termux bug.