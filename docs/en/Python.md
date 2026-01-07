Python is an interpreted, high-level, general-purpose programming
language. Created by Guido van Rossum and first released in 1991,
Python's design philosophy emphasizes code readability with its notable
use of significant whitespace. Its language constructs and
object-oriented approach aim to help programmers write clear, logical
code for small and large-scale projects.

In Termux Python v3.x can be installed by executing

`pkg install python`

Legacy, deprecated version 2.7.x can be installed by

`pkg install python2`

**Warning**: upgrading major/minor version of Python package, for
example from Python 3.8 to 3.9, will make all your currently installed
modules unusable. You will need to reinstall them. However upgrading
patch versions, for example from 3.8.1 to 3.8.2, is safe.

**Due to our infrastructure limits, we do not provide older versions of
packages. If you accidentally upgraded to unsuitable Python version and
do not have backups to rollback, do not complain! We recommend doing
backups of \$PREFIX for developers and other people who rely on specific
software versions.**

## Package management

After installing Python, `pip` (`pip2` if using python2) package manager
will be available. Here is a quick tutorial about its usage.

Installing a new Python module:

`pip install {module name}`

Uninstalling Python module:

`pip uninstall {module name}`

Listing installed modules:

`pip list`

When installing Python modules, it is highly recommended to have a
package `build-essential` to be installed - some modules compile native
extensions during their installation.

A few python packages are available from termux's package manager (for
python3 only), and should be installed from there to avoid compilation
errors. This is the case for:

- numpy, `pkg install python-numpy`
- electrum, `pkg install electrum`
- opencv, `pkg install opencv-python`
- asciinema, `pkg install asciinema`
- matplotlib, `pkg install matplotlib`
- cryptography, `pkg install python-cryptography`

## Python module installation tips and tricks

It is assumed that you have `build-essential` or at least `clang`,
`make` and `pkg-config` installed.

It also assumed that `termux-exec` is not broken and works on your
device. Environment variable `LD_PRELOAD` is not tampered or unset.
Otherwise you will need to patch modules' source code to fix all
shebangs!

*Tip: help us to collect more information about installing Python
modules in Termux. You can also help to keep this information
up-to-date, because current instructions may eventually become
obsolete.*

<table>
<thead>
<tr class="header">
<th><p>Package</p></th>
<th><p>Description</p></th>
<th><p>Dependencies</p></th>
<th><p>Special Instructions</p></th>
</tr>
</thead>
<tbody>
<tr class="odd">
<td><p>gmpy2</p></td>
<td><p>C-coded Python modules for fast multiple-precision
arithmetic.<br />
<a
href="https://github.com/aleaxit/gmpy">https://github.com/aleaxit/gmpy</a></p></td>
<td><p>libgmp libmpc libmpfr</p></td>
<td></td>
</tr>
<tr class="even">
<td><p>lxml</p></td>
<td><p>Bindings to libxml2 and libxslt.<br />
<a href="https://lxml.de/">https://lxml.de/</a></p></td>
<td><p>libxml2 libxslt</p></td>
<td></td>
</tr>
<tr class="odd">
<td><p>pandas</p></td>
<td><p>Flexible and powerful data analysis / manipulation library for
Python.<br />
<a
href="https://pandas.pydata.org/">https://pandas.pydata.org/</a></p></td>
<td></td>
<td><p><code>export CFLAGS="-Wno-deprecated-declarations -Wno-unreachable-code"</code><br />
<code>pip install pandas</code></p></td>
</tr>
<tr class="even">
<td><p>pynacl</p></td>
<td><p>Bindings to the Networking and Cryptography library.<br />
<a
href="https://pypi.python.org/pypi/PyNaCl">https://pypi.python.org/pypi/PyNaCl</a></p></td>
<td><p>libsodium</p></td>
<td></td>
</tr>
<tr class="odd">
<td><p>pillow</p></td>
<td><p>Python Imaging Library.<br />
<a
href="https://pillow.readthedocs.io/en/stable/">https://pillow.readthedocs.io/en/stable/</a></p></td>
<td><p>libjpeg-turbo libpng</p></td>
<td><p>64-bit devices require running
<code>export LDFLAGS="-L/system/lib64"</code> before pip
command.</p></td>
</tr>
<tr class="even">
<td><p>pyzmq</p></td>
<td><p>Bindings to libzmq.<br />
<a
href="https://pyzmq.readthedocs.io/en/latest/">https://pyzmq.readthedocs.io/en/latest/</a></p></td>
<td><p>libzmq</p></td>
<td><p>On some devices the libzmq library can't be found by setup.py. If
<code>pip install pyzmq</code> does not work, try:
<code>pip install pyzmq --install-option="--libzmq=/data/data/com.termux/files/usr/lib/libzmq.so"</code></p></td>
</tr>
</tbody>
</table>

