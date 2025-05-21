how to build

git clone -b master-x11-submodule https://github.com/jiaxinchen-max/termux-app &&
cd termux-app &&
git submodule update --init --recursive &&
./gradlew syncDebugLibJars &&
./gradlew assembleDebug

then, wait for a while.
