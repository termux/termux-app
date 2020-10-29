#!/usr/bin/env bash
# this is lazy and you should use build bootstrap instead
echo "zipping changes"
cd changes
zip -r ../changes.zip * > /dev/null 2>&1
cd ..
