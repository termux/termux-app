#!/bin/sh

dump_log_and_quit() {
        local exitcode=$1

        cat meson-logs/testlog.txt

        exit $exitcode
}

export SDKROOT=$( xcodebuild -version -sdk macosx Path )
export CPPFLAGS=-I/usr/local/include
export LDFLAGS=-L/usr/local/lib
export OBJC=$CC
export PATH=$HOME/tools:$PATH

srcdir=$( pwd )
builddir=$( mktemp -d build_XXXXXX )

meson ${BUILDOPTS} $builddir $srcdir || exit $?

cd $builddir

ninja || exit $?
meson test || dump_log_and_quit $?

cd ..
