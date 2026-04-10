**There is a newer and simpler solution, install
[termux-exec](termux-exec) to resolve this matter.**

Only files within Termux’s private space can be made executable. So any
executable files such as shell scripts need to be kept inside the apps
native space unless you run them through a shell, e.g. `sh script.sh`,
`bash script.sh`, `csh script.sh`, etc...

Since the usual location `/bin` is
`/data/data/com.termux/files/usr/bin`, also known as `$PREFIX/bin`, this
can cause problems with scripts which have a shebang such as
`#!/bin/sh`. This can be fixed by using `termux-fix-shebang` on the
shell scripts. For example:

`  termux-fix-shebang script.sh`
`  ./script.sh`

Now `script.sh` should run correctly.