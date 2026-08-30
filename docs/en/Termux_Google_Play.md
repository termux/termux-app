Since November 2, 2020 we no longer able to publish updates of Termux
application and add-ons because we are not ready for changes upcoming
with SDK level 29 (Android 10).

Everyone should move to F-Droid version, if possible. Check out [Backing
up Termux](Backing_up_Termux) if you are interested on how to
preserve data when re-installing the application.

## About the issue

Github discussion: <https://github.com/termux/termux-app/issues/1072>

If Termux application was built with target SDK level "29" or higher, it
will be eligible for SELinux restriction of execve() system call on data
files. That makes impossible to run package executables such as "apt",
"bash" and others located in the application's writable directory such
as \$PREFIX.

We have chosen to stick with target SDK level \<= 28 in order to keep
Termux running on Android devices with Android OS version 10 and higher
until we release next major version of Termux (v1.0). It will change app
design to comply with new SELinux configuration and Google Play policy,
potentially at cost of user experience.