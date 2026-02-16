package com.termux.shared.termux;

import android.annotation.SuppressLint;
import android.content.Intent;

import com.termux.shared.shell.command.ExecutionCommand;
import com.termux.shared.shell.command.ExecutionCommand.Runner;

import java.io.File;
import java.util.Arrays;
import java.util.Formatter;
import java.util.List;

/*
 * Version: v0.53.0
 * SPDX-License-Identifier: MIT
 *
 * Changelog
 *
 * - 0.1.0 (2021-03-08)
 *      - Initial Release.
 *
 * - 0.2.0 (2021-03-11)
 *      - Added `_DIR` and `_FILE` substrings to paths.
 *      - Added `INTERNAL_PRIVATE_APP_DATA_DIR*`, `TERMUX_CACHE_DIR*`, `TERMUX_DATABASES_DIR*`,
 *          `TERMUX_SHARED_PREFERENCES_DIR*`, `TERMUX_BIN_PREFIX_DIR*`, `TERMUX_ETC_DIR*`,
 *          `TERMUX_INCLUDE_DIR*`, `TERMUX_LIB_DIR*`, `TERMUX_LIBEXEC_DIR*`, `TERMUX_SHARE_DIR*`,
 *          `TERMUX_TMP_DIR*`, `TERMUX_VAR_DIR*`, `TERMUX_STAGING_PREFIX_DIR*`,
 *          `TERMUX_STORAGE_HOME_DIR*`, `TERMUX_DEFAULT_PREFERENCES_FILE_BASENAME*`,
 *          `TERMUX_DEFAULT_PREFERENCES_FILE`.
 *      - Renamed `DATA_HOME_PATH` to `TERMUX_DATA_HOME_DIR_PATH`.
 *      - Renamed `CONFIG_HOME_PATH` to `TERMUX_CONFIG_HOME_DIR_PATH`.
 *      - Updated javadocs and spacing.
 *
 * - 0.3.0 (2021-03-12)
 *      - Remove `TERMUX_CACHE_DIR_PATH*`, `TERMUX_DATABASES_DIR_PATH*`,
 *          `TERMUX_SHARED_PREFERENCES_DIR_PATH*` since they may not be consistent on all devices.
 *      - Renamed `TERMUX_DEFAULT_PREFERENCES_FILE_BASENAME` to
 *          `TERMUX_DEFAULT_PREFERENCES_FILE_BASENAME_WITHOUT_EXTENSION`. This should be used for
 *           accessing shared preferences between Termux app and its plugins if ever needed by first
 *           getting shared package context with {@link Context.createPackageContext(String,int}).
 *
 * - 0.4.0 (2021-03-16)
 *      - Added `BROADCAST_TERMUX_OPENED`,
 *          `TERMUX_API_DEFAULT_PREFERENCES_FILE_BASENAME_WITHOUT_EXTENSION`
 *          `TERMUX_BOOT_DEFAULT_PREFERENCES_FILE_BASENAME_WITHOUT_EXTENSION`,
 *          `TERMUX_FLOAT_DEFAULT_PREFERENCES_FILE_BASENAME_WITHOUT_EXTENSION`,
 *          `TERMUX_STYLING_DEFAULT_PREFERENCES_FILE_BASENAME_WITHOUT_EXTENSION`,
 *          `TERMUX_TASKER_DEFAULT_PREFERENCES_FILE_BASENAME_WITHOUT_EXTENSION`,
 *          `TERMUX_WIDGET_DEFAULT_PREFERENCES_FILE_BASENAME_WITHOUT_EXTENSION`.
 *
 * - 0.5.0 (2021-03-16)
 *      - Renamed "Termux Plugin app" labels to "Termux:Tasker app".
 *
 * - 0.6.0 (2021-03-16)
 *      - Added `TERMUX_FILE_SHARE_URI_AUTHORITY`.
 *
 * - 0.7.0 (2021-03-17)
 *      - Fixed javadocs.
 *
 * - 0.8.0 (2021-03-18)
 *      - Fixed Intent extra types javadocs.
 *      - Added following to `TERMUX_SERVICE`:
 *          `EXTRA_PENDING_INTENT`, `EXTRA_RESULT_BUNDLE`,
 *          `EXTRA_STDOUT`, `EXTRA_STDERR`, `EXTRA_EXIT_CODE`,
 *          `EXTRA_ERR`, `EXTRA_ERRMSG`.
 *
 * - 0.9.0 (2021-03-18)
 *      - Fixed javadocs.
 *
 * - 0.10.0 (2021-03-19)
 *      - Added following to `TERMUX_SERVICE`:
 *          `EXTRA_SESSION_ACTION`,
 *          `VALUE_EXTRA_SESSION_ACTION_SWITCH_TO_NEW_SESSION_AND_OPEN_ACTIVITY`,
 *          `VALUE_EXTRA_SESSION_ACTION_KEEP_CURRENT_SESSION_AND_OPEN_ACTIVITY`,
 *          `VALUE_EXTRA_SESSION_ACTION_SWITCH_TO_NEW_SESSION_AND_DONT_OPEN_ACTIVITY`
 *          `VALUE_EXTRA_SESSION_ACTION_KEEP_CURRENT_SESSION_AND_DONT_OPEN_ACTIVITY`.
 *      - Added following to `RUN_COMMAND_SERVICE`:
 *          `EXTRA_SESSION_ACTION`.
 *
 * - 0.11.0 (2021-03-24)
 *      - Added following to `TERMUX_SERVICE`:
 *          `EXTRA_COMMAND_LABEL`, `EXTRA_COMMAND_DESCRIPTION`, `EXTRA_COMMAND_HELP`, `EXTRA_PLUGIN_API_HELP`.
 *      - Added following to `RUN_COMMAND_SERVICE`:
 *          `EXTRA_COMMAND_LABEL`, `EXTRA_COMMAND_DESCRIPTION`, `EXTRA_COMMAND_HELP`.
 *      - Updated `RESULT_BUNDLE` related extras with `PLUGIN_RESULT_BUNDLE` prefixes.
 *
 * - 0.12.0 (2021-03-25)
 *      - Added following to `TERMUX_SERVICE`:
 *          `EXTRA_PLUGIN_RESULT_BUNDLE_STDOUT_ORIGINAL_LENGTH`,
 *          `EXTRA_PLUGIN_RESULT_BUNDLE_STDERR_ORIGINAL_LENGTH`.
 *
 * - 0.13.0 (2021-03-25)
 *      - Added following to `RUN_COMMAND_SERVICE`:
 *          `EXTRA_PENDING_INTENT`.
 *
 * - 0.14.0 (2021-03-25)
 *      - Added `FDROID_PACKAGES_BASE_URL`,
 *          `TERMUX_GITHUB_ORGANIZATION_NAME`, `TERMUX_GITHUB_ORGANIZATION_URL`,
 *          `TERMUX_GITHUB_REPO_NAME`, `TERMUX_GITHUB_REPO_URL`, `TERMUX_FDROID_PACKAGE_URL`,
 *          `TERMUX_API_GITHUB_REPO_NAME`,`TERMUX_API_GITHUB_REPO_URL`, `TERMUX_API_FDROID_PACKAGE_URL`,
 *          `TERMUX_BOOT_GITHUB_REPO_NAME`, `TERMUX_BOOT_GITHUB_REPO_URL`, `TERMUX_BOOT_FDROID_PACKAGE_URL`,
 *          `TERMUX_FLOAT_GITHUB_REPO_NAME`, `TERMUX_FLOAT_GITHUB_REPO_URL`, `TERMUX_FLOAT_FDROID_PACKAGE_URL`,
 *          `TERMUX_STYLING_GITHUB_REPO_NAME`, `TERMUX_STYLING_GITHUB_REPO_URL`, `TERMUX_STYLING_FDROID_PACKAGE_URL`,
 *          `TERMUX_TASKER_GITHUB_REPO_NAME`, `TERMUX_TASKER_GITHUB_REPO_URL`, `TERMUX_TASKER_FDROID_PACKAGE_URL`,
 *          `TERMUX_WIDGET_GITHUB_REPO_NAME`, `TERMUX_WIDGET_GITHUB_REPO_URL` `TERMUX_WIDGET_FDROID_PACKAGE_URL`.
 *
 * - 0.15.0 (2021-04-06)
 *      - Fixed some variables that had `PREFIX_` substring missing in their name.
 *      - Added `TERMUX_CRASH_LOG_FILE_PATH`, `TERMUX_CRASH_LOG_BACKUP_FILE_PATH`,
 *          `TERMUX_GITHUB_ISSUES_REPO_URL`, `TERMUX_API_GITHUB_ISSUES_REPO_URL`,
 *          `TERMUX_BOOT_GITHUB_ISSUES_REPO_URL`, `TERMUX_FLOAT_GITHUB_ISSUES_REPO_URL`,
 *          `TERMUX_STYLING_GITHUB_ISSUES_REPO_URL`, `TERMUX_TASKER_GITHUB_ISSUES_REPO_URL`,
 *          `TERMUX_WIDGET_GITHUB_ISSUES_REPO_URL`,
 *          `TERMUX_GITHUB_WIKI_REPO_URL`, `TERMUX_PACKAGES_GITHUB_WIKI_REPO_URL`,
 *          `TERMUX_PACKAGES_GITHUB_REPO_NAME`, `TERMUX_PACKAGES_GITHUB_REPO_URL`, `TERMUX_PACKAGES_GITHUB_ISSUES_REPO_URL`,
 *          `TERMUX_GAME_PACKAGES_GITHUB_REPO_NAME`, `TERMUX_GAME_PACKAGES_GITHUB_REPO_URL`, `TERMUX_GAME_PACKAGES_GITHUB_ISSUES_REPO_URL`,
 *          `TERMUX_SCIENCE_PACKAGES_GITHUB_REPO_NAME`, `TERMUX_SCIENCE_PACKAGES_GITHUB_REPO_URL`, `TERMUX_SCIENCE_PACKAGES_GITHUB_ISSUES_REPO_URL`,
 *          `TERMUX_ROOT_PACKAGES_GITHUB_REPO_NAME`, `TERMUX_ROOT_PACKAGES_GITHUB_REPO_URL`, `TERMUX_ROOT_PACKAGES_GITHUB_ISSUES_REPO_URL`,
 *          `TERMUX_UNSTABLE_PACKAGES_GITHUB_REPO_NAME`, `TERMUX_UNSTABLE_PACKAGES_GITHUB_REPO_URL`, `TERMUX_UNSTABLE_PACKAGES_GITHUB_ISSUES_REPO_URL`,
 *          `TERMUX_X11_PACKAGES_GITHUB_REPO_NAME`, `TERMUX_X11_PACKAGES_GITHUB_REPO_URL`, `TERMUX_X11_PACKAGES_GITHUB_ISSUES_REPO_URL`.
 *      - Added following to `RUN_COMMAND_SERVICE`:
 *          `RUN_COMMAND_API_HELP_URL`.
 *
 * - 0.16.0 (2021-04-06)
 *      - Added `TERMUX_SUPPORT_EMAIL`, `TERMUX_SUPPORT_EMAIL_URL`, `TERMUX_SUPPORT_EMAIL_MAILTO_URL`,
 *          `TERMUX_REDDIT_SUBREDDIT`, `TERMUX_REDDIT_SUBREDDIT_URL`.
 *      - The `TERMUX_SUPPORT_EMAIL_URL` value must be fixed later when email has been set up.
 *
 * - 0.17.0 (2021-04-07)
 *      - Added `TERMUX_APP_NOTIFICATION_CHANNEL_ID`, `TERMUX_APP_NOTIFICATION_CHANNEL_NAME`, `TERMUX_APP_NOTIFICATION_ID`,
 *          `TERMUX_RUN_COMMAND_NOTIFICATION_CHANNEL_ID`, `TERMUX_RUN_COMMAND_NOTIFICATION_CHANNEL_NAME`, `TERMUX_RUN_COMMAND_NOTIFICATION_ID`,
 *          `TERMUX_PLUGIN_COMMAND_ERRORS_NOTIFICATION_CHANNEL_ID`, `TERMUX_PLUGIN_COMMAND_ERRORS_NOTIFICATION_CHANNEL_NAME`,
 *          `TERMUX_CRASH_REPORTS_NOTIFICATION_CHANNEL_ID`, `TERMUX_CRASH_REPORTS_NOTIFICATION_CHANNEL_NAME`.
 *      - Updated javadocs.
 *
 * - 0.18.0 (2021-04-11)
 *      - Updated `TERMUX_SUPPORT_EMAIL_URL` to a valid email.
 *      - Removed `TERMUX_SUPPORT_EMAIL`.
 *
 * - 0.19.0 (2021-04-12)
 *      - Added `TERMUX_ACTIVITY.ACTION_REQUEST_PERMISSIONS`.
 *      - Added `TERMUX_SERVICE.EXTRA_STDIN`.
 *      - Added `RUN_COMMAND_SERVICE.EXTRA_STDIN`.
 *      - Deprecated `TERMUX_ACTIVITY.EXTRA_RELOAD_STYLE`.
 *
 * - 0.20.0 (2021-05-13)
 *      - Added `TERMUX_WIKI`, `TERMUX_WIKI_URL`, `TERMUX_PLUGIN_APP_NAMES_LIST`, `TERMUX_PLUGIN_APP_PACKAGE_NAMES_LIST`.
 *      - Added `TERMUX_SETTINGS_ACTIVITY_NAME`.
 *
 * - 0.21.0 (2021-05-13)
 *      - Added `APK_RELEASE_FDROID`, `APK_RELEASE_FDROID_SIGNING_CERTIFICATE_SHA256_DIGEST`,
 *          `APK_RELEASE_GITHUB_DEBUG_BUILD`, `APK_RELEASE_GITHUB_DEBUG_BUILD_SIGNING_CERTIFICATE_SHA256_DIGEST`,
 *          `APK_RELEASE_GOOGLE_PLAYSTORE`, `APK_RELEASE_GOOGLE_PLAYSTORE_SIGNING_CERTIFICATE_SHA256_DIGEST`.
 *
 * - 0.22.0 (2021-05-13)
 *      - Added `TERMUX_DONATE_URL`.
 *
 * - 0.23.0 (2021-06-12)
 *      - Rename `INTERNAL_PRIVATE_APP_DATA_DIR_PATH` to `TERMUX_INTERNAL_PRIVATE_APP_DATA_DIR_PATH`.
 *
 * - 0.24.0 (2021-06-27)
 *      - Add `COMMA_NORMAL`, `COMMA_ALTERNATIVE`.
 *      - Added following to `TERMUX_APP.TERMUX_SERVICE`:
 *          `EXTRA_RESULT_DIRECTORY`, `EXTRA_RESULT_SINGLE_FILE`, `EXTRA_RESULT_FILE_BASENAME`,
 *          `EXTRA_RESULT_FILE_OUTPUT_FORMAT`, `EXTRA_RESULT_FILE_ERROR_FORMAT`, `EXTRA_RESULT_FILES_SUFFIX`.
 *      - Added following to `TERMUX_APP.RUN_COMMAND_SERVICE`:
 *          `EXTRA_RESULT_DIRECTORY`, `EXTRA_RESULT_SINGLE_FILE`, `EXTRA_RESULT_FILE_BASENAME`,
 *          `EXTRA_RESULT_FILE_OUTPUT_FORMAT`, `EXTRA_RESULT_FILE_ERROR_FORMAT`, `EXTRA_RESULT_FILES_SUFFIX`,
 *          `EXTRA_REPLACE_COMMA_ALTERNATIVE_CHARS_IN_ARGUMENTS`, `EXTRA_COMMA_ALTERNATIVE_CHARS_IN_ARGUMENTS`.
 *      - Added following to `RESULT_SENDER`:
 *           `FORMAT_SUCCESS_STDOUT`, `FORMAT_SUCCESS_STDOUT__EXIT_CODE`, `FORMAT_SUCCESS_STDOUT__STDERR__EXIT_CODE`
 *           `FORMAT_FAILED_ERR__ERRMSG__STDOUT__STDERR__EXIT_CODE`,
 *           `RESULT_FILE_ERR_PREFIX`, `RESULT_FILE_ERRMSG_PREFIX` `RESULT_FILE_STDOUT_PREFIX`,
 *           `RESULT_FILE_STDERR_PREFIX`, `RESULT_FILE_EXIT_CODE_PREFIX`.
 *
 * - 0.25.0 (2021-08-19)
 *      - Added following to `TERMUX_APP.TERMUX_SERVICE`:
 *          `EXTRA_BACKGROUND_CUSTOM_LOG_LEVEL`.
 *      - Added following to `TERMUX_APP.RUN_COMMAND_SERVICE`:
 *          `EXTRA_BACKGROUND_CUSTOM_LOG_LEVEL`.
 *
 * - 0.26.0 (2021-08-25)
 *      - Changed `TERMUX_ACTIVITY.ACTION_FAILSAFE_SESSION` to `TERMUX_ACTIVITY.EXTRA_FAILSAFE_SESSION`.
 *
 * - 0.27.0 (2021-09-02)
 *      - Added `TERMUX_FLOAT_APP_NOTIFICATION_CHANNEL_ID`, `TERMUX_FLOAT_APP_NOTIFICATION_CHANNEL_NAME`,
 *          `TERMUX_FLOAT_APP.TERMUX_FLOAT_SERVICE_NAME`.
 *      - Added following to `TERMUX_FLOAT_APP.TERMUX_FLOAT_SERVICE`:
 *          `ACTION_STOP_SERVICE`, `ACTION_SHOW`, `ACTION_HIDE`.
 *
 * - 0.28.0 (2021-09-02)
 *      - Added `TERMUX_FLOAT_PROPERTIES_PRIMARY_FILE*` and `TERMUX_FLOAT_PROPERTIES_SECONDARY_FILE*`.
 *
 * - 0.29.0 (2021-09-04)
 *      - Added `TERMUX_SHORTCUT_TASKS_SCRIPTS_DIR_BASENAME`, `TERMUX_SHORTCUT_SCRIPT_ICONS_DIR_BASENAME`,
 *          `TERMUX_SHORTCUT_SCRIPT_ICONS_DIR_PATH`, `TERMUX_SHORTCUT_SCRIPT_ICONS_DIR`.
 *      - Added following to `TERMUX_WIDGET.TERMUX_WIDGET_PROVIDER`:
 *          `ACTION_WIDGET_ITEM_CLICKED`, `ACTION_REFRESH_WIDGET`, `EXTRA_FILE_CLICKED`.
 *      - Changed naming convention of `TERMUX_FLOAT_APP.TERMUX_FLOAT_SERVICE.ACTION_*`.
 *      - Fixed wrong path set for `TERMUX_SHORTCUT_SCRIPTS_DIR_PATH`.
 *
 * - 0.30.0 (2021-09-08)
 *      - Changed `APK_RELEASE_GITHUB_DEBUG_BUILD`to `APK_RELEASE_GITHUB` and
 *          `APK_RELEASE_GITHUB_DEBUG_BUILD_SIGNING_CERTIFICATE_SHA256_DIGEST` to
 *          `APK_RELEASE_GITHUB_SIGNING_CERTIFICATE_SHA256_DIGEST`.
 *
 * - 0.31.0 (2021-09-09)
 *      - Added following to `TERMUX_APP.TERMUX_SERVICE`:
 *          `MIN_VALUE_EXTRA_SESSION_ACTION` and `MAX_VALUE_EXTRA_SESSION_ACTION`.
 *
 * - 0.32.0 (2021-09-23)
 *      - Added `TERMUX_API.TERMUX_API_ACTIVITY_NAME`, `TERMUX_TASKER.TERMUX_TASKER_ACTIVITY_NAME`
 *          and `TERMUX_WIDGET.TERMUX_WIDGET_ACTIVITY_NAME`.
 *
 * - 0.33.0 (2021-10-08)
 *      - Added `TERMUX_PROPERTIES_FILE_PATHS_LIST` and `TERMUX_FLOAT_PROPERTIES_FILE_PATHS_LIST`.
 *
 * - 0.34.0 (2021-10-26)
 *      - Move `RESULT_SENDER` to `com.termux.shared.shell.command.ShellCommandConstants`.
 *
 * - 0.35.0 (2022-01-28)
 *      - Add `TERMUX_APP.TERMUX_ACTIVITY.EXTRA_RECREATE_ACTIVITY`.
 *
 * - 0.36.0 (2022-03-10)
 *      - Added `TERMUX_APP.TERMUX_SERVICE.EXTRA_RUNNER` and `TERMUX_APP.RUN_COMMAND_SERVICE.EXTRA_RUNNER`
 *
 * - 0.37.0 (2022-03-15)
 *  - Added `TERMUX_API_APT_*`.
 *
 * - 0.38.0 (2022-03-16)
 *      - Added `TERMUX_APP.TERMUX_ACTIVITY.ACTION_NOTIFY_APP_CRASH`.
 *
 * - 0.39.0 (2022-03-18)
 *      - Added `TERMUX_APP.TERMUX_SERVICE.EXTRA_SESSION_NAME`, `TERMUX_APP.RUN_COMMAND_SERVICE.EXTRA_SESSION_NAME`,
 *          `TERMUX_APP.TERMUX_SERVICE.EXTRA_SESSION_CREATE_MODE` and `TERMUX_APP.RUN_COMMAND_SERVICE.EXTRA_SESSION_CREATE_MODE`.
 *
 * - 0.40.0 (2022-04-17)
 *      - Added `TERMUX_APPS_DIR_PATH` and `TERMUX_APP.APPS_DIR_PATH`.
 *
 * - 0.41.0 (2022-04-17)
 *      - Added `TERMUX_APP.TERMUX_AM_SOCKET_FILE_PATH`.
 *
 * - 0.42.0 (2022-04-29)
 *      - Added `APK_RELEASE_TERMUX_DEVS` and `APK_RELEASE_TERMUX_DEVS_SIGNING_CERTIFICATE_SHA256_DIGEST`.
 *
 * - 0.43.0 (2022-05-29)
 *      - Changed `TERMUX_SUPPORT_EMAIL_URL` to support@termux.dev.
 *
 * - 0.44.0 (2022-05-29)
 *      - Changed `TERMUX_APP.APPS_DIR_PATH` basename from `termux-app` to `com.termux`.
 *
 * - 0.45.0 (2022-06-01)
 *      - Added `TERMUX_APP.BUILD_CONFIG_CLASS_NAME`.
 *
 * - 0.46.0 (2022-06-03)
 *      - Rename `TERMUX_APP.TERMUX_SERVICE.EXTRA_SESSION_NAME` to `*.EXTRA_SHELL_NAME`,
 *          `TERMUX_APP.RUN_COMMAND_SERVICE.EXTRA_SESSION_NAME` to `*.EXTRA_SHELL_NAME`,
 *          `TERMUX_APP.TERMUX_SERVICE.EXTRA_SESSION_CREATE_MODE` to `*.EXTRA_SHELL_CREATE_MODE` and
 *          `TERMUX_APP.RUN_COMMAND_SERVICE.EXTRA_SESSION_CREATE_MODE` to `*.EXTRA_SHELL_CREATE_MODE`.
 *
 * - 0.47.0 (2022-06-04)
 *      - Added `TERMUX_SITE` and `TERMUX_SITE_URL`.
 *      - Changed `TERMUX_DONATE_URL`.
 *
 * - 0.48.0 (2022-06-04)
 *      - Removed `TERMUX_GAME_PACKAGES_GITHUB_*`, `TERMUX_SCIENCE_PACKAGES_GITHUB_*`,
 *          `TERMUX_ROOT_PACKAGES_GITHUB_*`, `TERMUX_UNSTABLE_PACKAGES_GITHUB_*`
 *
 * - 0.49.0 (2022-06-11)
 *      - Added `TERMUX_ENV_PREFIX_ROOT`.
 *
 * - 0.50.0 (2022-06-11)
 *      - Added `TERMUX_CONFIG_PREFIX_DIR_PATH`, `TERMUX_ENV_FILE_PATH` and `TERMUX_ENV_TEMP_FILE_PATH`.
 *
 * - 0.51.0 (2022-06-13)
 *      - Added `TERMUX_APP.FILE_SHARE_RECEIVER_ACTIVITY_CLASS_NAME` and `TERMUX_APP.FILE_VIEW_RECEIVER_ACTIVITY_CLASS_NAME`.
 *
 * - 0.52.0 (2022-06-18)
 *      - Added `TERMUX_PREFIX_DIR_IGNORED_SUB_FILES_PATHS_TO_CONSIDER_AS_EMPTY`.
 *
 * - 0.53.0 (2025-01-12)
 *      - Renamed `TERMUX_API`, `TERMUX_STYLING`, `TERMUX_TASKER`, `TERMUX_WIDGET` classes with `_APP` suffix added.
 *      - Added `TERMUX_*_MAIN_ACTIVITY_NAME` and `TERMUX_*_LAUNCHER_ACTIVITY_NAME` constants to each app class.
 */

