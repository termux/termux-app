package com.invapp.shared.termux.shell.command.environment;

import android.content.Context;

import androidx.annotation.NonNull;

import com.invapp.shared.errors.Error;
import com.invapp.shared.file.FileUtils;
import com.invapp.shared.logger.Logger;
import com.invapp.shared.shell.command.ExecutionCommand;
import com.invapp.shared.shell.command.environment.AndroidShellEnvironment;
import com.invapp.shared.shell.command.environment.ShellEnvironmentUtils;
import com.invapp.shared.shell.command.environment.ShellCommandShellEnvironment;
import com.invapp.shared.termux.TermuxBootstrap;
import com.invapp.shared.termux.TermuxConstants;
import com.invapp.shared.termux.shell.TermuxShellUtils;

import android.system.Os;

import java.io.File;
import java.nio.charset.Charset;
import java.util.HashMap;

/**
 * Environment for Termux.
 */
public class TermuxShellEnvironment extends AndroidShellEnvironment {

    private static final String LOG_TAG = "TermuxShellEnvironment";

    /** Environment variable for the termux {@link TermuxConstants#TERMUX_PREFIX_DIR_PATH}. */
    public static final String ENV_PREFIX = "PREFIX";

    /** Filename of the path-redirector library installed under {@code $PREFIX/lib}. */
    public static final String REDIRECTOR_LIB_NAME = "libinvapp-redirector.so";

    /**
     * Bun-only seccomp shim: turns Android SIGSYS (openat2/fchmodat2) into ENOSYS.
     * Must not be combined with the path redirector in Bun's LD_PRELOAD.
     */
    public static final String BUN_SECCOMP_LIB_NAME = "libinvapp-bun-seccomp.so";

    /**
     * Absolute path where the redirector is installed inside the Termux prefix.
     * Must live under {@code $PREFIX} so Termux ELFs can LD_PRELOAD it (Android
     * linker namespaces often block preloading from the APK {@code nativeLibraryDir}).
     */
    public static final String REDIRECTOR_PREFIX_LIB_PATH =
        TermuxConstants.TERMUX_LIB_PREFIX_DIR_PATH + "/" + REDIRECTOR_LIB_NAME;

    public static final String BUN_SECCOMP_PREFIX_LIB_PATH =
        TermuxConstants.TERMUX_LIB_PREFIX_DIR_PATH + "/" + BUN_SECCOMP_LIB_NAME;

    public TermuxShellEnvironment() {
        super();
        shellCommandShellEnvironment = new TermuxShellCommandShellEnvironment();
    }


    /** Init {@link TermuxShellEnvironment} constants and caches. */
    public synchronized static void init(@NonNull Context currentPackageContext) {
        TermuxAppShellEnvironment.setTermuxAppEnvironment(currentPackageContext);
    }

    /**
     * Copy {@code libinvapp-redirector.so} from the APK native lib dir into
     * {@code $PREFIX/lib} so shell/dpkg processes can preload it, and ensure
     * {@code $PREFIX/bin/login} keeps it first in {@code LD_PRELOAD} (stock
     * login overwrites preload with termux-exec only).
     */
    public synchronized static void installRedirectorIntoPrefix(@NonNull Context context) {
        if ("com.termux".equals(TermuxConstants.TERMUX_PACKAGE_NAME)) {
            return;
        }
        if (!FileUtils.directoryFileExists(TermuxConstants.TERMUX_LIB_PREFIX_DIR_PATH, false)) {
            // Prefix not bootstrapped yet.
            return;
        }

        String nativeLibDir = context.getApplicationInfo().nativeLibraryDir;
        File src = new File(nativeLibDir, REDIRECTOR_LIB_NAME);
        File dest = new File(REDIRECTOR_PREFIX_LIB_PATH);
        if (!src.exists()) {
            Logger.logError(LOG_TAG, "Redirector not found in APK native libs: " + src.getAbsolutePath());
            return;
        }

        // Refresh when missing or APK copy is newer (app update).
        boolean needsCopy = !dest.exists()
            || dest.length() != src.length()
            || dest.lastModified() < src.lastModified();
        if (needsCopy) {
            Error error = FileUtils.copyRegularFile("invapp-redirector", src.getAbsolutePath(),
                dest.getAbsolutePath(), false);
            if (error != null) {
                Logger.logErrorExtended(LOG_TAG, "Failed to install redirector into prefix\n" + error);
                return;
            }
            try {
                //noinspection OctalInteger
                Os.chmod(dest.getAbsolutePath(), 0755);
            } catch (Exception e) {
                Logger.logStackTraceWithMessage(LOG_TAG, "Failed to chmod redirector in prefix", e);
            }
            Logger.logInfo(LOG_TAG, "Installed redirector to " + dest.getAbsolutePath());
        }

        installNativeLibIntoPrefix(context, BUN_SECCOMP_LIB_NAME, BUN_SECCOMP_PREFIX_LIB_PATH,
            "invapp-bun-seccomp");

        patchLoginScriptToPreserveRedirector();
        installPackageManagerPathOverrides();
    }

