#!/bin/sh
# Standalone, source-only harness: supply a JDK 17 and Maven on PATH (or JAVA_HOME/MVN).
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
MVN=${MVN:-mvn}
JAVA=${JAVA_HOME:+"$JAVA_HOME/bin/"}java
VERSION=$("$JAVA" -version 2>&1)
case "$VERSION" in
  *'version "17.'*) ;;
  *) printf '%s\n' 'This native Robolectric harness requires JDK 17 (set JAVA_HOME).' "$VERSION" >&2; exit 1 ;;
esac
# Maven and Robolectric use normal HTTPS repositories by default. Cache/offline
# options, if wanted, belong in caller-supplied flags, not in repository paths.
exec "$MVN" -B -f "$HERE/pom.xml" "$@" clean test