/**
 * A class that defines shared constants of the Termux app and its plugins.
 * This class will be hosted by termux-shared lib and should be imported by other termux plugin
 * apps as is instead of copying constants to random classes. The 3rd party apps can also import
 * it for interacting with termux apps. If changes are made to this file, increment the version number
 * and add an entry in the Changelog section above.
 *
 * Termux app default package name is "com.termux" and is used in {@link #TERMUX_PREFIX_DIR_PATH}.
 * The binaries compiled for termux have {@link #TERMUX_PREFIX_DIR_PATH} hardcoded in them but it
 * can be changed during compilation.
 *
 * The {@link #TERMUX_PACKAGE_NAME} must be the same as the applicationId of termux-app build.gradle
 * since its also used by {@link #TERMUX_FILES_DIR_PATH}.
 * If {@link #TERMUX_PACKAGE_NAME} is changed, then binaries, specially used in bootstrap need to be
 * compiled appropriately. Check https://github.com/termux/termux-packages/wiki/Building-packages
 * for more info.
 *
 * Ideally the only places where changes should be required if changing package name are the following:
 * - The {@link #TERMUX_PACKAGE_NAME} in {@link TermuxConstants}.
 * - The "applicationId" in "build.gradle" of termux-app. This is package name that android and app
 *      stores will use and is also the final package name stored in "AndroidManifest.xml".
 * - The "manifestPlaceholders" values for {@link #TERMUX_PACKAGE_NAME} and *_APP_NAME in
 *      "build.gradle" of termux-app.
 * - The "ENTITY" values for {@link #TERMUX_PACKAGE_NAME} and *_APP_NAME in "strings.xml" of
 *      termux-app and of termux-shared.
 * - The "shortcut.xml" and "*_preferences.xml" files of termux-app since dynamic variables don't
 *      work in it.
 * - Optionally the "package" in "AndroidManifest.xml" if modifying project structure of termux-app.
 *      This is package name for java classes project structure and is prefixed if activity and service
 *      names use dot (.) notation. This is currently not advisable since this will break lot of
 *      stuff, including termux-* packages.
 * - Optionally the *_PATH variables in {@link TermuxConstants} containing the string "termux".
 *
 * Check https://developer.android.com/studio/build/application-id for info on "package" in
 * "AndroidManifest.xml" and "applicationId" in "build.gradle".
 *
 * The {@link #TERMUX_PACKAGE_NAME} must be used in source code of Termux app and its plugins instead
 * of hardcoded "com.termux" paths.
 */
