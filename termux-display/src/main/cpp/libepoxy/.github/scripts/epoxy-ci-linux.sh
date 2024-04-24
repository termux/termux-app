#!/bin/bash

dump_log_and_quit() {
        local exitcode=$1

        cat meson-logs/testlog.txt

        exit $exitcode
}

# Start Xvfb
XVFB_WHD=${XVFB_WHD:-1280x720x16}

Xvfb :99 -ac -screen 0 $XVFB_WHD -nolisten tcp &
xvfb=$!

export DISPLAY=:99

srcdir=$( pwd )
builddir=$( mktemp -d build_XXXXXX )

meson --prefix /usr "$@" $builddir $srcdir || exit $?

cd $builddir

ninja || exit $?
meson test || dump_log_and_quit $?

cd ..

# Stop Xvfb
kill -9 ${xvfb}
