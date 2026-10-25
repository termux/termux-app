"""Use a USB FIDO2 key from Python on Termux, through fido-bridge.

    import termux_fido                 # import this before anything from fido2
    from fido2.ctap2 import Ctap2

    dev = termux_fido.open_device()    # starts the bridge if it isn't running
    print(Ctap2(dev).get_info())

`dev` is an ordinary python-fido2 CtapHidDevice, so Fido2Client, Ctap2,
CredentialManagement, ClientPin and the rest all work with it.
"""
import os
import socket
import subprocess
import sys

# Python >= 3.13 on Android reports sys.platform == "android", which
# python-fido2 rejects with "Unsupported platform". Its Linux backend loads
# fine there and is never used (we bring our own connection), so present
# "linux" while fido2.hid is first imported. Later fido2 imports reuse it.
_platform = sys.platform
if _platform == "android":
    sys.platform = "linux"
try:
    from fido2.hid import CtapHidDevice
    from fido2.hid.base import CtapHidConnection, HidDescriptor
except ImportError as e:
    raise ImportError(
        "python-fido2 is missing: pkg install python-cryptography && pip install fido2"
    ) from e
finally:
    sys.platform = _platform

__all__ = ["SOCKET", "BridgeConnection", "ensure_bridge", "open_device"]

REPORT = 64
SOCKET = os.environ.get("FIDO_BRIDGE_SOCKET") or os.path.join(
    os.environ.get("PREFIX", "/data/data/com.termux/files/usr"), "tmp", "fido.sock"
)


class BridgeConnection(CtapHidConnection):
    """CTAPHID connection that sends 64-byte reports over the bridge socket."""

    def __init__(self, path=SOCKET):
        self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.sock.connect(path)

    def write_packet(self, data):
        self.sock.sendall(bytes(data).ljust(REPORT, b"\0")[:REPORT])

    def read_packet(self):
        buf = b""
        while len(buf) < REPORT:
            chunk = self.sock.recv(REPORT - len(buf))
            if not chunk:
                raise OSError("fido-bridge closed the connection (key unplugged?)")
            buf += chunk
        return buf

    def close(self):
        self.sock.close()


def _listening(path):
    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    try:
        s.connect(path)
        return True
    except OSError:
        return False
    finally:
        s.close()


def ensure_bridge(path=SOCKET):
    """Start fido-bridge via `fido up` unless it is already listening on path."""
    if _listening(path):
        return
    env = dict(os.environ, FIDO_BRIDGE_SOCKET=path)
    try:
        subprocess.run(["fido", "up"], env=env, check=False)
    except FileNotFoundError:
        raise OSError("the fido command is not installed (make install)") from None
    if not _listening(path):
        raise OSError("fido-bridge is not running; see: fido log")


def open_device(path=SOCKET, autostart=True):
    """Return a python-fido2 CtapHidDevice for the key behind the bridge."""
    if autostart:
        ensure_bridge(path)
    desc = HidDescriptor(path, 0, 0, REPORT, REPORT, "fido-bridge", None)
    return CtapHidDevice(desc, BridgeConnection(path))
