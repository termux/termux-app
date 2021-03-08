package com.termux.app;

import android.annotation.SuppressLint;

import java.io.File;

// Version: v0.1.0

/**
 * A class that defines shared constants of the Termux app and its plugins.
 * This class will be hosted by termux-app and should be imported by other termux plugin apps as is
 * instead of copying constants to random classes. The 3rd party apps can also import it for
 * interacting with termux apps.
 *
 * Termux app default package name is "com.termux" and is used in PREFIX_PATH.
 * The binaries compiled for termux have PREFIX_PATH hardcoded in them but it can be changed during
 * compilation.
 *
 * The TERMUX_PACKAGE_NAME must be the same as the applicationId of termux-app build.gradle since
 * its also used by FILES_PATH.
 * If TERMUX_PACKAGE_NAME is changed, then binaries, specially used in bootstrap need to be compiled
 * appropriately. Check https://github.com/termux/termux-packages/wiki/Building-packages for more info.
 *
 * Ideally the only places where changes should be required if changing package name are the following:
 * - The TERMUX_PACKAGE_NAME in TermuxConstants.
 * - The "applicationId" in "build.gradle". This is package name that android and app stores will
 *       use and is also the final package name stored in "AndroidManifest.xml".
 * - The "manifestPlaceholders" values for TERMUX_PACKAGE_NAME and *_APP_NAME in "build.gradle".
 * - The "ENTITY" values for TERMUX_PACKAGE_NAME and *_APP_NAME in "strings.xml".
 * - The "shortcut.xml" files like in termux-app since dynamic variables don't work in it.
 * - Optionally the "package" in "AndroidManifest.xml" if modifying project structure. This is
 *      package name for java classes project structure and is prefixed if activity and service
 *      names use dot (.) notation.
 * - Optionally the *_PATH variables in TermuxConstants containing the string "termux".
 *
 * Check https://developer.android.com/studio/build/application-id for info on "package" in
 * "AndroidManifest.xml" and "applicationId" in "build.gradle".
 *
 * TERMUX_PACKAGE_NAME must be used in source code of Termux app and its plugins instead of hardcoded
 * "com.termux" paths.
 */

public final class TermuxConstants {

    /**
     * Termux app and plugin app and package names.
     */

    public static final String TERMUX_APP_NAME = "Termux"; // Default: "Termux"
    public static final String TERMUX_PACKAGE_NAME = "com.termux"; // Default: "com.termux"

    public static final String TERMUX_API_APP_NAME = "Termux:API"; // Default: "Termux:API"
    public static final String TERMUX_API_PACKAGE_NAME = TERMUX_PACKAGE_NAME + ".api"; // Default: "com.termux.api"

    public static final String TERMUX_BOOT_APP_NAME = "Termux:Boot"; // Default: "Termux:Boot"
    public static final String TERMUX_BOOT_PACKAGE_NAME = TERMUX_PACKAGE_NAME + ".boot"; // Default: "com.termux.boot"

    public static final String TERMUX_FLOAT_APP_NAME = "Termux:Float"; // Default: "Termux:Float"
    public static final String TERMUX_FLOAT_PACKAGE_NAME = TERMUX_PACKAGE_NAME + ".window"; // Default: "com.termux.window"

    public static final String TERMUX_STYLING_APP_NAME = "Termux:Styling"; // Default: "Termux:Styling"
    public static final String TERMUX_STYLING_PACKAGE_NAME = TERMUX_PACKAGE_NAME + ".styling"; // Default: "com.termux.styling"

    public static final String TERMUX_TASKER_APP_NAME = "Termux:Tasker"; // Default: "Termux:Tasker"
    public static final String TERMUX_TASKER_PACKAGE_NAME = TERMUX_PACKAGE_NAME + ".tasker"; // Default: "com.termux.tasker"

    public static final String TERMUX_WIDGET_APP_NAME = "Termux:Widget"; // Default: "Termux:Widget"
    public static final String TERMUX_WIDGET_PACKAGE_NAME = TERMUX_PACKAGE_NAME + ".widget"; // Default: "com.termux.widget"



    /**
     * Termux app core paths.
     */

    @SuppressLint("SdCardPath")
    public static final String FILES_PATH = "/data/data/" + TERMUX_PACKAGE_NAME + "/files"; // Default: "/data/data/com.termux/files"
    public static final String PREFIX_PATH = FILES_PATH + "/usr"; // Termux $PREFIX path. Default: "/data/data/com.termux/files/usr"
    public static final String HOME_PATH = FILES_PATH + "/home"; // Termux $HOME path. Default: "/data/data/com.termux/files/home"
    public static final String DATA_HOME_PATH = HOME_PATH + "/.termux"; // Default: "/data/data/com.termux/files/home/.termux"
    public static final String CONFIG_HOME_PATH = HOME_PATH + "/.config/termux"; // Default: "/data/data/com.termux/files/home/.config/termux"