public final class TermuxConstants {


    /*
     * Termux organization variables.
     */

    /** Termux GitHub organization name */
    public static final String TERMUX_GITHUB_ORGANIZATION_NAME = "termux"; // Default: "termux"
    /** Termux GitHub organization url */
    public static final String TERMUX_GITHUB_ORGANIZATION_URL = "https://github.com" + "/" + TERMUX_GITHUB_ORGANIZATION_NAME; // Default: "https://github.com/termux"

    /** F-Droid packages base url */
    public static final String FDROID_PACKAGES_BASE_URL = "https://f-droid.org/en/packages"; // Default: "https://f-droid.org/en/packages"





    /*
     * Termux and its plugin app and package names and urls.
     */

    /** Termux app name */
    public static final String TERMUX_APP_NAME = "Termux"; // Default: "Termux"
    /** Termux package name */
    public static final String TERMUX_PACKAGE_NAME = "com.termux"; // Default: "com.termux"
    /** Termux GitHub repo name */
    public static final String TERMUX_GITHUB_REPO_NAME = "termux-app"; // Default: "termux-app"
    /** Termux GitHub repo url */
    public static final String TERMUX_GITHUB_REPO_URL = TERMUX_GITHUB_ORGANIZATION_URL + "/" + TERMUX_GITHUB_REPO_NAME; // Default: "https://github.com/termux/termux-app"
    /** Termux GitHub issues repo url */
    public static final String TERMUX_GITHUB_ISSUES_REPO_URL = TERMUX_GITHUB_REPO_URL + "/issues"; // Default: "https://github.com/termux/termux-app/issues"
    /** Termux F-Droid package url */
    public static final String TERMUX_FDROID_PACKAGE_URL = FDROID_PACKAGES_BASE_URL + "/" + TERMUX_PACKAGE_NAME; // Default: "https://f-droid.org/en/packages/com.termux"


    /** Termux:API app name */
    public static final String TERMUX_API_APP_NAME = "Termux:API"; // Default: "Termux:API"
    /** Termux:API app package name */
    public static final String TERMUX_API_PACKAGE_NAME = TERMUX_PACKAGE_NAME + ".api"; // Default: "com.termux.api"
    /** Termux:API GitHub repo name */
    public static final String TERMUX_API_GITHUB_REPO_NAME = "termux-api"; // Default: "termux-api"
    /** Termux:API GitHub repo url */
    public static final String TERMUX_API_GITHUB_REPO_URL = TERMUX_GITHUB_ORGANIZATION_URL + "/" + TERMUX_API_GITHUB_REPO_NAME; // Default: "https://github.com/termux/termux-api"
    /** Termux:API GitHub issues repo url */
    public static final String TERMUX_API_GITHUB_ISSUES_REPO_URL = TERMUX_API_GITHUB_REPO_URL + "/issues"; // Default: "https://github.com/termux/termux-api/issues"
    /** Termux:API F-Droid package url */
    public static final String TERMUX_API_FDROID_PACKAGE_URL = FDROID_PACKAGES_BASE_URL + "/" + TERMUX_API_PACKAGE_NAME; // Default: "https://f-droid.org/en/packages/com.termux.api"


    /** Termux:Boot app name */
    public static final String TERMUX_BOOT_APP_NAME = "Termux:Boot"; // Default: "Termux:Boot"
    /** Termux:Boot app package name */
    public static final String TERMUX_BOOT_PACKAGE_NAME = TERMUX_PACKAGE_NAME + ".boot"; // Default: "com.termux.boot"
    /** Termux:Boot GitHub repo name */
    public static final String TERMUX_BOOT_GITHUB_REPO_NAME = "termux-boot"; // Default: "termux-boot"
    /** Termux:Boot GitHub repo url */
    public static final String TERMUX_BOOT_GITHUB_REPO_URL = TERMUX_GITHUB_ORGANIZATION_URL + "/" + TERMUX_BOOT_GITHUB_REPO_NAME; // Default: "https://github.com/termux/termux-boot"
    /** Termux:Boot GitHub issues repo url */
    public static final String TERMUX_BOOT_GITHUB_ISSUES_REPO_URL = TERMUX_BOOT_GITHUB_REPO_URL + "/issues"; // Default: "https://github.com/termux/termux-boot/issues"
    /** Termux:Boot F-Droid package url */
    public static final String TERMUX_BOOT_FDROID_PACKAGE_URL = FDROID_PACKAGES_BASE_URL + "/" + TERMUX_BOOT_PACKAGE_NAME; // Default: "https://f-droid.org/en/packages/com.termux.boot"


    /** Termux:Float app name */
    public static final String TERMUX_FLOAT_APP_NAME = "Termux:Float"; // Default: "Termux:Float"
    /** Termux:Float app package name */
    public static final String TERMUX_FLOAT_PACKAGE_NAME = TERMUX_PACKAGE_NAME + ".window"; // Default: "com.termux.window"
    /** Termux:Float GitHub repo name */
    public static final String TERMUX_FLOAT_GITHUB_REPO_NAME = "termux-float"; // Default: "termux-float"
    /** Termux:Float GitHub repo url */
    public static final String TERMUX_FLOAT_GITHUB_REPO_URL = TERMUX_GITHUB_ORGANIZATION_URL + "/" + TERMUX_FLOAT_GITHUB_REPO_NAME; // Default: "https://github.com/termux/termux-float"
    /** Termux:Float GitHub issues repo url */
    public static final String TERMUX_FLOAT_GITHUB_ISSUES_REPO_URL = TERMUX_FLOAT_GITHUB_REPO_URL + "/issues"; // Default: "https://github.com/termux/termux-float/issues"
    /** Termux:Float F-Droid package url */
    public static final String TERMUX_FLOAT_FDROID_PACKAGE_URL = FDROID_PACKAGES_BASE_URL + "/" + TERMUX_FLOAT_PACKAGE_NAME; // Default: "https://f-droid.org/en/packages/com.termux.window"


    /** Termux:Styling app name */
    public static final String TERMUX_STYLING_APP_NAME = "Termux:Styling"; // Default: "Termux:Styling"
    /** Termux:Styling app package name */
    public static final String TERMUX_STYLING_PACKAGE_NAME = TERMUX_PACKAGE_NAME + ".styling"; // Default: "com.termux.styling"
    /** Termux:Styling GitHub repo name */
    public static final String TERMUX_STYLING_GITHUB_REPO_NAME = "termux-styling"; // Default: "termux-styling"
    /** Termux:Styling GitHub repo url */
    public static final String TERMUX_STYLING_GITHUB_REPO_URL = TERMUX_GITHUB_ORGANIZATION_URL + "/" + TERMUX_STYLING_GITHUB_REPO_NAME; // Default: "https://github.com/termux/termux-styling"
    /** Termux:Styling GitHub issues repo url */
    public static final String TERMUX_STYLING_GITHUB_ISSUES_REPO_URL = TERMUX_STYLING_GITHUB_REPO_URL + "/issues"; // Default: "https://github.com/termux/termux-styling/issues"
    /** Termux:Styling F-Droid package url */
    public static final String TERMUX_STYLING_FDROID_PACKAGE_URL = FDROID_PACKAGES_BASE_URL + "/" + TERMUX_STYLING_PACKAGE_NAME; // Default: "https://f-droid.org/en/packages/com.termux.styling"


    /** Termux:Tasker app name */
    public static final String TERMUX_TASKER_APP_NAME = "Termux:Tasker"; // Default: "Termux:Tasker"
    /** Termux:Tasker app package name */
    public static final String TERMUX_TASKER_PACKAGE_NAME = TERMUX_PACKAGE_NAME + ".tasker"; // Default: "com.termux.tasker"
    /** Termux:Tasker GitHub repo name */
    public static final String TERMUX_TASKER_GITHUB_REPO_NAME = "termux-tasker"; // Default: "termux-tasker"
    /** Termux:Tasker GitHub repo url */
    public static final String TERMUX_TASKER_GITHUB_REPO_URL = TERMUX_GITHUB_ORGANIZATION_URL + "/" + TERMUX_TASKER_GITHUB_REPO_NAME; // Default: "https://github.com/termux/termux-tasker"
    /** Termux:Tasker GitHub issues repo url */
    public static final String TERMUX_TASKER_GITHUB_ISSUES_REPO_URL = TERMUX_TASKER_GITHUB_REPO_URL + "/issues"; // Default: "https://github.com/termux/termux-tasker/issues"
    /** Termux:Tasker F-Droid package url */
    public static final String TERMUX_TASKER_FDROID_PACKAGE_URL = FDROID_PACKAGES_BASE_URL + "/" + TERMUX_TASKER_PACKAGE_NAME; // Default: "https://f-droid.org/en/packages/com.termux.tasker"


    /** Termux:Widget app name */
    public static final String TERMUX_WIDGET_APP_NAME = "Termux:Widget"; // Default: "Termux:Widget"
    /** Termux:Widget app package name */
    public static final String TERMUX_WIDGET_PACKAGE_NAME = TERMUX_PACKAGE_NAME + ".widget"; // Default: "com.termux.widget"
    /** Termux:Widget GitHub repo name */
    public static final String TERMUX_WIDGET_GITHUB_REPO_NAME = "termux-widget"; // Default: "termux-widget"
    /** Termux:Widget GitHub repo url */
    public static final String TERMUX_WIDGET_GITHUB_REPO_URL = TERMUX_GITHUB_ORGANIZATION_URL + "/" + TERMUX_WIDGET_GITHUB_REPO_NAME; // Default: "https://github.com/termux/termux-widget"
    /** Termux:Widget GitHub issues repo url */
    public static final String TERMUX_WIDGET_GITHUB_ISSUES_REPO_URL = TERMUX_WIDGET_GITHUB_REPO_URL + "/issues"; // Default: "https://github.com/termux/termux-widget/issues"
    /** Termux:Widget F-Droid package url */
    public static final String TERMUX_WIDGET_FDROID_PACKAGE_URL = FDROID_PACKAGES_BASE_URL + "/" + TERMUX_WIDGET_PACKAGE_NAME; // Default: "https://f-droid.org/en/packages/com.termux.widget"





    /*
     * Termux plugin apps lists.
     */

    public static final List<String> TERMUX_PLUGIN_APP_NAMES_LIST = Arrays.asList(
        TERMUX_API_APP_NAME,
        TERMUX_BOOT_APP_NAME,
        TERMUX_FLOAT_APP_NAME,
        TERMUX_STYLING_APP_NAME,
        TERMUX_TASKER_APP_NAME,
        TERMUX_WIDGET_APP_NAME);

    public static final List<String> TERMUX_PLUGIN_APP_PACKAGE_NAMES_LIST = Arrays.asList(
        TERMUX_API_PACKAGE_NAME,
        TERMUX_BOOT_PACKAGE_NAME,
        TERMUX_FLOAT_PACKAGE_NAME,
        TERMUX_STYLING_PACKAGE_NAME,
        TERMUX_TASKER_PACKAGE_NAME,
        TERMUX_WIDGET_PACKAGE_NAME);





    /*
     * Termux APK releases.
     */

    /** F-Droid APK release */
    public static final String APK_RELEASE_FDROID = "F-Droid"; // Default: "F-Droid"

    /** F-Droid APK release signing certificate SHA-256 digest */
    public static final String APK_RELEASE_FDROID_SIGNING_CERTIFICATE_SHA256_DIGEST = "228FB2CFE90831C1499EC3CCAF61E96E8E1CE70766B9474672CE427334D41C42"; // Default: "228FB2CFE90831C1499EC3CCAF61E96E8E1CE70766B9474672CE427334D41C42"

    /** GitHub APK release */
    public static final String APK_RELEASE_GITHUB = "Github"; // Default: "Github"

