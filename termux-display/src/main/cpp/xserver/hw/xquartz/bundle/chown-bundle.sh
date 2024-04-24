#!/bin/sh

BUNDLE_ROOT=$1

if [[ $(id -u) == 0 ]] ; then
	chown -R root:admin ${BUNDLE_ROOT}
fi