    /** Copy an APK {@code jniLibs} shared object into {@code $PREFIX/lib}. */
    private static void installNativeLibIntoPrefix(@NonNull Context context,
            @NonNull String libName, @NonNull String destPath, @NonNull String label) {
        String nativeLibDir = context.getApplicationInfo().nativeLibraryDir;
        File src = new File(nativeLibDir, libName);
        File dest = new File(destPath);
        if (!src.exists()) {
            Logger.logWarn(LOG_TAG, label + " not found in APK native libs: " + src.getAbsolutePath());
            return;
        }
        boolean needsCopy = !dest.exists()
            || dest.length() != src.length()
            || dest.lastModified() < src.lastModified();
        if (!needsCopy) {
            return;
        }
        Error error = FileUtils.copyRegularFile(label, src.getAbsolutePath(),
            dest.getAbsolutePath(), false);
        if (error != null) {
            Logger.logErrorExtended(LOG_TAG, "Failed to install " + label + " into prefix\n" + error);
            return;
        }
        try {
            //noinspection OctalInteger
            Os.chmod(dest.getAbsolutePath(), 0755);
        } catch (Exception e) {
            Logger.logStackTraceWithMessage(LOG_TAG, "Failed to chmod " + label + " in prefix", e);
        }
        Logger.logInfo(LOG_TAG, "Installed " + label + " to " + dest.getAbsolutePath());
    }

    /**
     * Stock apt/dpkg ELFs embed {@code /data/data/com.termux/...} directory paths.
     * Drop-in config overrides point them at this app's real prefix/cache so
     * {@code apt update} does not exec methods under the inaccessible com.termux tree.
     */
    private synchronized static void installPackageManagerPathOverrides() {
        String prefix = TermuxConstants.TERMUX_PREFIX_DIR_PATH;
        String dataRoot = TermuxConstants.TERMUX_INTERNAL_PRIVATE_APP_DATA_DIR_PATH;
        String aptConfDir = prefix + "/etc/apt/apt.conf.d";
        String dpkgCfgDir = prefix + "/etc/dpkg/dpkg.cfg.d";
        String cacheApt = dataRoot + "/cache/apt";
        String logApt = prefix + "/var/log/apt";

        FileUtils.createDirectoryFile(aptConfDir);
        FileUtils.createDirectoryFile(dpkgCfgDir);
        FileUtils.createDirectoryFile(cacheApt + "/archives/partial");
        FileUtils.createDirectoryFile(prefix + "/var/lib/apt/lists/partial");
        FileUtils.createDirectoryFile(logApt);

        String aptConf = ""
            + "// Managed by termux-app fork — override compiled-in com.termux paths\n"
            + "Dir::State \"" + prefix + "/var/lib/apt\";\n"
            + "Dir::State::status \"" + prefix + "/var/lib/dpkg/status\";\n"
            + "Dir::Cache \"" + cacheApt + "\";\n"
            + "Dir::Etc \"" + prefix + "/etc/apt\";\n"
            + "Dir::Log \"" + logApt + "\";\n"
            + "Dir::Bin::methods \"" + prefix + "/lib/apt/methods\";\n"
            + "Dir::Bin::solvers:: \"" + prefix + "/lib/apt/solvers\";\n"
            + "Dir::Bin::planners:: \"" + prefix + "/lib/apt/planners\";\n"
            + "Dir::Bin::dpkg \"" + prefix + "/bin/dpkg\";\n"
            + "Dir::Bin::apt-key \"" + prefix + "/bin/apt-key\";\n"
            + "Dir::Bin::gpgv \"" + prefix + "/bin/gpgv\";\n"
            + "Dir::Bin::gzip \"" + prefix + "/bin/gzip\";\n"
            + "Dir::Bin::bzip2 \"" + prefix + "/bin/bzip2\";\n"
            + "Dir::Bin::xz \"" + prefix + "/bin/xz\";\n"
            + "Dir::Bin::lz4 \"" + prefix + "/bin/lz4\";\n"
            + "Dir::Bin::zstd \"" + prefix + "/bin/zstd\";\n"
            + "Dir::Bin::lzma \"" + prefix + "/bin/xz\";\n"
            + "DPkg::Path \"" + prefix + "/bin\";\n";

        String aptConfPath = aptConfDir + "/00invapp-prefix";
        Error aptErr = FileUtils.writeTextToFile("apt.conf.d/00invapp-prefix", aptConfPath,
            Charset.defaultCharset(), aptConf, false);
        if (aptErr != null) {
            Logger.logErrorExtended(LOG_TAG, "Failed to write apt path overrides\n" + aptErr);
        }

        String dpkgConf = ""
            + "# Managed by termux-app fork — override compiled-in com.termux admindir\n"
            + "admindir " + prefix + "/var/lib/dpkg\n";
        String dpkgConfPath = dpkgCfgDir + "/00invapp-prefix";
        Error dpkgErr = FileUtils.writeTextToFile("dpkg.cfg.d/00invapp-prefix", dpkgConfPath,
            Charset.defaultCharset(), dpkgConf, false);
        if (dpkgErr != null) {
            Logger.logErrorExtended(LOG_TAG, "Failed to write dpkg path overrides\n" + dpkgErr);
        } else {
            Logger.logInfo(LOG_TAG, "Installed apt/dpkg path overrides for "
                + TermuxConstants.TERMUX_PACKAGE_NAME);
        }

        // apt clears LD_PRELOAD before spawning apt-key/gpgv, but still passes
        // com.termux temp paths in argv — re-enable redirector inside those tools.
        patchBinaryWrapperForRedirector(prefix + "/bin/apt-key", true);
        patchBinaryWrapperForRedirector(prefix + "/bin/gpgv", false);
        patchBinaryWrapperForRedirector(prefix + "/lib/apt/methods/gpgv", false);
        patchBinaryWrapperForRedirector(prefix + "/lib/apt/methods/http", false);
        // apt/pkg → dpkg: keep redirector loaded so maintainer scripts in tmp.ci get shebang fixes.
        patchBinaryWrapperForRedirector(prefix + "/bin/apt", false);
        patchBinaryWrapperForRedirector(prefix + "/bin/apt-get", false);
        patchBinaryWrapperForRedirector(prefix + "/bin/pkg", true);
        // Keep redirector loaded in dpkg so execve can rewrite maintainer-script shebangs.
        patchBinaryWrapperForRedirector(prefix + "/bin/dpkg", false);
        // sshd clears LD_* for sessions; keep preload on the daemon so execve can reinject.
        patchBinaryWrapperForRedirector(prefix + "/bin/sshd", false);

        fixDpkgMaintainerScripts(prefix);
        fixStockTermuxPathsInPrefixConfigs(prefix);
        installSshSessionEnvOverrides(prefix);
        repairTermuxAmLauncher(prefix);
    }

