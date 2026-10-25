#!/usr/bin/env python3
"""Print a FIDO2 key's CTAP2 info through fido-bridge, using python-fido2.

usage: fido-info [socket]   (default: $PREFIX/tmp/fido.sock)

Starts the bridge with `fido up` if it isn't running. Doubles as the
shortest example of using termux_fido in your own scripts.
"""
import sys

try:
    import termux_fido  # must come before fido2 imports
except ImportError as e:
    sys.exit(str(e))
from fido2.ctap2 import Ctap2


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else termux_fido.SOCKET
    try:
        dev = termux_fido.open_device(path)
    except OSError as e:
        sys.exit(f"fido-info: {e}")

    info = Ctap2(dev).get_info()
    print(f"versions    {', '.join(info.versions)}")
    print(f"aaguid      {info.aaguid}")
    print(f"extensions  {', '.join(info.extensions) or '-'}")
    print("options     " + ", ".join(f"{k}={v}" for k, v in sorted(info.options.items())))
    if info.pin_uv_protocols:
        print(f"pin proto   {', '.join(map(str, info.pin_uv_protocols))}")
    if info.firmware_version:
        fw = info.firmware_version
        if fw > 0xFFFF:  # YubiKey style: major << 16 | minor << 8 | patch
            fw = f"{fw >> 16}.{fw >> 8 & 0xFF}.{fw & 0xFF}"
        print(f"firmware    {fw}")
    if info.remaining_disc_creds is not None:
        print(f"free slots  {info.remaining_disc_creds} passkeys")
    dev.close()


if __name__ == "__main__":
    main()
