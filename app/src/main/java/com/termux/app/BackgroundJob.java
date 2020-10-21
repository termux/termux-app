package com.termux.app;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.lang.reflect.Field;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

/**
 * A background job launched by Termux.
 */
public final class BackgroundJob {

    private static final String LOG_TAG = "termux-task";

    final Process mProcess;

    public BackgroundJob(String cwd, String fileToExecute, final String[] args, final TermuxService service){
        this(cwd, fileToExecute, args, service, null);
    }

    public BackgroundJob(String cwd, String fileToExecute, final String[] args, final TermuxService service, PendingIntent pendingIntent) {
        String[] env = buildEnvironment(false, cwd);
        if (cwd == null) cwd = TermuxService.HOME_PATH;

        final String[] progArray = setupProcessArgs(fileToExecute, args);
        final String processDescription = Arrays.toString(progArray);

        Process process;
        try {
            process = Runtime.getRuntime().exec(progArray, env, new File(cwd));
        } catch (IOException e) {
            mProcess = null;
            // TODO: Visible error message?
            Log.e(LOG_TAG, "Failed running background job: " + processDescription, e);
            return;
        }

        mProcess = process;
        final int pid = getPid(mProcess);
        final Bundle result = new Bundle();
        final StringBuilder outResult = new StringBuilder();
        final StringBuilder errResult = new StringBuilder();

        Thread errThread = new Thread() {
            @Override
            public void run() {
                InputStream stderr = mProcess.getErrorStream();
                BufferedReader reader = new BufferedReader(new InputStreamReader(stderr, StandardCharsets.UTF_8));
                String line;
                try {
                    // FIXME: Long lines.
                    while ((line = reader.readLine()) != null) {
                        errResult.append(line).append('\n');
                        Log.i(LOG_TAG, "[" + pid + "] stderr: " + line);
                    }
                } catch (IOException e) {
                    // Ignore.
                }
            }
        };
        errThread.start();

        new Thread() {
            @Override
            public void run() {
                Log.i(LOG_TAG, "[" + pid + "] starting: " + processDescription);
                InputStream stdout = mProcess.getInputStream();
                BufferedReader reader = new BufferedReader(new InputStreamReader(stdout, StandardCharsets.UTF_8));

                String line;
                try {
                    // FIXME: Long lines.
                    while ((line = reader.readLine()) != null) {
                        Log.i(LOG_TAG, "[" + pid + "] stdout: " + line);
                        outResult.append(line).append('\n');
                    }
                } catch (IOException e) {
                    Log.e(LOG_TAG, "Error reading output", e);
                }

                try {
                    int exitCode = mProcess.waitFor();
                    service.onBackgroundJobExited(BackgroundJob.this);
                    if (exitCode == 0) {
                        Log.i(LOG_TAG, "[" + pid + "] exited normally");
                    } else {
                        Log.w(LOG_TAG, "[" + pid + "] exited with code: " + exitCode);
                    }

                    result.putString("stdout", outResult.toString());
                    result.putInt("exitCode", exitCode);

                    errThread.join();
                    result.putString("stderr", errResult.toString());

                    Intent data = new Intent();
                    data.putExtra("result", result);

                    if(pendingIntent != null) {
                        try {
                            pendingIntent.send(service.getApplicationContext(), Activity.RESULT_OK, data);
                        } catch (PendingIntent.CanceledException e) {
                            // The caller doesn't want the result? That's fine, just ignore
                        }
                    }
                } catch (InterruptedException e) {
                    // Ignore
                }
            }
        }.start();
    }

    private static void addToEnvIfPresent(List<String> environment, String name) {
        String value = System.getenv(name);
        if (value != null) {
            environment.add(name + "=" + value);
        }
    }