    /**
     * Stock {@code $PREFIX/bin/am} launches TermuxAm via app_process using the
     * APK identity {@code com.termux}, which fails with Permission Denial on a
     * renamed package id. Prefer the in-app {@code termux-am} socket server
     * (runs as this app) and keep {@code am.apk} only as fallback docs.
     */
    private synchronized static void repairTermuxAmLauncher(String prefix) {
        String amPath = prefix + "/bin/am";
        File amFile = new File(amPath);
        String termuxAmPath = prefix + "/bin/termux-am";
        if (!new File(termuxAmPath).isFile()) {
            return;
        }

        String wrapper = "#!/data/data/" + TermuxConstants.TERMUX_PACKAGE_NAME
            + "/files/usr/bin/sh\n"
            + "# Managed by termux-app fork — route am through termux-am socket\n"
            + "# (stock TermuxAm APK always identifies as com.termux).\n"
            + "PREFIX=\"/data/data/" + TermuxConstants.TERMUX_PACKAGE_NAME + "/files/usr\"\n"
            + "export PATH=\"$PREFIX/bin${PATH:+:$PATH}\"\n"
            + "export LD_LIBRARY_PATH=\"$PREFIX/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}\"\n"
            + "export LD_PRELOAD=\"$PREFIX/lib/" + REDIRECTOR_LIB_NAME
            + "${LD_PRELOAD:+:$LD_PRELOAD}\"\n"
            + "if [ -x \"$PREFIX/bin/termux-am\" ] && [ -x \"$PREFIX/bin/bash\" ]; then\n"
            + "\texec \"$PREFIX/bin/bash\" \"$PREFIX/bin/termux-am\" \"$@\"\n"
            + "fi\n"
            + "echo \"termux-am not available; cannot run am on renamed package id\" 1>&2\n"
            + "exit 127\n";

        StringBuilder contents = new StringBuilder();
        Error readError = FileUtils.readTextFromFile("am", amPath, Charset.defaultCharset(),
            contents, true);
        if (readError == null && contents.toString().contains("route am through termux-am socket")) {
            return;
        }

        // Preserve original launcher beside the wrapper once.
        File realFile = new File(amPath + ".termuxam");
        if (amFile.isFile() && !realFile.exists()
            && contents.toString().contains("app_process")) {
            FileUtils.moveRegularFile("am", amPath, realFile.getAbsolutePath(), false);
        }

        Error writeError = FileUtils.writeTextToFile("am", amPath, Charset.defaultCharset(),
            wrapper, false);
        if (writeError != null) {
            Logger.logErrorExtended(LOG_TAG, "Failed to wrap am via termux-am\n" + writeError);
            return;
        }
        try {
            //noinspection OctalInteger
            Os.chmod(amPath, 0700);
        } catch (Exception ignored) {
        }
        Logger.logInfo(LOG_TAG, "Wrapped am to use termux-am socket server");
    }