    /** GitHub APK release signing certificate SHA-256 digest */
    public static final String APK_RELEASE_GITHUB_SIGNING_CERTIFICATE_SHA256_DIGEST = "B6DA01480EEFD5FBF2CD3771B8D1021EC791304BDD6C4BF41D3FAABAD48EE5E1"; // Default: "B6DA01480EEFD5FBF2CD3771B8D1021EC791304BDD6C4BF41D3FAABAD48EE5E1"

    /** Google Play Store APK release */
    public static final String APK_RELEASE_GOOGLE_PLAYSTORE = "Google Play Store"; // Default: "Google Play Store"

    /** Google Play Store APK release signing certificate SHA-256 digest */
    public static final String APK_RELEASE_GOOGLE_PLAYSTORE_SIGNING_CERTIFICATE_SHA256_DIGEST = "738F0A30A04D3C8A1BE304AF18D0779BCF3EA88FB60808F657A3521861C2EBF9"; // Default: "738F0A30A04D3C8A1BE304AF18D0779BCF3EA88FB60808F657A3521861C2EBF9"

    /** Termux Devs APK release */
    public static final String APK_RELEASE_TERMUX_DEVS = "Termux Devs"; // Default: "Termux Devs"

    /** Termux Devs APK release signing certificate SHA-256 digest */
    public static final String APK_RELEASE_TERMUX_DEVS_SIGNING_CERTIFICATE_SHA256_DIGEST = "F7A038EB551F1BE8FDF388686B784ABAB4552A5D82DF423E3D8F1B5CBE1C69AE"; // Default: "F7A038EB551F1BE8FDF388686B784ABAB4552A5D82DF423E3D8F1B5CBE1C69AE"





    /*
     * Termux packages urls.
     */

    /** Termux Packages GitHub repo name */
    public static final String TERMUX_PACKAGES_GITHUB_REPO_NAME = "termux-packages"; // Default: "termux-packages"
    /** Termux Packages GitHub repo url */
    public static final String TERMUX_PACKAGES_GITHUB_REPO_URL = TERMUX_GITHUB_ORGANIZATION_URL + "/" + TERMUX_PACKAGES_GITHUB_REPO_NAME; // Default: "https://github.com/termux/termux-packages"
    /** Termux Packages GitHub issues repo url */
    public static final String TERMUX_PACKAGES_GITHUB_ISSUES_REPO_URL = TERMUX_PACKAGES_GITHUB_REPO_URL + "/issues"; // Default: "https://github.com/termux/termux-packages/issues"


    /** Termux API apt package name */
    public static final String TERMUX_API_APT_PACKAGE_NAME = "termux-api"; // Default: "termux-api"
    /** Termux API apt GitHub repo name */
    public static final String TERMUX_API_APT_GITHUB_REPO_NAME = "termux-api-package"; // Default: "termux-api-package"
    /** Termux API apt GitHub repo url */
    public static final String TERMUX_API_APT_GITHUB_REPO_URL = TERMUX_GITHUB_ORGANIZATION_URL + "/" + TERMUX_API_APT_GITHUB_REPO_NAME; // Default: "https://github.com/termux/termux-api-package"
    /** Termux API apt GitHub issues repo url */
    public static final String TERMUX_API_APT_GITHUB_ISSUES_REPO_URL = TERMUX_API_APT_GITHUB_REPO_URL + "/issues"; // Default: "https://github.com/termux/termux-api-package/issues"





    /*
     * Termux miscellaneous urls.
     */

    /** Termux Site */
    public static final String TERMUX_SITE = TERMUX_APP_NAME + " Site"; // Default: "Termux Site"

    /** Termux Site url */
    public static final String TERMUX_SITE_URL = "https://termux.dev"; // Default: "https://termux.dev"

    /** Termux Wiki */
    public static final String TERMUX_WIKI = TERMUX_APP_NAME + " Wiki"; // Default: "Termux Wiki"

    /** Termux Wiki url */
    public static final String TERMUX_WIKI_URL = "https://wiki.termux.com"; // Default: "https://wiki.termux.com"

    /** Termux GitHub wiki repo url */
    public static final String TERMUX_GITHUB_WIKI_REPO_URL = TERMUX_GITHUB_REPO_URL + "/wiki"; // Default: "https://github.com/termux/termux-app/wiki"

    /** Termux Packages wiki repo url */
    public static final String TERMUX_PACKAGES_GITHUB_WIKI_REPO_URL = TERMUX_PACKAGES_GITHUB_REPO_URL + "/wiki"; // Default: "https://github.com/termux/termux-packages/wiki"


    /** Termux support email url */
    public static final String TERMUX_SUPPORT_EMAIL_URL = "support@termux.dev"; // Default: "support@termux.dev"

    /** Termux support email mailto url */
    public static final String TERMUX_SUPPORT_EMAIL_MAILTO_URL = "mailto:" + TERMUX_SUPPORT_EMAIL_URL; // Default: "mailto:support@termux.dev"


    /** Termux Reddit subreddit */
    public static final String TERMUX_REDDIT_SUBREDDIT = "r/termux"; // Default: "r/termux"

    /** Termux Reddit subreddit url */
    public static final String TERMUX_REDDIT_SUBREDDIT_URL = "https://www.reddit.com/r/termux"; // Default: "https://www.reddit.com/r/termux"


    /** Termux donate url */
    public static final String TERMUX_DONATE_URL = TERMUX_SITE_URL + "/donate"; // Default: "https://termux.dev/donate"





    /*
     * Termux app core directory paths.
     */

    /** Termux app internal private app data directory path */
    @SuppressLint("SdCardPath")
    public static final String TERMUX_INTERNAL_PRIVATE_APP_DATA_DIR_PATH = "/data/data/" + TERMUX_PACKAGE_NAME; // Default: "/data/data/com.termux"
    /** Termux app internal private app data directory */
    public static final File TERMUX_INTERNAL_PRIVATE_APP_DATA_DIR = new File(TERMUX_INTERNAL_PRIVATE_APP_DATA_DIR_PATH);



    /** Termux app Files directory path */
    public static final String TERMUX_FILES_DIR_PATH = TERMUX_INTERNAL_PRIVATE_APP_DATA_DIR_PATH + "/files"; // Default: "/data/data/com.termux/files"
    /** Termux app Files directory */
    public static final File TERMUX_FILES_DIR = new File(TERMUX_FILES_DIR_PATH);



    /** Termux app $PREFIX directory path */
    public static final String TERMUX_PREFIX_DIR_PATH = TERMUX_FILES_DIR_PATH + "/usr"; // Default: "/data/data/com.termux/files/usr"
    /** Termux app $PREFIX directory */
    public static final File TERMUX_PREFIX_DIR = new File(TERMUX_PREFIX_DIR_PATH);


    /** Termux app $PREFIX/bin directory path */
    public static final String TERMUX_BIN_PREFIX_DIR_PATH = TERMUX_PREFIX_DIR_PATH + "/bin"; // Default: "/data/data/com.termux/files/usr/bin"
    /** Termux app $PREFIX/bin directory */
    public static final File TERMUX_BIN_PREFIX_DIR = new File(TERMUX_BIN_PREFIX_DIR_PATH);


    /** Termux app $PREFIX/etc directory path */
    public static final String TERMUX_ETC_PREFIX_DIR_PATH = TERMUX_PREFIX_DIR_PATH + "/etc"; // Default: "/data/data/com.termux/files/usr/etc"
    /** Termux app $PREFIX/etc directory */
    public static final File TERMUX_ETC_PREFIX_DIR = new File(TERMUX_ETC_PREFIX_DIR_PATH);


    /** Termux app $PREFIX/include directory path */
    public static final String TERMUX_INCLUDE_PREFIX_DIR_PATH = TERMUX_PREFIX_DIR_PATH + "/include"; // Default: "/data/data/com.termux/files/usr/include"
    /** Termux app $PREFIX/include directory */
    public static final File TERMUX_INCLUDE_PREFIX_DIR = new File(TERMUX_INCLUDE_PREFIX_DIR_PATH);


    /** Termux app $PREFIX/lib directory path */
    public static final String TERMUX_LIB_PREFIX_DIR_PATH = TERMUX_PREFIX_DIR_PATH + "/lib"; // Default: "/data/data/com.termux/files/usr/lib"
    /** Termux app $PREFIX/lib directory */
    public static final File TERMUX_LIB_PREFIX_DIR = new File(TERMUX_LIB_PREFIX_DIR_PATH);


    /** Termux app $PREFIX/libexec directory path */
    public static final String TERMUX_LIBEXEC_PREFIX_DIR_PATH = TERMUX_PREFIX_DIR_PATH + "/libexec"; // Default: "/data/data/com.termux/files/usr/libexec"
    /** Termux app $PREFIX/libexec directory */
    public static final File TERMUX_LIBEXEC_PREFIX_DIR = new File(TERMUX_LIBEXEC_PREFIX_DIR_PATH);


    /** Termux app $PREFIX/share directory path */
    public static final String TERMUX_SHARE_PREFIX_DIR_PATH = TERMUX_PREFIX_DIR_PATH + "/share"; // Default: "/data/data/com.termux/files/usr/share"
    /** Termux app $PREFIX/share directory */
    public static final File TERMUX_SHARE_PREFIX_DIR = new File(TERMUX_SHARE_PREFIX_DIR_PATH);


    /** Termux app $PREFIX/tmp and $TMPDIR directory path */
    public static final String TERMUX_TMP_PREFIX_DIR_PATH = TERMUX_PREFIX_DIR_PATH + "/tmp"; // Default: "/data/data/com.termux/files/usr/tmp"
    /** Termux app $PREFIX/tmp and $TMPDIR directory */
    public static final File TERMUX_TMP_PREFIX_DIR = new File(TERMUX_TMP_PREFIX_DIR_PATH);


    /** Termux app $PREFIX/var directory path */
    public static final String TERMUX_VAR_PREFIX_DIR_PATH = TERMUX_PREFIX_DIR_PATH + "/var"; // Default: "/data/data/com.termux/files/usr/var"
    /** Termux app $PREFIX/var directory */
    public static final File TERMUX_VAR_PREFIX_DIR = new File(TERMUX_VAR_PREFIX_DIR_PATH);



    /** Termux app usr-staging directory path */
    public static final String TERMUX_STAGING_PREFIX_DIR_PATH = TERMUX_FILES_DIR_PATH + "/usr-staging"; // Default: "/data/data/com.termux/files/usr-staging"
    /** Termux app usr-staging directory */
    public static final File TERMUX_STAGING_PREFIX_DIR = new File(TERMUX_STAGING_PREFIX_DIR_PATH);



    /** Termux app $HOME directory path */
    public static final String TERMUX_HOME_DIR_PATH = TERMUX_FILES_DIR_PATH + "/home"; // Default: "/data/data/com.termux/files/home"
    /** Termux app $HOME directory */
    public static final File TERMUX_HOME_DIR = new File(TERMUX_HOME_DIR_PATH);


    /** Termux app config home directory path */
    public static final String TERMUX_CONFIG_HOME_DIR_PATH = TERMUX_HOME_DIR_PATH + "/.config/termux"; // Default: "/data/data/com.termux/files/home/.config/termux"
    /** Termux app config home directory */
    public static final File TERMUX_CONFIG_HOME_DIR = new File(TERMUX_CONFIG_HOME_DIR_PATH);

    /** Termux app config $PREFIX directory path */
    public static final String TERMUX_CONFIG_PREFIX_DIR_PATH = TERMUX_ETC_PREFIX_DIR_PATH + "/termux"; // Default: "/data/data/com.termux/files/usr/etc/termux"
    /** Termux app config $PREFIX directory */
    public static final File TERMUX_CONFIG_PREFIX_DIR = new File(TERMUX_CONFIG_PREFIX_DIR_PATH);


    /** Termux app data home directory path */
    public static final String TERMUX_DATA_HOME_DIR_PATH = TERMUX_HOME_DIR_PATH + "/.termux"; // Default: "/data/data/com.termux/files/home/.termux"
    /** Termux app data home directory */
    public static final File TERMUX_DATA_HOME_DIR = new File(TERMUX_DATA_HOME_DIR_PATH);


    /** Termux app storage home directory path */
    public static final String TERMUX_STORAGE_HOME_DIR_PATH = TERMUX_HOME_DIR_PATH + "/storage"; // Default: "/data/data/com.termux/files/home/storage"
    /** Termux app storage home directory */
    public static final File TERMUX_STORAGE_HOME_DIR = new File(TERMUX_STORAGE_HOME_DIR_PATH);



    /** Termux and plugin apps directory path */
    public static final String TERMUX_APPS_DIR_PATH = TERMUX_FILES_DIR_PATH + "/apps"; // Default: "/data/data/com.termux/files/apps"
    /** Termux and plugin apps directory */
    public static final File TERMUX_APPS_DIR = new File(TERMUX_APPS_DIR_PATH);


    /** Termux app $PREFIX directory path ignored sub file paths to consider it empty */
    public static final List<String> TERMUX_PREFIX_DIR_IGNORED_SUB_FILES_PATHS_TO_CONSIDER_AS_EMPTY = Arrays.asList(
        TermuxConstants.TERMUX_TMP_PREFIX_DIR_PATH, TermuxConstants.TERMUX_ENV_TEMP_FILE_PATH, TermuxConstants.TERMUX_ENV_FILE_PATH);



