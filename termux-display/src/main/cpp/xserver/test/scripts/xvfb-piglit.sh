#!/bin/sh

# this times out on Travis, because the tests take too long.
#if test "x$TRAVIS_BUILD_DIR" != "x"; then
#    exit 77
#fi

export SERVER_COMMAND="$XSERVER_BUILDDIR/hw/vfb/Xvfb \
        -noreset \
        -screen scrn 1280x1024x24"
export PIGLIT_RESULTS_DIR=$XSERVER_BUILDDIR/test/piglit-results/xvfb

exec $XSERVER_DIR/test/scripts/run-piglit.sh

