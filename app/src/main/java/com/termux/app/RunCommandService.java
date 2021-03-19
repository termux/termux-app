package com.termux.app;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Binder;
import android.os.Build;
import android.os.IBinder;
import android.util.Log;

import com.termux.R;
import com.termux.app.TermuxConstants.TERMUX_APP.RUN_COMMAND_SERVICE;
import com.termux.app.TermuxConstants.TERMUX_APP.TERMUX_SERVICE;
import com.termux.app.utils.FileUtils;
import com.termux.app.utils.Logger;
import com.termux.app.utils.PluginUtils;

import java.util.Arrays;
import java.util.HashMap;

/**
 * Third-party apps that are not part of termux world can run commands in termux context by either
 * sending an intent to RunCommandService or becoming a plugin host for the termux-tasker plugin
 * client.
 *
 * For the {@link RUN_COMMAND_SERVICE#ACTION_RUN_COMMAND} intent to work, here are the requirements:
 *
 * 1. `com.termux.permission.RUN_COMMAND` permission (Mandatory)
 *      The Intent sender/third-party app must request the `com.termux.permission.RUN_COMMAND`
 *      permission in its `AndroidManifest.xml` and it should be granted by user to the app through the
 *      app's `App Info` `Permissions` page in Android Settings, likely under `Additional Permissions`.
 *      This is a security measure to prevent any other apps from running commands in `Termux` context
 *      which do not have the required permission granted to them.
 *
 *      For `Tasker` you can grant it with:
 *      `Android Settings` -> `Apps` -> `Tasker` -> `Permissions` -> `Additional permissions` ->
 *      `Run commands in Termux environment`.
 *
 * 2. `allow-external-apps` property (Mandatory)
 *      The `allow-external-apps` property must be set to "true" in `~/.termux/termux.properties` in
 *      Termux app, regardless of if the executable path is inside or outside the `~/.termux/tasker/`
 *      directory. Check https://github.com/termux/termux-tasker#allow-external-apps-property-optional
 *      for more info.
 *
 * 3. `Draw Over Apps` permission (Optional)
 *      For android `>= 10` there are new
 *      [restrictions](https://developer.android.com/guide/components/activities/background-starts)
 *      that prevent activities from starting from the background. This prevents the background
 *      {@link TermuxService} from starting a terminal session in the foreground and running the
 *      commands until the user manually clicks `Termux` notification in the status bar dropdown
 *      notifications list. This only affects commands that are to be executed in a terminal
 *      session and not the background ones. `Termux` version `>= 0.100`
 *      requests the `Draw Over Apps` permission so that users can bypass this restriction so
 *      that commands can automatically start running without user intervention.
 *      You can grant `Termux` the `Draw Over Apps` permission from its `App Info` activity:
 *      `Android Settings` -> `Apps` -> `Termux` -> `Advanced` -> `Draw over other apps`.
 *
 * 4. `Storage` permission (Optional)
 *      Termux app must be granted `Storage` permission if the executable is accessing or working
 *      directory is set to path in external shared storage. The common paths which usually refer to
 *      it are `~/storage`, `/sdcard`, `/storage/emulated/0` etc.
 *       You can grant `Termux` the `Storage` permission from its `App Info` activity:
 *       For Android version < 11:
 *          `Android Settings` -> `Apps` -> `Termux` -> `Permissions` -> `Storage`.
 *       For Android version >= 11
 *          `Android Settings` -> `Apps` -> `Termux` -> `Permissions` -> `Files and media` ->
 *          `Allowed management of all files`.
 *       NOTE: For Android version >= 11, sometimes you will get `Permission Denied` errors for
 *       external shared storage even when you have granted `Files and media` permission. To solve
 *       this, Deny the permission and then Allow it again and restart Termux.
 *       Also check https://wiki.termux.com/wiki/Termux-setup-storage
 *
 * 5. Battery Optimizations (May be mandatory depending on device)
 *      Some devices kill apps aggressively or prevent apps from starting from background.
 *      If Termux is running into such problems, then exempt it from such restrictions.
 *      The user may also disable battery optimizations for Termux to reduce the chances of Termux
 *      being killed by Android even further due to violation of not being able to call
 *      `startForeground()` within ~5s of service start in android >= 8.
 *      Check https://dontkillmyapp.com/ for device specfic info to opt-out of battery optimiations.
 *
 *  You may also want to check https://github.com/termux/termux-tasker
 *
 *
 *
 * The {@link RUN_COMMAND_SERVICE#ACTION_RUN_COMMAND} intent expects the following extras:
 *
 * 1. The {@code String} {@link RUN_COMMAND_SERVICE#EXTRA_COMMAND_PATH} extra for absolute path of
 *      command. This is mandatory.
 * 2. The {@code String[]} {@link RUN_COMMAND_SERVICE#EXTRA_ARGUMENTS} extra for any arguments to
 *      pass to command. This is optional.
 * 3. The {@code String} {@link RUN_COMMAND_SERVICE#EXTRA_WORKDIR} extra for current working directory
 *      of command. This is optional and defaults to {@link TermuxConstants#TERMUX_HOME_DIR_PATH}.
 * 4. The {@code boolean} {@link RUN_COMMAND_SERVICE#EXTRA_BACKGROUND} extra whether to run command
 *      in background or foreground terminal session. This is optional and defaults to {@code false}.
 * 5. The {@code String} {@link RUN_COMMAND_SERVICE#EXTRA_SESSION_ACTION} extra for for session action
 *      of foreground commands. This is optional and defaults to
 *      {@link TERMUX_SERVICE#VALUE_EXTRA_SESSION_ACTION_SWITCH_TO_NEW_SESSION_AND_OPEN_ACTIVITY}.
 *
 *
 *
 * The {@link RUN_COMMAND_SERVICE#EXTRA_COMMAND_PATH} and {@link RUN_COMMAND_SERVICE#EXTRA_WORKDIR}
 * can optionally be prefixed with "$PREFIX/" or "~/" if an absolute path is not to be given.
 * The "$PREFIX/" will expand to {@link TermuxConstants#TERMUX_PREFIX_DIR_PATH} and
 * "~/" will expand to {@link TermuxConstants#TERMUX_HOME_DIR_PATH}, followed by a forward slash "/".
 *
 * If your third-party app is targeting sdk 30 (android 11), then it needs to add `com.termux`
 * package to the `queries` element or request `QUERY_ALL_PACKAGES` permission in its
 * `AndroidManifest.xml`. Otherwise it will get `PackageSetting{...... com.termux/......} BLOCKED`
 * errors in logcat and `RUN_COMMAND` won't work.
 * https://developer.android.com/training/basics/intents/package-visibility#package-name
 *
 *
 *
 * Sample code to run command "top" with java:
 *   Intent intent = new Intent();
 *   intent.setClassName("com.termux", "com.termux.app.RunCommandService");
 *   intent.setAction("com.termux.RUN_COMMAND");
 *   intent.putExtra("com.termux.RUN_COMMAND_PATH", "/data/data/com.termux/files/usr/bin/top");
 *   intent.putExtra("com.termux.RUN_COMMAND_ARGUMENTS", new String[]{"-n", "5"});
 *   intent.putExtra("com.termux.RUN_COMMAND_WORKDIR", "/data/data/com.termux/files/home");
 *   intent.putExtra("com.termux.RUN_COMMAND_BACKGROUND", false);
 *   intent.putExtra("com.termux.RUN_COMMAND_SESSION_ACTION", "0");
 *   startService(intent);
 *
 * Sample code to run command "top" with "am startservice" command:
 * am startservice --user 0 -n com.termux/com.termux.app.RunCommandService \
 * -a com.termux.RUN_COMMAND \
 * --es com.termux.RUN_COMMAND_PATH '/data/data/com.termux/files/usr/bin/top' \
 * --esa com.termux.RUN_COMMAND_ARGUMENTS '-n,5' \
 * --es com.termux.RUN_COMMAND_WORKDIR '/data/data/com.termux/files/home' \
 * --ez com.termux.RUN_COMMAND_BACKGROUND 'false' \
 * --es com.termux.RUN_COMMAND_SESSION_ACTION '0'
 */
