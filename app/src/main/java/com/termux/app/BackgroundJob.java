package com.termux.app;

import android.util.Log;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;

/**
 * A background job launched by Termux.
 */
public final class BackgroundJob {

    private static final String LOG_TAG = "termux-background";

    final Process mProcess;

    public BackgroundJob(File cwd, File fileToExecute, String[] args) throws IOException {
        String[] env = buildEnvironment(false, cwd.getAbsolutePath());

        String[] progArray = new String[args.length + 1];

        mProcess = Runtime.getRuntime().exec(progArray, env, cwd);

        new Thread() {
            @Override
            public void run() {
                while (true) {
                    try {
                        int exitCode = mProcess.waitFor();
                        if (exitCode == 0) {
                            Log.i(LOG_TAG, "exited normally");
                            return;
                        } else {
                            Log.i(LOG_TAG, "exited with exit code: " + exitCode);
                        }
                    } catch (InterruptedException e) {
                        // Ignore.
                    }
                }
            }
        }.start();

        new Thread() {
            @Override
            public void run() {
                InputStream stdout = mProcess.getInputStream();
                BufferedReader reader = new BufferedReader(new InputStreamReader(stdout, StandardCharsets.UTF_8));
                String line;
                try {
                    // FIXME: Long lines.
                    while ((line = reader.readLine()) != null) {
                        Log.i(LOG_TAG, line);
                    }
                } catch (IOException e) {
                    // Ignore.
                }
            }
        }.start();


        new Thread() {
            @Override
            public void run() {
                InputStream stderr = mProcess.getErrorStream();
                BufferedReader reader = new BufferedReader(new InputStreamReader(stderr, StandardCharsets.UTF_8));
                String line;
                try {
                    // FIXME: Long lines.
                    while ((line = reader.readLine()) != null) {
                        Log.e(LOG_TAG, line);
                    }
                } catch (IOException e) {
                    // Ignore.
                }
            }
        };
    }

    public String[] buildEnvironment(boolean failSafe, String cwd) {
        new File(TermuxService.HOME_PATH).mkdirs();

        if (cwd == null) cwd = TermuxService.HOME_PATH;

        final String termEnv = "TERM=xterm-256color";
        final String homeEnv = "HOME=" + TermuxService.HOME_PATH;
        final String prefixEnv = "PREFIX=" + TermuxService.PREFIX_PATH;
        final String androidRootEnv = "ANDROID_ROOT=" + System.getenv("ANDROID_ROOT");
        final String androidDataEnv = "ANDROID_DATA=" + System.getenv("ANDROID_DATA");
        // EXTERNAL_STORAGE is needed for /system/bin/am to work on at least
        // Samsung S7 - see https://plus.google.com/110070148244138185604/posts/gp8Lk3aCGp3.
        final String externalStorageEnv = "EXTERNAL_STORAGE=" + System.getenv("EXTERNAL_STORAGE");
        String[] env;
        if (failSafe) {
            // Keep the default path so that system binaries can be used in the failsafe session.
            final String pathEnv = "PATH=" + System.getenv("PATH");
            return new String[]{termEnv, homeEnv, prefixEnv, androidRootEnv, androidDataEnv, pathEnv, externalStorageEnv};
        } else {
            final String ps1Env = "PS1=$ ";
            final String ldEnv = "LD_LIBRARY_PATH=" + TermuxService.PREFIX_PATH + "/lib";
            final String langEnv = "LANG=en_US.UTF-8";
            final String pathEnv = "PATH=" + TermuxService.PREFIX_PATH + "/bin:" + TermuxService.PREFIX_PATH + "/bin/applets";
            final String pwdEnv = "PWD=" + cwd;

            return new String[]{termEnv, homeEnv, prefixEnv, ps1Env, ldEnv, langEnv, pathEnv, pwdEnv, androidRootEnv, androidDataEnv, externalStorageEnv};
        }
    }

}
