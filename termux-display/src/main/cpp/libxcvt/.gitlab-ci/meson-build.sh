#!/bin/bash

if [[ -f .meson_environment ]]; then
	. .meson_environment
fi

if [[ -z "$MESON_BUILDDIR" ]]; then
	echo "\$MESON_BUILDDIR undefined."
	exit 1
fi

# emulate a few gitlab variables to make it easier to
# run and debug locally.
if [[ -z "$CI_JOB_ID" ]] || [[ -z "$CI_JOB_NAME" ]] || [[ -z "$CI_PROJECT_NAME" ]]; then
	echo "Missing \$CI_JOB_ID or \$CI_JOB_NAME".
	CI_PROJECT_NAME=$(basename $PWD)
	CI_JOB_ID=$(date +%s)
	CI_JOB_NAME='${CI_PROJECT_NAME}-job-local'
	echo "Simulating gitlab environment: "
	echo " CI_JOB_ID=$CI_JOB_ID"
	echo " CI_JOB_NAME=$CI_JOB_NAME"
	echo " CI_PROJECT_NAME=$CI_PROJECT_NAME"
fi


echo "*************************************************"
echo "builddir: $MESON_BUILDDIR"
echo "meson args: $MESON_ARGS"
echo "ninja args: $NINJA_ARGS"
echo "meson test args: $MESON_TEST_ARGS"
echo "*************************************************"

set -e

rm -rf "$MESON_BUILDDIR"
meson "$MESON_BUILDDIR" $MESON_ARGS
meson configure "$MESON_BUILDDIR"
ninja -C "$MESON_BUILDDIR" $NINJA_ARGS

if [[ -z "$MESON_TEST_ARGS" ]]; then
    exit 0
fi

# we still want to generate the reports, even if meson test fails
meson test -C "$MESON_BUILDDIR" $MESON_TEST_ARGS --print-errorlogs
exit_code=$?

exit $exit_code
