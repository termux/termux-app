This addon provides color schemes and fonts to customize the appearance
of your Termux terminal.

## Installation

Download add-on from
[F-Droid](https://f-droid.org/packages/com.termux.styling/)

**Important: Do not mix installations of Termux and Addons between
Google Play and F-Droid.** They are presented at these portals for your
convenience. There are compatibility issues when mixing installations
from these Internet portals. This is because each download website uses
a specific key for keysigning Termux and [Addons](Addons).

## How it works

Styling add-on is just a package with pre-defined color schemes and font
files. When you use it to change terminal style, some files are being
extracted and placed at specific locations in home directory.

Color schemes are written to

`/data/data/com.termux/files/home/.termux/colors.properties`

Fonts are placed at

`/data/data/com.termux/files/home/.termux/font.ttf`

User can place own files to these paths, which can be useful if a
certain font or color scheme is not available through the add-on. If you
wish to do styling configuration manually, Termux:Styling application is
not necessary.