    static String[] buildEnvironment(boolean failSafe, String cwd) {
        new File(TermuxService.HOME_PATH).mkdirs();

        if (cwd == null) cwd = TermuxService.HOME_PATH;

        List<String> environment = new ArrayList<>();

        environment.add("TERM=xterm-256color");
        environment.add("COLORTERM=truecolor");
        environment.add("HOME=" + TermuxService.HOME_PATH);
        environment.add("PREFIX=" + TermuxService.PREFIX_PATH);
        environment.add("BOOTCLASSPATH=" + System.getenv("BOOTCLASSPATH"));
        environment.add("ANDROID_ROOT=" + System.getenv("ANDROID_ROOT"));
        environment.add("ANDROID_DATA=" + System.getenv("ANDROID_DATA"));
        // EXTERNAL_STORAGE is needed for /system/bin/am to work on at least
        // Samsung S7 - see https://plus.google.com/110070148244138185604/posts/gp8Lk3aCGp3.
        environment.add("EXTERNAL_STORAGE=" + System.getenv("EXTERNAL_STORAGE"));

        // These variables are needed if running on Android 10 and higher.
        addToEnvIfPresent(environment, "ANDROID_ART_ROOT");
        addToEnvIfPresent(environment, "DEX2OATBOOTCLASSPATH");
        addToEnvIfPresent(environment, "ANDROID_I18N_ROOT");
        addToEnvIfPresent(environment, "ANDROID_RUNTIME_ROOT");
        addToEnvIfPresent(environment, "ANDROID_TZDATA_ROOT");

        if (failSafe) {
            // Keep the default path so that system binaries can be used in the failsafe session.
            environment.add("PATH= " + System.getenv("PATH"));
        } else {
            environment.add("LANG=en_US.UTF-8");
            environment.add("PATH=" + TermuxService.PREFIX_PATH + "/bin:" + TermuxService.PREFIX_PATH + "/bin/applets");
            environment.add("PWD=" + cwd);
            environment.add("TMPDIR=" + TermuxService.PREFIX_PATH + "/tmp");
        }

        return environment.toArray(new String[0]);
    }

    public static int getPid(Process p) {
        try {
            Field f = p.getClass().getDeclaredField("pid");
            f.setAccessible(true);
            try {
                return f.getInt(p);
            } finally {
                f.setAccessible(false);
            }
        } catch (Throwable e) {
            return -1;
        }
    }

    static String[] setupProcessArgs(String fileToExecute, String[] args) {
        // The file to execute may either be:
        // - An elf file, in which we execute it directly.
        // - A script file without shebang, which we execute with our standard shell $PREFIX/bin/sh instead of the
        //   system /system/bin/sh. The system shell may vary and may not work at all due to LD_LIBRARY_PATH.
        // - A file with shebang, which we try to handle with e.g. /bin/foo -> $PREFIX/bin/foo.
        String interpreter = null;
        try {
            File file = new File(fileToExecute);
            try (FileInputStream in = new FileInputStream(file)) {
                byte[] buffer = new byte[256];
                int bytesRead = in.read(buffer);
                if (bytesRead > 4) {
                    if (buffer[0] == 0x7F && buffer[1] == 'E' && buffer[2] == 'L' && buffer[3] == 'F') {
                        // Elf file, do nothing.
                    } else if (buffer[0] == '#' && buffer[1] == '!') {
                        // Try to parse shebang.
                        StringBuilder builder = new StringBuilder();
                        for (int i = 2; i < bytesRead; i++) {
                            char c = (char) buffer[i];
                            if (c == ' ' || c == '\n') {
                                if (builder.length() == 0) {
                                    // Skip whitespace after shebang.
                                } else {
                                    // End of shebang.
                                    String executable = builder.toString();
                                    if (executable.startsWith("/usr") || executable.startsWith("/bin")) {
                                        String[] parts = executable.split("/");
                                        String binary = parts[parts.length - 1];
                                        interpreter = TermuxService.PREFIX_PATH + "/bin/" + binary;
                                    }
                                    break;
                                }
                            } else {
                                builder.append(c);
                            }
                        }
                    } else {
                        // No shebang and no ELF, use standard shell.
                        interpreter = TermuxService.PREFIX_PATH + "/bin/sh";
                    }
                }
            }
        } catch (IOException e) {
            // Ignore.
        }

        List<String> result = new ArrayList<>();
        if (interpreter != null) result.add(interpreter);
        result.add(fileToExecute);
        if (args != null) Collections.addAll(result, args);
        return result.toArray(new String[0]);
    }

}
