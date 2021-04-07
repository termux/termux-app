package com.termux.app;

import android.app.Activity;
import android.app.IntentService;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Intent;
import android.net.Uri;
import android.os.Binder;
import android.os.Build;
import android.os.IBinder;

import com.termux.R;
import com.termux.shared.termux.TermuxConstants;
import com.termux.shared.termux.TermuxConstants.TERMUX_APP.RUN_COMMAND_SERVICE;
import com.termux.shared.termux.TermuxConstants.TERMUX_APP.TERMUX_SERVICE;
import com.termux.shared.file.FileUtils;
import com.termux.shared.logger.Logger;
import com.termux.shared.notification.NotificationUtils;
import com.termux.app.utils.PluginUtils;
import com.termux.shared.data.DataUtils;
import com.termux.app.models.ExecutionCommand;

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
 * 1. The **mandatory** {@code String} {@link RUN_COMMAND_SERVICE#EXTRA_COMMAND_PATH} extra for
 *      absolute path of command.
 * 2. The {@code String[]} {@link RUN_COMMAND_SERVICE#EXTRA_ARGUMENTS} extra for any arguments to
 *      pass to command.
 * 3. The {@code String} {@link RUN_COMMAND_SERVICE#EXTRA_WORKDIR} extra for current working directory
 *      of command. This defaults to {@link TermuxConstants#TERMUX_HOME_DIR_PATH}.
 * 4. The {@code boolean} {@link RUN_COMMAND_SERVICE#EXTRA_BACKGROUND} extra whether to run command
 *      in background or foreground terminal session. This defaults to {@code false}.
 * 5. The {@code String} {@link RUN_COMMAND_SERVICE#EXTRA_SESSION_ACTION} extra for for session action
 *      of foreground commands. This defaults to
 *      {@link TERMUX_SERVICE#VALUE_EXTRA_SESSION_ACTION_SWITCH_TO_NEW_SESSION_AND_OPEN_ACTIVITY}.
 * 6. The {@code String} {@link RUN_COMMAND_SERVICE#EXTRA_COMMAND_LABEL} extra for label of the command.
 * 7. The markdown {@code String} {@link RUN_COMMAND_SERVICE#EXTRA_COMMAND_DESCRIPTION} extra for
 *      description of the command. This should ideally be get short.
 * 8. The markdown {@code String} {@link RUN_COMMAND_SERVICE#EXTRA_COMMAND_HELP} extra for help of
 *      the command. This can add details about the command. 3rd party apps can provide more info
 *      to users for setting up commands. Ideally a url link should be provided that goes into full
 *      details.
 * 9. The {@code Parcelable} {@link RUN_COMMAND_SERVICE#EXTRA_PENDING_INTENT} extra containing the
 *      pending intent with which result of commands should be returned to the caller. The results
 *      will be sent in the {@link TERMUX_SERVICE#EXTRA_PLUGIN_RESULT_BUNDLE} bundle. This is optional
 *      and only needed if caller wants the results back.
 *
 *
 * The {@link RUN_COMMAND_SERVICE#EXTRA_COMMAND_PATH} and {@link RUN_COMMAND_SERVICE#EXTRA_WORKDIR}
 * can optionally be prefixed with "$PREFIX/" or "~/" if an absolute path is not to be given.
 * The "$PREFIX/" will expand to {@link TermuxConstants#TERMUX_PREFIX_DIR_PATH} and
 * "~/" will expand to {@link TermuxConstants#TERMUX_HOME_DIR_PATH}, followed by a forward slash "/".
 *
 *
 * The `EXTRA_COMMAND_*` extras are used for logging and are their values are provided to users in case
 * of failure in a popup. The popup shown is in commonmark-spec markdown using markwon library so
 * make sure to follow its formatting rules. Also make sure to end lines with 2 blank spaces to prevent
 * word-wrap wherever needed.
 * It's the users and 3rd party apps responsibility to use them wisely. There are also android
 * internal intent size limits (roughly 500KB) that must not exceed when sending intents so make sure
 * the combined size of ALL extras is less than that.
 * There are also limits on the arguments size you can pass to commands or the full command string
 * length that can be run, which is likely equal to 131072 bytes or 128KB on an android device.
 * Check https://github.com/termux/termux-tasker#arguments-and-result-data-limits for more details.
 *
 *
 *
 * If your third-party app is targeting sdk 30 (android 11), then it needs to add `com.termux`
 * package to the `queries` element or request `QUERY_ALL_PACKAGES` permission in its
 * `AndroidManifest.xml`. Otherwise it will get `PackageSetting{...... com.termux/......} BLOCKED`
 * errors in logcat and `RUN_COMMAND` won't work.
 * https://developer.android.com/training/basics/intents/package-visibility#package-name
 *
 *
 * Its probably wiser for apps to import the {@link TermuxConstants} class and use the variables
 * provided for actions and extras instead of using hardcoded string values.
 *
 * Sample code to run command "top" with java:
 * ```
 * intent.setClassName("com.termux", "com.termux.app.RunCommandService");
 * intent.setAction("com.termux.RUN_COMMAND");
 * intent.putExtra("com.termux.RUN_COMMAND_PATH", "/data/data/com.termux/files/usr/bin/top");
 * intent.putExtra("com.termux.RUN_COMMAND_ARGUMENTS", new String[]{"-n", "5"});
 * intent.putExtra("com.termux.RUN_COMMAND_WORKDIR", "/data/data/com.termux/files/home");
 * intent.putExtra("com.termux.RUN_COMMAND_BACKGROUND", false);
 * intent.putExtra("com.termux.RUN_COMMAND_SESSION_ACTION", "0");
 * startService(intent);
 * ```
 *
 * Sample code to run command "top" with "am startservice" command:
 * ```
 * am startservice --user 0 -n com.termux/com.termux.app.RunCommandService \
 * -a com.termux.RUN_COMMAND \
 * --es com.termux.RUN_COMMAND_PATH '/data/data/com.termux/files/usr/bin/top' \
 * --esa com.termux.RUN_COMMAND_ARGUMENTS '-n,5' \
 * --es com.termux.RUN_COMMAND_WORKDIR '/data/data/com.termux/files/home' \
 * --ez com.termux.RUN_COMMAND_BACKGROUND 'false' \
 * --es com.termux.RUN_COMMAND_SESSION_ACTION '0'
 *
 *
 *
 *
 * The {@link RUN_COMMAND_SERVICE#ACTION_RUN_COMMAND} intent returns the following extras
 * in the {@link TERMUX_SERVICE#EXTRA_PLUGIN_RESULT_BUNDLE} bundle if a pending intent is sent by the
 * called in {@code Parcelable} {@link RUN_COMMAND_SERVICE#EXTRA_PENDING_INTENT} extra:
 *
 * For foreground commands ({@link RUN_COMMAND_SERVICE#EXTRA_BACKGROUND} is `false`):
 * - {@link TERMUX_SERVICE#EXTRA_PLUGIN_RESULT_BUNDLE_STDOUT} will contain session transcript.
 * - {@link TERMUX_SERVICE#EXTRA_PLUGIN_RESULT_BUNDLE_STDERR} will be null since its not used.
 * - {@link TERMUX_SERVICE#EXTRA_PLUGIN_RESULT_BUNDLE_EXIT_CODE} will contain exit code of session.

 * For background commands ({@link RUN_COMMAND_SERVICE#EXTRA_BACKGROUND} is `true`):
 * - {@link TERMUX_SERVICE#EXTRA_PLUGIN_RESULT_BUNDLE_STDOUT} will contain stdout of commands.
 * - {@link TERMUX_SERVICE#EXTRA_PLUGIN_RESULT_BUNDLE_STDERR} will contain stderr of commands.
 * - {@link TERMUX_SERVICE#EXTRA_PLUGIN_RESULT_BUNDLE_EXIT_CODE} will contain exit code of command.
 *
 * The {@link TERMUX_SERVICE#EXTRA_PLUGIN_RESULT_BUNDLE_STDOUT_ORIGINAL_LENGTH} and
 * {@link TERMUX_SERVICE#EXTRA_PLUGIN_RESULT_BUNDLE_STDERR_ORIGINAL_LENGTH} will contain
 * the original length of stdout and stderr respectively. This is useful to detect cases where
 * stdout and stderr was too large to be sent back via an intent, otherwise
 *
 * The internal errors raised by termux outside the shell will be sent in the the
 * {@link TERMUX_SERVICE#EXTRA_PLUGIN_RESULT_BUNDLE_ERR} and {@link TERMUX_SERVICE#EXTRA_PLUGIN_RESULT_BUNDLE_ERRMSG}
 * extras. These will contain errors like if starting a termux command failed or if the user manually
 * exited the termux sessions or android killed the termux service before the commands had finished executing.
 * The err value will be {@link Activity#RESULT_OK}(-1) if no internal errors are raised.
 *
 * Note that if stdout or stderr are too large in length, then a {@link android.os.TransactionTooLargeException}
 * exception will be raised when the pending intent is sent back containing the results, But it cannot
 * be caught by the intent sender and intent will silently fail with logcat entries for the exception
 * raised internally by android os components. To prevent this, the stdout and stderr sent
 * back will be truncated from the start to max 100KB combined. The original length of stdout and
 * stderr will be provided in
 * {@link TERMUX_SERVICE#EXTRA_PLUGIN_RESULT_BUNDLE_STDOUT_ORIGINAL_LENGTH} and
 * {@link TERMUX_SERVICE#EXTRA_PLUGIN_RESULT_BUNDLE_STDERR_ORIGINAL_LENGTH} extras respectively, so
 * that the caller can check if either of them were truncated. The errmsg will also be truncated
 * from end to max 25KB to preserve start of stacktraces.
 *
 *
 *
 * If your app (not shell) wants to receive termux session command results, then put the
 * pending intent for your app like for an {@link IntentService} in the "com.termux.RUN_COMMAND_PENDING_INTENT"
 * extra.
 * ```
 * // Create intent for your IntentService class
 * Intent pluginResultsServiceIntent = new Intent(MainActivity.this, PluginResultsService.class);
 * // Create PendingIntent that will be used by termux service to send result of commands back to PluginResultsService
 * PendingIntent pendingIntent = PendingIntent.getService(context, 1, pluginResultsServiceIntent, PendingIntent.FLAG_ONE_SHOT);
 * intent.putExtra("com.termux.RUN_COMMAND_PENDING_INTENT", pendingIntent);
 * ```
 *
 *
 * Declare `PluginResultsService` entry in AndroidManifest.xml
 * ```
 * <service android:name=".PluginResultsService" />
 * ```
 *
 *
 * Define the `PluginResultsService` class
 * ```
 * public class PluginResultsService extends IntentService {
 *
 *     public static final String PLUGIN_SERVICE_LABEL = "PluginResultsService";
 *
 *     private static final String LOG_TAG = "PluginResultsService";
 *
 *     public PluginResultsService(){
 *         super(PLUGIN_SERVICE_LABEL);
 *     }
 *
 *     @Override
 *     protected void onHandleIntent(@Nullable Intent intent) {
 *         if (intent == null) return;
 *
 *         if(intent.getComponent() != null)
 *             Log.d(LOG_TAG, PLUGIN_SERVICE_LABEL + " received execution result from " + intent.getComponent().toString());
 *
 *
 *         final Bundle resultBundle = intent.getBundleExtra("result");
 *         if (resultBundle == null) {
 *             Log.e(LOG_TAG, "The intent does not contain the result bundle at the \"result\" key.");
 *             return;
 *         }
 *
 *         Log.d(LOG_TAG, "stdout:\n```\n" + resultBundle.getString("stdout", "") + "\n```\n" +
 *                 "stdout_original_length: `" + resultBundle.getString("stdout_original_length") + "`\n" +
 *                 "stderr:\n```\n" + resultBundle.getString("stderr", "") + "\n```\n" +
 *                 "stderr_original_length: `" + resultBundle.getString("stderr_original_length") + "`\n" +
 *                 "exitCode: `" + resultBundle.getInt("exitCode") + "`\n" +
 *                 "errCode: `" + resultBundle.getInt("err") + "`\n" +
 *                 "errmsg: `" + resultBundle.getString("errmsg", "") + "`");
 *     }
 *
 * }
 *```
 *
 *
 *
 *
 *
 * A service that receives {@link RUN_COMMAND_SERVICE#ACTION_RUN_COMMAND} intent from third party apps and
 * plugins that contains info on command execution and forwards the extras to {@link TermuxService}
 * for the actual execution.
 */
public class RunCommandService extends Service {

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

        if(intent == null) return Service.START_NOT_STICKY;

        // Run again in case service is already started and onCreate() is not called
        runStartForeground();

        ExecutionCommand executionCommand = new ExecutionCommand();
        executionCommand.pluginAPIHelp = this.getString(R.string.error_run_command_service_api_help, RUN_COMMAND_SERVICE.RUN_COMMAND_API_HELP_URL);

        String errmsg;

        // If invalid action passed, then just return
        if (!RUN_COMMAND_SERVICE.ACTION_RUN_COMMAND.equals(intent.getAction())) {
            errmsg = this.getString(R.string.error_run_command_service_invalid_intent_action, intent.getAction());
            executionCommand.setStateFailed(ExecutionCommand.RESULT_CODE_FAILED, errmsg, null);
            PluginUtils.processPluginExecutionCommandError(this, LOG_TAG, executionCommand, false);
            return Service.START_NOT_STICKY;
        }

        executionCommand.executable = intent.getStringExtra(RUN_COMMAND_SERVICE.EXTRA_COMMAND_PATH);
        executionCommand.arguments = intent.getStringArrayExtra(RUN_COMMAND_SERVICE.EXTRA_ARGUMENTS);
        executionCommand.workingDirectory = intent.getStringExtra(RUN_COMMAND_SERVICE.EXTRA_WORKDIR);
        executionCommand.inBackground = intent.getBooleanExtra(RUN_COMMAND_SERVICE.EXTRA_BACKGROUND, false);
        executionCommand.sessionAction = intent.getStringExtra(RUN_COMMAND_SERVICE.EXTRA_SESSION_ACTION);
        executionCommand.commandLabel = DataUtils.getDefaultIfNull(intent.getStringExtra(RUN_COMMAND_SERVICE.EXTRA_COMMAND_LABEL), "RUN_COMMAND Execution Intent Command");
        executionCommand.commandDescription = intent.getStringExtra(RUN_COMMAND_SERVICE.EXTRA_COMMAND_DESCRIPTION);
        executionCommand.commandHelp = intent.getStringExtra(RUN_COMMAND_SERVICE.EXTRA_COMMAND_HELP);
        executionCommand.isPluginExecutionCommand = true;
        executionCommand.pluginPendingIntent = intent.getParcelableExtra(RUN_COMMAND_SERVICE.EXTRA_PENDING_INTENT);



        // If "allow-external-apps" property to not set to "true", then just return
        // We enable force notifications if "allow-external-apps" policy is violated so that the
        // user knows someone tried to run a command in termux context, since it may be malicious
        // app or imported (tasker) plugin project and not the user himself. If a pending intent is
        // also sent, then its creator is also logged and shown.
        errmsg = PluginUtils.checkIfRunCommandServiceAllowExternalAppsPolicyIsViolated(this);
        if (errmsg != null) {
            executionCommand.setStateFailed(ExecutionCommand.RESULT_CODE_FAILED, errmsg, null);
            PluginUtils.processPluginExecutionCommandError(this, LOG_TAG, executionCommand, true);
            return Service.START_NOT_STICKY;
        }



        // If executable is null or empty, then exit here instead of getting canonical path which would expand to "/"
        if (executionCommand.executable == null || executionCommand.executable.isEmpty()) {
            errmsg  = this.getString(R.string.error_run_command_service_mandatory_extra_missing, RUN_COMMAND_SERVICE.EXTRA_COMMAND_PATH);
            executionCommand.setStateFailed(ExecutionCommand.RESULT_CODE_FAILED, errmsg, null);
            PluginUtils.processPluginExecutionCommandError(this, LOG_TAG, executionCommand, false);
            return Service.START_NOT_STICKY;
        }

        // Get canonical path of executable
        executionCommand.executable = FileUtils.getCanonicalPath(executionCommand.executable, null, true);

        // If executable is not a regular file, or is not readable or executable, then just return
        // Setting of missing read and execute permissions is not done
        errmsg = FileUtils.validateRegularFileExistenceAndPermissions(this, "executable", executionCommand.executable, null,
            PluginUtils.PLUGIN_EXECUTABLE_FILE_PERMISSIONS, true, true,
            false);
        if (errmsg != null) {
            errmsg  += "\n" + this.getString(R.string.msg_executable_absolute_path, executionCommand.executable);
            executionCommand.setStateFailed(ExecutionCommand.RESULT_CODE_FAILED, errmsg, null);
            PluginUtils.processPluginExecutionCommandError(this, LOG_TAG, executionCommand, false);
            return Service.START_NOT_STICKY;
        }



        // If workingDirectory is not null or empty
        if (executionCommand.workingDirectory != null && !executionCommand.workingDirectory.isEmpty()) {
            // Get canonical path of workingDirectory
            executionCommand.workingDirectory = FileUtils.getCanonicalPath(executionCommand.workingDirectory, null, true);

            // If workingDirectory is not a directory, or is not readable or writable, then just return
            // Creation of missing directory and setting of read, write and execute permissions are only done if workingDirectory is
            // under {@link TermuxConstants#TERMUX_FILES_DIR_PATH}
            // We try to set execute permissions, but ignore if they are missing, since only read and write permissions are required
            // for working directories.
            errmsg = FileUtils.validateDirectoryFileExistenceAndPermissions(this, "working", executionCommand.workingDirectory, TermuxConstants.TERMUX_FILES_DIR_PATH, true,
                PluginUtils.PLUGIN_WORKING_DIRECTORY_PERMISSIONS, true, true,
                true, true);
            if (errmsg != null) {
                errmsg  += "\n" + this.getString(R.string.msg_working_directory_absolute_path, executionCommand.workingDirectory);
                executionCommand.setStateFailed(ExecutionCommand.RESULT_CODE_FAILED, errmsg, null);
                PluginUtils.processPluginExecutionCommandError(this, LOG_TAG, executionCommand, false);
                return Service.START_NOT_STICKY;
            }
        }



        executionCommand.executableUri = new Uri.Builder().scheme(TERMUX_SERVICE.URI_SCHEME_SERVICE_EXECUTE).path(FileUtils.getExpandedTermuxPath(executionCommand.executable)).build();

        Logger.logVerbose(LOG_TAG, executionCommand.toString());

        // Create execution intent with the action TERMUX_SERVICE#ACTION_SERVICE_EXECUTE to be sent to the TERMUX_SERVICE
        Intent execIntent = new Intent(TERMUX_SERVICE.ACTION_SERVICE_EXECUTE, executionCommand.executableUri);
        execIntent.setClass(this, TermuxService.class);
        execIntent.putExtra(TERMUX_SERVICE.EXTRA_ARGUMENTS, executionCommand.arguments);
        if (executionCommand.workingDirectory != null && !executionCommand.workingDirectory.isEmpty()) execIntent.putExtra(TERMUX_SERVICE.EXTRA_WORKDIR, executionCommand.workingDirectory);
        execIntent.putExtra(TERMUX_SERVICE.EXTRA_BACKGROUND, executionCommand.inBackground);
        execIntent.putExtra(TERMUX_SERVICE.EXTRA_SESSION_ACTION, executionCommand.sessionAction);
        execIntent.putExtra(TERMUX_SERVICE.EXTRA_COMMAND_LABEL, executionCommand.commandLabel);
        execIntent.putExtra(TERMUX_SERVICE.EXTRA_COMMAND_DESCRIPTION, executionCommand.commandDescription);
        execIntent.putExtra(TERMUX_SERVICE.EXTRA_COMMAND_HELP, executionCommand.commandHelp);
        execIntent.putExtra(TERMUX_SERVICE.EXTRA_PLUGIN_API_HELP, executionCommand.pluginAPIHelp);
        execIntent.putExtra(TERMUX_SERVICE.EXTRA_PENDING_INTENT, executionCommand.pluginPendingIntent);

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
            startForeground(TermuxConstants.TERMUX_RUN_COMMAND_NOTIFICATION_ID, buildNotification());
        }
    }

    private void runStopForeground() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            stopForeground(true);
        }
    }

    private Notification buildNotification() {
        // Build the notification
        Notification.Builder builder =  NotificationUtils.geNotificationBuilder(this,
            TermuxConstants.TERMUX_RUN_COMMAND_NOTIFICATION_CHANNEL_ID, Notification.PRIORITY_LOW,
            TermuxConstants.TERMUX_RUN_COMMAND_NOTIFICATION_CHANNEL_NAME, null, null,
            null, NotificationUtils.NOTIFICATION_MODE_SILENT);
        if(builder == null)  return null;

        // No need to show a timestamp:
        builder.setShowWhen(false);

        // Set notification icon
        builder.setSmallIcon(R.drawable.ic_service_notification);

        // Set background color for small notification icon
        builder.setColor(0xFF607D8B);

        return builder.build();
    }

    private void setupNotificationChannel() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) return;

        NotificationUtils.setupNotificationChannel(this, TermuxConstants.TERMUX_RUN_COMMAND_NOTIFICATION_CHANNEL_ID,
            TermuxConstants.TERMUX_RUN_COMMAND_NOTIFICATION_CHANNEL_NAME, NotificationManager.IMPORTANCE_LOW);
    }

}
