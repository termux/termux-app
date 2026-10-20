# fido-bridge

Lets Termux programs talk to a USB FIDO2/U2F security key plugged into an
Android phone, without root.

Android won't let apps open `/dev/hidraw*`. `termux-usb` can ask for permission
and hand over a USB file descriptor. `fido-bridge` takes that descriptor and
serves the key's HID reports on a Unix socket.

## Build and install (in Termux)

    pkg install clang make pkg-config libusb termux-api python
    make && make install

You also need the Termux:API app installed.

## Run

    termux-usb -l                                   # find the key, e.g. /dev/bus/usb/001/002
    termux-usb -r -e fido-bridge /dev/bus/usb/001/002

Android shows its permission popup, then the bridge:

1. wraps the fd with `libusb_wrap_sys_device()` (device discovery turned off, as Android requires);
2. goes through the HID interfaces, detaching the kernel driver from each and claiming it, and keeps
   the first one whose report descriptor has **Usage Page 0xF1D0**. The others go back to the kernel;
3. picks that interface's interrupt IN and OUT endpoints;
4. listens on `$PREFIX/tmp/fido.sock` (mode 0600) and passes 64-byte reports both ways.

Ctrl-C, SIGTERM or unplugging the key stops it. It then releases the interface, gives it back to
the kernel driver and removes the socket file.

Options: `-v` logs every CTAPHID packet (channel, command, length). `-s PATH` picks a different
socket path. To pass options through termux-usb, wrap them in a script:
`termux-usb -r -e ./run.sh ...` with `exec fido-bridge -v "$@"` inside.

## Check that it works

    fido-ping
    # channel   1a2b3c4d
    # protocol  2
    # device    5.4.3
    # caps      0x05 WINK CBOR
    # wink      ok          <- the key blinks

## Socket protocol

- `SOCK_STREAM`. Each message in either direction is **exactly 64 bytes**: one raw CTAPHID
  packet, **with no report-ID byte in front** (unlike a hidraw write).
- Up to 16 clients at once. Like hidraw, every report from the key goes to every client.
  CTAPHID clients already filter by channel ID, and by nonce for `INIT`.
- A client that stops reading gets disconnected, so it can't hold up the others.

Any FIDO library with a pluggable transport can use this socket. Map "write report" to
`send(64 bytes)` and "read report" to `recv` until you have 64 bytes.
