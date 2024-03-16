#!/bin/sh

# this times out on Travis, because the tests take too long.
if test "x$TRAVIS_BUILD_DIR" != "x"; then
    exit 77
fi

# Start a Xephyr server using glamor.  Since the test environment is
# headless, we start an Xvfb first to host the Xephyr.
export PIGLIT_RESULTS_DIR=$XSERVER_BUILDDIR/test/piglit-results/xephyr-glamor

export SERVER_COMMAND="$XSERVER_BUILDDIR/hw/kdrive/ephyr/Xephyr \
        -glamor \
        -glamor-skip-present \
        -noreset \
        -schedMax 2000 \
        -screen 1280x1024"

# Tests that currently fail on llvmpipe on CI
PIGLIT_ARGS="$PIGLIT_ARGS -x xcleararea@6"
PIGLIT_ARGS="$PIGLIT_ARGS -x xcleararea@7"
PIGLIT_ARGS="$PIGLIT_ARGS -x xclearwindow@4"
PIGLIT_ARGS="$PIGLIT_ARGS -x xclearwindow@5"
PIGLIT_ARGS="$PIGLIT_ARGS -x xcopyarea@1"

export PIGLIT_ARGS

$XSERVER_BUILDDIR/test/simple-xinit \
        $XSERVER_DIR/test/scripts/run-piglit.sh \
        -- \
        $XSERVER_BUILDDIR/hw/vfb/Xvfb \
        -screen scrn 1280x1024x24
