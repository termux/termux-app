package com.termux.shared.shell;

import android.content.Context;

import androidx.annotation.NonNull;

import com.termux.shared.models.errors.Error;
import com.termux.shared.termux.TermuxConstants;
import com.termux.shared.file.FileUtils;
import com.termux.shared.logger.Logger;
import com.termux.shared.packages.PackageUtils;
import com.termux.shared.termux.TermuxUtils;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class TermuxShellUtils {

    public static String TERMUX_VERSION_NAME;
    public static String TERMUX_IS_DEBUGGABLE_BUILD;
    public static String TERMUX_APP_PID;
    public static String TERMUX_APK_RELEASE;

    public static String TERMUX_API_VERSION_NAME;

    public static String getDefaultWorkingDirectoryPath() {
        return TermuxConstants.TERMUX_HOME_DIR_PATH;
    }

    public static String getDefaultBinPath() {
        return TermuxConstants.TERMUX_BIN_PREFIX_DIR_PATH;
    }

    public static String[] buildEnvironment(Context currentPackageContext, boolean isFailSafe, String workingDirectory) {
        TermuxConstants.TERMUX_HOME_DIR.mkdirs();

        if (workingDirectory == null || workingDirectory.isEmpty())
            workingDirectory = getDefaultWorkingDirectoryPath();

        List<String> environment = new ArrayList<>();

        loadTermuxEnvVariables(currentPackageContext);

        if (TERMUX_VERSION_NAME != null)
            environment.add("TERMUX_VERSION=" + TERMUX_VERSION_NAME);
        if (TERMUX_IS_DEBUGGABLE_BUILD != null)
            environment.add("TERMUX_IS_DEBUGGABLE_BUILD=" + TERMUX_IS_DEBUGGABLE_BUILD);
        if (TERMUX_APP_PID != null)
            environment.add("TERMUX_APP_PID=" + TERMUX_APP_PID);
        if (TERMUX_APK_RELEASE != null)
            environment.add("TERMUX_APK_RELEASE=" + TERMUX_APK_RELEASE);

        if (TERMUX_API_VERSION_NAME != null)
            environment.add("TERMUX_API_VERSION=" + TERMUX_API_VERSION_NAME);

        environment.add("TERM=xterm-256color");
        environment.add("COLORTERM=truecolor");
        environment.add("HOME=" + TermuxConstants.TERMUX_HOME_DIR_PATH);
        environment.add("PREFIX=" + TermuxConstants.TERMUX_PREFIX_DIR_PATH);
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

        if (isFailSafe) {
            // Keep the default path so that system binaries can be used in the failsafe session.
            environment.add("PATH= " + System.getenv("PATH"));
        } else {
            environment.add("LANG=en_US.UTF-8");
            environment.add("PATH=" + TermuxConstants.TERMUX_BIN_PREFIX_DIR_PATH);
            environment.add("PWD=" + workingDirectory);
            environment.add("TMPDIR=" + TermuxConstants.TERMUX_TMP_PREFIX_DIR_PATH);
        }

        return environment.toArray(new String[0]);
    }

    public static void addToEnvIfPresent(List<String> environment, String name) {
        String value = System.getenv(name);
        if (value != null) {
            environment.add(name + "=" + value);
        }
    }

    public static String[] setupProcessArgs(@NonNull String fileToExecute, String[] arguments) {
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
                                        interpreter = TermuxConstants.TERMUX_BIN_PREFIX_DIR_PATH + "/" + binary;
                                    }
                                    break;
                                }
                            } else {
                                builder.append(c);
                            }
                        }
                    } else {
                        // No shebang and no ELF, use standard shell.
                        interpreter = TermuxConstants.TERMUX_BIN_PREFIX_DIR_PATH + "/sh";
                    }
                }
            }
        } catch (IOException e) {
            // Ignore.
        }

        List<String> result = new ArrayList<>();
        if (interpreter != null) result.add(interpreter);
        result.add(fileToExecute);
        if (arguments != null) Collections.addAll(result, arguments);
        return result.toArray(new String[0]);
    }

    public static void clearTermuxTMPDIR(boolean onlyIfExists) {
        if(onlyIfExists && !FileUtils.directoryFileExists(TermuxConstants.TERMUX_TMP_PREFIX_DIR_PATH, false))
            return;

        Error error;
        error = FileUtils.clearDirectory("$TMPDIR", FileUtils.getCanonicalPath(TermuxConstants.TERMUX_TMP_PREFIX_DIR_PATH, null));
        if (error != null) {
            Logger.logErrorExtended(error.toString());
        }
    }

    public static void loadTermuxEnvVariables(Context currentPackageContext) {
        String termuxAPKReleaseOld = TERMUX_APK_RELEASE;
        TERMUX_VERSION_NAME = TERMUX_IS_DEBUGGABLE_BUILD = TERMUX_APP_PID = TERMUX_APK_RELEASE = null;

        // Check if Termux app is installed and not disabled
        if (TermuxUtils.isTermuxAppInstalled(currentPackageContext) == null) {
            // This function may be called by a different package like a plugin, so we get version for Termux package via its context
            Context termuxPackageContext = TermuxUtils.getTermuxPackageContext(currentPackageContext);
            if (termuxPackageContext != null) {
                TERMUX_VERSION_NAME = PackageUtils.getVersionNameForPackage(termuxPackageContext);
                TERMUX_IS_DEBUGGABLE_BUILD = PackageUtils.isAppForPackageADebuggableBuild(termuxPackageContext) ? "1" : "0";

                TERMUX_APP_PID = TermuxUtils.getTermuxAppPID(currentPackageContext);

                // Getting APK signature is a slightly expensive operation, so do it only when needed
                if (termuxAPKReleaseOld == null) {
                    String signingCertificateSHA256Digest = PackageUtils.getSigningCertificateSHA256DigestForPackage(termuxPackageContext);
                    if (signingCertificateSHA256Digest != null)
                        TERMUX_APK_RELEASE = TermuxUtils.getAPKRelease(signingCertificateSHA256Digest).replaceAll("[^a-zA-Z]", "_").toUpperCase();
                } else {
                    TERMUX_APK_RELEASE = termuxAPKReleaseOld;
                }
            }
        }


        TERMUX_API_VERSION_NAME = null;

        // Check if Termux:API app is installed and not disabled
        if (TermuxUtils.isTermuxAPIAppInstalled(currentPackageContext) == null) {
            // This function may be called by a different package like a plugin, so we get version for Termux:API package via its context
            Context termuxAPIPackageContext = TermuxUtils.getTermuxAPIPackageContext(currentPackageContext);
            if (termuxAPIPackageContext != null)
                TERMUX_API_VERSION_NAME = PackageUtils.getVersionNameForPackage(termuxAPIPackageContext);
        }
    }

}
