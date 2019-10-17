#!/bin/sh
set -e -u

for APP in api boot styling tasker widget; do
	APPDIR=../../termux-$APP
	for file in ic_foreground ic_launcher; do
		cp ../app/src/main/res/drawable/$file.xml \
			$APPDIR/app/src/main/res/drawable/$file.xml
	done

	cp ../app/src/main/res/drawable-anydpi-v26/ic_launcher.xml \
		$APPDIR/app/src/main/res/drawable-anydpi-v26/$file.xml
done
