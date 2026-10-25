# fido-bridge

Lets Termux programs talk to a USB FIDO2/U2F security key plugged into an
Android phone, without root.

Android won't let apps open `/dev/hidraw*`. `termux-usb` can ask for permission
and hand over a USB file descriptor. `fido-bridge` takes that descriptor and
serves the key's HID reports on a Unix socket. A small `fido` command and a
Python module hide the setup.

## Install (once)

You need the Termux:API app, plus:

    pkg install clang make pkg-config libusb termux-api python python-cryptography
    pip install fido2          # if pip tries to build cryptography: pip install --no-deps fido2
    make install

## Use

Plug in the key, then:

    fido            # finds the key, starts the bridge in the background
    fido ping       # the key blinks
    fido info       # FIDO2 details: versions, AAGUID, PIN set, free passkey slots

The first time, Android asks whether Termux may use the device. Tap OK.

    fido status     # running or not
    fido log        # what the bridge said (fido log -f to follow)
    fido down       # stop it; unplugging the key also stops it
    fido up -v      # start with every CTAPHID packet logged
    fido update     # fetch the latest version of this code and reinstall

`fido` finds the key on its own, so there is no USB path to type. It runs the
bridge detached from the terminal, so you can close the session it started
from. `fido ping` and `fido info` start the bridge first if it's not running.

**Home-screen button:** `make widget` adds a `fido-up` shortcut for the
Termux:Widget app.

## Your own Python scripts

    import termux_fido               # before any fido2 import
    from fido2.ctap2 import Ctap2

    dev = termux_fido.open_device()  # starts the bridge if needed
    print(Ctap2(dev).get_info())

`dev` is an ordinary python-fido2 `CtapHidDevice`, so `Fido2Client`, `ClientPin`,
`CredentialManagement`, hmac-secret and the rest all work. Importing
`termux_fido` also fixes python-fido2's "Unsupported platform" error on Android
(Python 3.13 and later report the platform as `android`, which fido2 doesn't recognize).
`fido-info.py` is a complete example.

## What the bridge does

`fido up` runs `termux-usb -r -e fido-bridge /dev/bus/usb/BBB/DDD` (you can
also do this by hand). The bridge then:

1. wraps the fd with `libusb_wrap_sys_device()` (device discovery turned off, as Android requires);
2. goes through the HID interfaces, detaching the kernel driver from each and claiming it, and keeps
   the first one whose report descriptor has **Usage Page 0xF1D0**. The others go back to the kernel;
3. picks that interface's interrupt IN and OUT endpoints;
4. listens on `$PREFIX/tmp/fido.sock` (mode 0600) and passes 64-byte reports both ways.

On SIGINT/SIGTERM or unplug, it releases the interface, gives it back to the
kernel driver and removes the socket file.

`termux-usb` can't pass options through, so settings are also read from the
environment: `FIDO_BRIDGE_SOCKET=PATH` and `FIDO_BRIDGE_VERBOSE=1`
(the same as `-s PATH` and `-v`).

## Socket protocol

- `SOCK_STREAM`. Each message in either direction is **exactly 64 bytes**: one raw CTAPHID
  packet, **with no report-ID byte in front** (unlike a hidraw write).
- Up to 16 clients at once. Like hidraw, every report from the key goes to every client.
  CTAPHID clients already filter by channel ID, and by nonce for `INIT`.
- A client that stops reading gets disconnected, so it can't hold up the others.
