#!/usr/bin/env bash
# Download official bootstrap packages, merge changes and pack zips
ARCHS="aarch64 arm i686 x86_64"

# Iterate the string variable using for loop
BOOTSTRAP_URL="https://bintray.com/termux/bootstrap/download_file?file_path=bootstrap-"
BOOTSTRAP_URL_SUFFIX="-v35.zip"
git clone https://github.com/t-e-l/bootstrap-changes
for ARCH in $ARCHS; do
    URL=$BOOTSTRAP_URL$ARCH$BOOTSTRAP_URL_SUFFIX
    echo "working on $ARCH"
    rm -f bootstrap-$ARCH.zip
	echo "downloading orginal bootstrap from $URL"
	wget -O bootstrap-$ARCH.zip $URL > /dev/null 2>&1
	unzip bootstrap-$ARCH.zip -d bootstrap-$ARCH > /dev/null 2>&1
	cp -r bootstrap-changes/* bootstrap-$ARCH 
	cd bootstrap-$ARCH
	echo "zipping package"
	zip -r ../app/src/main/cpp/bootstrap-$ARCH.zip * > /dev/null 2>&1
	cd ..
	rm -rf bootstrap-$ARCH
done
rm -rf bootstrap-changes
