#! /bin/sh

srcdir=`dirname "$0"`
test -z "$srcdir" && srcdir=.

ORIGDIR=`pwd`
cd "$srcdir"

autoreconf -v --install || exit 1
cd "$ORIGDIR" || exit $?

git config --local --get format.subjectPrefix ||
        git config --local format.subjectPrefix "PATCH libxtrans"

if test -z "$NOCONFIGURE"; then
    exec "$srcdir"/configure "$@"
fi