    /**
     * Rewrite leftover {@code com.termux} paths in prefix text configs (profile,
     * sshd_config, bashrc, …) so login shells and OpenSSH do not reference the
     * inaccessible stock Termux data directory.
     */
    private synchronized static void fixStockTermuxPathsInPrefixConfigs(String prefix) {
        String[] relativePaths = new String[] {
            "etc/profile",
            "etc/bash.bashrc",
            "etc/ssh/sshd_config",
            "etc/ssh/ssh_config",
        };
        String oldPkg = "com.termux";
        String newPkg = TermuxConstants.TERMUX_PACKAGE_NAME;
        int fixed = 0;
        for (String relative : relativePaths) {
            File file = new File(prefix + "/" + relative);
            if (!file.isFile()) {
                continue;
            }
            StringBuilder contents = new StringBuilder();
            Error readError = FileUtils.readTextFromFile(relative, file.getAbsolutePath(),
                Charset.defaultCharset(), contents, true);
            if (readError != null) {
                continue;
            }
            String text = contents.toString();
            if (!text.contains(oldPkg)) {
                continue;
            }
            String patched = text.replace(oldPkg, newPkg);
            Error writeError = FileUtils.writeTextToFile(relative, file.getAbsolutePath(),
                Charset.defaultCharset(), patched, false);
            if (writeError != null) {
                Logger.logErrorExtended(LOG_TAG, "Failed to fix " + relative + "\n" + writeError);
                continue;
            }
            fixed++;
        }
        // profile.d snippets ship with hardcoded stock paths too.
        File profileD = new File(prefix + "/etc/profile.d");
        File[] snippets = profileD.listFiles();
        if (snippets != null) {
            for (File file : snippets) {
                if (!file.isFile() || !file.getName().endsWith(".sh")) {
                    continue;
                }
                StringBuilder contents = new StringBuilder();
                Error readError = FileUtils.readTextFromFile(file.getName(), file.getAbsolutePath(),
                    Charset.defaultCharset(), contents, true);
                if (readError != null) {
                    continue;
                }
                String text = contents.toString();
                if (!text.contains(oldPkg)) {
                    continue;
                }
                Error writeError = FileUtils.writeTextToFile(file.getName(), file.getAbsolutePath(),
                    Charset.defaultCharset(), text.replace(oldPkg, newPkg), false);
                if (writeError == null) {
                    fixed++;
                }
            }
        }
        if (fixed > 0) {
            Logger.logInfo(LOG_TAG, "Rewrote com.termux paths in " + fixed
                + " prefix config file(s)");
        }
    }

    /**
     * Drop-in sshd config so sessions get a usable HOME/PREFIX even when clients
     * do not forward environment. LD_* is still reinjected by the redirector on
     * execve; SetEnv covers HOME/PREFIX for tools that only read those.
     */
    private synchronized static void installSshSessionEnvOverrides(String prefix) {
        String confDir = prefix + "/etc/ssh/sshd_config.d";
        FileUtils.createDirectoryFile(confDir);
        String home = TermuxConstants.TERMUX_HOME_DIR_PATH;
        String usr = TermuxConstants.TERMUX_PREFIX_DIR_PATH;
        String conf = ""
            + "# Managed by termux-app fork — OpenSSH session paths for renamed package id\n"
            + "SetEnv HOME=" + home + "\n"
            + "SetEnv PREFIX=" + usr + "\n"
            + "SetEnv PATH=" + usr + "/bin\n"
            + "SetEnv LD_LIBRARY_PATH=" + usr + "/lib\n"
            + "SetEnv LD_PRELOAD=" + REDIRECTOR_PREFIX_LIB_PATH + "\n";
        String confPath = confDir + "/00invapp-session-env.conf";
        Error err = FileUtils.writeTextToFile("sshd_config.d/00invapp-session-env.conf", confPath,
            Charset.defaultCharset(), conf, false);
        if (err != null) {
            Logger.logErrorExtended(LOG_TAG, "Failed to write sshd session env overrides\n" + err);
        }
    }

    /**
     * Rewrite leftover {@code com.termux} paths in dpkg maintainer scripts under
     * {@code var/lib/dpkg/info/} and in-flight {@code var/lib/dpkg/tmp.ci/} (unpack).
     */
    private synchronized static void fixDpkgMaintainerScripts(String prefix) {
        int fixed = fixDpkgScriptsInDirectory(new File(prefix + "/var/lib/dpkg/info"), prefix);
        fixed += fixDpkgScriptsInDirectory(new File(prefix + "/var/lib/dpkg/tmp.ci"), prefix);
        if (fixed > 0) {
            Logger.logInfo(LOG_TAG, "Patched " + fixed + " dpkg maintainer script file(s)");
        }
    }

    /**
     * Patch stock Termux package-id paths in a maintainer script body. Used when
     * rewriting {@code tmp.ci/preinst} before dpkg executes it (redirector may be absent).
     */
    @NonNull
    public static String patchDpkgMaintainerScriptText(@NonNull String text, @NonNull String prefix) {
        return patchDpkgMaintainerScriptText(text, prefix, "");
    }

