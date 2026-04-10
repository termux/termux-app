# Building packages

## Using Termux build environment

A recommended way of building is to use official build environment
available on Github: <https://github.com/termux/termux-packages>. See
[Termux Developer's
Wiki](https://github.com/termux/termux-packages/wiki) for tips about its
usage.

- <https://github.com/termux/termux-packages/wiki/Build-environment> -
  setting up build environment.

<!-- -->

- <https://github.com/termux/termux-packages/wiki/Building-packages> -
  how to build a package.

<!-- -->

- <https://github.com/termux/termux-packages/wiki/Creating-new-package> -
  information about writing package scripts (build.sh).

### On-device usage

It is possible to use our build environment directly on device without
Docker image or VM setup. You need only to:

1\. Clone the git repository:

    git clone https://github.com/termux/termux-packages

2\. Execute setup script:

`cd termux-packages`
`./scripts/setup-termux.sh`

Packages are built by executing `./build-package.sh -I package_name`.
Note that option "-I" tells build-package.sh to download and install
dependency packages automatically instead of building them which makes
build a lot faster.

By default, with Termux build environment you can build only existing
packages. If package is not exist in `./packages` directory, then you
will have to write its build.sh manually.

**If you managed to successfully build a new package, consider to make a
pull request.**

Some notes about on-device building:

- Some packages are considered as unsafe for building on device as they
  remove stuff from `$PREFIX`.

<!-- -->

- During build process, the compiled stuff is installed directly to
  `$PREFIX` and is not tracked by dpkg. This is expected behaviour. If
  you don't want this stuff, install generated deb files and uninstall.

<!-- -->

- Some packages may throw errors when building on device. This is known
  issue and tracked in
  <https://github.com/termux/termux-packages/issues/4157>.

<!-- -->

- On-device package building for android-5 branch is not tested well.

## Manually building packages

There no universal guide about building/porting packages in Termux,
especially since Termux isn't a standard platform.

Though you can follow some recommendations mentioned here:

1\. Make sure that minimal set of build tools is installed:

`pkg install build-essential`

2\. After extracting package source code, check for files named like
"README" or "INSTALL". They usually contain information about how to
build a package.

3\. Autotools based projects (have ./configure script in project's root)
in most cases can be built with the following commands:

`./configure --prefix=$PREFIX`
`make && make install`

It is highly recommended to check the accepted configuration options by
executing `./configure --help`.

In case of configuration failure, read the output printed on screen and
check the file `config.log` (contains a lot of text, but all information
about error's source exist in it).

4\. CMake based projects (have CMakeLists in project's root) should be
possible to build with:

`mkdir build`
`cd build`
`cmake -DCMAKE_INSTALL_PREFIX="${PREFIX}" ..`
`make`
`make install`

Be careful when running `make install` as it will unconditionally write
files to `$PREFIX`. Never execute any of build commands as root.

**Note about "bug reports":** if you trying to build custom package and
it fails, do not submit bug reports regarding it.

**Note about "package requests":** if you decided to submit a package
request for package that you can't build, make sure this package will be
useful for others. Otherwise such requests will be just rejected.