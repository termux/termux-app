#!/usr/bin/env python3
"""Sanity check for fido-bridge: send CTAPHID_INIT, print the reply, wink.

usage: fido-ping [socket]   (default: $PREFIX/tmp/fido.sock)
"""
import os
import socket
import struct
import sys

REPORT = 64
BROADCAST = 0xFFFFFFFF
INIT, WINK, ERROR = 0x86, 0x88, 0xBF


def packet(cid, cmd, payload=b""):
    body = struct.pack(">IBH", cid, cmd, len(payload)) + payload
    return body.ljust(REPORT, b"\0")


def recv_report(s):
    buf = b""
    while len(buf) < REPORT:
        chunk = s.recv(REPORT - len(buf))
        if not chunk:
            sys.exit("bridge closed the connection")
        buf += chunk
    return buf


def main():
    prefix = os.environ.get("PREFIX", "/data/data/com.termux/files/usr")
    path = sys.argv[1] if len(sys.argv) > 1 else prefix + "/tmp/fido.sock"
    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    s.settimeout(5)
    s.connect(path)

    nonce = os.urandom(8)
    s.sendall(packet(BROADCAST, INIT, nonce))
    while True:
        r = recv_report(s)
        if r[4] == INIT and r[7:15] == nonce:
            break
    cid = struct.unpack(">I", r[15:19])[0]
    proto, major, minor, build, caps = r[19:24]
    flags = [n for b, n in ((1, "WINK"), (4, "CBOR"), (8, "NMSG")) if caps & b]
    print(f"channel   {cid:08x}")
    print(f"protocol  {proto}")
    print(f"device    {major}.{minor}.{build}")
    print(f"caps      {caps:#04x} {' '.join(flags)}")

    if caps & 1:
        s.sendall(packet(cid, WINK))
        while True:
            r = recv_report(s)
            if struct.unpack(">I", r[:4])[0] == cid:
                print("wink      ok" if r[4] == WINK else f"wink      error {r[7]:#04x}")
                break


if __name__ == "__main__":
    main()