    private static boolean isDpkgMaintainerScriptFileName(@NonNull String fileName) {
        if ("preinst".equals(fileName) || "postinst".equals(fileName)
            || "prerm".equals(fileName) || "postrm".equals(fileName)
            || "config".equals(fileName)) {
            return true;
        }
        return fileName.endsWith(".preinst") || fileName.endsWith(".postinst")
            || fileName.endsWith(".prerm") || fileName.endsWith(".postrm")
            || fileName.endsWith(".config");
    }

    @NonNull
    public static String patchDpkgMaintainerScriptText(@NonNull String text, @NonNull String prefix,
            @NonNull String fileName) {
        String oldPkg = "com.termux";
        String newPkg = TermuxConstants.TERMUX_PACKAGE_NAME;
        boolean needsListNewline = fileName.endsWith(".list") && !text.isEmpty() && !text.endsWith("\n");
        boolean needsShebang = isDpkgMaintainerScriptFileName(fileName)
            && !text.isEmpty() && !text.startsWith("#!");
        if (!text.contains(oldPkg)
            && !text.startsWith("#!/bin/sh")
            && !text.startsWith("#!/bin/bash")
            && !needsListNewline
            && !needsShebang) {
            return text;
        }
        String patched = text;
        if (needsShebang) {
            patched = "#!" + prefix + "/bin/sh\n" + patched;
        }
        patched = patched.replace(oldPkg, newPkg);
        if (patched.startsWith("#!/bin/sh\n") || patched.startsWith("#!/bin/sh\r\n")) {
            patched = "#!" + prefix + "/bin/sh" + patched.substring("#!/bin/sh".length());
        } else if (patched.startsWith("#!/bin/bash\n") || patched.startsWith("#!/bin/bash\r\n")) {
            patched = "#!" + prefix + "/bin/bash" + patched.substring("#!/bin/bash".length());
        }
        if (needsListNewline && !patched.endsWith("\n")) {
            patched = patched + "\n";
        }
        return patched;
    }

    private static int fixDpkgScriptsInDirectory(@NonNull File dir, @NonNull String prefix) {
        File[] files = dir.listFiles();
        if (files == null) {
            return 0;
        }
        int fixed = 0;
        for (File file : files) {
            if (!file.isFile()) {
                continue;
            }
            String name = file.getName();
            StringBuilder contents = new StringBuilder();
            Error readError = FileUtils.readTextFromFile(name, file.getAbsolutePath(),
                Charset.defaultCharset(), contents, true);
            if (readError != null) {
                continue;
            }
            String text = contents.toString();
            String patched = patchDpkgMaintainerScriptText(text, prefix, name);
            if (patched.equals(text)) {
                continue;
            }
            Error writeError = FileUtils.writeTextToFile(name, file.getAbsolutePath(),
                Charset.defaultCharset(), patched, false);
            if (writeError != null) {
                Logger.logErrorExtended(LOG_TAG, "Failed to fix " + name + "\n" + writeError);
                continue;
            }
            try {
                //noinspection OctalInteger
                Os.chmod(file.getAbsolutePath(), 0700);
            } catch (Exception ignored) {
            }
            fixed++;
        }
        return fixed;
    }

