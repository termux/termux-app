#!/bin/sh

set -ex

./autogen.sh
make -sj4 check
