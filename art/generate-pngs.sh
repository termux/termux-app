#!/bin/sh

for DENSITY in mdpi hdpi xhdpi xxhdpi xxxhdpi; do
	case $DENSITY in
		mdpi) SIZE=48;;
		hdpi) SIZE=72;;
		xhdpi) SIZE=96;;
		xxhdpi) SIZE=144;;
		xxxhdpi) SIZE=192;;
	esac

	FOLDER=../app/src/main/res/mipmap-$DENSITY
	mkdir -p $FOLDER

	for FILE in ic_launcher ic_launcher_round; do
		PNG=$FOLDER/$FILE.png
		rsvg-convert -w $SIZE -h $SIZE $FILE.svg > $PNG
		zopflipng -y $PNG $PNG
	done
done