### Advanced installation instructions

Some Python modules may not be easy to install. Here are collected
information on how to get them available in your Termux.

#### Tkinter

Tkinter is splitted of from the `python` package and can be installed by

`pkg install python-tkinter`

We do not provide Tkinter for Python v2.7.x.

Since Tkinter is a graphical library, it will work only if X Windows
System environment is installed and running. How to do this, see page
[Graphical Environment](Graphical_Environment).

### Installing Python modules from source

Some modules may not be installable without patching. They should be
installed from source code. Here is a quick how-to about installing
Python modules from source code.

1\. Obtain the source code. You can clone a git repository of your
package:

    git clone https://your-package-repo-url
    cd ./your-package-repo

or download source bundle with `pip`:

    pip download {module name}
    unzip {module name}.zip
    cd {module name}

2\. Optionally, apply the desired changes to source code. There no
universal guides on that, perform this step on your own.

3\. Optionally, fix the all shebangs. This is not needed if
`termux-exec` is installed and works correctly.

    find . -type f -not -path '*/\.*' -exec termux-fix-shebang "{}" \;

4\. Finally install the package:

    python setup.py install

## Troubleshooting

### pip doesn't read config in `~/.config/pip/pip.conf`

A.k.a.

- virtualenv doesn't read config in
  `~/.config/virtualenv/virtualenv.ini` / stores its data in
  `/data/data/com.termux/files/virtualenv` .

<!-- -->

- pip / virtualenv doesn't follow freedesktop `$XDG_CONFIG_HOME` /
  `$XDG_DATA_HOME` / `$XDG_CACHE_HOME` .

<!-- -->

- pylint / black doesn't store its cache in `~/.cache` but stores its
  cache in `/data/data/com.termux/cache` .

All of above are because of
[platformdirs](https://github.com/platformdirs/platformdirs).
Platformdirs aims to replace
[appdirs](https://github.com/ActiveState/appdirs), since [pip
v21.3.0](https://github.com/pypa/pip/pull/10202) and [virtualenv
v20.5.0](https://github.com/pypa/virtualenv/pull/2142), they started to
use platformdirs instead of appdirs. Appdirs doesn't do anything else on
Android, it just follows freedesktop standards. But platformdirs is
different, it takes termux as a simple Android app but not a unix
evironment.

See details: [platformdirs issue
70](https://github.com/platformdirs/platformdirs/issues/70).

It it predictable that all packages using platformdirs can't behave well
on termux, see: [1](https://libraries.io/pypi/platformdirs/dependents).
Before [PR 72](https://github.com/platformdirs/platformdirs/pull/72) is
merged, the only way to fix it is to patch it manually.

There are two copies of platformdirs we need to patch:

1.  Pip vendors its own copy in
    `$PREFIX/lib/pythonX.Y/site-packages/pip/_vendor`.
2.  Platformdirs is installed as a dependency in
    `$PREFIX/lib/pythonX.Y/site-packages`. (If it was installed by
    `pip install --user`, the path is
    `~/.local/lib/pythonX.Y/site-packages`.)

Every time after we upgrade pip or platformdirs, we need to patch it
again.

Patch for platformdirs before v2.5.0:

    --- __init__.py.bak 2022-03-09 02:21:09.888903935 +0800
    +++ __init__.py 2022-04-02 02:37:05.802427311 +0800
    @@ -18,7 +18,7 @@


     def _set_platform_dir_class() -> type[PlatformDirsABC]:
    -    if os.getenv("ANDROID_DATA") == "/data" and os.getenv("ANDROID_ROOT") == "/system":
    +    if os.getenv("ANDROID_DATA") == "/data" and os.getenv("ANDROID_ROOT") == "/system" and os.getenv("SHELL") is None:
             module, name = "pip._vendor.platformdirs.android", "Android"
         elif sys.platform == "win32":
             module, name = "pip._vendor.platformdirs.windows", "Windows"

Patch for platformdirs v2.5.0 or later:

    --- __init__.py.bak 2022-03-09 02:29:15.338903750 +0800
    +++ __init__.py 2022-04-02 02:44:38.992427138 +0800
    @@ -25,6 +25,10 @@
             from platformdirs.unix import Unix as Result

         if os.getenv("ANDROID_DATA") == "/data" and os.getenv("ANDROID_ROOT") == "/system":
    +
    +        if os.getenv("SHELL") is not None:
    +            return Result
    +
             from platformdirs.android import _android_folder

             if _android_folder() is not None:

We can simply patch it by:

    patch ~/../usr/lib/python3.10/site-packages/pip/_vendor/platformdirs/__init__.py -i platformdirs.patch

Notice that the two copies may be different versions, so they need
different patches. For example, pip v21.3 and v22.0 use platformdirs
v2.4, and the lastest version (on 2022-04-02) is v2.5.1.