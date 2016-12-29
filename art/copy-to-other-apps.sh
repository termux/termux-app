#!/bin/sh

for DENSITY in mdpi hdpi xhdpi xxhdpi xxxhdpi; do
	FOLDER=../app/src/main/res/mipmap-$DENSITY

	for FILE in ic_launcher ic_launcher_round; do
		PNG=$FOLDER/$FILE.png

		# Update other apps:
		for APP in api boot styling tasker widget; do
			APPDIR=../../termux-$APP
			if [ -d $APPDIR ]; then
				APP_FOLDER=$APPDIR/app/src/main/res/mipmap-$DENSITY
				mkdir -p $APP_FOLDER
				cp $PNG $APP_FOLDER/$FILE.png
			fi
		done
	done

done
