#!/bin/sh
# Script to build jni libraries while waiting for the new gradle build system:
# http://tools.android.com/tech-docs/new-build-system/gradle-experimental#TOC-Ndk-Integration

set -e -u

PROJECTDIR=`mktemp -d`
JNIDIR=$PROJECTDIR/jni
LIBSDIR=$PROJECTDIR/libs

mkdir $JNIDIR
cp app/src/main/jni/* $JNIDIR/

ndk-build NDK_PROJECT_PATH=$PROJECTDIR
cp -Rf $LIBSDIR/* app/src/main/jniLibs/

rm -Rf $PROJECTDIR
