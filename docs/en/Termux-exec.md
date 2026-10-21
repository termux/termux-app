Termux-exec allows you to execute scripts with shebangs for traditional
Unix file structures. So shebangs like `#!/bin/sh` and
`#!/usr/bin/env python` should be able to run without
[termux-fix-shebang](termux-fix-shebang).

After upgrading to apt version 1.4.8-1 or more recent, termux-exec
should be already installed as dependency of apt, if not — it is time to
run `pkg upgrade` and then restart Termux application.

If you are not able to run scripts with shebangs like `#!/bin/sh` even
if termux-exec is installed, try to reset LD_PRELOAD environment
variable:

`export LD_PRELOAD=${PREFIX}/lib/libtermux-exec.so`

and then restart Termux session.