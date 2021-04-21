# Termux application

[![Build status](https://github.com/termux/termux-app/workflows/Build/badge.svg)](https://github.com/termux/termux-app/actions)
[![Testing status](https://github.com/termux/termux-app/workflows/Unit%20tests/badge.svg)](https://github.com/termux/termux-app/actions)
[![Join the chat at https://gitter.im/termux/termux](https://badges.gitter.im/termux/termux.svg)](https://gitter.im/termux/termux)

[Termux](https://termux.com) is an Android terminal application and Linux environment.

- [Termux Reddit community](https://reddit.com/r/termux)
- [Termux Wiki](https://wiki.termux.com/wiki/)
- [Termux Twitter](http://twitter.com/termux/)

Note that this repository is for the app itself (the user interface and the
terminal emulation). For the packages installable inside the app, see
[termux/termux-packages](https://github.com/termux/termux-packages)

***

**@termux is looking for Termux Application maintainer for implementing new features,
fixing bugs and reviewing pull requests since current one (@fornwall) is inactive.**

Issue https://github.com/termux/termux-app/issues/1072 needs extra attention.

***

## Installation

Termux can be obtained through various sources listed below.

The APK files of different sources are signed with different signature keys. The `Termux` app and all its plugins use the same [sharedUserId](https://developer.android.com/guide/topics/manifest/manifest-element) `com.termux` and so all their APKs installed on a device must have been signed with the same signature key to work together and so they must all be installed from the same source. Do not attempt to mix them together, i.e do not try to install an app or plugin from F-Droid and another one from a different source. Android Package Manager will also normally not allow installation of APKs with a different signatures and you will get an error on installation but this restriction can be bypassed with root or with custom roms. If you wish to install from a different source, then you must uninstall any and all existing Termux app or its plugin APKs from your device first, then install all new APKs from the same new source.

Following is a list of Termux app and its plugins.

- [Termux](https://github.com/termux/termux-app)
- [Termux:API](https://github.com/termux/termux-api)
- [Termux:Boot](https://github.com/termux/termux-boot)
- [Termux:Float](https://github.com/termux/termux-float)
- [Termux:Styling](https://github.com/termux/termux-styling)
- [Termux:Tasker](https://github.com/termux/termux-tasker)
- [Termux:Widget](https://github.com/termux/termux-widget)

 If you wish to install Termux from a difference source, you must uninstall all the apps listed above before installing from new source. Go to `Android Settings` -> `Applications` and then look for the following apps. You can also use the search feature if its available on your device and search `termux` in the applications list. Even if you think you have not installed any of the plugins, its strongly suggesting to go through the application list in Android settings and double check if installation is failing.

### F-Droid

Termux application can be obtained from F-Droid [here](https://f-droid.org/en/packages/com.termux/). It usually takes a few days (or even a week or more) for updates to be available on F-Droid once an update has been released on Github. F-Droid releases are built and published by F-Droid once they detect a new Github release. The Termux maintainers **do not** have any control over building and publishing of Termux app on F-Droid. Moreover, the Termux maintainers also do not have access to the APK signing keys of F-Droid releases, so we cannot release an APK ourselves on Github that would be compatible with F-Droid releases.

### Debug Builds

For users who don't want to wait for F-Droid releases and want to try out the latest features immediately or want to test their pull requests can get the APKs from [Github Actions](https://github.com/termux/termux-app/actions) page from the workflow runs labeled `Build`. The APK will be listed under `Artifacts` section. These are published for each commit done to the repository. These APKs are [debuggable](https://developer.android.com/studio/debug) and are also not compatible with other sources.
##



## Terminal resources

- [XTerm control sequences](http://invisible-island.net/xterm/ctlseqs/ctlseqs.html)
- [vt100.net](http://vt100.net/)
- [Terminal codes (ANSI and terminfo equivalents)](http://wiki.bash-hackers.org/scripting/terminalcodes)

## Terminal emulators

- VTE (libvte): Terminal emulator widget for GTK+, mainly used in gnome-terminal.
  [Source](https://github.com/GNOME/vte), [Open Issues](https://bugzilla.gnome.org/buglist.cgi?quicksearch=product%3A%22vte%22+),
  and [All (including closed) issues](https://bugzilla.gnome.org/buglist.cgi?bug_status=RESOLVED&bug_status=VERIFIED&chfield=resolution&chfieldfrom=-2000d&chfieldvalue=FIXED&product=vte&resolution=FIXED).

- iTerm 2: OS X terminal application. [Source](https://github.com/gnachman/iTerm2),
  [Issues](https://gitlab.com/gnachman/iterm2/issues) and [Documentation](http://www.iterm2.com/documentation.html)
  (which includes [iTerm2 proprietary escape codes](http://www.iterm2.com/documentation-escape-codes.html)).

- Konsole: KDE terminal application. [Source](https://projects.kde.org/projects/kde/applications/konsole/repository),
  in particular [tests](https://projects.kde.org/projects/kde/applications/konsole/repository/revisions/master/show/tests),
  [Bugs](https://bugs.kde.org/buglist.cgi?bug_severity=critical&bug_severity=grave&bug_severity=major&bug_severity=crash&bug_severity=normal&bug_severity=minor&bug_status=UNCONFIRMED&bug_status=NEW&bug_status=ASSIGNED&bug_status=REOPENED&product=konsole)
  and [Wishes](https://bugs.kde.org/buglist.cgi?bug_severity=wishlist&bug_status=UNCONFIRMED&bug_status=NEW&bug_status=ASSIGNED&bug_status=REOPENED&product=konsole).

- hterm: JavaScript terminal implementation from Chromium. [Source](https://github.com/chromium/hterm),
  including [tests](https://github.com/chromium/hterm/blob/master/js/hterm_vt_tests.js),
  and [Google group](https://groups.google.com/a/chromium.org/forum/#!forum/chromium-hterm).

- xterm: The grandfather of terminal emulators.
  [Source](http://invisible-island.net/datafiles/release/xterm.tar.gz).

- Connectbot: Android SSH client. [Source](https://github.com/connectbot/connectbot)

- Android Terminal Emulator: Android terminal app which Termux terminal handling
  is based on. Inactive. [Source](https://github.com/jackpal/Android-Terminal-Emulator).
##



## For Devs and Contributors

The [termux-shared](termux-shared) library was added in [`v0.109`](https://github.com/termux/termux-app/releases/tag/v0.109). It defines shared constants and utils of Termux app and its plugins. It was created to allow for removal of all hardcoded paths in Termux app. The termux plugins will hopefully use this in future as well. If you are contributing code that is using a constant or a util that may be shared, then define it in `termux-shared` library if it currently doesn't exist and reference it from there. Update the relevant changelogs as well. Pull requests using hardcoded values **will not** be accepted.

The main Termux constants are defined by [`TermuxConstants`](https://github.com/termux/termux-app/blob/master/termux-shared/src/main/java/com/termux/shared/termux/TermuxConstants.java) class. It also contains information on how to fork Termux or build it with your own package name. Changing the package name will require building the bootstrap zip packages and other packages with the new `$PREFIX`, check [Building Packages](https://github.com/termux/termux-packages/wiki/Building-packages) for more info.
