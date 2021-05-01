# Termux application

[![Build status](https://github.com/termux/termux-app/workflows/Build/badge.svg)](https://github.com/termux/termux-app/actions)
[![Testing status](https://github.com/termux/termux-app/workflows/Unit%20tests/badge.svg)](https://github.com/termux/termux-app/actions)
[![Join the chat at https://gitter.im/termux/termux](https://badges.gitter.im/termux/termux.svg)](https://gitter.im/termux/termux)

[Termux](https://termux.com) is an Android terminal application and Linux environment.

Note that this repository is for the app itself (the user interface and the terminal emulation). For the packages installable inside the app, see [termux/termux-packages](https://github.com/termux/termux-packages).

***

**@termux is looking for Termux Application maintainers for implementing new features, fixing bugs and reviewing pull requests since current one (@fornwall) is inactive.**

Issue https://github.com/termux/termux-app/issues/1072 needs extra attention.

***

### Contents
- [Termux App and Plugins](#Termux-App-and-Plugins)
- [Installation](#Installation)
- [Uninstallation](#Uninstallation)
- [Important Links](#Important-Links)
- [For Devs and Contributors](#For-Devs-and-Contributors)
##



## Termux App and Plugins

The core [Termux](https://github.com/termux/termux-app) app comes with the following optional plugin apps.

- [Termux:API](https://github.com/termux/termux-api)
- [Termux:Boot](https://github.com/termux/termux-boot)
- [Termux:Float](https://github.com/termux/termux-float)
- [Termux:Styling](https://github.com/termux/termux-styling)
- [Termux:Tasker](https://github.com/termux/termux-tasker)
- [Termux:Widget](https://github.com/termux/termux-widget)
##



## Installation

Termux can be obtained through various sources listed below for **only** Android `>= 7`. Support was dropped for Android `5` and `6` on [2020-01-01](https://www.reddit.com/r/termux/comments/dnzdbs/end_of_android56_support_on_20200101/) at `v0.83`, old builds are available on [archive.org](https://archive.org/details/termux-repositories-legacy).

The APK files of different sources are signed with different signature keys. The `Termux` app and all its plugins use the same [sharedUserId](https://developer.android.com/guide/topics/manifest/manifest-element) `com.termux` and so all their APKs installed on a device must have been signed with the same signature key to work together and so they must all be installed from the same source. Do not attempt to mix them together, i.e do not try to install an app or plugin from F-Droid and another one from a different source. Android Package Manager will also normally not allow installation of APKs with a different signatures and you will get errors on installation like `App not installed`, `Failed to install due to an unknown error`, `INSTALL_FAILED_UPDATE_INCOMPATIBLE`, `INSTALL_FAILED_SHARED_USER_INCOMPATIBLE`, `signatures do not match previously installed version`, etc. This restriction can be bypassed with root or with custom roms.

If you wish to install from a different source, then you must uninstall **any and all existing Termux or its plugin app APKs** from your device first, then install all new APKs from the same new source. Check [Uninstallation](#Uninstallation) section for details. You may also want to consider [Backing up Termux](https://wiki.termux.com/wiki/Backing_up_Termux) before uninstallation.

### F-Droid

Termux application can be obtained from F-Droid [here](https://f-droid.org/en/packages/com.termux/). It usually takes a few days (or even a week or more) for updates to be available on F-Droid once an update has been released on Github. F-Droid releases are built and published by F-Droid once they detect a new Github release. The Termux maintainers **do not** have any control over building and publishing of Termux app on F-Droid. Moreover, the Termux maintainers also do not have access to the APK signing keys of F-Droid releases, so we cannot release an APK ourselves on Github that would be compatible with F-Droid releases.

### Debug Builds

For users who don't want to wait for F-Droid releases and want to try out the latest features immediately or want to test their pull requests can get the APKs from [Github Actions](https://github.com/termux/termux-app/actions) page from the workflow runs labeled `Build`. The APK will be listed under `Artifacts` section. These are published for each commit done to the repository. These APKs are [debuggable](https://developer.android.com/studio/debug) and are also not compatible with other sources.

### Google Playstore **(Deprecated)**

**Termux and its plugins are no longer updated on [Google playstore](https://play.google.com/store/apps/details?id=com.termux) due to [android 10 issues](https://github.com/termux/termux-packages/wiki/Termux-and-Android-10).** The last version released for Android `>= 7` was `v0.101`. There are currently no immediate plans to resume updates on Google playstore. **It is highly recommended to not install Termux from playstore for now.** Any current users **should switch** to a different source like F-Droid.

If for some reason you don't want to switch, then at least check [Package Management](https://github.com/termux/termux-packages/wiki/Package-Management) to switch to a different mirror and then run `pkg upgrade` command to update all packages to the latest available versions if you want, or at least update `termux-tools` package with `pkg install termux-tools` command.
##



## Uninstallation

Uninstallation may be required if a user doesn't want Termux installed in their device anymore or is switching to a different [install source](#Installation). You may also want to consider [Backing up Termux](https://wiki.termux.com/wiki/Backing_up_Termux) before uninstallation.

To uninstall Termux completely, you must uninstall **any and all existing Termux or its plugin app APKs** listed in [Termux App and Plugins](#Termux-App-and-Plugins).

Go to `Android Settings` -> `Applications` and then look for those apps. You can also use the search feature if its available on your device and search `termux` in the applications list.

Even if you think you have not installed any of the plugins, its strongly suggesting to go through the application list in Android settings and double check.
##



## Important Links

### Community
All community links are available [here](https://wiki.termux.com/wiki/Community).

The main ones are the following.

- [Termux Reddit community](https://reddit.com/r/termux)
- [Termux Matrix Channel](https://matrix.to/#termux_termux:gitter.im)
- [Termux Dev Matrix Channel](https://matrix.to/#termux_dev:gitter.im)
- [Termux Twitter](http://twitter.com/termux/)
- [Termux Reports Email](mailto:termuxreports@groups.io)

### Wikis

- [Termux Wiki](https://wiki.termux.com/wiki/)
- [Termux App Wiki](https://github.com/termux/termux-app/wiki)
- [Termux Packages Wiki](https://github.com/termux/termux-packages/wiki)

### Miscellaneous
- [FAQ](https://wiki.termux.com/wiki/FAQ)
- [Termux File System Layout](https://github.com/termux/termux-packages/wiki/Termux-file-system-layout)
- [Differences From Linux](https://wiki.termux.com/wiki/Differences_from_Linux)
- [Package Management](https://wiki.termux.com/wiki/Package_Management)
- [Remote_Access](https://wiki.termux.com/wiki/Remote_Access)
- [Backing up Termux](https://wiki.termux.com/wiki/Backing_up_Termux)
- [Terminal Settings](https://wiki.termux.com/wiki/Terminal_Settings)
- [Touch Keyboard](https://wiki.termux.com/wiki/Touch_Keyboard)
- [Android Storage and Sharing Data with Other Apps](https://wiki.termux.com/wiki/Internal_and_external_storage)
- [Android APIs](https://wiki.termux.com/wiki/Termux:API)
- [Moved Termux Packages Hosting From Bintray to IPFS](https://github.com/termux/termux-packages/issues/6348)
- [Termux and Android 10](https://github.com/termux/termux-packages/wiki/Termux-and-Android-10)

### Terminal resources

- [XTerm control sequences](http://invisible-island.net/xterm/ctlseqs/ctlseqs.html)
- [vt100.net](http://vt100.net/)
- [Terminal codes (ANSI and terminfo equivalents)](http://wiki.bash-hackers.org/scripting/terminalcodes)

### Terminal emulators

- VTE (libvte): Terminal emulator widget for GTK+, mainly used in gnome-terminal. [Source](https://github.com/GNOME/vte), [Open Issues](https://bugzilla.gnome.org/buglist.cgi?quicksearch=product%3A%22vte%22+), and [All (including closed) issues](https://bugzilla.gnome.org/buglist.cgi?bug_status=RESOLVED&bug_status=VERIFIED&chfield=resolution&chfieldfrom=-2000d&chfieldvalue=FIXED&product=vte&resolution=FIXED).

- iTerm 2: OS X terminal application. [Source](https://github.com/gnachman/iTerm2), [Issues](https://gitlab.com/gnachman/iterm2/issues) and [Documentation](http://www.iterm2.com/documentation.html) (which includes [iTerm2 proprietary escape codes](http://www.iterm2.com/documentation-escape-codes.html)).

- Konsole: KDE terminal application. [Source](https://projects.kde.org/projects/kde/applications/konsole/repository), in particular [tests](https://projects.kde.org/projects/kde/applications/konsole/repository/revisions/master/show/tests), [Bugs](https://bugs.kde.org/buglist.cgi?bug_severity=critical&bug_severity=grave&bug_severity=major&bug_severity=crash&bug_severity=normal&bug_severity=minor&bug_status=UNCONFIRMED&bug_status=NEW&bug_status=ASSIGNED&bug_status=REOPENED&product=konsole) and [Wishes](https://bugs.kde.org/buglist.cgi?bug_severity=wishlist&bug_status=UNCONFIRMED&bug_status=NEW&bug_status=ASSIGNED&bug_status=REOPENED&product=konsole).

- hterm: JavaScript terminal implementation from Chromium. [Source](https://github.com/chromium/hterm), including [tests](https://github.com/chromium/hterm/blob/master/js/hterm_vt_tests.js), and [Google group](https://groups.google.com/a/chromium.org/forum/#!forum/chromium-hterm).

- xterm: The grandfather of terminal emulators. [Source](http://invisible-island.net/datafiles/release/xterm.tar.gz).

- Connectbot: Android SSH client. [Source](https://github.com/connectbot/connectbot)

- Android Terminal Emulator: Android terminal app which Termux terminal handling is based on. Inactive. [Source](https://github.com/jackpal/Android-Terminal-Emulator).
##



## For Devs and Contributors

The [termux-shared](termux-shared) library was added in [`v0.109`](https://github.com/termux/termux-app/releases/tag/v0.109). It defines shared constants and utils of Termux app and its plugins. It was created to allow for removal of all hardcoded paths in Termux app. The termux plugins will hopefully use this in future as well. If you are contributing code that is using a constant or a util that may be shared, then define it in `termux-shared` library if it currently doesn't exist and reference it from there. Update the relevant changelogs as well. Pull requests using hardcoded values **will/should not** be accepted.

The main Termux constants are defined by [`TermuxConstants`](https://github.com/termux/termux-app/blob/master/termux-shared/src/main/java/com/termux/shared/termux/TermuxConstants.java) class. It also contains information on how to fork Termux or build it with your own package name. Changing the package name will require building the bootstrap zip packages and other packages with the new `$PREFIX`, check [Building Packages](https://github.com/termux/termux-packages/wiki/Building-packages) for more info.