    /*
     * Termux app and plugin preferences and properties file paths.
     */

    /** Termux app default SharedPreferences file basename without extension */
    public static final String TERMUX_DEFAULT_PREFERENCES_FILE_BASENAME_WITHOUT_EXTENSION = TERMUX_PACKAGE_NAME + "_preferences"; // Default: "com.termux_preferences"

    /** Termux:API app default SharedPreferences file basename without extension */
    public static final String TERMUX_API_DEFAULT_PREFERENCES_FILE_BASENAME_WITHOUT_EXTENSION = TERMUX_API_PACKAGE_NAME + "_preferences"; // Default: "com.termux.api_preferences"

    /** Termux:Boot app default SharedPreferences file basename without extension */
    public static final String TERMUX_BOOT_DEFAULT_PREFERENCES_FILE_BASENAME_WITHOUT_EXTENSION = TERMUX_BOOT_PACKAGE_NAME + "_preferences"; // Default: "com.termux.boot_preferences"

    /** Termux:Float app default SharedPreferences file basename without extension */
    public static final String TERMUX_FLOAT_DEFAULT_PREFERENCES_FILE_BASENAME_WITHOUT_EXTENSION = TERMUX_FLOAT_PACKAGE_NAME + "_preferences"; // Default: "com.termux.window_preferences"

    /** Termux:Styling app default SharedPreferences file basename without extension */
    public static final String TERMUX_STYLING_DEFAULT_PREFERENCES_FILE_BASENAME_WITHOUT_EXTENSION = TERMUX_STYLING_PACKAGE_NAME + "_preferences"; // Default: "com.termux.styling_preferences"

    /** Termux:Tasker app default SharedPreferences file basename without extension */
    public static final String TERMUX_TASKER_DEFAULT_PREFERENCES_FILE_BASENAME_WITHOUT_EXTENSION = TERMUX_TASKER_PACKAGE_NAME + "_preferences"; // Default: "com.termux.tasker_preferences"

    /** Termux:Widget app default SharedPreferences file basename without extension */
    public static final String TERMUX_WIDGET_DEFAULT_PREFERENCES_FILE_BASENAME_WITHOUT_EXTENSION = TERMUX_WIDGET_PACKAGE_NAME + "_preferences"; // Default: "com.termux.widget_preferences"



    /** Termux app properties primary file path */
    public static final String TERMUX_PROPERTIES_PRIMARY_FILE_PATH = TERMUX_DATA_HOME_DIR_PATH + "/termux.properties"; // Default: "/data/data/com.termux/files/home/.termux/termux.properties"
    /** Termux app properties primary file */
    public static final File TERMUX_PROPERTIES_PRIMARY_FILE = new File(TERMUX_PROPERTIES_PRIMARY_FILE_PATH);

    /** Termux app properties secondary file path */
    public static final String TERMUX_PROPERTIES_SECONDARY_FILE_PATH = TERMUX_CONFIG_HOME_DIR_PATH + "/termux.properties"; // Default: "/data/data/com.termux/files/home/.config/termux/termux.properties"
    /** Termux app properties secondary file */
    public static final File TERMUX_PROPERTIES_SECONDARY_FILE = new File(TERMUX_PROPERTIES_SECONDARY_FILE_PATH);

    /** Termux app properties file paths list. **DO NOT** allow these files to be modified by
     * {@link android.content.ContentProvider} exposed to external apps, since they may silently
     * modify the values for security properties like {@link #PROP_ALLOW_EXTERNAL_APPS} set by users
     * without their explicit consent. */
    public static final List<String> TERMUX_PROPERTIES_FILE_PATHS_LIST = Arrays.asList(
        TERMUX_PROPERTIES_PRIMARY_FILE_PATH,
        TERMUX_PROPERTIES_SECONDARY_FILE_PATH);



    /** Termux:Float app properties primary file path */
    public static final String TERMUX_FLOAT_PROPERTIES_PRIMARY_FILE_PATH = TERMUX_DATA_HOME_DIR_PATH + "/termux.float.properties"; // Default: "/data/data/com.termux/files/home/.termux/termux.float.properties"
    /** Termux:Float app properties primary file */
    public static final File TERMUX_FLOAT_PROPERTIES_PRIMARY_FILE = new File(TERMUX_FLOAT_PROPERTIES_PRIMARY_FILE_PATH);

    /** Termux:Float app properties secondary file path */
    public static final String TERMUX_FLOAT_PROPERTIES_SECONDARY_FILE_PATH = TERMUX_CONFIG_HOME_DIR_PATH + "/termux.float.properties"; // Default: "/data/data/com.termux/files/home/.config/termux/termux.float.properties"
    /** Termux:Float app properties secondary file */
    public static final File TERMUX_FLOAT_PROPERTIES_SECONDARY_FILE = new File(TERMUX_FLOAT_PROPERTIES_SECONDARY_FILE_PATH);

    /** Termux:Float app properties file paths list. **DO NOT** allow these files to be modified by
     * {@link android.content.ContentProvider} exposed to external apps, since they may silently
     * modify the values for security properties like {@link #PROP_ALLOW_EXTERNAL_APPS} set by users
     * without their explicit consent. */
    public static final List<String> TERMUX_FLOAT_PROPERTIES_FILE_PATHS_LIST = Arrays.asList(
        TERMUX_FLOAT_PROPERTIES_PRIMARY_FILE_PATH,
        TERMUX_FLOAT_PROPERTIES_SECONDARY_FILE_PATH);



    /** Termux app and Termux:Styling colors.properties file path */
    public static final String TERMUX_COLOR_PROPERTIES_FILE_PATH = TERMUX_DATA_HOME_DIR_PATH + "/colors.properties"; // Default: "/data/data/com.termux/files/home/.termux/colors.properties"
    /** Termux app and Termux:Styling colors.properties file */
    public static final File TERMUX_COLOR_PROPERTIES_FILE = new File(TERMUX_COLOR_PROPERTIES_FILE_PATH);

    /** Termux app and Termux:Styling font.ttf file path */
    public static final String TERMUX_FONT_FILE_PATH = TERMUX_DATA_HOME_DIR_PATH + "/font.ttf"; // Default: "/data/data/com.termux/files/home/.termux/font.ttf"
    /** Termux app and Termux:Styling font.ttf file */
    public static final File TERMUX_FONT_FILE = new File(TERMUX_FONT_FILE_PATH);


    /** Termux app and plugins crash log file path */
    public static final String TERMUX_CRASH_LOG_FILE_PATH = TERMUX_HOME_DIR_PATH + "/crash_log.md"; // Default: "/data/data/com.termux/files/home/crash_log.md"

    /** Termux app and plugins crash log backup file path */
    public static final String TERMUX_CRASH_LOG_BACKUP_FILE_PATH = TERMUX_HOME_DIR_PATH + "/crash_log_backup.md"; // Default: "/data/data/com.termux/files/home/crash_log_backup.md"


    /** Termux app environment file path */
    public static final String TERMUX_ENV_FILE_PATH = TERMUX_CONFIG_PREFIX_DIR_PATH + "/termux.env"; // Default: "/data/data/com.termux/files/usr/etc/termux/termux.env"

    /** Termux app environment temp file path */
    public static final String TERMUX_ENV_TEMP_FILE_PATH = TERMUX_CONFIG_PREFIX_DIR_PATH + "/termux.env.tmp"; // Default: "/data/data/com.termux/files/usr/etc/termux/termux.env.tmp"




    /*
     * Termux app plugin specific paths.
     */

    /** Termux app directory path to store scripts to be run at boot by Termux:Boot */
    public static final String TERMUX_BOOT_SCRIPTS_DIR_PATH = TERMUX_DATA_HOME_DIR_PATH + "/boot"; // Default: "/data/data/com.termux/files/home/.termux/boot"
    /** Termux app directory to store scripts to be run at boot by Termux:Boot */
    public static final File TERMUX_BOOT_SCRIPTS_DIR = new File(TERMUX_BOOT_SCRIPTS_DIR_PATH);


    /** Termux app directory path to store foreground scripts that can be run by the termux launcher
     * widget provided by Termux:Widget */
    public static final String TERMUX_SHORTCUT_SCRIPTS_DIR_PATH = TERMUX_HOME_DIR_PATH + "/.shortcuts"; // Default: "/data/data/com.termux/files/home/.shortcuts"
    /** Termux app directory to store foreground scripts that can be run by the termux launcher widget provided by Termux:Widget */
    public static final File TERMUX_SHORTCUT_SCRIPTS_DIR = new File(TERMUX_SHORTCUT_SCRIPTS_DIR_PATH);


    /** Termux app directory basename that stores background scripts that can be run by the termux
     * launcher widget provided by Termux:Widget */
    public static final String TERMUX_SHORTCUT_TASKS_SCRIPTS_DIR_BASENAME =  "tasks"; // Default: "tasks"
    /** Termux app directory path to store background scripts that can be run by the termux launcher
     * widget provided by Termux:Widget */
    public static final String TERMUX_SHORTCUT_TASKS_SCRIPTS_DIR_PATH = TERMUX_SHORTCUT_SCRIPTS_DIR_PATH + "/" + TERMUX_SHORTCUT_TASKS_SCRIPTS_DIR_BASENAME; // Default: "/data/data/com.termux/files/home/.shortcuts/tasks"
    /** Termux app directory to store background scripts that can be run by the termux launcher widget provided by Termux:Widget */
    public static final File TERMUX_SHORTCUT_TASKS_SCRIPTS_DIR = new File(TERMUX_SHORTCUT_TASKS_SCRIPTS_DIR_PATH);


    /** Termux app directory basename that stores icons for the foreground and background scripts
     * that can be run by the termux launcher widget provided by Termux:Widget */
    public static final String TERMUX_SHORTCUT_SCRIPT_ICONS_DIR_BASENAME =  "icons"; // Default: "icons"
    /** Termux app directory path to store icons for the foreground and background scripts that can
     * be run by the termux launcher widget provided by Termux:Widget */
    public static final String TERMUX_SHORTCUT_SCRIPT_ICONS_DIR_PATH = TERMUX_SHORTCUT_SCRIPTS_DIR_PATH + "/" + TERMUX_SHORTCUT_SCRIPT_ICONS_DIR_BASENAME; // Default: "/data/data/com.termux/files/home/.shortcuts/icons"
    /** Termux app directory to store icons for the foreground and background scripts that can be
     * run by the termux launcher widget provided by Termux:Widget */
    public static final File TERMUX_SHORTCUT_SCRIPT_ICONS_DIR = new File(TERMUX_SHORTCUT_SCRIPT_ICONS_DIR_PATH);


    /** Termux app directory path to store scripts to be run by 3rd party twofortyfouram locale plugin
     * host apps like Tasker app via the Termux:Tasker plugin client */
    public static final String TERMUX_TASKER_SCRIPTS_DIR_PATH = TERMUX_DATA_HOME_DIR_PATH + "/tasker"; // Default: "/data/data/com.termux/files/home/.termux/tasker"
    /** Termux app directory to store scripts to be run by 3rd party twofortyfouram locale plugin host apps like Tasker app via the Termux:Tasker plugin client */
    public static final File TERMUX_TASKER_SCRIPTS_DIR = new File(TERMUX_TASKER_SCRIPTS_DIR_PATH);





    /*
     * Termux app and plugins notification variables.
     */

    /** Termux app notification channel id used by {@link TERMUX_APP.TERMUX_SERVICE} */
    public static final String TERMUX_APP_NOTIFICATION_CHANNEL_ID = "termux_notification_channel";
    /** Termux app notification channel name used by {@link TERMUX_APP.TERMUX_SERVICE} */
    public static final String TERMUX_APP_NOTIFICATION_CHANNEL_NAME = TermuxConstants.TERMUX_APP_NAME + " App";
    /** Termux app unique notification id used by {@link TERMUX_APP.TERMUX_SERVICE} */
    public static final int TERMUX_APP_NOTIFICATION_ID = 1337;

    /** Termux app notification channel id used by {@link TERMUX_APP.RUN_COMMAND_SERVICE} */
    public static final String TERMUX_RUN_COMMAND_NOTIFICATION_CHANNEL_ID = "termux_run_command_notification_channel";
    /** Termux app notification channel name used by {@link TERMUX_APP.RUN_COMMAND_SERVICE} */
    public static final String TERMUX_RUN_COMMAND_NOTIFICATION_CHANNEL_NAME = TermuxConstants.TERMUX_APP_NAME + " RunCommandService";
    /** Termux app unique notification id used by {@link TERMUX_APP.RUN_COMMAND_SERVICE} */
    public static final int TERMUX_RUN_COMMAND_NOTIFICATION_ID = 1338;

    /** Termux app notification channel id used for plugin command errors */
    public static final String TERMUX_PLUGIN_COMMAND_ERRORS_NOTIFICATION_CHANNEL_ID = "termux_plugin_command_errors_notification_channel";
    /** Termux app notification channel name used for plugin command errors */
    public static final String TERMUX_PLUGIN_COMMAND_ERRORS_NOTIFICATION_CHANNEL_NAME = TermuxConstants.TERMUX_APP_NAME + " Plugin Commands Errors";

