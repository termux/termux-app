This addon will run scripts immediately after device was booted.

## Installation

Download add-on from
[F-Droid](https://f-droid.org/packages/com.termux.boot/)

**Important: Do not mix installations of Termux and Addons between
Google Play and F-Droid.** They are presented at these portals for your
convenience. There are compatibility issues when mixing installations
from these Internet portals. This is because each download website uses
a specific key for keysigning Termux and [Addons](Addons).

## Usage

1\. Install the Termux:Boot app.

2\. Go to Android settings and turn off battery optimizations for Termux
and Termux:Boot applications.

3\. Start the Termux:Boot app once by clicking on its launcher icon.
This allows the app to be run at boot.

4\. Create the \~/.termux/boot/ directory: Put scripts you want to
execute inside the \~/.termux/boot/ directory. If there are multiple
files, they will be executed in a sorted order.

5\. It is helpful to run termux-wake-lock as first thing to prevent the
device from sleeping.

Example: to start an sshd server and prevent the device from sleeping at
boot, create the following file at \~/.termux/boot/start-sshd:

`#!/data/data/com.termux/files/usr/bin/sh`
`termux-wake-lock`
`sshd`

If you want [Termux-services](Termux-services) to start
services on boot, you can use:

`#!/data/data/com.termux/files/usr/bin/sh`
`termux-wake-lock`
`. $PREFIX/etc/profile`