public class RunCommandService extends Service {

    private static final String NOTIFICATION_CHANNEL_ID = "termux_run_command_notification_channel";
    private static final int NOTIFICATION_ID = 1338;

    private static final String LOG_TAG = "RunCommandService";

    class LocalBinder extends Binder {
        public final RunCommandService service = RunCommandService.this;
    }

    private final IBinder mBinder = new RunCommandService.LocalBinder();

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    @Override
    public void onCreate() {
        Logger.logVerbose(LOG_TAG, "onCreate");
        runStartForeground();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Logger.logDebug(LOG_TAG, "onStartCommand");

        // Run again in case service is already started and onCreate() is not called
        runStartForeground();

        String errmsg;

        // If invalid action passed, then just return
        if (!RUN_COMMAND_SERVICE.ACTION_RUN_COMMAND.equals(intent.getAction())) {
            errmsg = this.getString(R.string.run_command_service_invalid_action, intent.getAction());
            Logger.logError(LOG_TAG, errmsg);
            return Service.START_NOT_STICKY;
        }

        // If "allow-external-apps" property to not set to "true", then just return
        errmsg = PluginUtils.checkIfRunCommandServiceAllowExternalAppsPolicyIsViolated(this);
        if (errmsg != null) {
            Logger.logError(LOG_TAG, errmsg);
            return Service.START_NOT_STICKY;
        }


        String executable = intent.getStringExtra(RUN_COMMAND_SERVICE.EXTRA_COMMAND_PATH);
        String[] arguments = intent.getStringArrayExtra(RUN_COMMAND_SERVICE.EXTRA_ARGUMENTS);
        boolean inBackground = intent.getBooleanExtra(RUN_COMMAND_SERVICE.EXTRA_BACKGROUND, false);
        String workingDirectory = intent.getStringExtra(RUN_COMMAND_SERVICE.EXTRA_WORKDIR);
        String sessionAction = intent.getStringExtra(RUN_COMMAND_SERVICE.EXTRA_SESSION_ACTION);

        // Get canonical path of executable
        executable = FileUtils.getCanonicalPath(executable, null, true);

        // If executable is not a regular file, or is not readable or executable, then just return
        // Setting of missing read and execute permissions is not done
        errmsg = FileUtils.validateRegularFileExistenceAndPermissions(this, executable,
            null, PluginUtils.PLUGIN_EXECUTABLE_FILE_PERMISSIONS,
            false, false);
        if (errmsg != null) {
            errmsg  += "\n" + this.getString(R.string.executable_absolute_path, executable);
            Logger.logError(LOG_TAG, errmsg);
            return Service.START_NOT_STICKY;
        }

        // If workingDirectory is not null or empty
        if (workingDirectory != null && !workingDirectory.isEmpty()) {
            // Get canonical path of workingDirectory
            workingDirectory = FileUtils.getCanonicalPath(workingDirectory, null, true);

            // If workingDirectory is not a directory, or is not readable or writable, then just return
            // Creation of missing directory and setting of read, write and execute permissions are only done if workingDirectory is
            // under {@link TermuxConstants#TERMUX_FILES_DIR_PATH}
            // We try to set execute permissions, but ignore if they are missing, since only read and write permissions are required
            // for working directories.
            errmsg = FileUtils.validateDirectoryExistenceAndPermissions(this, workingDirectory,
                TermuxConstants.TERMUX_FILES_DIR_PATH, PluginUtils.PLUGIN_WORKING_DIRECTORY_PERMISSIONS,
                true, true, false,
                true);
            if (errmsg != null) {
                errmsg  += "\n" + this.getString(R.string.working_directory_absolute_path, workingDirectory);
                Logger.logError(LOG_TAG, errmsg);
                return Service.START_NOT_STICKY;
            }
        }

        PluginUtils.dumpExecutionIntentToLog(Log.VERBOSE, LOG_TAG, "RUN_COMMAND Intent", executable, Arrays.asList(arguments), workingDirectory, inBackground, new HashMap<String, Object>() {{
            put("sessionAction", sessionAction);
        }});

        Uri executableUri = new Uri.Builder().scheme(TERMUX_SERVICE.URI_SCHEME_SERVICE_EXECUTE).path(FileUtils.getExpandedTermuxPath(executable)).build();

        // Create execution intent with the action TERMUX_SERVICE#ACTION_SERVICE_EXECUTE to be sent to the TERMUX_SERVICE
        Intent execIntent = new Intent(TERMUX_SERVICE.ACTION_SERVICE_EXECUTE, executableUri);
        execIntent.setClass(this, TermuxService.class);
        execIntent.putExtra(TERMUX_SERVICE.EXTRA_ARGUMENTS, arguments);
        execIntent.putExtra(TERMUX_SERVICE.EXTRA_BACKGROUND, inBackground);
        if (workingDirectory != null && !workingDirectory.isEmpty()) execIntent.putExtra(TERMUX_SERVICE.EXTRA_WORKDIR, workingDirectory);
        execIntent.putExtra(TERMUX_SERVICE.EXTRA_SESSION_ACTION, sessionAction);


        // Start TERMUX_SERVICE and pass it execution intent
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            this.startForegroundService(execIntent);
        } else {
            this.startService(execIntent);
        }

        runStopForeground();

        return Service.START_NOT_STICKY;
    }

    private void runStartForeground() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            setupNotificationChannel();
            startForeground(NOTIFICATION_ID, buildNotification());
        }
    }

    private void runStopForeground() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            stopForeground(true);
        }
    }

    private Notification buildNotification() {
        Notification.Builder builder = new Notification.Builder(this);
        builder.setContentTitle(TermuxConstants.TERMUX_APP_NAME + " Run Command");
        builder.setSmallIcon(R.drawable.ic_service_notification);

        // Use a low priority:
        builder.setPriority(Notification.PRIORITY_LOW);

        // No need to show a timestamp:
        builder.setShowWhen(false);

        // Background color for small notification icon:
        builder.setColor(0xFF607D8B);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            builder.setChannelId(NOTIFICATION_CHANNEL_ID);
        }

        return builder.build();
    }

    private void setupNotificationChannel() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) return;

        String channelName = TermuxConstants.TERMUX_APP_NAME + " Run Command";
        int importance = NotificationManager.IMPORTANCE_LOW;

        NotificationChannel channel = new NotificationChannel(NOTIFICATION_CHANNEL_ID, channelName, importance);
        NotificationManager manager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
        manager.createNotificationChannel(channel);
    }

}