    /** Termux app notification channel id used for crash reports */
    public static final String TERMUX_CRASH_REPORTS_NOTIFICATION_CHANNEL_ID = "termux_crash_reports_notification_channel";
    /** Termux app notification channel name used for crash reports */
    public static final String TERMUX_CRASH_REPORTS_NOTIFICATION_CHANNEL_NAME = TermuxConstants.TERMUX_APP_NAME + " Crash Reports";


    /** Termux app notification channel id used by {@link TERMUX_FLOAT_APP.TERMUX_FLOAT_SERVICE} */
    public static final String TERMUX_FLOAT_APP_NOTIFICATION_CHANNEL_ID = "termux_float_notification_channel";
    /** Termux app notification channel name used by {@link TERMUX_FLOAT_APP.TERMUX_FLOAT_SERVICE} */
    public static final String TERMUX_FLOAT_APP_NOTIFICATION_CHANNEL_NAME = TermuxConstants.TERMUX_FLOAT_APP_NAME + " App";
    /** Termux app unique notification id used by {@link TERMUX_APP.TERMUX_SERVICE} */
    public static final int TERMUX_FLOAT_APP_NOTIFICATION_ID = 1339;





    /*
     * Termux app and plugins miscellaneous variables.
     */

    /** Android OS permission declared by Termux app in AndroidManifest.xml which can be requested by
     * 3rd party apps to run various commands in Termux app context */
    public static final String PERMISSION_RUN_COMMAND = TERMUX_PACKAGE_NAME + ".permission.RUN_COMMAND"; // Default: "com.termux.permission.RUN_COMMAND"

    /** Termux property defined in termux.properties file as a secondary check to PERMISSION_RUN_COMMAND
     * to allow 3rd party apps to run various commands in Termux app context */
    public static final String PROP_ALLOW_EXTERNAL_APPS = "allow-external-apps"; // Default: "allow-external-apps"
    /** Default value for {@link #PROP_ALLOW_EXTERNAL_APPS} */
    public static final String PROP_DEFAULT_VALUE_ALLOW_EXTERNAL_APPS = "false"; // Default: "false"

    /** The broadcast action sent when Termux App opens */
    public static final String BROADCAST_TERMUX_OPENED = TERMUX_PACKAGE_NAME + ".app.OPENED";

    /** The Uri authority for Termux app file shares */
    public static final String TERMUX_FILE_SHARE_URI_AUTHORITY = TERMUX_PACKAGE_NAME + ".files"; // Default: "com.termux.files"

    /** The normal comma character (U+002C, &comma;, &#44;, comma) */
    public static final String COMMA_NORMAL = ","; // Default: ","

    /** The alternate comma character (U+201A, &sbquo;, &#8218;, single low-9 quotation mark) that
     * may be used instead of {@link #COMMA_NORMAL} */
    public static final String COMMA_ALTERNATIVE = "‚"; // Default: "‚"

    /** Environment variable prefix root for the Termux app. */
    public static final String TERMUX_ENV_PREFIX_ROOT = "TERMUX";






    /**
     * Termux app constants.
     */
    public static final class TERMUX_APP {

        /** Termux apps directory path */
        public static final String APPS_DIR_PATH = TERMUX_APPS_DIR_PATH + "/" + TERMUX_PACKAGE_NAME; // Default: "/data/data/com.termux/files/apps/com.termux"

        /** termux-am socket file path */
        public static final String TERMUX_AM_SOCKET_FILE_PATH = APPS_DIR_PATH + "/termux-am/am.sock"; // Default: "/data/data/com.termux/files/apps/com.termux/termux-am/am.sock"


        /** Termux app BuildConfig class name */
        public static final String BUILD_CONFIG_CLASS_NAME = TERMUX_PACKAGE_NAME + ".BuildConfig"; // Default: "com.termux.BuildConfig"

        /** Termux app FileShareReceiverActivity class name */
        public static final String FILE_SHARE_RECEIVER_ACTIVITY_CLASS_NAME = TERMUX_PACKAGE_NAME + ".app.api.file.FileShareReceiverActivity"; // Default: "com.termux.app.api.file.FileShareReceiverActivity"

        /** Termux app FileViewReceiverActivity class name */
        public static final String FILE_VIEW_RECEIVER_ACTIVITY_CLASS_NAME = TERMUX_PACKAGE_NAME + ".app.api.file.FileViewReceiverActivity"; // Default: "com.termux.app.api.file.FileViewReceiverActivity"


        /** Termux app core activity name. */
        public static final String TERMUX_ACTIVITY_NAME = TERMUX_PACKAGE_NAME + ".app.TermuxActivity"; // Default: "com.termux.app.TermuxActivity"

        /**
         * Termux app core activity.
         */
        public static final class TERMUX_ACTIVITY {

            /** Intent extra for if termux failsafe session needs to be started and is used by {@link TERMUX_ACTIVITY} and {@link TERMUX_SERVICE#ACTION_STOP_SERVICE} */
            public static final String EXTRA_FAILSAFE_SESSION = TermuxConstants.TERMUX_PACKAGE_NAME + ".app.failsafe_session"; // Default: "com.termux.app.failsafe_session"


            /** Intent action to make termux app notify user that a crash happened. */
            public static final String ACTION_NOTIFY_APP_CRASH = TermuxConstants.TERMUX_PACKAGE_NAME + ".app.notify_app_crash"; // Default: "com.termux.app.notify_app_crash"


            /** Intent action to make termux reload its termux session styling */
            public static final String ACTION_RELOAD_STYLE = TermuxConstants.TERMUX_PACKAGE_NAME + ".app.reload_style"; // Default: "com.termux.app.reload_style"
            /** Intent {@code String} extra for what to reload for the TERMUX_ACTIVITY.ACTION_RELOAD_STYLE intent. This has been deperecated. */
            @Deprecated
            public static final String EXTRA_RELOAD_STYLE = TermuxConstants.TERMUX_PACKAGE_NAME + ".app.reload_style"; // Default: "com.termux.app.reload_style"

            /**  Intent {@code boolean} extra for whether to recreate activity for the TERMUX_ACTIVITY.ACTION_RELOAD_STYLE intent. */
            public static final String EXTRA_RECREATE_ACTIVITY = TERMUX_APP.TERMUX_ACTIVITY_NAME + ".EXTRA_RECREATE_ACTIVITY"; // Default: "com.termux.app.TermuxActivity.EXTRA_RECREATE_ACTIVITY"


            /** Intent action to make termux request storage permissions */
            public static final String ACTION_REQUEST_PERMISSIONS = TermuxConstants.TERMUX_PACKAGE_NAME + ".app.request_storage_permissions"; // Default: "com.termux.app.request_storage_permissions"
        }





        /** Termux app settings activity name. */
        public static final String TERMUX_SETTINGS_ACTIVITY_NAME = TERMUX_PACKAGE_NAME + ".app.activities.SettingsActivity"; // Default: "com.termux.app.activities.SettingsActivity"





        /** Termux app core service name. */
        public static final String TERMUX_SERVICE_NAME = TERMUX_PACKAGE_NAME + ".app.TermuxService"; // Default: "com.termux.app.TermuxService"

        /**
         * Termux app core service.
         */
        public static final class TERMUX_SERVICE {

            /** Intent action to stop TERMUX_SERVICE */
            public static final String ACTION_STOP_SERVICE = TERMUX_PACKAGE_NAME + ".service_stop"; // Default: "com.termux.service_stop"


            /** Intent action to make TERMUX_SERVICE acquire a wakelock */
            public static final String ACTION_WAKE_LOCK = TERMUX_PACKAGE_NAME + ".service_wake_lock"; // Default: "com.termux.service_wake_lock"


            /** Intent action to make TERMUX_SERVICE release wakelock */
            public static final String ACTION_WAKE_UNLOCK = TERMUX_PACKAGE_NAME + ".service_wake_unlock"; // Default: "com.termux.service_wake_unlock"


            /** Intent action to execute command with TERMUX_SERVICE */
            public static final String ACTION_SERVICE_EXECUTE = TERMUX_PACKAGE_NAME + ".service_execute"; // Default: "com.termux.service_execute"

            /** Uri scheme for paths sent via intent to TERMUX_SERVICE */
            public static final String URI_SCHEME_SERVICE_EXECUTE = TERMUX_PACKAGE_NAME + ".file"; // Default: "com.termux.file"
            /** Intent {@code String[]} extra for arguments to the executable of the command for the TERMUX_SERVICE.ACTION_SERVICE_EXECUTE intent */
            public static final String EXTRA_ARGUMENTS = TERMUX_PACKAGE_NAME + ".execute.arguments"; // Default: "com.termux.execute.arguments"
            /** Intent {@code String} extra for stdin of the command for the TERMUX_SERVICE.ACTION_SERVICE_EXECUTE intent */
            public static final String EXTRA_STDIN = TERMUX_PACKAGE_NAME + ".execute.stdin"; // Default: "com.termux.execute.stdin"
            /** Intent {@code String} extra for command current working directory for the TERMUX_SERVICE.ACTION_SERVICE_EXECUTE intent */
            public static final String EXTRA_WORKDIR = TERMUX_PACKAGE_NAME + ".execute.cwd"; // Default: "com.termux.execute.cwd"
            /** Intent {@code boolean} extra for whether to run command in background {@link Runner#APP_SHELL} or foreground {@link Runner#TERMINAL_SESSION} for the TERMUX_SERVICE.ACTION_SERVICE_EXECUTE intent */
            @Deprecated
            public static final String EXTRA_BACKGROUND = TERMUX_PACKAGE_NAME + ".execute.background"; // Default: "com.termux.execute.background"
            /** Intent {@code String} extra for command the {@link Runner} for the TERMUX_SERVICE.ACTION_SERVICE_EXECUTE intent */
            public static final String EXTRA_RUNNER = TERMUX_PACKAGE_NAME + ".execute.runner"; // Default: "com.termux.execute.runner"
            /** Intent {@code String} extra for custom log level for background commands defined by {@link com.termux.shared.logger.Logger} for the TERMUX_SERVICE.ACTION_SERVICE_EXECUTE intent */
            public static final String EXTRA_BACKGROUND_CUSTOM_LOG_LEVEL = TERMUX_PACKAGE_NAME + ".execute.background_custom_log_level"; // Default: "com.termux.execute.background_custom_log_level"
            /** Intent {@code String} extra for session action for {@link Runner#TERMINAL_SESSION} commands for the TERMUX_SERVICE.ACTION_SERVICE_EXECUTE intent */
            public static final String EXTRA_SESSION_ACTION = TERMUX_PACKAGE_NAME + ".execute.session_action"; // Default: "com.termux.execute.session_action"
            /** Intent {@code String} extra for shell name for commands for the TERMUX_SERVICE.ACTION_SERVICE_EXECUTE intent */
            public static final String EXTRA_SHELL_NAME = TERMUX_PACKAGE_NAME + ".execute.shell_name"; // Default: "com.termux.execute.shell_name"
            /** Intent {@code String} extra for the {@link ExecutionCommand.ShellCreateMode}  for the TERMUX_SERVICE.ACTION_SERVICE_EXECUTE intent. */
            public static final String EXTRA_SHELL_CREATE_MODE = TERMUX_PACKAGE_NAME + ".execute.shell_create_mode"; // Default: "com.termux.execute.shell_create_mode"
            /** Intent {@code String} extra for label of the command for the TERMUX_SERVICE.ACTION_SERVICE_EXECUTE intent */
            public static final String EXTRA_COMMAND_LABEL = TERMUX_PACKAGE_NAME + ".execute.command_label"; // Default: "com.termux.execute.command_label"
            /** Intent markdown {@code String} extra for description of the command for the TERMUX_SERVICE.ACTION_SERVICE_EXECUTE intent */
            public static final String EXTRA_COMMAND_DESCRIPTION = TERMUX_PACKAGE_NAME + ".execute.command_description"; // Default: "com.termux.execute.command_description"
            /** Intent markdown {@code String} extra for help of the command for the TERMUX_SERVICE.ACTION_SERVICE_EXECUTE intent */
            public static final String EXTRA_COMMAND_HELP = TERMUX_PACKAGE_NAME + ".execute.command_help"; // Default: "com.termux.execute.command_help"
            /** Intent markdown {@code String} extra for help of the plugin API for the TERMUX_SERVICE.ACTION_SERVICE_EXECUTE intent (Internal Use Only) */
            public static final String EXTRA_PLUGIN_API_HELP = TERMUX_PACKAGE_NAME + ".execute.plugin_api_help"; // Default: "com.termux.execute.plugin_help"
            /** Intent {@code Parcelable} extra for the pending intent that should be sent with the
             * result of the execution command to the execute command caller for the TERMUX_SERVICE.ACTION_SERVICE_EXECUTE intent */
            public static final String EXTRA_PENDING_INTENT = "pendingIntent"; // Default: "pendingIntent"
            /** Intent {@code String} extra for the directory path in which to write the result of the
             * execution command for the execute command caller for the TERMUX_SERVICE.ACTION_SERVICE_EXECUTE intent */
            public static final String EXTRA_RESULT_DIRECTORY = TERMUX_PACKAGE_NAME + ".execute.result_directory"; // Default: "com.termux.execute.result_directory"
            /** Intent {@code boolean} extra for whether the result should be written to a single file
             * or multiple files (err, errmsg, stdout, stderr, exit_code) in
             * {@link #EXTRA_RESULT_DIRECTORY} for the TERMUX_SERVICE.ACTION_SERVICE_EXECUTE intent */
            public static final String EXTRA_RESULT_SINGLE_FILE = TERMUX_PACKAGE_NAME + ".execute.result_single_file"; // Default: "com.termux.execute.result_single_file"
            /** Intent {@code String} extra for the basename of the result file that should be created
             * in {@link #EXTRA_RESULT_DIRECTORY} if {@link #EXTRA_RESULT_SINGLE_FILE} is {@code true}
             * for the TERMUX_SERVICE.ACTION_SERVICE_EXECUTE intent */
            public static final String EXTRA_RESULT_FILE_BASENAME = TERMUX_PACKAGE_NAME + ".execute.result_file_basename"; // Default: "com.termux.execute.result_file_basename"
            /** Intent {@code String} extra for the output {@link Formatter} format of the
             * {@link #EXTRA_RESULT_FILE_BASENAME} result file for the TERMUX_SERVICE.ACTION_SERVICE_EXECUTE intent */
            public static final String EXTRA_RESULT_FILE_OUTPUT_FORMAT = TERMUX_PACKAGE_NAME + ".execute.result_file_output_format"; // Default: "com.termux.execute.result_file_output_format"
            /** Intent {@code String} extra for the error {@link Formatter} format of the
             * {@link #EXTRA_RESULT_FILE_BASENAME} result file for the TERMUX_SERVICE.ACTION_SERVICE_EXECUTE intent */
            public static final String EXTRA_RESULT_FILE_ERROR_FORMAT = TERMUX_PACKAGE_NAME + ".execute.result_file_error_format"; // Default: "com.termux.execute.result_file_error_format"
            /** Intent {@code String} extra for the optional suffix of the result files that should
             * be created in {@link #EXTRA_RESULT_DIRECTORY} if {@link #EXTRA_RESULT_SINGLE_FILE} is
             * {@code false} for the TERMUX_SERVICE.ACTION_SERVICE_EXECUTE intent */
            public static final String EXTRA_RESULT_FILES_SUFFIX = TERMUX_PACKAGE_NAME + ".execute.result_files_suffix"; // Default: "com.termux.execute.result_files_suffix"



