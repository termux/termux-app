FISH is a smart and user-friendly command line shell for macOS, Linux,
and the rest of the family.
The FISH shell init files are `~/.fish`, `$PREFIX/etc/fish/config.fish`
and more. See \`man fish\` and \`info fish\` for more information.

## Oh-My-Fish

Oh-my-fish of fish shell working without any known issue in termux, you
can install it with official manual in the repository
[here](https://github.com/oh-my-fish/oh-my-fish/blob/master/README.md).
Run `$ omf` to get some help installing plugin and theme.

## Fisherman

Fisherman is a fish-shell plugin manager.

### Installation

`curl -Lo ~/.config/fish/functions/fisher.fish --create-dirs `[`https://git.io/fisher`](https://git.io/fisher)

### Usage

Install a plugin. `fisher z`

Install several plugins concurrently.
`fisher fzf edc/bass omf/thefuck omf/theme-bobthefish`

Install a specific branch. `fisher edc/bass:master`

Install a specific tag. `fisher edc/bass@1.2.0`

Install a gist.
`fisher `[`https://gist.github.com/username/1f40e1c6e0551b2666b2`](https://gist.github.com/username/1f40e1c6e0551b2666b2)

Install a local plugin. `fisher ~/path/to/my_plugin` Edit your fishfile
and run fisher to commit changes, e.g. install missing plugins.
`$EDITOR ~/.config/fish/fishfile fisher`

Show everything you've installed. <code>fisher ls @ my_plugin \# a local
plugin

- bobthefish \# current theme

` bass`
` fzf`
` thefuck`
` z`</code>

Show everything available to install. `fisher ls-remote`

Show additional information about plugins:
`fisher ls-remote --format="%name(%stars): %info [%url]\n"`

Update everything. `fisher up`

Update specific plugins. `fisher up bass z fzf`

Remove plugins. `fisher rm thefuck`

Remove all the plugins. `fisher ls | fisher rm`

Get help. `fisher help z`

Homepage: <https://fisherman.github.io>

Source code: <https://github.com/fisherman/fisherman>