    /**
     * Ensure a bin tool re-exports the path redirector in LD_PRELOAD.
     * For shell scripts, inject after the shebang. For ELF binaries, replace the
     * file with a small shell wrapper that preloads then execs {@code .real}.
     */
    private synchronized static void patchBinaryWrapperForRedirector(String absolutePath, boolean isShellScriptHint) {
        File target = new File(absolutePath);
        if (!target.exists()) {
            return;
        }

        String markerBegin = "# --- invapp-redirector LD_PRELOAD (managed by app) ---";
        String markerEnd = "# --- end invapp-redirector LD_PRELOAD ---";
        String libDir = TermuxConstants.TERMUX_LIB_PREFIX_DIR_PATH;
        String preloadExport = "export LD_PRELOAD=\"" + REDIRECTOR_PREFIX_LIB_PATH
            + "${LD_PRELOAD:+:$LD_PRELOAD}\"\n";
        String libPathExport = "export LD_LIBRARY_PATH=\"" + libDir
            + "${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}\"\n";
        String injectBlock = markerBegin + "\n"
            + "if [ -f \"" + REDIRECTOR_PREFIX_LIB_PATH + "\" ]; then\n"
            + "\t" + libPathExport
            + "\t" + preloadExport
            + "fi\n"
            + markerEnd + "\n";

        boolean treatAsScript = isShellScriptHint;
        try (java.io.FileInputStream in = new java.io.FileInputStream(target)) {
            byte[] hdr = new byte[2];
            if (in.read(hdr) == 2 && hdr[0] == '#' && hdr[1] == '!') {
                treatAsScript = true;
            } else if (hdr[0] == 0x7f && hdr[1] == 'E') {
                treatAsScript = false;
            }
        } catch (Exception e) {
            Logger.logStackTraceWithMessage(LOG_TAG, "Failed reading " + absolutePath, e);
            return;
        }

        if (treatAsScript) {
            StringBuilder contents = new StringBuilder();
            Error readError = FileUtils.readTextFromFile(target.getName(), absolutePath,
                Charset.defaultCharset(), contents, false);
            if (readError != null) {
                Logger.logErrorExtended(LOG_TAG, "Failed to read " + absolutePath + "\n" + readError);
                return;
            }
            String text = contents.toString();
            int prevBegin = text.indexOf(markerBegin);
            if (prevBegin >= 0) {
                int prevEnd = text.indexOf(markerEnd, prevBegin);
                if (prevEnd >= 0) {
                    text = text.substring(0, prevBegin)
                        + text.substring(prevEnd + markerEnd.length());
                    if (text.startsWith("\n", prevBegin)) {
                        text = text.substring(0, prevBegin) + text.substring(prevBegin + 1);
                    }
                }
            }
            int afterShebang = 0;
            if (text.startsWith("#!")) {
                int nl = text.indexOf('\n');
                afterShebang = nl >= 0 ? nl + 1 : text.length();
            }
            String patched = text.substring(0, afterShebang) + injectBlock + text.substring(afterShebang);
            Error writeError = FileUtils.writeTextToFile(target.getName(), absolutePath,
                Charset.defaultCharset(), patched, false);
            if (writeError != null) {
                Logger.logErrorExtended(LOG_TAG, "Failed to patch " + absolutePath + "\n" + writeError);
                return;
            }
        } else {
            // ELF: move aside and wrap.
            String realPath = absolutePath + ".real";
            File realFile = new File(realPath);
            if (!realFile.exists()) {
                Error moveErr = FileUtils.moveRegularFile(target.getName(), absolutePath, realPath, false);
                if (moveErr != null) {
                    Logger.logErrorExtended(LOG_TAG, "Failed to move " + absolutePath + " for wrapper\n" + moveErr);
                    return;
                }
            } else if (target.exists() && !isManagedWrapper(absolutePath, markerBegin)) {
                // Already have .real; refresh wrapper only.
            }
            String wrapper = "#!/data/data/" + TermuxConstants.TERMUX_PACKAGE_NAME
                + "/files/usr/bin/sh\n"
                + injectBlock
                + "exec \"" + realPath + "\" \"$@\"\n";
            Error writeError = FileUtils.writeTextToFile(target.getName() + "-wrapper", absolutePath,
                Charset.defaultCharset(), wrapper, false);
            if (writeError != null) {
                Logger.logErrorExtended(LOG_TAG, "Failed to write wrapper for " + absolutePath + "\n" + writeError);
                return;
            }
        }

        try {
            //noinspection OctalInteger
            Os.chmod(absolutePath, 0700);
        } catch (Exception e) {
            Logger.logStackTraceWithMessage(LOG_TAG, "Failed to chmod " + absolutePath, e);
        }
        Logger.logInfo(LOG_TAG, "Ensured redirector preload in " + absolutePath);
    }

    private static boolean isManagedWrapper(String absolutePath, String markerBegin) {
        StringBuilder contents = new StringBuilder();
        Error readError = FileUtils.readTextFromFile("wrapper-check", absolutePath,
            Charset.defaultCharset(), contents, true);
        return readError == null && contents.toString().contains(markerBegin);
    }

