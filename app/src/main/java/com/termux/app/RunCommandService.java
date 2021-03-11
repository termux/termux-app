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

import java.io.File;
import java.io.FileInputStream;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;
import java.util.Properties;

/**
 * Third-party apps that are not part of termux world can run commands in termux context by either
 * sending an intent to RunCommandService or becoming a plugin host for the termux-tasker plugin
 * client.
 *
 * For the RunCommandService intent to work, there are 2 main requirements:
 * 1. The `allow-external-apps` property must be set to "true" in ~/.termux/termux.properties in
 * termux app, regardless of if the executable path is inside or outside the `~/.termux/tasker/`
 * directory.
 * 2. The intent sender/third-party app must request the `com.termux.permission.RUN_COMMAND`
 * permission in its `AndroidManifest.xml` and it should be granted by user to the app through the
 * app's App Info permissions page in android settings, likely under Additional Permissions.
 *
 * The absolute path of executable or script must be given in "RUN_COMMAND_PATH" extra.
 * The "RUN_COMMAND_ARGUMENTS", "RUN_COMMAND_WORKDIR" and "RUN_COMMAND_BACKGROUND" extras are
 * optional. The workdir defaults to termux home. The background mode defaults to "false".
 * The command path and workdir can optionally be prefixed with "$PREFIX/" or "~/" if an absolute
 * path is not to be given.
 *
 * To automatically bring termux session to foreground and start termux commands that were started
 * with background mode "false" in android >= 10 without user having to click the notification
 * manually requires termux to be granted draw over apps permission due to new restrictions
 * of starting activities from the background, this also applies to Termux:Tasker plugin.
 *
 * Check https://github.com/termux/termux-tasker for more details on allow-external-apps and draw
 * over apps and other limitations.
 *
 * To reduce the chance of termux being killed by android even further due to violation of not
 * being able to call startForeground() within ~5s of service start in android >= 8, the user
 * may disable battery optimizations for termux.
 *
 * Sample code to run command "top" with java:
 *   Intent intent = new Intent();
 *   intent.setClassName("com.termux", "com.termux.app.RunCommandService");
 *   intent.setAction("com.termux.RUN_COMMAND");
 *   intent.putExtra("com.termux.RUN_COMMAND_PATH", "/data/data/com.termux/files/usr/bin/top");
 *   intent.putExtra("com.termux.RUN_COMMAND_ARGUMENTS", new String[]{"-n", "5"});
 *   intent.putExtra("com.termux.RUN_COMMAND_WORKDIR", "/data/data/com.termux/files/home");
 *   intent.putExtra("com.termux.RUN_COMMAND_BACKGROUND", false);
 *   startService(intent);
 *
 * Sample code to run command "top" with "am startservice" command:
 * am startservice --user 0 -n com.termux/com.termux.app.RunCommandService \
 * -a com.termux.RUN_COMMAND \
 * --es com.termux.RUN_COMMAND_PATH '/data/data/com.termux/files/usr/bin/top' \
 * --esa com.termux.RUN_COMMAND_ARGUMENTS '-n,5' \
 * --es com.termux.RUN_COMMAND_WORKDIR '/data/data/com.termux/files/home' \
 * --ez com.termux.RUN_COMMAND_BACKGROUND 'false'
 *
 * If your third-party app is targeting sdk 30 (android 11), then it needs to add `com.termux`
 * package to the `queries` element or request `QUERY_ALL_PACKAGES` permission in its
 * `AndroidManifest.xml`. Otherwise it will get `PackageSetting{...... com.termux/......} BLOCKED`
 * errors in logcat and `RUN_COMMAND` won't work.
 * https://developer.android.com/training/basics/intents/package-visibility#package-name
 */
public class RunCommandService extends Service {

    private static final String NOTIFICATION_CHANNEL_ID = "termux_run_command_notification_channel";
    private static final int NOTIFICATION_ID = 1338;

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
        runStartForeground();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        // Run again in case service is already started and onCreate() is not called
        runStartForeground();

        // If wrong action passed, then just return
        if (!RUN_COMMAND_SERVICE.ACTION_RUN_COMMAND.equals(intent.getAction())) {
            Log.e("termux", "Unexpected intent action to RunCommandService: " + intent.getAction());
            return Service.START_NOT_STICKY;
        }

        // If allow-external-apps property to not set to "true"
        if (!allowExternalApps()) {
            Log.e("termux", "RunCommandService requires allow-external-apps property to be set to \"true\" in ~/.termux/termux.properties file.");
            return Service.START_NOT_STICKY;
        }

        Uri programUri = new Uri.Builder().scheme(TERMUX_SERVICE.URI_SCHEME_SERVICE_EXECUTE).path(getExpandedTermuxPath(intent.getStringExtra(RUN_COMMAND_SERVICE.EXTRA_COMMAND_PATH))).build();

        Intent execIntent = new Intent(TERMUX_SERVICE.ACTION_SERVICE_EXECUTE, programUri);
        execIntent.setClass(this, TermuxService.class);
        execIntent.putExtra(TERMUX_SERVICE.EXTRA_ARGUMENTS, intent.getStringArrayExtra(RUN_COMMAND_SERVICE.EXTRA_ARGUMENTS));
        execIntent.putExtra(TERMUX_SERVICE.EXTRA_BACKGROUND, intent.getBooleanExtra(RUN_COMMAND_SERVICE.EXTRA_BACKGROUND, false));

        String workingDirectory = intent.getStringExtra(RUN_COMMAND_SERVICE.EXTRA_WORKDIR);
        if (workingDirectory != null && !workingDirectory.isEmpty()) {
            execIntent.putExtra(TERMUX_SERVICE.EXTRA_WORKDIR, getExpandedTermuxPath(workingDirectory));
        }

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
        builder.setContentTitle(getText(R.string.application_name) + " Run Command");
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

        String channelName = "Termux Run Command";
        int importance = NotificationManager.IMPORTANCE_LOW;

        NotificationChannel channel = new NotificationChannel(NOTIFICATION_CHANNEL_ID, channelName, importance);
        NotificationManager manager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
        manager.createNotificationChannel(channel);
    }

    private boolean allowExternalApps() {
        File propsFile = new File(TermuxConstants.TERMUX_PROPERTIES_PRIMARY_FILE_PATH);
        if (!propsFile.exists())
            propsFile = new File(TermuxConstants.TERMUX_PROPERTIES_SECONDARY_FILE_PATH);

        Properties props = new Properties();
        try {
            if (propsFile.isFile() && propsFile.canRead()) {
                try (FileInputStream in = new FileInputStream(propsFile)) {
                    props.load(new InputStreamReader(in, StandardCharsets.UTF_8));
                }
            }
        } catch (Exception e) {
            Log.e("termux", "Error loading props", e);
        }

        return props.getProperty("allow-external-apps", "false").equals("true");
    }

    /** Replace "$PREFIX/" or "~/" prefix with termux absolute paths */
    public static String getExpandedTermuxPath(String path) {
        if(path != null && !path.isEmpty()) {
            path = path.replaceAll("^\\$PREFIX$", TermuxConstants.TERMUX_PREFIX_DIR_PATH);
            path = path.replaceAll("^\\$PREFIX/", TermuxConstants.TERMUX_PREFIX_DIR_PATH + "/");
            path = path.replaceAll("^~/$", TermuxConstants.TERMUX_HOME_DIR_PATH);
            path = path.replaceAll("^~/", TermuxConstants.TERMUX_HOME_DIR_PATH + "/");
        }

        return path;
    }
}
