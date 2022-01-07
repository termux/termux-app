# Termux application

[![Build status](https://github.com/termux/termux-app/workflows/Build/badge.svg)](https://github.com/termux/termux-app/actions)
[![Testing status](https://github.com/termux/termux-app/workflows/Unit%20tests/badge.svg)](https://github.com/termux/termux-app/actions)
[![Join the chat at https://gitter.im/termux/termux](https://badges.gitter.im/termux/termux.svg)](https://gitter.im/termux/termux)
[![Join the Termux discord server](https://img.shields.io/discord/641256914684084234.svg?label=&logo=discord&logoColor=ffffff&color=5865F2)](https://discord.gg/HXpF69X)
[![Termux library releases at Jitpack](https://jitpack.io/v/termux/termux-app.svg)](https://jitpack.io/#termux/termux-app)


[Termux](https://termux.com) is an Android terminal application and Linux environment.

Note that this repository is for the app itself (the user interface and the terminal emulation). For the packages installable inside the app, see [termux/termux-packages](https://github.com/termux/termux-packages).

Quick how-to about Termux package management is available at [Package Management](https://github.com/termux/termux-packages/wiki/Package-Management). It also has info on how to fix **`repository is under maintenance or down`** errors when running `apt` or `pkg` commands.

***

**@termux is looking for Termux Application maintainers for implementing new features, fixing bugs and reviewing pull requests since the current one (@fornwall) is inactive.**

Issue https://github.com/termux/termux-app/issues/1072 needs extra attention.

***

### Contents
- [Termux App and Plugins](#Termux-App-and-Plugins)
- [Installation](#Installation)
- [Uninstallation](#Uninstallation)
- [Important Links](#Important-Links)
- [Debugging](#Debugging)
- [For Maintainers and Contributors](#For-Maintainers-and-Contributors)
- [Forking](#Forking)
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

Latest version is `v0.118.0`.

Termux can be obtained through various sources listed below for **only** Android `>= 7`. Support was dropped for Android `5` and `6` on [2020-01-01](https://www.reddit.com/r/termux/comments/dnzdbs/end_of_android56_support_on_20200101/) at `v0.83`, old builds are available on [archive.org](https://archive.org/details/termux-repositories-legacy).

The APK files of different sources are signed with different signature keys. The `Termux` app and all its plugins use the same [`sharedUserId`](https://developer.android.com/guide/topics/manifest/manifest-element) `com.termux` and so all their APKs installed on a device must have been signed with the same signature key to work together and so they must all be installed from the same source. Do not attempt to mix them together, i.e do not try to install an app or plugin from `F-Droid` and another one from a different source like `Github`. Android Package Manager will also normally not allow installation of APKs with different signatures and you will get errors on installation like `App not installed`, `Failed to install due to an unknown error`, `INSTALL_FAILED_UPDATE_INCOMPATIBLE`, `INSTALL_FAILED_SHARED_USER_INCOMPATIBLE`, `signatures do not match previously installed version`, etc. This restriction can be bypassed with root or with custom roms.

If you wish to install from a different source, then you must **uninstall any and all existing Termux or its plugin app APKs** from your device first, then install all new APKs from the same new source. Check [Uninstallation](#Uninstallation) section for details. You may also want to consider [Backing up Termux](https://wiki.termux.com/wiki/Backing_up_Termux) before the uninstallation so that you can restore it after re-installing from Termux different source.

In the following paragraphs, *"bootstrap"* refers to the minimal packages that are shipped with the `termux-app` itself to start a working shell environment. Its zips are built and released [here](https://github.com/termux/termux-packages/releases).

### F-Droid

Termux application can be obtained from `F-Droid` from [here](https://f-droid.org/en/packages/com.termux/).

You **do not** need to download the `F-Droid` app (via the `Download F-Droid` link) to install Termux. You can download the Termux APK directly from the site by clicking the `Download APK` link at the bottom of each version section.

It usually takes a few days (or even a week or more) for updates to be available on `F-Droid` once an update has been released on `Github`. The `F-Droid` releases are built and published by `F-Droid` once they [detect](https://gitlab.com/fdroid/fdroiddata/-/blob/master/metadata/com.termux.yml) a new `Github` release. The Termux maintainers **do not** have any control over the building and publishing of the Termux apps on `F-Droid`. Moreover, the Termux maintainers also do not have access to the APK signing keys of `F-Droid` releases, so we cannot release an APK ourselves on `Github` that would be compatible with `F-Droid` releases.

The `F-Droid` app often may not notify you of updates and you will manually have to do a pull down swipe action in the `Updates` tab of the app for it to check updates. Make sure battery optimizations are disabled for the app, check https://dontkillmyapp.com/ for details on how to do that.

Only a universal APK is released, which will work on all supported architectures. The APK and bootstrap installation size will be `~180MB`. `F-Droid` does [not support](https://github.com/termux/termux-app/pull/1904) architecture specific APKs.

### Github

Termux application can be obtained on `Github` either from [`Github Releases`](https://github.com/termux/termux-app/releases) for version `>= 0.118.0` or from [`Github Build`](https://github.com/termux/termux-app/actions/workflows/debug_build.yml) action workflows.

The APKs for `Github Releases` will be listed under `Assets` drop-down of a release. These are automatically attached when a new version is released.

The APKs for `Github Build` action workflows will be listed under `Artifacts` section of a workflow run. These are created for each commit/push done to the repository and can be used by users who don't want to wait for releases and want to try out the latest features immediately or want to test their pull requests. Note that for action workflows, you need to be [**logged into a `Github` account**](https://github.com/login) for the `Artifacts` links to be enabled/clickable. If you are using the [`Github` app](https://github.com/mobile), then make sure to open workflow link in a browser like Chrome or Firefox that has your Github account logged in since the in-app browser may not be logged in. 

The APKs for both of these are [`debuggable`](https://developer.android.com/studio/debug) and are compatible with each other but they are not compatible with other sources.

Both universal and architecture specific APKs are released. The APK and bootstrap installation size will be `~180MB` if using universal and `~120MB` if using architecture specific. Check [here](https://github.com/termux/termux-app/issues/2153) for details.

### Google Play Store **(Deprecated)**

**Termux and its plugins are no longer updated on [Google Play Store](https://play.google.com/store/apps/details?id=com.termux) due to [android 10 issues](https://github.com/termux/termux-packages/wiki/Termux-and-Android-10) and have been deprecated.** The last version released for Android `>= 7` was `v0.101`. **It is highly recommended to not install Termux apps from Play Store any more.**

There are plans for **unpublishing** the Termux app and all its plugins on Play Store soon so that new users cannot install it and for **disabling** the Termux apps with updates so that existing users **cannot continue using outdated versions**. You are encouraged to move to `F-Droid` or `Github` builds as soon as possible.

You **will not need to buy plugins again** if you bought them on Play Store. All plugins are free on `F-Droid` and  `Github`.

You can backup all your data under `$HOME/` and `$PREFIX/` before changing installation source, and then restore it afterwards, by following instructions at [Backing up Termux](https://wiki.termux.com/wiki/Backing_up_Termux) before the uninstallation.

There is currently no work being done to solve android `10` issues and *working* updates will not be resumed on Google Play Store any time soon. We will continue targeting sdk `28` for now. So there is not much point in staying on Play Store builds and waiting for updates to be resumed. If for some reason you don't want to move to `F-Droid` or `Github` sources for now, then at least check [Package Management](https://github.com/termux/termux-packages/wiki/Package-Management) to **change your mirror**, otherwise, you will get **`repository is under maintenance or down`** errors when running `apt` or `pkg` commands. After that, it is also **highly advisable** to run `pkg upgrade` command to update all packages to the latest available versions, or at least update `termux-tools` package with `pkg install termux-tools` command. 

Note that by upgrading old packages to latest versions, like that of `python` may break your setups/scripts since they may not be compatible anymore. Moreover, you will not be able to downgrade the package versions since termux repos only keep the latest version and you will have to manually rebuild the old versions of the packages if required as per https://github.com/termux/termux-packages/wiki/Building-packages.

If you plan on staying on Play Store sources in future as well, then you may want to **disable automatic updates in Play Store** for Termux apps, since if and when updates to disable Termux apps are released, then **you will not be able to downgrade** and **will be forced** to move since apps won't work anymore. Only a way to backup `termux-app` data may be provided. The `termux-tools` [version `>= 0.135`](https://github.com/termux/termux-packages/pull/7493) will also show a banner at the top of the terminal saying `You are likely using a very old version of Termux, probably installed from the Google Play Store.`, you can remove it by running `rm -f /data/data/com.termux/files/usr/etc/motd-playstore` and restarting the app.

#### Why Disable?

<details>
<summary></summary>

- They should be disabled because deprecated things get removed and are not supported after some time, its the standard practice. It has been many months now since deprecation was announced and updates have not been released on Play Store since after `29 September 2020`.

- The new versions have lots of **new features and fixes** which you can mostly check out in the Changelog of [`Github Releases`](https://github.com/termux/termux-app/releases) that you may be missing out. Extra detail is usually provided in [commit messages](https://github.com/termux/termux-app/commits/master).

- Users on old versions are quite often reporting issues in multiple repositories and support forums that were **fixed months ago**, which we then have to deal with. The maintainers of @termux work in their free time, majorly for free, to work on development and provide support and having to re-re-deal with old issues takes away the already limited time from current work and is not possible to continue doing. Play Store page of `termux-app` has been filled with bad reviews of *"broken app"*, even though its clearly mentioned on the page that app is not being updated, yet users don't read and still install and report issues.

- Asking people to pay for plugins when the `termux-app` at installation time is broken due to repository issues and has bugs is unethical.

- Old versions don't have proper logging/debugging and crash report support. Reporting bugs without logs or detailed info is not helpful in solving them.

- It's also easier for us to solve package related issues and provide custom functionality with app updates, which can't be done if users continue using old versions. For example, the [bintray shudown](https://github.com/termux/termux-packages/wiki/Package-Management) causing package install/update failures for new Play Store users is/was not an issue for F-Droid users since it is being shipped with updated bootstrap and repo info, hence no reported issues from new F-Droid users.
</details>

##



## Uninstallation

Uninstallation may be required if a user doesn't want Termux installed in their device anymore or is switching to a different [install source](#Installation). You may also want to consider [Backing up Termux](https://wiki.termux.com/wiki/Backing_up_Termux) before the uninstallation.

To uninstall Termux completely, you must uninstall **any and all existing Termux or its plugin app APKs** listed in [Termux App and Plugins](#Termux-App-and-Plugins).

Go to `Android Settings` -> `Applications` and then look for those apps. You can also use the search feature if it’s available on your device and search `termux` in the applications list.

Even if you think you have not installed any of the plugins, it's strongly suggested to go through the application list in Android settings and double-check.
##



## Important Links

### Community
All community links are available [here](https://wiki.termux.com/wiki/Community).

The main ones are the following.

- [Termux Reddit community](https://reddit.com/r/termux)
- [Termux Matrix Channel](https://matrix.to/#termux_termux:gitter.im)
- [Termux Dev Matrix Channel](https://matrix.to/#termux_dev:gitter.im)
- [Termux Twitter](https://twitter.com/termux/)
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
- [Remote Access](https://wiki.termux.com/wiki/Remote_Access)
- [Backing up Termux](https://wiki.termux.com/wiki/Backing_up_Termux)
- [Terminal Settings](https://wiki.termux.com/wiki/Terminal_Settings)
- [Touch Keyboard](https://wiki.termux.com/wiki/Touch_Keyboard)
- [Android Storage and Sharing Data with Other Apps](https://wiki.termux.com/wiki/Internal_and_external_storage)
- [Android APIs](https://wiki.termux.com/wiki/Termux:API)
- [Moved Termux Packages Hosting From Bintray to IPFS](https://github.com/termux/termux-packages/issues/6348)
- [Running Commands in Termux From Other Apps via `RUN_COMMAND` intent](https://github.com/termux/termux-app/wiki/RUN_COMMAND-Intent)
- [Termux and Android 10](https://github.com/termux/termux-packages/wiki/Termux-and-Android-10)


### Terminal

<details>
<summary></summary>

### Terminal resources

- [XTerm control sequences](https://invisible-island.net/xterm/ctlseqs/ctlseqs.html)
- [vt100.net](https://vt100.net/)
- [Terminal codes (ANSI and terminfo equivalents)](https://wiki.bash-hackers.org/scripting/terminalcodes)

### Terminal emulators

- VTE (libvte): Terminal emulator widget for GTK+, mainly used in gnome-terminal. [Source](https://github.com/GNOME/vte), [Open Issues](https://bugzilla.gnome.org/buglist.cgi?quicksearch=product%3A%22vte%22+), and [All (including closed) issues](https://bugzilla.gnome.org/buglist.cgi?bug_status=RESOLVED&bug_status=VERIFIED&chfield=resolution&chfieldfrom=-2000d&chfieldvalue=FIXED&product=vte&resolution=FIXED).

- iTerm 2: OS X terminal application. [Source](https://github.com/gnachman/iTerm2), [Issues](https://gitlab.com/gnachman/iterm2/issues) and [Documentation](https://iterm2.com/documentation.html) (which includes [iTerm2 proprietary escape codes](https://iterm2.com/documentation-escape-codes.html)).

- Konsole: KDE terminal application. [Source](https://projects.kde.org/projects/kde/applications/konsole/repository), in particular [tests](https://projects.kde.org/projects/kde/applications/konsole/repository/revisions/master/show/tests), [Bugs](https://bugs.kde.org/buglist.cgi?bug_severity=critical&bug_severity=grave&bug_severity=major&bug_severity=crash&bug_severity=normal&bug_severity=minor&bug_status=UNCONFIRMED&bug_status=NEW&bug_status=ASSIGNED&bug_status=REOPENED&product=konsole) and [Wishes](https://bugs.kde.org/buglist.cgi?bug_severity=wishlist&bug_status=UNCONFIRMED&bug_status=NEW&bug_status=ASSIGNED&bug_status=REOPENED&product=konsole).

- hterm: JavaScript terminal implementation from Chromium. [Source](https://github.com/chromium/hterm), including [tests](https://github.com/chromium/hterm/blob/master/js/hterm_vt_tests.js), and [Google group](https://groups.google.com/a/chromium.org/forum/#!forum/chromium-hterm).

- xterm: The grandfather of terminal emulators. [Source](https://invisible-island.net/datafiles/release/xterm.tar.gz).

- Connectbot: Android SSH client. [Source](https://github.com/connectbot/connectbot)

- Android Terminal Emulator: Android terminal app which Termux terminal handling is based on. Inactive. [Source](https://github.com/jackpal/Android-Terminal-Emulator).
</details>

##



### Debugging

You can help debug problems of the `Termux` app and its plugins by setting appropriate `logcat` `Log Level` in `Termux` app settings -> `<APP_NAME>` -> `Debugging` -> `Log Level` (Requires `Termux` app version `>= 0.118.0`). The `Log Level` defaults to `Normal` and log level `Verbose` currently logs additional information. Its best to revert log level to `Normal` after you have finished debugging since private data may otherwise be passed to `logcat` during normal operation and moreover, additional logging increases execution time.

The plugin apps **do not execute the commands themselves** but send execution intents to `Termux` app, which has its own log level which can be set in `Termux` app settings -> `Termux` -> `Debugging` -> `Log Level`. So you must set log level for both `Termux` and the respective plugin app settings to get all the info.

Once log levels have been set, you can run the `logcat` command in `Termux` app terminal to view the logs in realtime (`Ctrl+c` to stop) or use `logcat -d > logcat.txt` to take a dump of the log. You can also view the logs from a PC over `ADB`. For more information, check official android `logcat` guide [here](https://developer.android.com/studio/command-line/logcat).

Moreover, users can generate termux files `stat` info and `logcat` dump automatically too with terminal's long hold options menu `More` -> `Report Issue` option and selecting `YES` in the prompt shown to add debug info. This can be helpful for reporting and debugging other issues. If the report generated is too large, then `Save To File` option in context menu (3 dots on top right) of `ReportActivity` can be used and the file viewed/shared instead.

Users must post complete report (optionally without sensitive info) when reporting issues. Issues opened with **(partial) screenshots of error reports** instead of text will likely be automatically closed/deleted.

##### Log Levels

- `Off` - Log nothing.
- `Normal` - Start logging error, warn and info messages and stacktraces.
- `Debug` - Start logging debug messages.
- `Verbose` - Start logging verbose messages.
##



## For Maintainers and Contributors

The [termux-shared](termux-shared) library was added in [`v0.109`](https://github.com/termux/termux-app/releases/tag/v0.109). It defines shared constants and utils of the Termux app and its plugins. It was created to allow for the removal of all hardcoded paths in the Termux app. Some of the termux plugins are using this as well and rest will in future. If you are contributing code that is using a constant or a util that may be shared, then define it in `termux-shared` library if it currently doesn't exist and reference it from there. Update the relevant changelogs as well. Pull requests using hardcoded values **will/should not** be accepted. Termux app and plugin specific classes must be added under `com.termux.shared.termux` package and general classes outside it. The [`termux-shared` `LICENSE`](termux-shared/LICENSE.md) must also be checked and updated if necessary when contributing code. The licenses of any external library or code must be honoured.

The main Termux constants are defined by [`TermuxConstants`](https://github.com/termux/termux-app/blob/master/termux-shared/src/main/java/com/termux/shared/termux/TermuxConstants.java) class. It also contains information on how to fork Termux or build it with your own package name. Changing the package name will require building the bootstrap zip packages and other packages with the new `$PREFIX`, check [Building Packages](https://github.com/termux/termux-packages/wiki/Building-packages) for more info.

Check [Termux Libraries](https://github.com/termux/termux-app/wiki/Termux-Libraries) for how to import termux libraries in plugin apps and [Forking and Local Development](https://github.com/termux/termux-app/wiki/Termux-Libraries#forking-and-local-development) for how to update termux libraries for plugins.

Commit messages **must** use [Conventional Commits](https://www.conventionalcommits.org) specs so that chagelogs can automatically be generated by the [`create-conventional-changelog`](https://github.com/termux/create-conventional-changelog) script, check its repo for further details on the spec. Use the following `types` as `Added: Add foo`, `Added|Fixed: Add foo and fix bar`, `Changed!: Change baz as a breaking change`, etc. You can optionally add a scope as well, like `Fixed(terminal): Some bug`. The space after `:` is necessary.

- **Added** for new features.
- **Changed** for changes in existing functionality.
- **Deprecated** for soon-to-be removed features.
- **Removed** for now removed features.
- **Fixed** for any bug fixes.
- **Security** in case of vulnerabilities.
- **Docs** for updating documentation.

Changelogs for releases are generated based on [Keep a Changelog](https://github.com/olivierlacan/keep-a-changelog) specs.

The `versionName` in `build.gradle` files of Termux and its plugin apps must follow the [semantic version `2.0.0` spec](https://semver.org/spec/v2.0.0.html) in the format `major.minor.patch(-prerelease)(+buildmetadata)`. When bumping `versionName` in `build.gradle` files and when creating a tag for new releases on github, make sure to include the patch number as well, like `v0.1.0` instead of just `v0.1`. The `build.gradle` files and `attach_debug_apks_to_release` workflow validates the version as well and the build/attachment will fail if `versionName` does not follow the spec.
##



## Forking

- Check [`TermuxConstants`](https://github.com/termux/termux-app/blob/master/termux-shared/src/main/java/com/termux/shared/termux/TermuxConstants.java) javadocs for instructions on what changes to make in the app to change package name.
- You also need to recompile bootstrap zip for the new package name. Check [here](https://github.com/termux/termux-app/issues/1983) and [here](https://github.com/termux/termux-app/issues/2081#issuecomment-865280111) for experimental work on it.
- Currently, not all plugins use `TermuxConstants` from `termux-shared` library and have hardcoded `com.termux` values and will need to be manually patched.
- If forking termux plugins, check [Forking and Local Development](https://github.com/termux/termux-app/wiki/Termux-Libraries#forking-and-local-development) for info on how to use termux libraries for plugins.
