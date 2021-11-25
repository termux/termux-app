#!/usr/bin/env bash
# Download official bootstrap packages, merge changes and pack zips
ARCHS="aarch64 arm i686 x86_64"

# Iterate the string variable using for loop
BOOTSTRAP_URL="https://github.com/termux/termux-packages/releases/download/bootstrap-"
VERSION="2021.02.11-r1"
git clone --depth=1 https://github.com/t-e-l/bootstrap-changes
git clone --depth=1 https://github.com/t-e-l/bin
rm -f bin/README.md > /dev/null 2>&1
mv -f bin/* bootstrap-changes/bin/ > /dev/null 2>&1

for ARCH in $ARCHS; do
    URL=$BOOTSTRAP_URL$VERSION/bootstrap-$ARCH.zip
    echo "working on $ARCH"
    rm -f bootstrap-$ARCH.zip
	echo "downloading orginal bootstrap from $URL"
	wget -O bootstrap-$ARCH.zip $URL > /dev/null 2>&1
	unzip bootstrap-$ARCH.zip -d bootstrap-$ARCH > /dev/null 2>&1
	cp -r bootstrap-changes/* bootstrap-$ARCH 
	cd bootstrap-$ARCH
        rm -f etc/apt/sources.list.d/*.list
	echo "zipping package"
	zip -r ../app/src/main/cpp/bootstrap-$ARCH.zip * > /dev/null 2>&1
	cd ..
	rm -rf bootstrap-$ARCH
done
rm -rf bootstrap-changes
rm -rf bin
