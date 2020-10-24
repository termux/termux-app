#!/usr/bin/env bash
echo "zipping changes"
cd changes
zip -r ../changes.zip * > /dev/null 2>&1
cd ..
