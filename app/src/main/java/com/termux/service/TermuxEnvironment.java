package com.termux.service;

import android.util.Log;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public final class TermuxEnvironment {
    private static final String TAG = "TermuxEnvironment";

    public static String[] getBuildEnvironment(boolean failSafe, String cwd) {
        new File(TermuxConfig.HOME_PATH).mkdirs();

        if (cwd == null) cwd = TermuxConfig.HOME_PATH;

        List<String> environment = new ArrayList<>();

        environment.add("TERM=xterm-256color");
        environment.add("HOME=" + TermuxConfig.HOME_PATH);
        environment.add("PREFIX=" + TermuxConfig.PREFIX_PATH);
        environment.add("BOOTCLASSPATH" + System.getenv("BOOTCLASSPATH"));
        environment.add("ANDROID_ROOT=" + System.getenv("ANDROID_ROOT"));
        environment.add("ANDROID_DATA=" + System.getenv("ANDROID_DATA"));
        // EXTERNAL_STORAGE is needed for /system/bin/am to work on at least
        // Samsung S7 - see https://plus.google.com/110070148244138185604/posts/gp8Lk3aCGp3.
        environment.add("EXTERNAL_STORAGE=" + System.getenv("EXTERNAL_STORAGE"));
        // ANDROID_RUNTIME_ROOT and ANDROID_TZDATA_ROOT are required for `am` to run on Android Q
        addToEnvIfPresent(environment, "ANDROID_RUNTIME_ROOT");
        addToEnvIfPresent(environment, "ANDROID_TZDATA_ROOT");
        if (failSafe) {
            // Keep the default path so that system binaries can be used in the failsafe session.
            environment.add("PATH= " + System.getenv("PATH"));
        } else {
            if (shouldAddLdLibraryPath()) {
                environment.add("LD_LIBRARY_PATH=" + TermuxConfig.PREFIX_PATH + "/lib");
            }
            environment.add("LANG=en_US.UTF-8");
            environment.add("PATH=" + TermuxConfig.PREFIX_PATH + "/bin:" + TermuxConfig.PREFIX_PATH + "/bin/applets");
            environment.add("PWD=" + cwd);
            environment.add("TMPDIR=" + TermuxConfig.PREFIX_PATH + "/tmp");
        }

        return environment.toArray(new String[0]);
    }

    private static void addToEnvIfPresent(List<String> environment, String name) {
        String value = System.getenv(name);
        if (value != null) {
            environment.add(name + "=" + value);
        }
    }

    private static boolean shouldAddLdLibraryPath() {
        try (BufferedReader in = new BufferedReader(new InputStreamReader(new FileInputStream(TermuxConfig.PREFIX_PATH + "/etc/apt/sources.list")))) {
            String line;
            while ((line = in.readLine()) != null) {
                if (!line.startsWith("#") && line.contains("//termux.net stable")) {
                    return false;
                }
            }
        } catch (IOException e) {
            Log.e(TAG, "Error trying to read sources.list", e);
        }
        return false;
    }

    public static String[] getSetupProcessArgs(String fileToExecute, String[] args) {
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
                                        interpreter = TermuxConfig.PREFIX_PATH + "/bin/" + binary;
                                    }
                                    break;
                                }
                            } else {
                                builder.append(c);
                            }
                        }
                    } else {
                        // No shebang and no ELF, use standard shell.
                        interpreter = TermuxConfig.PREFIX_PATH + "/bin/sh";
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
