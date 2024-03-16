#!/bin/sh

set -e

if test "x$XTEST_DIR" = "x"; then
    echo "XTEST_DIR must be set to the directory of the xtest repository."
    # Exit as a "skip" so make check works even without piglit.
    exit 77
fi

if test "x$PIGLIT_DIR" = "x"; then
    echo "PIGLIT_DIR must be set to the directory of the piglit repository."
    # Exit as a "skip" so make check works even without piglit.
    exit 77
fi

if test "x$PIGLIT_RESULTS_DIR" = "x"; then
    echo "PIGLIT_RESULTS_DIR must be set to where to output piglit results."
    # Exit as a real failure because it should always be set.
    exit 1
fi

if test "x$XSERVER_DIR" = "x"; then
    echo "XSERVER_DIR must be set to the directory of the xserver repository."
    # Exit as a real failure because it should always be set.
    exit 1
fi

if test "x$XSERVER_BUILDDIR" = "x"; then
    echo "XSERVER_BUILDDIR must be set to the build directory of the xserver repository."
    # Exit as a real failure because it should always be set.
    exit 1
fi

if test "x$SERVER_COMMAND" = "x"; then
    echo "SERVER_COMMAND must be set to the server to be spawned."
    # Exit as a real failure because it should always be set.
    exit 1
fi

$XSERVER_BUILDDIR/test/simple-xinit \
    $XSERVER_DIR/test/scripts/xinit-piglit-session.sh \
    -- \
    $SERVER_COMMAND

# Write out piglit-summaries.
SHORT_SUMMARY=$PIGLIT_RESULTS_DIR/summary
LONG_SUMMARY=$PIGLIT_RESULTS_DIR/long-summary
$PIGLIT_DIR/piglit summary console -s $PIGLIT_RESULTS_DIR > $SHORT_SUMMARY
$PIGLIT_DIR/piglit summary console $PIGLIT_RESULTS_DIR > $LONG_SUMMARY

# Write the short summary to make check's log file.
cat $SHORT_SUMMARY

# Parse the piglit summary to decide on our exit status.
status=0
# "pass: 0" would mean no tests actually ran.
if grep "^ *pass: *0$" $SHORT_SUMMARY > /dev/null; then
    status=1
fi
# Fails or crashes should be failures from make check's perspective.
if ! grep "^ *fail: *0$" $SHORT_SUMMARY > /dev/null; then
    status=1
fi
if ! grep "^ *crash: *0$" $SHORT_SUMMARY > /dev/null; then
    status=1
fi

$PIGLIT_DIR/piglit summary html \
	--overwrite \
	$PIGLIT_RESULTS_DIR/html \
	$PIGLIT_RESULTS_DIR

if test $status != 0; then
    echo "Some piglit tests failed."
    echo "The list of failing tests can be found in $LONG_SUMMARY."
fi
echo "An html page of the test status can be found at $PIGLIT_RESULTS_DIR/html/index.html"

exit $status
