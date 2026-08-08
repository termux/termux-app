---
page_ref: /docs/apps/termux/phantom-processes.html
---

# Phantom Processes

Android 12 introduced a mechanism to monitor forked child processes started by apps and
kills them if more than the default `32` are found running (the limit applies to **all apps
combined**, not per app). The app or its child processes may **also** be killed if they use
excessive CPU, which is done periodically. Check the related issue
[#2366](https://github.com/termux/termux-app/issues/2366), the
[issue tracker](https://issuetracker.google.com/u/1/issues/205156966) and the
[phantom cached and empty processes docs](https://github.com/agnostic-apollo/Android-Docs/blob/master/en/docs/apps/processes/phantom-cached-and-empty-processes.md)
for details.

This affects all commands run in the Termux `shell` and the background tasks started by the
[`RUN_COMMAND Intent`](https://github.com/termux/termux-app/wiki/RUN_COMMAND-Intent) and
plugins like [`termux-boot`](https://github.com/termux/termux-boot),
[`termux-tasker`](https://github.com/termux/termux-tasker) and
[`termux-widget`](https://github.com/termux/termux-widget). Expect processes to be killed at
any time if using Android 12.

## How to detect if a process was killed

If a process was killed by the phantom process or excessive cpu usage trimming, you may get
`[Process completed (signal 9) - press Enter]` message in the terminal without actually
exiting the shell process yourself. You can check the `logcat` for `Killing...` entries with
`Trimming phantom processes` or `excessive cpu` reasons.

```
adb shell "logcat -d | grep -E 'Killing|excessive cpu|Trimming phantom processes'"
```

## How to check the max phantom processes limit

The currently enforced limit can be checked with the following commands. The value shown
should normally be the same as the `max_phantom_processes` value stored in device config,
which defaults to `32` when unset.

- `root`: `su -c "/system/bin/dumpsys activity settings | grep max_phantom_processes"`
- `adb`: `adb shell "/system/bin/dumpsys activity settings | grep max_phantom_processes"`

The value stored in device config can be checked with the following commands. It will be
`null` by default if not set.

- `root`: `su -c "/system/bin/device_config get activity_manager max_phantom_processes"`
- `adb`: `adb shell "/system/bin/device_config get activity_manager max_phantom_processes"`

## How to disable the phantom processes killing

### Android 12

On Android 12, **no setting exists** that can be changed with `adb` or `root` to disable
killing of **processes using excessive cpu**.

However, the killing of **extra phantom processes `> 32`** can be disabled by setting the
`max_phantom_processes` value in the `activity_manager` device config namespace to
`Integer.MAX_VALUE` (`2147483647`). The command must be run with the `shell` (`adb`) or
`root` user.

- `root`: `su -c "/system/bin/device_config put activity_manager max_phantom_processes 2147483647"`
- `adb`: `adb shell "/system/bin/device_config put activity_manager max_phantom_processes 2147483647"`

To revert back to the default value:

- `root`: `su -c "/system/bin/device_config delete activity_manager max_phantom_processes"`
- `adb`: `adb shell "/system/bin/device_config delete activity_manager max_phantom_processes"`

### Android 12L, 13 and higher

The `settings_enable_monitor_phantom_procs` feature flag was added in Android 12L beta 3
(AOSP commit [`09dcdad`](https://cs.android.com/android/_/android/platform/frameworks/base/+/09dcdad5ebc159861920f090e07da60fac71ac0a))
which allows disabling both the killing of **extra phantom processes `> 32`** and of
**processes using excessive cpu**. Availability on other devices will depend on if vendors
merged the commit or not and if they actually want to support the flag.

The flag value can be modified in two ways. The settings global value takes precedence over
the sysprop value. Override precedence: `Settings.Global` -> `sys.fflag.override.*` -> static
list.

1. Sysprop with `setprop` command with **root**. Will be unset by default. Running `setprop`
   requires root and even the adb `shell` user cannot modify the values since selinux will
   not allow it by default.

   - Set value: `setprop persist.sys.fflag.override.settings_enable_monitor_phantom_procs false`
   - Get value: `getprop persist.sys.fflag.override.settings_enable_monitor_phantom_procs`
   - Unset value: `setprop persist.sys.fflag.override.settings_enable_monitor_phantom_procs ""`

2. Settings global list with `adb` or `root`. Will be unset by default.

   - Set value: `adb shell settings put global settings_enable_monitor_phantom_procs false`
   - Get value: `adb shell settings get global settings_enable_monitor_phantom_procs`
   - Unset value: `adb shell settings delete global settings_enable_monitor_phantom_procs`

On Android 14 and higher, the flag can also be changed from `Android Settings` -> `System` ->
`Developer options` -> `Disable child process restrictions` toggle. The flag value will revert
to its default value `true` if `Developer options` are disabled.

An option to disable the killing should be available in Android 12L or 13, so upgrade at your
own risk if you are on Android 11, specially if you are not rooted.
