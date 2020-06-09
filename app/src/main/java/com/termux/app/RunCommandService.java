package com.termux.app;

import android.app.Service;
import android.content.Intent;
import android.net.Uri;
import android.os.Binder;
import android.os.Build;
import android.os.IBinder;
import android.util.Log;

import java.io.File;
import java.io.FileInputStream;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;
import java.util.Properties;

/**
 * When allow-external-apps property is set to "true", Termux is able to process execute intents
 * sent by third-party applications.
 *
 * Third-party program must declare com.termux.permission.RUN_COMMAND permission and it should be
 * granted by user.
 *
 * Sample code to run command "top":
 *   Intent intent = new Intent();
 *   intent.setClassName("com.termux", "com.termux.app.RunCommandService");
 *   intent.setAction("com.termux.RUN_COMMAND");
 *   intent.putExtra("com.termux.RUN_COMMAND_PATH", "/data/data/com.termux/files/usr/bin/top");
 *   startService(intent);
 */
public class RunCommandService extends Service {

    public static final String RUN_COMMAND_ACTION = "com.termux.RUN_COMMAND";
    public static final String RUN_COMMAND_PATH = "com.termux.RUN_COMMAND_PATH";
    public static final String RUN_COMMAND_ARGUMENTS = "com.termux.RUN_COMMAND_ARGUMENTS";
    public static final String RUN_COMMAND_WORKDIR = "com.termux.RUN_COMMAND_WORKDIR";

    class LocalBinder extends Binder {
        public final RunCommandService service = RunCommandService.this;
    }

    private final IBinder mBinder = new RunCommandService.LocalBinder();

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    public int onStartCommand(Intent intent, int flags, int startId) {
        if (allowExternalApps() && RUN_COMMAND_ACTION.equals(intent.getAction())) {
            Uri programUri = new Uri.Builder().scheme("com.termux.file").path(intent.getStringExtra(RUN_COMMAND_PATH)).build();

            Intent execIntent = new Intent(TermuxService.ACTION_EXECUTE, programUri);
            execIntent.setClass(this, TermuxService.class);
            execIntent.putExtra(TermuxService.EXTRA_ARGUMENTS, intent.getStringExtra(RUN_COMMAND_ARGUMENTS));
            execIntent.putExtra(TermuxService.EXTRA_CURRENT_WORKING_DIRECTORY, intent.getStringExtra(RUN_COMMAND_WORKDIR));

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                this.startForegroundService(execIntent);
            } else {
                this.startService(execIntent);
            }
        }

        return Service.START_NOT_STICKY;
    }

    private boolean allowExternalApps() {
        File propsFile = new File(TermuxService.HOME_PATH + "/.termux/termux.properties");
        if (!propsFile.exists())
            propsFile = new File(TermuxService.HOME_PATH + "/.config/termux/termux.properties");

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
}
