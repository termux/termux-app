Termux uses apt and dpkg for package management, similar to Ubuntu or
Debian.

## Limitations

Termux does not support use of packages from the Debian, Ubuntu and
other Linux distribution repositories. For this reason do not attempt
put these ones into your sources.list or manually installing their .deb
files. See [Differences from Linux](Differences_from_Linux)
to learn why.

Additional restrictions:

- Only single architecture is supported at the moment. You can't have
  both 64 and 32 bit packages installed at the same time.
- Apt usage under root is restricted to prevent messing up ownership and
  SELinux labels on Android /data partition.
- Downgrading is not supported. In order to reclaim disk space, we do
  not keep history of package versions.

## Using the package manager

We strongly recommend to use a `pkg` utility instead of `apt` directly.
It is a wrapper that performs a number of tasks:

- Provides command shortcuts. Use "pkg in" instead of "pkg install" or
  "apt install".
- Automatically runs "apt update" before installing a package if
  necessary.
- Performs some client side repository load-balancing by automatically
  switching mirrors on a regular basis. *That is important to prevent us
  hitting quota limit on hosting.*


Installing a new package:

`pkg install package-name`

It is highly recommended to upgrade existing packages before installing
the new one. You can install updates by running this command:

`pkg upgrade`

Additionally, we suggesting to check for updates at least once a week.
Otherwise there is a certain risk that during package installation or
upgrade you will encounter issues.

Removing the installed package:

`pkg uninstall package-name`

This will remove package but modified configuration files will be left
intact. If you want to remove them, use `apt purge` instead.

See below for additional supported commands:

| Command                   | Description                                |
|---------------------------|--------------------------------------------|
| `pkg autoclean`           | Remove outdated .deb files from the cache. |
| `pkg clean`               | Remove all .deb files from the cache.      |
| `pkg files `<package>     | List files installed by specified package. |
| `pkg list-all`            | List all available packages.               |
| `pkg list-installed`      | List currently installed packages.         |
| `pkg reinstall `<package> | Re-install specified package.              |
| `pkg search `<query>      | Search package by query.                   |
| `pkg show `<package>      | Show information about specific package.   |

## Official repositories

The main Termux repository is accessible through
<https://packages.termux.org/apt/termux-main/>.

We have some optional repositories which provide content on specific
topic and can be enabled by installing packages with name ending in
`-repo`.

List of current optional repositories:

| Repository                                                               | Command to subscribe to repository |
|--------------------------------------------------------------------------|------------------------------------|
| [game-packages](https://github.com/termux/game-packages)                 | `pkg install game-repo`            |
| [science-packages](https://github.com/termux/science-packages)           | `pkg install science-repo`         |
| [termux-root-packages](https://github.com/termux/termux-root-packages)   | `pkg install root-repo`            |
| [x11-packages](https://github.com/termux/x11-packages) (Android 7+ only) | `pkg install x11-repo`             |

Packages for our official repositories are built from scripts located in
[github.com/termux/](https://github.com/termux) and are maintained and
signed by member of the [Termux developer
team](https://github.com/orgs/termux/people). Public keys for verifying
signatures are provided in package `termux-keyring`. For more
information about how the repositories are signed, see
[termux-keyring](termux-keyring).

The mirrors of Termux apt repositories are available. See up-to-date
information about them [on
Github](https://github.com/termux/termux-packages/wiki/Mirrors).

You can pick a mirror by using utility `termux-change-repo`.

## Community repositories

In addition to the official repositories, there are repositories hosted
by community members. You are welcome to host own Termux repository too.

You can create own repository by using
[termux-apt-repo](https://github.com/termux/termux-apt-repo) from the
command line and [Github Pages](https://pages.github.com/) as hosting.
Be aware that Github has a strict limit of 100 MB per file and if your
repository exceeds total size of 1 GB, you might receive a polite email
from GitHub Support requesting that you reduce the size of the
repository. So if you have really big packages you may want to use a
different hostings. Choose hostings according to filetypes, for example,
videos can be hosted at <https://YouTube.com> or similar.

### By [its-pointless](https://github.com/its-pointless/its-pointless.github.io)

Repository of this community member includes **gcc**, **gfortran**,
**octave**, **r-cran** (R language), **scipy** and lots of games!

To add this repository, execute:

    curl -LO https://its-pointless.github.io/setup-pointless-repo.sh
    bash setup-pointless-repo.sh

The script essentially installs gnupg on your device, downloads and adds
a public key to your apt keychain ensuring that all subsequent downloads
are from the same source.

## Package requests

Packages can be requested at
<https://github.com/termux/termux-packages/issues>. Note that your
opened issue with request can be moved to another repository, like
[termux-root-packages](https://github.com/termux/termux-root-packages)
or [x11-packages](https://github.com/termux/x11-packages) if it is not
suitable for the main repository.

Please ensure that you have read our [Packaging
Policy](https://github.com/termux/termux-packages/blob/master/CONTRIBUTING.md#a-note-about-package-requests).

### Other package managers

Some programming languages have their own package managers. We tend not
to package things installable with this ones due to issues when
cross-compiling them in our build environment.

- [Node.js Package Management (npm)](Node.js)
- [Perl Package Management (cpan)](Perl)
- [Python Package Management (pip)](Python)
- [Ruby Package Management (gem)](Ruby)
- [TeX Live Package Management (tlmgr)](TeX_Live)
- [Rust Package Manager
  (Cargo)](Development_Environments#Rust)

# See Also

- [Backing up Termux](Backing_up_Termux)
- [Building packages](Building_packages)