    /**
     * Stock {@code login} sets {@code LD_PRELOAD} to termux-exec alone (or unsets it),
     * which drops our path redirector. Re-inject the redirector immediately before
     * {@code exec "$SHELL"} so bash/dpkg keep seeing {@code /data/data/com.termux} → real prefix.
     */
    private synchronized static void patchLoginScriptToPreserveRedirector() {
        String loginPath = TermuxConstants.TERMUX_BIN_PREFIX_DIR_PATH + "/login";
        File loginFile = new File(loginPath);
        if (!loginFile.isFile()) {
            return;
        }

        String markerBegin = "# --- invapp-redirector LD_PRELOAD (managed by app) ---";
        String markerEnd = "# --- end invapp-redirector LD_PRELOAD ---";
        String libDir = TermuxConstants.TERMUX_LIB_PREFIX_DIR_PATH;
        String injectBlock = markerBegin + "\n"
            + "if [ -f \"" + REDIRECTOR_PREFIX_LIB_PATH + "\" ]; then\n"
            + "\texport LD_LIBRARY_PATH=\"" + libDir + "${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}\"\n"
            + "\texport LD_PRELOAD=\"" + REDIRECTOR_PREFIX_LIB_PATH + "${LD_PRELOAD:+:$LD_PRELOAD}\"\n"
            + "\texport HOME=\"" + TermuxConstants.TERMUX_HOME_DIR_PATH + "\"\n"
            + "\texport PREFIX=\"" + TermuxConstants.TERMUX_PREFIX_DIR_PATH + "\"\n"
            + "fi\n"
            + markerEnd + "\n";

        StringBuilder contents = new StringBuilder();
        Error readError = FileUtils.readTextFromFile("login", loginPath, Charset.defaultCharset(),
            contents, false);
        if (readError != null) {
            Logger.logErrorExtended(LOG_TAG, "Failed to read login script\n" + readError);
            return;
        }

        String text = contents.toString();
        // Remove a previous managed block so upgrades stay idempotent.
        int prevBegin = text.indexOf(markerBegin);
        if (prevBegin >= 0) {
            int prevEnd = text.indexOf(markerEnd, prevBegin);
            if (prevEnd >= 0) {
                text = text.substring(0, prevBegin)
                    + text.substring(prevEnd + markerEnd.length());
                // Drop a single leftover newline after removal.
                if (text.startsWith("\n", prevBegin)) {
                    text = text.substring(0, prevBegin) + text.substring(prevBegin + 1);
                } else if (text.startsWith("\r\n", prevBegin)) {
                    text = text.substring(0, prevBegin) + text.substring(prevBegin + 2);
                }
            }
        }

        String execNeedle = "if [ -n \"$TERM\" ]; then";
        int execAt = text.lastIndexOf(execNeedle);
        if (execAt < 0) {
            // Older login scripts may only have a single exec line.
            execNeedle = "exec \"$SHELL\"";
            execAt = text.lastIndexOf(execNeedle);
        }
        if (execAt < 0) {
            Logger.logWarn(LOG_TAG, "login script has no shell exec block; skipping redirector patch");
            return;
        }

        // Find start of the line containing the needle.
        int lineStart = text.lastIndexOf('\n', execAt - 1) + 1;
        String patched = text.substring(0, lineStart) + injectBlock + text.substring(lineStart);
        if (patched.equals(contents.toString())) {
            return;
        }

        Error writeError = FileUtils.writeTextToFile("login", loginPath, Charset.defaultCharset(),
            patched, false);
        if (writeError != null) {
            Logger.logErrorExtended(LOG_TAG, "Failed to patch login script\n" + writeError);
            return;
        }
        try {
            //noinspection OctalInteger
            Os.chmod(loginPath, 0700);
        } catch (Exception e) {
            Logger.logStackTraceWithMessage(LOG_TAG, "Failed to chmod patched login", e);
        }
        Logger.logInfo(LOG_TAG, "Patched login to keep invapp-redirector in LD_PRELOAD");
    }

    /** Init {@link TermuxShellEnvironment} constants and caches. */
    public synchronized static void writeEnvironmentToFile(@NonNull Context currentPackageContext) {
        installRedirectorIntoPrefix(currentPackageContext);
        HashMap<String, String> environmentMap = new TermuxShellEnvironment().getEnvironment(currentPackageContext, false);
        String environmentString = ShellEnvironmentUtils.convertEnvironmentToDotEnvFile(environmentMap);

        // Write environment string to temp file and then move to final location since otherwise
        // writing may happen while file is being sourced/read
        Error error = FileUtils.writeTextToFile("termux.env.tmp", TermuxConstants.TERMUX_ENV_TEMP_FILE_PATH,
            Charset.defaultCharset(), environmentString, false);
        if (error != null) {
            Logger.logErrorExtended(LOG_TAG, error.toString());
            return;
        }

        error = FileUtils.moveRegularFile("termux.env.tmp", TermuxConstants.TERMUX_ENV_TEMP_FILE_PATH, TermuxConstants.TERMUX_ENV_FILE_PATH, true);
        if (error != null) {
            Logger.logErrorExtended(LOG_TAG, error.toString());
        }
    }

