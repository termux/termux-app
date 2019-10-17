#!/bin/sh
set -e -u

echo "Generating ~/termux-icons/ic_launcher.png..."
mkdir -p ~/termux-icons/

vector2svg ../app/src/main/res/drawable/ic_launcher.xml ~/termux-icons/ic_launcher.svg

sed -i "" 's/viewBox="0 0 108 108"/viewBox="18 18 72 72"/' ~/termux-icons/ic_launcher.svg

SIZE=512
rsvg-convert \
	-w $SIZE \
	-h $SIZE \
	-o ~/termux-icons/ic_launcher_$SIZE.png \
	~/termux-icons/ic_launcher.svg

rsvg-convert \
	-b black \
	-w $SIZE \
	-h $SIZE \
	-o ~/termux-icons/ic_launcher_square_$SIZE.png \
	~/termux-icons/ic_launcher.svg
