#!/bin/sh
#
# 'Cause xcodebuild is hard to deal with

SRCDIR=$1
BUILDDIR=$2
BUNDLE_ROOT=$3

localities="Dutch English French German Italian Japanese Spanish ar ca cs da el fi he hr hu ko no pl pt pt_PT ro ru sk sv th tr uk zh_CN zh_TW"
for lang in ${localities} ; do
    [ -d ${BUNDLE_ROOT}/Contents/Resources/${lang}.lproj ] && rm -rf ${BUNDLE_ROOT}/Contents/Resources/${lang}.lproj
    mkdir -p ${BUNDLE_ROOT}/Contents/Resources/${lang}.lproj/main.nib
    [ -d ${BUNDLE_ROOT}/Contents/Resources/${lang}.lproj/main.nib ] || exit 1

    for f in Localizable.strings main.nib/keyedobjects.nib main.nib/keyedobjects-110000.nib ; do
        install -m 644 ${SRCDIR}/Resources/${lang}.lproj/$f ${BUNDLE_ROOT}/Contents/Resources/${lang}.lproj/${f}
    done
done

install -m 644 ${SRCDIR}/Resources/English.lproj/main.nib/designable.nib ${BUNDLE_ROOT}/Contents/Resources/English.lproj/main.nib
install -m 644 ${SRCDIR}/Resources/X11.icns ${BUNDLE_ROOT}/Contents/Resources

install -m 644 ${BUILDDIR}/Info.plist ${BUNDLE_ROOT}/Contents
install -m 644 ${SRCDIR}/PkgInfo ${BUNDLE_ROOT}/Contents

mkdir -p ${BUNDLE_ROOT}/Contents/MacOS
install -m 755 ${SRCDIR}/X11.sh ${BUNDLE_ROOT}/Contents/MacOS

if [[ $(id -u) == 0 ]] ; then
	chown -R root:admin ${BUNDLE_ROOT}
fi