    /** Get shell environment for Termux. */
    @NonNull
    @Override
    public HashMap<String, String> getEnvironment(@NonNull Context currentPackageContext, boolean isFailSafe) {

        // Termux environment builds upon the Android environment
        HashMap<String, String> environment = super.getEnvironment(currentPackageContext, isFailSafe);

        HashMap<String, String> termuxAppEnvironment = TermuxAppShellEnvironment.getEnvironment(currentPackageContext);
        if (termuxAppEnvironment != null)
            environment.putAll(termuxAppEnvironment);

        HashMap<String, String> termuxApiAppEnvironment = TermuxAPIShellEnvironment.getEnvironment(currentPackageContext);
        if (termuxApiAppEnvironment != null)
            environment.putAll(termuxApiAppEnvironment);

        environment.put(ENV_HOME, TermuxConstants.TERMUX_HOME_DIR_PATH);
        environment.put(ENV_PREFIX, TermuxConstants.TERMUX_PREFIX_DIR_PATH);

        // If failsafe is not enabled, then we keep default PATH and TMPDIR so that system binaries can be used
        if (!isFailSafe) {
            environment.put(ENV_TMPDIR, TermuxConstants.TERMUX_TMP_PREFIX_DIR_PATH);

            // Path redirector: map hardcoded /data/data/com.termux → real package data dir.
            // Prefer $PREFIX/lib copy — APK nativeLibraryDir is often blocked as LD_PRELOAD
            // for executables under $PREFIX (Android linker namespaces).
            installRedirectorIntoPrefix(currentPackageContext);
            String nativeLibDir = currentPackageContext.getApplicationInfo().nativeLibraryDir;
            File prefixRedirector = new File(REDIRECTOR_PREFIX_LIB_PATH);
            String redirectorLib = prefixRedirector.exists()
                ? prefixRedirector.getAbsolutePath()
                : (nativeLibDir + "/" + REDIRECTOR_LIB_NAME);
            String existingPreload = environment.get("LD_PRELOAD");
            if (existingPreload != null && !existingPreload.isEmpty()) {
                environment.put("LD_PRELOAD", redirectorLib + ":" + existingPreload);
            } else {
                environment.put("LD_PRELOAD", redirectorLib);
            }

            if (TermuxBootstrap.isAppPackageVariantAPTAndroid5()) {
                // Termux in android 5/6 era shipped busybox binaries in applets directory
                environment.put(ENV_PATH, TermuxConstants.TERMUX_BIN_PREFIX_DIR_PATH + ":" + TermuxConstants.TERMUX_BIN_PREFIX_DIR_PATH + "/applets");
                environment.put(ENV_LD_LIBRARY_PATH, TermuxConstants.TERMUX_LIB_PREFIX_DIR_PATH);
            } else {
                // Include Bun global bin (~/.bun/bin) — bun pm bin -g warns when missing.
                environment.put(ENV_PATH,
                    TermuxConstants.TERMUX_BIN_PREFIX_DIR_PATH + ":"
                        + TermuxConstants.TERMUX_HOME_DIR_PATH + "/.bun/bin");
                // Stock Termux Android 7+ binaries embed DT_RUNPATH for /data/data/com.termux/.../lib.
                // Official Termux can leave LD_LIBRARY_PATH unset; renamed forks cannot in-place
                // patch ELF when the package name length differs, so the dynamic linker still
                // searches the com.termux RUNPATH and fails with "library ... not found".
                // Point LD_LIBRARY_PATH at the real prefix lib dir to fix that.
                if (!"com.termux".equals(TermuxConstants.TERMUX_PACKAGE_NAME)) {
                    environment.put(ENV_LD_LIBRARY_PATH, TermuxConstants.TERMUX_LIB_PREFIX_DIR_PATH);
                } else {
                    environment.remove(ENV_LD_LIBRARY_PATH);
                }
            }

            // Keep Bun / npm caches and installs inside the exec-capable app prefix,
            // never under /storage/emulated/0 (noexec → "Permission denied").
            environment.put("BUN_INSTALL", TermuxConstants.TERMUX_PREFIX_DIR_PATH);
            environment.put("BUN_INSTALL_BIN", TermuxConstants.TERMUX_BIN_PREFIX_DIR_PATH);
            environment.put("BUN_INSTALL_CACHE_DIR",
                TermuxConstants.TERMUX_HOME_DIR_PATH + "/.bun/install/cache");
            environment.put("BUN_TMPDIR", TermuxConstants.TERMUX_TMP_PREFIX_DIR_PATH);
            environment.put("XDG_CACHE_HOME",
                TermuxConstants.TERMUX_HOME_DIR_PATH + "/.cache");
            environment.put("npm_config_cache",
                TermuxConstants.TERMUX_HOME_DIR_PATH + "/.npm");
            environment.put("npm_config_prefix",
                TermuxConstants.TERMUX_PREFIX_DIR_PATH);
            // Stock Termux node/openssl are compiled with PREFIX=/data/data/com.termux/...
            // Point at this fork's TLS files so node/npm work even when a child
            // clears LD_PRELOAD (bunx → npm pack → node).
            environment.put("OPENSSL_CONF",
                TermuxConstants.TERMUX_PREFIX_DIR_PATH + "/etc/tls/openssl.cnf");
            environment.put("SSL_CERT_FILE",
                TermuxConstants.TERMUX_PREFIX_DIR_PATH + "/etc/tls/cert.pem");
            environment.put("NODE_EXTRA_CA_CERTS",
                TermuxConstants.TERMUX_PREFIX_DIR_PATH + "/etc/tls/cert.pem");
            // Do NOT set npm_config_platform/os here — npm treats npm_config_* as
            // config keys and warns "Unknown env config platform". Android platform
            // hints stay in the Bun wrapper only (TermuxBunInstaller).
        }

        return environment;
    }


    @NonNull
    @Override
    public String getDefaultWorkingDirectoryPath() {
        File repos = new File(TermuxConstants.TERMUX_HOME_DIR_PATH, "repos");
        if (!repos.exists()) {
            //noinspection ResultOfMethodCallIgnored
            repos.mkdirs();
        }
        if (repos.isDirectory() && repos.canRead()) {
            return repos.getAbsolutePath();
        }
        return TermuxConstants.TERMUX_HOME_DIR_PATH;
    }

    @NonNull
    @Override
    public String getDefaultBinPath() {
        return TermuxConstants.TERMUX_BIN_PREFIX_DIR_PATH;
    }

    @NonNull
    @Override
    public String[] setupShellCommandArguments(@NonNull String executable, String[] arguments) {
        return TermuxShellUtils.setupShellCommandArguments(executable, arguments);
    }

}