            /**
             * The value for {@link #EXTRA_SESSION_ACTION} extra that will set the new session as
             * the current session and will start {@link TERMUX_ACTIVITY} if its not running to bring
             * the new session to foreground.
             */
            public static final int VALUE_EXTRA_SESSION_ACTION_SWITCH_TO_NEW_SESSION_AND_OPEN_ACTIVITY = 0;

            /**
             * The value for {@link #EXTRA_SESSION_ACTION} extra that will keep any existing session
             * as the current session and will start {@link TERMUX_ACTIVITY} if its not running to
             * bring the existing session to foreground. The new session will be added to the left
             * sidebar in the sessions list.
             */
            public static final int VALUE_EXTRA_SESSION_ACTION_KEEP_CURRENT_SESSION_AND_OPEN_ACTIVITY = 1;

            /**
             * The value for {@link #EXTRA_SESSION_ACTION} extra that will set the new session as
             * the current session but will not start {@link TERMUX_ACTIVITY} if its not running
             * and session(s) will be seen in Termux notification and can be clicked to bring new
             * session to foreground. If the {@link TERMUX_ACTIVITY} is already running, then this
             * will behave like {@link #VALUE_EXTRA_SESSION_ACTION_KEEP_CURRENT_SESSION_AND_OPEN_ACTIVITY}.
             */
            public static final int VALUE_EXTRA_SESSION_ACTION_SWITCH_TO_NEW_SESSION_AND_DONT_OPEN_ACTIVITY = 2;

            /**
             * The value for {@link #EXTRA_SESSION_ACTION} extra that will keep any existing session
             * as the current session but will not start {@link TERMUX_ACTIVITY} if its not running
             * and session(s) will be seen in Termux notification and can be clicked to bring
             * existing session to foreground. If the {@link TERMUX_ACTIVITY} is already running,
             * then this will behave like {@link #VALUE_EXTRA_SESSION_ACTION_KEEP_CURRENT_SESSION_AND_OPEN_ACTIVITY}.
             */
            public static final int VALUE_EXTRA_SESSION_ACTION_KEEP_CURRENT_SESSION_AND_DONT_OPEN_ACTIVITY = 3;

            /** The minimum allowed value for {@link #EXTRA_SESSION_ACTION}. */
            public static final int MIN_VALUE_EXTRA_SESSION_ACTION = VALUE_EXTRA_SESSION_ACTION_SWITCH_TO_NEW_SESSION_AND_OPEN_ACTIVITY;

            /** The maximum allowed value for {@link #EXTRA_SESSION_ACTION}. */
            public static final int MAX_VALUE_EXTRA_SESSION_ACTION = VALUE_EXTRA_SESSION_ACTION_KEEP_CURRENT_SESSION_AND_DONT_OPEN_ACTIVITY;


            /** Intent {@code Bundle} extra to store result of execute command that is sent back for the
             * TERMUX_SERVICE.ACTION_SERVICE_EXECUTE intent if the {@link #EXTRA_PENDING_INTENT} is not
             * {@code null} */
            public static final String EXTRA_PLUGIN_RESULT_BUNDLE = "result"; // Default: "result"
            /** Intent {@code String} extra for stdout value of execute command of the {@link #EXTRA_PLUGIN_RESULT_BUNDLE} */
            public static final String EXTRA_PLUGIN_RESULT_BUNDLE_STDOUT = "stdout"; // Default: "stdout"
            /** Intent {@code String} extra for original length of stdout value of execute command of the {@link #EXTRA_PLUGIN_RESULT_BUNDLE} */
            public static final String EXTRA_PLUGIN_RESULT_BUNDLE_STDOUT_ORIGINAL_LENGTH = "stdout_original_length"; // Default: "stdout_original_length"
            /** Intent {@code String} extra for stderr value of execute command of the {@link #EXTRA_PLUGIN_RESULT_BUNDLE} */
            public static final String EXTRA_PLUGIN_RESULT_BUNDLE_STDERR = "stderr"; // Default: "stderr"
            /** Intent {@code String} extra for original length of stderr value of execute command of the {@link #EXTRA_PLUGIN_RESULT_BUNDLE} */
            public static final String EXTRA_PLUGIN_RESULT_BUNDLE_STDERR_ORIGINAL_LENGTH = "stderr_original_length"; // Default: "stderr_original_length"
            /** Intent {@code int} extra for exit code value of execute command of the {@link #EXTRA_PLUGIN_RESULT_BUNDLE} */
            public static final String EXTRA_PLUGIN_RESULT_BUNDLE_EXIT_CODE = "exitCode"; // Default: "exitCode"
            /** Intent {@code int} extra for err value of execute command of the {@link #EXTRA_PLUGIN_RESULT_BUNDLE} */
            public static final String EXTRA_PLUGIN_RESULT_BUNDLE_ERR = "err"; // Default: "err"
            /** Intent {@code String} extra for errmsg value of execute command of the {@link #EXTRA_PLUGIN_RESULT_BUNDLE} */
            public static final String EXTRA_PLUGIN_RESULT_BUNDLE_ERRMSG = "errmsg"; // Default: "errmsg"

        }





        /** Termux app run command service name. */
        public static final String RUN_COMMAND_SERVICE_NAME = TERMUX_PACKAGE_NAME + ".app.RunCommandService"; // Termux app service to receive commands from 3rd party apps "com.termux.app.RunCommandService"

        /**
         * Termux app run command service to receive commands sent by 3rd party apps.
         */
        public static final class RUN_COMMAND_SERVICE {

            /** Termux RUN_COMMAND Intent help url */
            public static final String RUN_COMMAND_API_HELP_URL = TERMUX_GITHUB_WIKI_REPO_URL + "/RUN_COMMAND-Intent"; // Default: "https://github.com/termux/termux-app/wiki/RUN_COMMAND-Intent"


            /** Intent action to execute command with RUN_COMMAND_SERVICE */
            public static final String ACTION_RUN_COMMAND = TERMUX_PACKAGE_NAME + ".RUN_COMMAND"; // Default: "com.termux.RUN_COMMAND"

            /** Intent {@code String} extra for absolute path of command for the RUN_COMMAND_SERVICE.ACTION_RUN_COMMAND intent */
            public static final String EXTRA_COMMAND_PATH = TERMUX_PACKAGE_NAME + ".RUN_COMMAND_PATH"; // Default: "com.termux.RUN_COMMAND_PATH"
            /** Intent {@code String[]} extra for arguments to the executable of the command for the RUN_COMMAND_SERVICE.ACTION_RUN_COMMAND intent */
            public static final String EXTRA_ARGUMENTS = TERMUX_PACKAGE_NAME + ".RUN_COMMAND_ARGUMENTS"; // Default: "com.termux.RUN_COMMAND_ARGUMENTS"
            /** Intent {@code boolean} extra for whether to replace comma alternative characters in arguments with comma characters for the RUN_COMMAND_SERVICE.ACTION_RUN_COMMAND intent */
            public static final String EXTRA_REPLACE_COMMA_ALTERNATIVE_CHARS_IN_ARGUMENTS = TERMUX_PACKAGE_NAME + ".RUN_COMMAND_REPLACE_COMMA_ALTERNATIVE_CHARS_IN_ARGUMENTS"; // Default: "com.termux.RUN_COMMAND_REPLACE_COMMA_ALTERNATIVE_CHARS_IN_ARGUMENTS"
            /** Intent {@code String} extra for the comma alternative characters in arguments that should be replaced instead of the default {@link #COMMA_ALTERNATIVE} for the RUN_COMMAND_SERVICE.ACTION_RUN_COMMAND intent */
            public static final String EXTRA_COMMA_ALTERNATIVE_CHARS_IN_ARGUMENTS = TERMUX_PACKAGE_NAME + ".RUN_COMMAND_COMMA_ALTERNATIVE_CHARS_IN_ARGUMENTS"; // Default: "com.termux.RUN_COMMAND_COMMA_ALTERNATIVE_CHARS_IN_ARGUMENTS"