    /**
     * Termux app plugin specific paths.
     */

    // Path to store scripts to be run at boot by Termux:Boot
    public static final String BOOT_SCRIPTS_PATH = DATA_HOME_PATH + "/boot"; // Default: "/data/data/com.termux/files/home/.termux/boot"

    // Path to store foreground scripts that can be run by the termux launcher widget provided by Termux:Widget
    public static final String SHORTCUT_SCRIPTS_PATH = DATA_HOME_PATH + "/shortcuts"; // Default: "/data/data/com.termux/files/home/.termux/shortcuts"

    // Path to store background scripts that can be run by the termux launcher widget provided by Termux:Widget
    public static final String SHORTCUT_TASKS_SCRIPTS_PATH = DATA_HOME_PATH + "/shortcuts/tasks"; // Default: "/data/data/com.termux/files/home/.termux/shortcuts/tasks"

    // Path to store scripts to be run by 3rd party twofortyfouram locale plugin host apps like Tasker app via the Termux:Tasker plugin client
    public static final String TASKER_SCRIPTS_PATH = DATA_HOME_PATH + "/tasker"; // Default: "/data/data/com.termux/files/home/.termux/tasker"

    // Termux app termux.properties primary path
    public static final String TERMUX_PROPERTIES_PRIMARY_PATH = DATA_HOME_PATH + "/termux.properties"; // Default: "/data/data/com.termux/files/home/.termux/termux.properties"

    // Termux app termux.properties secondary path
    public static final String TERMUX_PROPERTIES_SECONDARY_PATH = CONFIG_HOME_PATH + "/termux.properties"; // Default: "/data/data/com.termux/files/home/.config/termux/termux.properties"

    // Termux app and Termux:Styling colors.properties path
    public static final String COLOR_PROPERTIES_PATH = DATA_HOME_PATH + "/colors.properties"; // Default: "/data/data/com.termux/files/home/.termux/colors.properties"

    // Termux app and Termux:Styling font.ttf path
    public static final String FONT_PATH = DATA_HOME_PATH + "/font.ttf"; // Default: "/data/data/com.termux/files/home/.termux/font.ttf"



    /**
     * Termux app plugin specific path File objects.
     */

    public static final File FILES_DIR = new File(FILES_PATH);
    public static final File PREFIX_DIR = new File(PREFIX_PATH);
    public static final File HOME_DIR = new File(HOME_PATH);
    public static final File DATA_HOME_DIR = new File(DATA_HOME_PATH);
    public static final File CONFIG_HOME_DIR = new File(CONFIG_HOME_PATH);
    public static final File BOOT_SCRIPTS_DIR = new File(BOOT_SCRIPTS_PATH);
    public static final File SHORTCUT_SCRIPTS_DIR = new File(SHORTCUT_SCRIPTS_PATH);
    public static final File SHORTCUT_TASKS_SCRIPTS_DIR = new File(SHORTCUT_TASKS_SCRIPTS_PATH);
    public static final File TASKER_SCRIPTS_DIR = new File(TASKER_SCRIPTS_PATH);



    // Android OS permission declared by Termux app in AndroidManifest.xml which can be requested by 3rd party apps to run various commands in Termux app context
    public static final String PERMISSION_RUN_COMMAND = TERMUX_PACKAGE_NAME + ".permission.RUN_COMMAND"; // Default: "com.termux.permission.RUN_COMMAND"

    // Termux property defined in termux.properties file as a secondary check to PERMISSION_RUN_COMMAND to allow 3rd party apps to run various commands in Termux app context
    public static final String PROP_ALLOW_EXTERNAL_APPS = "allow-external-apps"; // Default: "allow-external-apps"
    public static final String PROP_DEFAULT_VALUE_ALLOW_EXTERNAL_APPS = "false"; // Default: "false"


    /**
     * Termux app constants.
     */

    public static final class TERMUX_APP {

        /**
         * Termux app core activity.
         */

        public static final String TERMUX_ACTIVITY_NAME = TERMUX_PACKAGE_NAME + ".app.TermuxActivity"; // Default: "com.termux.app.TermuxActivity"
        public static final class TERMUX_ACTIVITY {

            // Intent action to start termux failsafe session
            public static final String ACTION_FAILSAFE_SESSION = TermuxConstants.TERMUX_PACKAGE_NAME + ".app.failsafe_session"; // Default: "com.termux.app.failsafe_session"

            // Intent action to make termux reload its termux session styling
            public static final String ACTION_RELOAD_STYLE = TermuxConstants.TERMUX_PACKAGE_NAME + ".app.reload_style"; // Default: "com.termux.app.reload_style"
            // Intent extra for what to reload for the TERMUX_ACTIVITY.ACTION_RELOAD_STYLE intent
            public static final String EXTRA_RELOAD_STYLE = TermuxConstants.TERMUX_PACKAGE_NAME + ".app.reload_style"; // Default: "com.termux.app.reload_style"

        }



