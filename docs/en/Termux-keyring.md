The various official package repositories are maintained and signed by
the members of the [termux-team](https://github.com/orgs/termux/people).
Each member has there own GPG key that is used for signing and the
public component of these keys are available in the package
`termux-keyring`. Installing termux-keyring makes apt recognize and
trust these keys, which is necessary for updating, and downloading from,
the repositories.

## Current keys in termux-keyring

### Grimler's key

[Grimler's](https://github.com/Grimler91) key is available as the file
[grimler.gpg](https://github.com/termux/termux-packages/blob/master/packages/termux-keyring/grimler.gpg)
and has fingerprint

`2C7F 29AE 9789 1F64 19A9  E2CD B007 6E49 0B71 616B`

### Automatic build's key

When packages are built and uploaded automatically using CI/CD, the
repository is signed with the key
[termux-autobuilds.gpg](https://github.com/termux/termux-packages/blob/master/packages/termux-keyring/termux-autobuilds.gpg)
and has fingerprint

`CC72 CF8B A7DB FA01 8287  7D04 5A89 7D96 E57C F20C`