            /** Intent {@code String} extra for stdin of the command for the RUN_COMMAND_SERVICE.ACTION_RUN_COMMAND intent */
            public static final String EXTRA_STDIN = TERMUX_PACKAGE_NAME + ".RUN_COMMAND_STDIN"; // Default: "com.termux.RUN_COMMAND_STDIN"
            /** Intent {@code String} extra for current working directory of command for the RUN_COMMAND_SERVICE.ACTION_RUN_COMMAND intent */
            public static final String EXTRA_WORKDIR = TERMUX_PACKAGE_NAME + ".RUN_COMMAND_WORKDIR"; // Default: "com.termux.RUN_COMMAND_WORKDIR"
            /** Intent {@code boolean} extra for whether to run command in background {@link Runner#APP_SHELL} or foreground {@link Runner#TERMINAL_SESSION} for the RUN_COMMAND_SERVICE.ACTION_RUN_COMMAND intent */
            @Deprecated
            public static final String EXTRA_BACKGROUND = TERMUX_PACKAGE_NAME + ".RUN_COMMAND_BACKGROUND"; // Default: "com.termux.RUN_COMMAND_BACKGROUND"
            /** Intent {@code String} extra for command the {@link Runner} for the RUN_COMMAND_SERVICE.ACTION_RUN_COMMAND intent */
            public static final String EXTRA_RUNNER = TERMUX_PACKAGE_NAME + ".RUN_COMMAND_RUNNER"; // Default: "com.termux.RUN_COMMAND_RUNNER"
            /** Intent {@code String} extra for custom log level for background commands defined by {@link com.termux.shared.logger.Logger} for the RUN_COMMAND_SERVICE.ACTION_RUN_COMMAND intent */
            public static final String EXTRA_BACKGROUND_CUSTOM_LOG_LEVEL = TERMUX_PACKAGE_NAME + ".RUN_COMMAND_BACKGROUND_CUSTOM_LOG_LEVEL"; // Default: "com.termux.RUN_COMMAND_BACKGROUND_CUSTOM_LOG_LEVEL"
            /** Intent {@code String} extra for session action of {@link Runner#TERMINAL_SESSION} commands for the RUN_COMMAND_SERVICE.ACTION_RUN_COMMAND intent */
            public static final String EXTRA_SESSION_ACTION = TERMUX_PACKAGE_NAME + ".RUN_COMMAND_SESSION_ACTION"; // Default: "com.termux.RUN_COMMAND_SESSION_ACTION"
            /** Intent {@code String} extra for shell name of commands for the RUN_COMMAND_SERVICE.ACTION_RUN_COMMAND intent */
            public static final String EXTRA_SHELL_NAME = TERMUX_PACKAGE_NAME + ".RUN_COMMAND_SHELL_NAME"; // Default: "com.termux.RUN_COMMAND_SHELL_NAME"
            /** Intent {@code String} extra for the {@link ExecutionCommand.ShellCreateMode}  for the RUN_COMMAND_SERVICE.ACTION_RUN_COMMAND intent. */
            public static final String EXTRA_SHELL_CREATE_MODE = TERMUX_PACKAGE_NAME + ".RUN_COMMAND_SHELL_CREATE_MODE"; // Default: "com.termux.RUN_COMMAND_SHELL_CREATE_MODE"
            /** Intent {@code String} extra for label of the command for the RUN_COMMAND_SERVICE.ACTION_RUN_COMMAND intent */
            public static final String EXTRA_COMMAND_LABEL = TERMUX_PACKAGE_NAME + ".RUN_COMMAND_COMMAND_LABEL"; // Default: "com.termux.RUN_COMMAND_COMMAND_LABEL"
            /** Intent markdown {@code String} extra for description of the command for the RUN_COMMAND_SERVICE.ACTION_RUN_COMMAND intent */
            public static final String EXTRA_COMMAND_DESCRIPTION = TERMUX_PACKAGE_NAME + ".RUN_COMMAND_COMMAND_DESCRIPTION"; // Default: "com.termux.RUN_COMMAND_COMMAND_DESCRIPTION"
            /** Intent markdown {@code String} extra for help of the command for the RUN_COMMAND_SERVICE.ACTION_RUN_COMMAND intent */
            public static final String EXTRA_COMMAND_HELP = TERMUX_PACKAGE_NAME + ".RUN_COMMAND_COMMAND_HELP"; // Default: "com.termux.RUN_COMMAND_COMMAND_HELP"
            /** Intent {@code Parcelable} extra for the pending intent that should be sent with the result of the execution command to the execute command caller for the RUN_COMMAND_SERVICE.ACTION_RUN_COMMAND intent */
            public static final String EXTRA_PENDING_INTENT = TERMUX_PACKAGE_NAME + ".RUN_COMMAND_PENDING_INTENT"; // Default: "com.termux.RUN_COMMAND_PENDING_INTENT"
            /** Intent {@code String} extra for the directory path in which to write the result of
             * the execution command for the execute command caller for the RUN_COMMAND_SERVICE.ACTION_RUN_COMMAND intent */
            public static final String EXTRA_RESULT_DIRECTORY = TERMUX_PACKAGE_NAME + ".RUN_COMMAND_RESULT_DIRECTORY"; // Default: "com.termux.RUN_COMMAND_RESULT_DIRECTORY"
            /** Intent {@code boolean} extra for whether the result should be written to a single file
             * or multiple files (err, errmsg, stdout, stderr, exit_code) in
             * {@link #EXTRA_RESULT_DIRECTORY} for the RUN_COMMAND_SERVICE.ACTION_RUN_COMMAND intent */
            public static final String EXTRA_RESULT_SINGLE_FILE = TERMUX_PACKAGE_NAME + ".RUN_COMMAND_RESULT_SINGLE_FILE"; // Default: "com.termux.RUN_COMMAND_RESULT_SINGLE_FILE"
            /** Intent {@code String} extra for the basename of the result file that should be created
             * in {@link #EXTRA_RESULT_DIRECTORY} if {@link #EXTRA_RESULT_SINGLE_FILE} is {@code true}
             * for the RUN_COMMAND_SERVICE.ACTION_RUN_COMMAND intent */
            public static final String EXTRA_RESULT_FILE_BASENAME = TERMUX_PACKAGE_NAME + ".RUN_COMMAND_RESULT_FILE_BASENAME"; // Default: "com.termux.RUN_COMMAND_RESULT_FILE_BASENAME"
            /** Intent {@code String} extra for the output {@link Formatter} format of the
             * {@link #EXTRA_RESULT_FILE_BASENAME} result file for the RUN_COMMAND_SERVICE.ACTION_RUN_COMMAND intent */
            public static final String EXTRA_RESULT_FILE_OUTPUT_FORMAT = TERMUX_PACKAGE_NAME + ".RUN_COMMAND_RESULT_FILE_OUTPUT_FORMAT"; // Default: "com.termux.RUN_COMMAND_RESULT_FILE_OUTPUT_FORMAT"
            /** Intent {@code String} extra for the error {@link Formatter} format of the
             * {@link #EXTRA_RESULT_FILE_BASENAME} result file for the RUN_COMMAND_SERVICE.ACTION_RUN_COMMAND intent */
            public static final String EXTRA_RESULT_FILE_ERROR_FORMAT = TERMUX_PACKAGE_NAME + ".RUN_COMMAND_RESULT_FILE_ERROR_FORMAT"; // Default: "com.termux.RUN_COMMAND_RESULT_FILE_ERROR_FORMAT"
            /** Intent {@code String} extra for the optional suffix of the result files that should be
             * created in {@link #EXTRA_RESULT_DIRECTORY} if {@link #EXTRA_RESULT_SINGLE_FILE} is
             * {@code false} for the RUN_COMMAND_SERVICE.ACTION_RUN_COMMAND intent */
            public static final String EXTRA_RESULT_FILES_SUFFIX = TERMUX_PACKAGE_NAME + ".RUN_COMMAND_RESULT_FILES_SUFFIX"; // Default: "com.termux.RUN_COMMAND_RESULT_FILES_SUFFIX"

        }
    }


    /**
     * Termux:API app constants.
     */
    public static final class TERMUX_API_APP {

        /** Termux:API app main activity name. */
        public static final String TERMUX_API_MAIN_ACTIVITY_NAME = TERMUX_API_PACKAGE_NAME + ".activities.TermuxAPIMainActivity"; // Default: "com.termux.api.activities.TermuxAPIMainActivity"

        /** Termux:API app launcher activity name. This is an `activity-alias` for {@link #TERMUX_API_MAIN_ACTIVITY_NAME} used for launchers with {@link Intent#CATEGORY_LAUNCHER}. */
        public static final String TERMUX_API_LAUNCHER_ACTIVITY_NAME = TERMUX_API_PACKAGE_NAME + ".activities.TermuxAPILauncherActivity"; // Default: "com.termux.api.activities.TermuxAPILauncherActivity"

    }





    /**
     * Termux:Boot app constants.
     */
    public static final class TERMUX_BOOT_APP {

        /** Termux:Boot app main activity name. */
        public static final String TERMUX_BOOT_MAIN_ACTIVITY_NAME = TERMUX_BOOT_PACKAGE_NAME + ".activities.TermuxBootMainActivity"; // Default: "com.termux.boot.activities.TermuxBootMainActivity"

        /** Termux:Boot app launcher activity name. This is an `activity-alias` for {@link #TERMUX_BOOT_MAIN_ACTIVITY_NAME} used for launchers with {@link Intent#CATEGORY_LAUNCHER}. */
        public static final String TERMUX_BOOT_LAUNCHER_ACTIVITY_NAME = TERMUX_BOOT_PACKAGE_NAME + ".activities.TermuxBootLauncherActivity"; // Default: "com.termux.boot.activities.TermuxBootLauncherActivity"

    }





    /**
     * Termux:Float app constants.
     */
    public static final class TERMUX_FLOAT_APP {

        /** Termux:Float app core activity name. */
        public static final String TERMUX_FLOAT_ACTIVITY_NAME = TERMUX_FLOAT_PACKAGE_NAME + ".TermuxFloatActivity"; // Default: "com.termux.window.TermuxFloatActivity"

        /** Termux:Float app core service name. */
        public static final String TERMUX_FLOAT_SERVICE_NAME = TERMUX_FLOAT_PACKAGE_NAME + ".TermuxFloatService"; // Default: "com.termux.window.TermuxFloatService"

        /**
         * Termux:Float app core service.
         */
        public static final class TERMUX_FLOAT_SERVICE {

            /** Intent action to stop TERMUX_FLOAT_SERVICE. */
            public static final String ACTION_STOP_SERVICE = TERMUX_FLOAT_PACKAGE_NAME + ".ACTION_STOP_SERVICE"; // Default: "com.termux.float.ACTION_STOP_SERVICE"

            /** Intent action to show float window. */
            public static final String ACTION_SHOW = TERMUX_FLOAT_PACKAGE_NAME + ".ACTION_SHOW"; // Default: "com.termux.float.ACTION_SHOW"

            /** Intent action to hide float window. */
            public static final String ACTION_HIDE = TERMUX_FLOAT_PACKAGE_NAME + ".ACTION_HIDE"; // Default: "com.termux.float.ACTION_HIDE"

        }

    }





    /**
     * Termux:Styling app constants.
     */
    public static final class TERMUX_STYLING_APP {

        /** Termux:Styling app core activity name. */
        public static final String TERMUX_STYLING_ACTIVITY_NAME = TERMUX_STYLING_PACKAGE_NAME + ".TermuxStyleActivity"; // Default: "com.termux.styling.TermuxStyleActivity"


        /** Termux:Styling app main activity name. */
        public static final String TERMUX_STYLING_MAIN_ACTIVITY_NAME = TERMUX_STYLING_PACKAGE_NAME + ".activities.TermuxStylingMainActivity"; // Default: "com.termux.styling.activities.TermuxStylingMainActivity"

        /** Termux:Styling app launcher activity name. This is an `activity-alias` for {@link #TERMUX_STYLING_MAIN_ACTIVITY_NAME} used for launchers with {@link Intent#CATEGORY_LAUNCHER}. */
        public static final String TERMUX_STYLING_LAUNCHER_ACTIVITY_NAME = TERMUX_STYLING_PACKAGE_NAME + ".activities.TermuxStylingLauncherActivity"; // Default: "com.termux.styling.activities.TermuxStylingLauncherActivity"

    }





    /**
     * Termux:Tasker app constants.
     */
    public static final class TERMUX_TASKER_APP {

        /** Termux:Tasker app main activity name. */
        public static final String TERMUX_TASKER_MAIN_ACTIVITY_NAME = TERMUX_TASKER_PACKAGE_NAME + ".activities.TermuxTaskerMainActivity"; // Default: "com.termux.tasker.activities.TermuxTaskerMainActivity"

        /** Termux:Tasker app launcher activity name. This is an `activity-alias` for {@link #TERMUX_TASKER_MAIN_ACTIVITY_NAME} used for launchers with {@link Intent#CATEGORY_LAUNCHER}. */
        public static final String TERMUX_TASKER_LAUNCHER_ACTIVITY_NAME = TERMUX_TASKER_PACKAGE_NAME + ".activities.TermuxTaskerLauncherActivity"; // Default: "com.termux.tasker.activities.TermuxTaskerLauncherActivity"

    }





    /**
     * Termux:Widget app constants.
     */
    public static final class TERMUX_WIDGET_APP {

        /** Termux:Widget app main activity name. */
        public static final String TERMUX_WIDGET_MAIN_ACTIVITY_NAME = TERMUX_WIDGET_PACKAGE_NAME + ".activities.TermuxWidgetMainActivity"; // Default: "com.termux.widget.activities.TermuxWidgetMainActivity"

        /** Termux:Widget app launcher activity name. This is an `activity-alias` for {@link #TERMUX_WIDGET_MAIN_ACTIVITY_NAME} used for launchers with {@link Intent#CATEGORY_LAUNCHER}. */
        public static final String TERMUX_WIDGET_LAUNCHER_ACTIVITY_NAME = TERMUX_WIDGET_PACKAGE_NAME + ".activities.TermuxWidgetLauncherActivity"; // Default: "com.termux.widget.activities.TermuxWidgetLauncherActivity"


        /**  Intent {@code String} extra for the token of the Termux:Widget app shortcuts. */
        public static final String EXTRA_TOKEN_NAME = TERMUX_PACKAGE_NAME + ".shortcut.token"; // Default: "com.termux.shortcut.token"


        /**
         * Termux:Widget app {@link android.appwidget.AppWidgetProvider} class.
         */
        public static final class TERMUX_WIDGET_PROVIDER {

            /** Intent action for if an item is clicked in the widget. */
            public static final String ACTION_WIDGET_ITEM_CLICKED = TERMUX_WIDGET_PACKAGE_NAME + ".ACTION_WIDGET_ITEM_CLICKED"; // Default: "com.termux.widget.ACTION_WIDGET_ITEM_CLICKED"


            /** Intent action to refresh files in the widget. */
            public static final String ACTION_REFRESH_WIDGET = TERMUX_WIDGET_PACKAGE_NAME + ".ACTION_REFRESH_WIDGET"; // Default: "com.termux.widget.ACTION_REFRESH_WIDGET"


            /**  Intent {@code String} extra for the file clicked for the TERMUX_WIDGET_PROVIDER.ACTION_WIDGET_ITEM_CLICKED intent. */
            public static final String EXTRA_FILE_CLICKED = TERMUX_WIDGET_PACKAGE_NAME + ".EXTRA_FILE_CLICKED"; // Default: "com.termux.widget.EXTRA_FILE_CLICKED"

        }

    }

}