        /**
         * Termux app core service.
         */

        public static final String TERMUX_SERVICE_NAME = TERMUX_PACKAGE_NAME + ".app.TermuxService"; // Default: "com.termux.app.TermuxService"
        public static final class TERMUX_SERVICE {

            // Intent action to stop TERMUX_SERVICE
            public static final String ACTION_STOP_SERVICE = TERMUX_PACKAGE_NAME + ".service_stop"; // Default: "com.termux.service_stop"

            // Intent action to make TERMUX_SERVICE acquire a wakelock
            public static final String ACTION_WAKE_LOCK = TERMUX_PACKAGE_NAME + ".service_wake_lock"; // Default: "com.termux.service_wake_lock"

            // Intent action to make TERMUX_SERVICE release wakelock
            public static final String ACTION_WAKE_UNLOCK = TERMUX_PACKAGE_NAME + ".service_wake_unlock"; // Default: "com.termux.service_wake_unlock"

            // Intent action to execute command with TERMUX_SERVICE
            public static final String ACTION_SERVICE_EXECUTE = TERMUX_PACKAGE_NAME + ".service_execute"; // Default: "com.termux.service_execute"
            // Uri scheme for paths sent via intent to TERMUX_SERVICE
            public static final String URI_SCHEME_SERVICE_EXECUTE = TERMUX_PACKAGE_NAME + ".file"; // Default: "com.termux.file"
            // Intent extra for command arguments for the TERMUX_SERVICE.ACTION_SERVICE_EXECUTE intent
            public static final String EXTRA_ARGUMENTS = TERMUX_PACKAGE_NAME + ".execute.arguments"; // Default: "com.termux.execute.arguments"
            // Intent extra for command current working directory for the TERMUX_SERVICE.ACTION_SERVICE_EXECUTE intent
            public static final String EXTRA_WORKDIR = TERMUX_PACKAGE_NAME + ".execute.cwd"; // Default: "com.termux.execute.cwd"
            // Intent extra for command background mode for the TERMUX_SERVICE.ACTION_SERVICE_EXECUTE intent
            public static final String EXTRA_BACKGROUND = TERMUX_PACKAGE_NAME + ".execute.background"; // Default: "com.termux.execute.background"

        }



        /**
         * Termux app service to receive commands sent by 3rd party apps.
         */

        public static final String RUN_COMMAND_SERVICE_NAME = TERMUX_PACKAGE_NAME + ".app.RunCommandService"; // Termux app service to receive commands from 3rd party apps "com.termux.app.RunCommandService"
        public static final class RUN_COMMAND_SERVICE {

            // Intent action to execute command with RUN_COMMAND_SERVICE
            public static final String ACTION_RUN_COMMAND = TERMUX_PACKAGE_NAME + ".RUN_COMMAND"; // Default: "com.termux.RUN_COMMAND"
            // Intent extra for command path for the RUN_COMMAND_SERVICE.ACTION_RUN_COMMAND intent
            public static final String EXTRA_COMMAND_PATH = TERMUX_PACKAGE_NAME + ".RUN_COMMAND_PATH"; // Default: "com.termux.RUN_COMMAND_PATH"
            // Intent extra for command arguments for the RUN_COMMAND_SERVICE.ACTION_RUN_COMMAND intent
            public static final String EXTRA_ARGUMENTS = TERMUX_PACKAGE_NAME + ".RUN_COMMAND_ARGUMENTS"; // Default: "com.termux.RUN_COMMAND_ARGUMENTS"
            // Intent extra for command current working directory for the RUN_COMMAND_SERVICE.ACTION_RUN_COMMAND intent
            public static final String EXTRA_WORKDIR = TERMUX_PACKAGE_NAME + ".RUN_COMMAND_WORKDIR"; // Default: "com.termux.RUN_COMMAND_WORKDIR"
            // Intent extra for command background mode for the RUN_COMMAND_SERVICE.ACTION_RUN_COMMAND intent
            public static final String EXTRA_BACKGROUND = TERMUX_PACKAGE_NAME + ".RUN_COMMAND_BACKGROUND"; // Default: "com.termux.RUN_COMMAND_BACKGROUND"

        }
    }



    /**
     * Termux:Styling app constants.
     */

    public static final class TERMUX_STYLING {

        /**
         * Termux:Styling app core activity constants.
         */

        public static final String TERMUX_STYLING_ACTIVITY_NAME = TERMUX_STYLING_PACKAGE_NAME + ".TermuxStyleActivity"; // Default: "com.termux.styling.TermuxStyleActivity"

    }

}
