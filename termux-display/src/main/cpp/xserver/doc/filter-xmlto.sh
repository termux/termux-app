#!/bin/sh
#
# Run the xmlto command, filtering its output to
# reduce the amount of useless warnings in the build log.
#
# Exit with the status of the xmlto process, not the status of the
# output filtering commands
#
# This is a bit twisty, but avoids any temp files by using pipes for
# everything. It routes the command output through file
# descriptor 4 while sending the (numeric) exit status through
# standard output.
#
(((("$@" 2>&1; echo $? >&3) |
       grep -v overflows |
       grep -v 'Making' |
       grep -v 'hyphenation' |
       grep -v 'Font.*not found' |
       grep -v '/tmp/xml' |
       grep -v Rendered >&4) 3>&1) |
     (read status; exit $status)) 4>&1
