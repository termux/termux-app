Termux is a terminal emulator application enhanced with a large set of
command line utilities ported to Android OS. The main goal is to bring
the Linux command line experience to users of mobile devices with no
rooting or other special setup required.

# How does it work

The terminal emulator is basically an application that launches the
command line program by using system call
[execve(2)](https://www.man7.org/linux/man-pages/man2/execve.2.html) and
redirecting standard input, output and error streams onto the display.

Most terminal applications available on Android OS work with a very
limited set of utilities which are usually provided either by the
operating system or other rooting tools such as Magisk. We have decided
to go further and port common software usually available on GNU/Linux
systems to Android OS.

Termux is neither a virtual machine nor any other kind of emulated or
simulated environment. All provided packages are cross-compiled with
Android NDK and only have compatibility patches to get them working on
Android. The operating system does not provide full access to its file
systems, so Termux cannot install package files into standard
directories such as /bin, /etc, /usr or /var. Instead, all files are
installed into the private application directory located at

`/data/data/com.termux/files/usr`

We call that directory "prefix" and usually refer to it as "\$PREFIX",
which is also an exported environment variable in the Termux shell. Note
that this directory cannot be changed or moved to an SD-Card because:

- The file system must have support for unix permissions and special
  files such as symlinks or sockets.
- The prefix path is hardcoded into all binaries.

In addition to prefix, users can store files in the home directory (or
"\$HOME") available at

`/data/data/com.termux/files/home`

However, the file system is not the only difference from the traditional
Linux distributions. For more information, read [Differences from
Linux](Differences_from_Linux).

# What can I do with Termux?

There are a number of common use-cases for the Termux application:

- Data processing with Python.
- Programming in a development environment.
- Downloading and managing files and pages using time-established tools.
- Learning the basics of the Linux command line environment.
- Running an SSH client.
- Synchronizing and backing up your files.


Of course, usage is not limited to the topics listed above. There are
more than 1000 packages in our repositories. If these packages don't
have what you're looking for, you can compile your own - we have a
variety of build tools, including compilers for languages like C, C++,
Go, Rust. Interpreters for common languages like NodeJS, Python, Ruby
are available too.

Please note that Termux is not a rooting tool and will not give you root
privileges unless you are skilled enough to break the Android OS
security.

# Is root required

Normally Termux does not require device to be rooted. In fact it's
primarily targeted for non-root users.

You may want to root your device to:

- Modify a device's firmware.
- Manipulate the parameters of the operating system or kernel.
- Non-interactively install/uninstall APKs.
- Have full R/W access to all file systems on device.
- Have direct access to hardware devices such as BT/Wi-Fi modules or
  serial lines (e.g. to access modem).
- Install a Linux distribution on top of Android through chroot (not
  proot!) or containerization.
- Generally have "full" control over your device.


Otherwise root isn't necessary and is rather bad than good.

# Tips

Here are basic tips on how to use Termux and survive:

- Learn shell scripting!
- Always keep your packages up-to-date! Run command `pkg upgrade` on
  regular basis or at least before installing a new package. Not
  updating packages or downgrading them voids your warranty.
- Do backups, always! Without backups, you will be probably unable to
  roll back if something goes wrong. Please note that software
  developers should pay attention to backing up debfiles of used
  compilers, interpreters or dependencies because Termux does not
  provide older package versions and it is a rolling-release. Check
  [Backing up Termux](https://wiki.termux.com/wiki/Backing_up_Termux)
  for info on how to backup and restore.
- Do not execute things which you do not know! Review scripts downloaded
  from the Internet. Always think about what you are typing into the
  terminal.
- Carefully read everything that has been printed to the terminal!
  Understanding the informational messages helps resolving issues which
  may occur.

# Are there any tutorials

We are not capable of maintaining the whole documentation about Linux
commands, shell scripting and other general-purpose information so links
to external resources are provided instead.

**We strongly suggest that you avoid YouTube tutorials, especially
related to hacking.** There a lot of clickbait targeted at inexperienced
users. If you decide to follow them, ensure that you fully understand
the executed commands. Also always check the content of downloaded
files. If the content of downloaded scripts is obfuscated, that should
be an alarm about potentially unsafe content. Do not complain that
Termux has not fulfilled your expectations.

## Commands and Shell Scripting

Discover the commands and learn how to use shell effectively.

- <https://linuxjourney.com/>
- <http://mywiki.wooledge.org/BashGuide>
- <https://www.tldp.org/LDP/Bash-Beginners-Guide/html/>


These links may be useful for advanced users:

- <https://wiki-dev.bash-hackers.org/> reference with examples of Bash
  built-in commands use,
- <https://debian-handbook.info/> The Debian Administrator's Handbook.


*When following tutorial examples, remember that Termux is not a Linux
distribution.* Some commands may not work, for example `ls /home`, due
to non-existent paths and other
[differences](Differences_from_Linux) Linux and Android have
regarding Termux distributions.

# How can I contribute?

The best ways to contribute are:

- Improving the Termux Wiki pages, i.e. by fleshing out sections that
  could use additional information or by correcting errors in grammar.
- Submitting bug reports. Please only submit reports that are about
  Termux packages or applications. Other errors should be submitted
  elsewhere.
- Submitting package updates.
- Submitting pull requests with bug fixes and improvements.


All of the source code for Termux can be found at
<https://github.com/termux>.

# See also

- [FAQ](FAQ)
- [Development Environments](Development_Environments)
- [Editors](Editors)
- [Differences from Linux](Differences_from_Linux)
- [Package Management](Package_Management)
- [Recover a broken
  environment](Recover_a_broken_environment)
- [Software](Software)