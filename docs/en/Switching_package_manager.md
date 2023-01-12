In this page, it is written how to change the package manager in termux.
Termux currently has 2 package managers, `apt` (default) and `pacman`.
To switch to another package manager, you need to change bootstrap based
on the specific package manager.

#### Instruction:

First you need to install bootstrap based on a specific package manager.

- apt based bootstrap you can find
  [here](https://github.com/termux/termux-packages/releases)
- pacman based bootstrap you can find
  [here](https://github.com/termux-pacman/termux-packages/releases)

The second step is to create a `usr-n/` directory next to `usr/`, then
move the bootstrap archive to `usr-n/` and unzip it.
The third step is to run the following command in the `usr-n/` directory
to create symbolic links.

`cat SYMLINKS.txt | awk -F "←" '{system("ln -s '"'"'"$1"'"'"' '"'"'"$2"'"'"'")}'`

The fourth step is to replace `usr/` with `usr-n/`, to do this, run
failsafe (how to run it is written [here](Fail-safe)), then
delete the old `usr/` directory and rename `usr-n/`.

`cd ..`
`rm -fr usr/`
`mv usr-n/ usr/`

**If you are switching to pacman then you need to follow this step.**
The fifth step, exit failsafe and start termux normally, then run the
following command to set up gpg keys for pacman.

`pacman-key --init`
`pacman-key --populate`