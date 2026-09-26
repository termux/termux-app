package com.invapp.app;

import android.content.Context;
import android.system.Os;

import androidx.annotation.NonNull;

import com.invapp.shared.logger.Logger;
import com.invapp.shared.termux.TermuxConstants;

import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

/**
 * Installs the official Android Bun binary and a single {@code $PREFIX/bin/bun}
 * wrapper.
 *
 * <p>Real binary lives at {@code $PREFIX/libexec/bun}. The {@code bin/bun}
 * wrapper drops a bare path-redirector-only preload and instead loads
 * {@code libinvapp-bun-seccomp.so} (+ redirector for {@code #!/usr/bin/env}
 * shebang rewrite so bunx/package bins work). Sets OPENSSL + Android
 * {@code --os}/{@code --cpu} install filters.
 *
 * <p>Uses oven-sh {@code bun-linux-*-android.zip} (Bionic PIE), not glibc Linux
 * builds.
 */
public final class TermuxBunInstaller {

    private static final String LOG_TAG = "TermuxBunInstaller";

    /** Must match the zips downloaded in {@code app/build.gradle}. */
    public static final String BUNDLED_BUN_VERSION = "1.4.2";

    private static final String LIBEXEC_REL = "libexec/bun";

    private TermuxBunInstaller() {}

    /**
     * Extract bundled Bun into the Termux prefix when missing or outdated.
     * Safe to call on every app start; refreshes wrappers even when current.
     */
    public static void installIfNeeded(@NonNull Context context) {
        File binDir = new File(TermuxConstants.TERMUX_BIN_PREFIX_DIR_PATH);
        if (!binDir.isDirectory()) {
            Logger.logWarn(LOG_TAG, "Prefix bin missing; skip Bun install");
            return;
        }

        File libexecDir = new File(TermuxConstants.TERMUX_PREFIX_DIR_PATH, "libexec");
        File bunReal = new File(TermuxConstants.TERMUX_PREFIX_DIR_PATH, LIBEXEC_REL);
        File stamp = new File(TermuxConstants.TERMUX_PREFIX_DIR_PATH,
            "var/lib/invapp/bun.version");

        boolean realOk = bunReal.isFile() && bunReal.canExecute()
            && isAndroidBunBinary(bunReal);
        if (realOk && stamp.isFile()) {
            try {
                if (BUNDLED_BUN_VERSION.equals(readStamp(stamp))) {
                    installShellHelpers();
                    installOpencodeShimFromAssets(context);
                    ensureWorkspaceDirs();
                    return;
                }
            } catch (Exception ignored) {
                // Reinstall below.
            }
        }

        if (bunReal.isFile() && !isAndroidBunBinary(bunReal)) {
            Logger.logWarn(LOG_TAG,
                "Replacing non-Android Bun at " + bunReal
                    + " (curl|bash glibc builds → required file not found / SIGSYS 31)");
        }

        byte[] zipBytes;
        try {
            zipBytes = loadZipBytes();
        } catch (UnsatisfiedLinkError e) {
            Logger.logWarn(LOG_TAG, "libinvapp-bun not loaded: " + e.getMessage());
            return;
        }
        if (zipBytes == null || zipBytes.length == 0) {
            Logger.logInfo(LOG_TAG,
                "No bundled Bun for this ABI (need aarch64 or x86_64)");
            return;
        }

        try {
            if (!libexecDir.exists() && !libexecDir.mkdirs()) {
                Logger.logWarn(LOG_TAG, "Could not create " + libexecDir);
            }

            File staging = new File(libexecDir, "bun.new");
            if (staging.exists() && !staging.delete()) {
                Logger.logWarn(LOG_TAG, "Could not delete old staging bun");
            }

            boolean extracted = false;
            try (ZipInputStream zip = new ZipInputStream(
                    new ByteArrayInputStream(zipBytes))) {
                ZipEntry entry;
                byte[] buffer = new byte[8192];
                while ((entry = zip.getNextEntry()) != null) {
                    String name = entry.getName();
                    if (entry.isDirectory()) {
                        continue;
                    }
                    if (!name.endsWith("/bun") && !name.equals("bun")) {
                        continue;
                    }
                    try (FileOutputStream out = new FileOutputStream(staging)) {
                        int n;
                        while ((n = zip.read(buffer)) != -1) {
                            out.write(buffer, 0, n);
                        }
                    }
                    extracted = true;
                    break;
                }
            }

            if (!extracted || !staging.isFile()) {
                Logger.logError(LOG_TAG, "Bun binary not found inside bundled zip");
                //noinspection ResultOfMethodCallIgnored
                staging.delete();
                return;
            }

            //noinspection OctalInteger
            Os.chmod(staging.getAbsolutePath(), 0700);
            if (bunReal.exists() && !bunReal.delete()) {
                Logger.logWarn(LOG_TAG, "Could not replace existing libexec bun");
            }
            if (!staging.renameTo(bunReal)) {
                throw new RuntimeException("Failed to move bun into libexec");
            }

            // Remove legacy ELF that used to live at $PREFIX/bin/bun.
            File legacyBin = new File(binDir, "bun");
            if (legacyBin.isFile() && isAndroidBunBinary(legacyBin)) {
                //noinspection ResultOfMethodCallIgnored
                legacyBin.delete();
            }

            File stampDir = stamp.getParentFile();
            if (stampDir != null && !stampDir.exists() && !stampDir.mkdirs()) {
                Logger.logWarn(LOG_TAG, "Could not create stamp directory");
            }
            try (FileOutputStream out = new FileOutputStream(stamp)) {
                out.write(BUNDLED_BUN_VERSION.getBytes(
                    java.nio.charset.StandardCharsets.UTF_8));
            }

            Logger.logInfo(LOG_TAG,
                "Installed Bun " + BUNDLED_BUN_VERSION + " → " + bunReal);

            ensureWorkspaceDirs();
        } catch (Exception e) {
            Logger.logStackTraceWithMessage(LOG_TAG, "Bun install failed", e);
        }

        installShellHelpers();
        installOpencodeShimFromAssets(context);
    }

    /**
     * Ships a prebuilt glibc {@code LD_PRELOAD} shim so OpenCode can resolve DNS
     * without {@code gcc-glibc}. Stock glibc hardcodes {@code /data/data/com.termux}
     * paths; the shim intercepts {@code gethostbyname2}/{@code getaddrinfo} and
     * queries 8.8.8.8 directly (and-code instead uses musl OpenCode under proot).
     */
    private static void installOpencodeShimFromAssets(@NonNull Context context) {
        String abi = android.os.Build.SUPPORTED_ABIS.length > 0
            ? android.os.Build.SUPPORTED_ABIS[0] : "";
        String asset;
        if ("arm64-v8a".equals(abi) || "aarch64".equals(abi)) {
            asset = "opencode/libinvapp-opencode-shim-aarch64.so";
        } else if ("x86_64".equals(abi)) {
            // Built on demand via opencode-setup when gcc-glibc is present.
            return;
        } else {
            return;
        }
        File libDir = new File(TermuxConstants.TERMUX_LIB_PREFIX_DIR_PATH);
        File dest = new File(libDir, "libinvapp-opencode-shim.so");
        try {
            if (!libDir.exists() && !libDir.mkdirs()) {
                Logger.logWarn(LOG_TAG, "Could not create " + libDir);
                return;
            }
            try (java.io.InputStream in = context.getAssets().open(asset);
                 FileOutputStream out = new FileOutputStream(dest)) {
                byte[] buf = new byte[8192];
                int n;
                while ((n = in.read(buf)) != -1) {
                    out.write(buf, 0, n);
                }
            }
            //noinspection OctalInteger
            Os.chmod(dest.getAbsolutePath(), 0755);
            Logger.logInfo(LOG_TAG, "Installed OpenCode DNS shim → " + dest);
        } catch (Exception e) {
            Logger.logWarn(LOG_TAG, "OpenCode shim asset install failed: " + e.getMessage());
        }
    }

    private static void ensureWorkspaceDirs() {
        ensureDir(new File(TermuxConstants.TERMUX_HOME_DIR_PATH,
            ".bun/install/cache"));
        ensureDir(new File(TermuxConstants.TERMUX_HOME_DIR_PATH, ".bun/bin"));
        ensureDir(new File(TermuxConstants.TERMUX_HOME_DIR_PATH, ".cache"));
        ensureDir(new File(TermuxConstants.TERMUX_HOME_DIR_PATH, ".npm"));
        ensureDir(new File(TermuxConstants.TERMUX_HOME_DIR_PATH, "repos"));
    }

    /**
     * Writes {@code $PREFIX/bin/bun} and {@code bunx} wrappers. No per-package
     * helpers — all CLIs go through the single shim.
     */
    private static void installShellHelpers() {
        File binDir = new File(TermuxConstants.TERMUX_BIN_PREFIX_DIR_PATH);
        if (!binDir.isDirectory()) {
            return;
        }
        String prefix = TermuxConstants.TERMUX_PREFIX_DIR_PATH;
        String home = TermuxConstants.TERMUX_HOME_DIR_PATH;
        String bash = prefix + "/bin/bash";

        // Drop obsolete per-package helper if present from earlier builds.
        File oldCreateExpo = new File(binDir, "create-expo");
        if (oldCreateExpo.isFile()) {
            //noinspection ResultOfMethodCallIgnored
            oldCreateExpo.delete();
        }

        String bunWrapper = ""
            + "#!" + bash + "\n"
            + "PREFIX=\"" + prefix + "\"\n"
            + "HOME=\"" + home + "\"\n"
            + "REAL=\"$PREFIX/libexec/bun\"\n"
            + "export PATH=\"$PREFIX/bin:$HOME/.bun/bin:$PATH\"\n"
            + "export LD_LIBRARY_PATH=\"$PREFIX/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}\"\n"
            + "export OPENSSL_CONF=\"$PREFIX/etc/tls/openssl.cnf\"\n"
            + "export SSL_CERT_FILE=\"$PREFIX/etc/tls/cert.pem\"\n"
            + "export NODE_EXTRA_CA_CERTS=\"$PREFIX/etc/tls/cert.pem\"\n"
            + "export BUN_INSTALL=\"$PREFIX\"\n"
            + "export BUN_INSTALL_BIN=\"$PREFIX/bin\"\n"
            + "export BUN_INSTALL_CACHE_DIR=\"$HOME/.bun/install/cache\"\n"
            + "export BUN_TMPDIR=\"$PREFIX/tmp\"\n"
            + "export TMPDIR=\"$PREFIX/tmp\"\n"
            + "export XDG_CACHE_HOME=\"$HOME/.cache\"\n"
            + "export npm_config_cache=\"$HOME/.npm\"\n"
            + "export npm_config_prefix=\"$PREFIX\"\n"
            + "export npm_config_platform=android\n"
            + "export npm_config_os=android\n"
            + "case \"$(uname -m)\" in\n"
            + "  aarch64|arm64) export npm_config_arch=arm64; export npm_config_cpu=arm64 ;;\n"
            + "  x86_64|amd64)  export npm_config_arch=x64;   export npm_config_cpu=x64 ;;\n"
            + "  *)             export npm_config_arch=arm64; export npm_config_cpu=arm64 ;;\n"
            + "esac\n"
            + "mkdir -p \"$BUN_INSTALL_CACHE_DIR\" \"$HOME/.bun/bin\" \"$HOME/repos\" \"$TMPDIR\"\n"
            + "if [ ! -x \"$REAL\" ]; then\n"
            + "  echo \"bun: missing Android binary at $REAL (reopen the app)\" >&2\n"
            + "  exit 127\n"
            + "fi\n"
            // Seccomp SIGSYS→ENOSYS (openat2/fchmodat2) PLUS path redirector so
            // bunx/child bins with #!/usr/bin/env node get shebang rewrite.
            // Redirector alone used to SIGSYS on install; seccomp fixes that.
            + "SECCOMP_SO=\"$PREFIX/lib/libinvapp-bun-seccomp.so\"\n"
            + "REDIRECTOR_SO=\"$PREFIX/lib/libinvapp-redirector.so\"\n"
            + "preload=\"\"\n"
            + "[ -f \"$SECCOMP_SO\" ] && preload=\"$SECCOMP_SO\"\n"
            + "[ -f \"$REDIRECTOR_SO\" ] && preload=\"${preload:+$preload:}$REDIRECTOR_SO\"\n"
            + "export LD_PRELOAD=\"$preload\"\n"
            + "set -- \"$@\"\n"
            + "cmd=\"${1-}\"\n"
            // Force Android optionalDependency filter. Bun reports platform=android but
            // still resolves linux-* natives (e.g. @rolldown/binding-linux-arm-gnueabihf)
            // for many scaffolds / Windows lockfiles → bad optional extracts.
            + "case \"$cmd\" in\n"
            + "  install|i|add|update|remove|rm|create)\n"
            + "    shift\n"
            + "    has_os=0; has_cpu=0\n"
            + "    for a in \"$@\"; do\n"
            + "      case \"$a\" in --os|--os=*) has_os=1 ;; --cpu|--cpu=*) has_cpu=1 ;; esac\n"
            + "    done\n"
            + "    extra=\"\"\n"
            + "    [ \"$has_os\" = 0 ] && extra=\"$extra --os=android\"\n"
            + "    if [ \"$has_cpu\" = 0 ]; then\n"
            + "      case \"$(uname -m)\" in aarch64|arm64) extra=\"$extra --cpu=arm64\" ;;\n"
            + "        x86_64|amd64) extra=\"$extra --cpu=x64\" ;; *) extra=\"$extra --cpu=arm64\" ;; esac\n"
            + "    fi\n"
            + "    # shellcheck disable=SC2086\n"
            + "    exec \"$REAL\" \"$cmd\"$extra \"$@\"\n"
            + "    ;;\n"
            + "esac\n"
            + "exec \"$REAL\" \"$@\"\n";
        writeExec(new File(binDir, "bun"), bunWrapper);

        // bunx must run package bins under Bun. Stock `bun x` execs the bin
        // shebang (#!/usr/bin/env node); without node that exits 127 and often
        // prints nothing — the "silent fail" users hit for create-vite / serve.
        String bunx = ""
            + "#!" + bash + "\n"
            + "PREFIX=\"" + prefix + "\"\n"
            + "has_bun=0\n"
            + "for a in \"$@\"; do\n"
            + "  case \"$a\" in --bun) has_bun=1 ;; esac\n"
            + "done\n"
            + "if [ \"$has_bun\" = 1 ]; then\n"
            + "  exec \"$PREFIX/bin/bun\" x \"$@\"\n"
            + "fi\n"
            + "exec \"$PREFIX/bin/bun\" x --bun \"$@\"\n";
        writeExec(new File(binDir, "bunx"), bunx);

        // Provide `node` → bun when nodejs package is not installed so
        // #!/usr/bin/env node scripts (npm bins, bunx children) actually run.
        ensureNodeBunShim(binDir, bash, prefix);

        String bunDoctor = ""
            + "#!" + bash + "\n"
            + "PREFIX=\"" + prefix + "\"\n"
            + "HOME=\"" + home + "\"\n"
            + "echo \"wrapper: $PREFIX/bin/bun\"\n"
            + "echo \"real:    $PREFIX/libexec/bun\"\n"
            + "ls -la \"$PREFIX/bin/bun\" \"$PREFIX/libexec/bun\" \"$PREFIX/bin/bunx\" \"$PREFIX/bin/node\" 2>&1\n"
            + "if command -v readelf >/dev/null; then\n"
            + "  readelf -l \"$PREFIX/libexec/bun\" 2>/dev/null | grep -A1 INTERP || true\n"
            + "fi\n"
            + "\"$PREFIX/bin/bun\" --version\n"
            + "echo \"node → $(command -v node 2>/dev/null || echo missing)\"\n"
            + "if [ -f \"$PREFIX/bin/node\" ]; then head -2 \"$PREFIX/bin/node\"; fi\n"
            + "echo \"LD_PRELOAD in shell: ${LD_PRELOAD:-unset}\"\n"
            + "echo \"OPENSSL_CONF=${OPENSSL_CONF:-unset}\"\n"
            + "echo \"--- bunx smoke ---\"\n"
            + "bunx --bun cowsay ok 2>&1 | head -15 || true\n";
        writeExec(new File(binDir, "bun-doctor"), bunDoctor);

        // OpenCode bootstrap (optional AI CLI → Preview).
        // No opencode-android-* (anomalyco/opencode#12515). Do NOT bun-install
        // opencode-ai: bare libexec/bun hits SIGSYS (seccomp), and bun's
        // global bin is a JS stub that demands postinstall. Fetch the official
        // linux-glibc tarball from GitHub releases, then wrap with ld-linux.
        // glibc lives in glibc-repo (not tur-repo); trusted=yes for apt-key flakes.
        String opencodeSetup = ""
            + "#!" + bash + "\n"
            + "set -e\n"
            + "PREFIX=\"" + prefix + "\"\n"
            + "HOME=\"" + home + "\"\n"
            + "export PATH=\"$PREFIX/bin:$HOME/.bun/bin:$PATH\"\n"
            + "export LD_LIBRARY_PATH=\"$PREFIX/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}\"\n"
            + "export TMPDIR=\"${TMPDIR:-$PREFIX/tmp}\"\n"
            + "mkdir -p \"$TMPDIR\"\n"
            + "REDIRECTOR_SO=\"$PREFIX/lib/libinvapp-redirector.so\"\n"
            + "[ -f \"$REDIRECTOR_SO\" ] && export LD_PRELOAD=\"$REDIRECTOR_SO${LD_PRELOAD:+:$LD_PRELOAD}\"\n"
            // Drop bun's JS stub immediately — it only prints the postinstall error.
            + "rm -f \"$PREFIX/bin/opencode\" \"$HOME/.bun/bin/opencode\" 2>/dev/null || true\n"
            + "case \"$(uname -m)\" in\n"
            + "  aarch64|arm64) OC_ASSET=opencode-linux-arm64.tar.gz; OC_LD=ld-linux-aarch64.so.1 ;;\n"
            + "  x86_64|amd64)  OC_ASSET=opencode-linux-x64.tar.gz;  OC_LD=ld-linux-x86-64.so.2 ;;\n"
            + "  *)             OC_ASSET=opencode-linux-arm64.tar.gz; OC_LD=ld-linux-aarch64.so.1 ;;\n"
            + "esac\n"
            + "OC_DIR=\"$PREFIX/libexec/opencode\"\n"
            + "OC_BIN=\"$OC_DIR/opencode\"\n"
            + "OC_VER=\"${OPENCODE_VERSION:-}\"\n"
            + "if [ -n \"$OC_VER\" ]; then\n"
            + "  case \"$OC_VER\" in v*) ;; *) OC_VER=\"v$OC_VER\" ;; esac\n"
            + "  OC_URL=\"https://github.com/anomalyco/opencode/releases/download/$OC_VER/$OC_ASSET\"\n"
            + "else\n"
            + "  OC_URL=\"https://github.com/anomalyco/opencode/releases/latest/download/$OC_ASSET\"\n"
            + "fi\n"
            + "echo \"Downloading $OC_ASSET (official linux binary; no bun install)…\"\n"
            + "echo \"  $OC_URL\"\n"
            + "mkdir -p \"$OC_DIR\" \"$TMPDIR\"\n"
            + "OC_TAR=\"$TMPDIR/$OC_ASSET\"\n"
            + "if command -v curl >/dev/null 2>&1; then\n"
            + "  curl -fL --retry 3 --retry-delay 2 -o \"$OC_TAR\" \"$OC_URL\"\n"
            + "elif command -v wget >/dev/null 2>&1; then\n"
            + "  wget -O \"$OC_TAR\" \"$OC_URL\"\n"
            + "else\n"
            + "  echo \"opencode-setup: need curl or wget\" >&2\n"
            + "  exit 127\n"
            + "fi\n"
            + "rm -rf \"$OC_DIR.new\"\n"
            + "mkdir -p \"$OC_DIR.new\"\n"
            + "tar -xzf \"$OC_TAR\" -C \"$OC_DIR.new\"\n"
            + "rm -f \"$OC_TAR\"\n"
            + "FOUND=$(find \"$OC_DIR.new\" -type f -name opencode 2>/dev/null | head -1)\n"
            + "if [ -z \"$FOUND\" ] || [ ! -f \"$FOUND\" ]; then\n"
            + "  echo \"opencode-setup: tarball had no opencode binary\" >&2\n"
            + "  rm -rf \"$OC_DIR.new\"\n"
            + "  exit 1\n"
            + "fi\n"
            + "chmod +x \"$FOUND\"\n"
            + "rm -rf \"$OC_DIR\"\n"
            + "mv \"$OC_DIR.new\" \"$OC_DIR\"\n"
            + "OC_BIN=$(find \"$OC_DIR\" -type f -name opencode 2>/dev/null | head -1)\n"
            + "if [ -z \"$OC_BIN\" ] || [ ! -x \"$OC_BIN\" ]; then\n"
            + "  echo \"opencode-setup: install failed (binary missing)\" >&2\n"
            + "  exit 1\n"
            + "fi\n"
            // glibc packages hardcode /data/data/com.termux — rewrite for this app id.
            // Prefer ld-linux --library-path over stock grun (grun also hardcodes com.termux
            // and does not pass --library-path, so libc.so.6 resolves to EACCES).
            + "fixup_glibc_paths() {\n"
            + "  local f\n"
            + "  for f in \"$PREFIX/opt/glibc-runner/glibc-runner.sh\" \"$PREFIX/bin/grun\"; do\n"
            + "    [ -f \"$f\" ] || continue\n"
            + "    grep -q 'com\\.termux' \"$f\" 2>/dev/null || continue\n"
            + "    sed -i 's|/data/data/com\\.termux/files|"
            + TermuxConstants.TERMUX_FILES_DIR_PATH + "|g' \"$f\" 2>/dev/null || true\n"
            + "  done\n"
            // Linker scripts named libc.so break runtime loads (invalid ELF). Prefer SONAME symlinks.
            + "  if [ -f \"$PREFIX/glibc/lib/libc.so.6\" ]; then\n"
            + "    ln -sfn libc.so.6 \"$PREFIX/glibc/lib/libc.so\" 2>/dev/null || true\n"
            + "  fi\n"
            + "  if [ -f \"$PREFIX/glibc/lib/libm.so.6\" ]; then\n"
            + "    ln -sfn libm.so.6 \"$PREFIX/glibc/lib/libm.so\" 2>/dev/null || true\n"
            + "  fi\n"
            + "  ln -sfn libdl.so.2 \"$PREFIX/glibc/lib/libdl.so\" 2>/dev/null || true\n"
            + "  ln -sfn libpthread.so.0 \"$PREFIX/glibc/lib/libpthread.so\" 2>/dev/null || true\n"
            // nostdlib stub: some glibc loads look for liblog; system liblog is Bionic-only.
            + "  if [ ! -f \"$PREFIX/glibc/lib/liblog.so\" ] && [ -x \"$PREFIX/bin/clang\" ]; then\n"
            + "    cat > \"$PREFIX/tmp/invapp-liblog-stub.c\" <<'STUBEOF'\n"
            + "int __android_log_write(int a, const char* b, const char* c) { (void)a;(void)b;(void)c; return 0; }\n"
            + "int __android_log_print(int a, const char* b, const char* c, ...) { (void)a;(void)b;(void)c; return 0; }\n"
            + "int __android_log_vprint(int a, const char* b, const char* c, void* d) { (void)a;(void)b;(void)c;(void)d; return 0; }\n"
            + "void __android_log_assert(const char* a, const char* b, const char* c, ...) { (void)a;(void)b;(void)c; }\n"
            + "STUBEOF\n"
            + "    \"$PREFIX/bin/clang\" -shared -nostdlib -fPIC -Wl,-soname,liblog.so \\\n"
            + "      -o \"$PREFIX/glibc/lib/liblog.so\" \"$PREFIX/tmp/invapp-liblog-stub.c\" 2>/dev/null || true\n"
            + "  fi\n"
            // Bionic curl works; glibc OpenCode reads resolv/CA under $PREFIX/glibc/etc
            // (and sometimes hardcoded /etc/*). Seed both so models.opencode.ai resolves.
            + "  mkdir -p \"$PREFIX/etc\" \"$PREFIX/glibc/etc/ssl/certs\"\n"
            + "  if [ ! -s \"$PREFIX/etc/resolv.conf\" ]; then\n"
            + "    printf '%s\\n' 'nameserver 8.8.8.8' 'nameserver 1.1.1.1' > \"$PREFIX/etc/resolv.conf\"\n"
            + "  fi\n"
            + "  cp -f \"$PREFIX/etc/resolv.conf\" \"$PREFIX/glibc/etc/resolv.conf\" 2>/dev/null || \\\n"
            + "    printf '%s\\n' 'nameserver 8.8.8.8' 'nameserver 1.1.1.1' > \"$PREFIX/glibc/etc/resolv.conf\"\n"
            + "  if [ ! -s \"$PREFIX/etc/hosts\" ]; then\n"
            + "    printf '%s\\n' '127.0.0.1 localhost' '::1 localhost' > \"$PREFIX/etc/hosts\"\n"
            + "  fi\n"
            + "  cp -f \"$PREFIX/etc/hosts\" \"$PREFIX/glibc/etc/hosts\" 2>/dev/null || true\n"
            + "  if [ -f \"$PREFIX/etc/tls/cert.pem\" ]; then\n"
            + "    ln -sfn \"$PREFIX/etc/tls/cert.pem\" \\\n"
            + "      \"$PREFIX/glibc/etc/ssl/certs/ca-certificates.crt\" 2>/dev/null || true\n"
            + "    ln -sfn \"$PREFIX/etc/tls/cert.pem\" \\\n"
            + "      \"$PREFIX/glibc/etc/ssl/cert.pem\" 2>/dev/null || true\n"
            + "  fi\n"
            + "}\n"
            + "ensure_glibc() {\n"
            + "  if [ -x \"$PREFIX/glibc/lib/$OC_LD\" ]; then\n"
            + "    fixup_glibc_paths\n"
            + "    return 0\n"
            + "  fi\n"
            + "  echo \"opencode-setup: installing glibc (linux binaries need it on Android)…\"\n"
            + "  if ! command -v pkg >/dev/null 2>&1 && ! command -v apt-get >/dev/null 2>&1; then\n"
            + "    return 1\n"
            + "  fi\n"
            + "  mkdir -p \"$PREFIX/etc/apt/sources.list.d\"\n"
            // trusted=yes: apt-key under path-redirector sometimes fails on new repos.
            + "  printf '%s\\n' \\\n"
            + "    'deb [trusted=yes] https://packages-cf.termux.dev/apt/termux-glibc/ glibc stable' \\\n"
            + "    > \"$PREFIX/etc/apt/sources.list.d/glibc.list\"\n"
            + "  export DEBIAN_FRONTEND=noninteractive\n"
            + "  pkg install -y glibc-repo 2>/dev/null || true\n"
            + "  apt-get update -y 2>/dev/null || pkg update -y 2>/dev/null || true\n"
            + "  pkg install -y glibc glibc-runner 2>/dev/null \\\n"
            + "    || apt-get install -y glibc glibc-runner 2>/dev/null \\\n"
            + "    || pkg install -y glibc 2>/dev/null \\\n"
            + "    || apt-get install -y glibc 2>/dev/null \\\n"
            + "    || true\n"
            + "  pkg install -y openssl-glibc ca-certificates gcc-glibc 2>/dev/null \\\n"
            + "    || apt-get install -y openssl-glibc ca-certificates gcc-glibc 2>/dev/null \\\n"
            + "    || true\n"
            + "  if [ ! -x \"$PREFIX/glibc/lib/$OC_LD\" ]; then\n"
            + "    return 1\n"
            + "  fi\n"
            + "  fixup_glibc_paths\n"
            + "  return 0\n"
            + "}\n"
            + "if ! ensure_glibc; then\n"
            + "  echo \"opencode-setup: glibc missing.\" >&2\n"
            + "  echo \"  pkg install glibc-repo && pkg install glibc glibc-runner\" >&2\n"
            + "  echo \"Then re-run: opencode-setup\" >&2\n"
            + "  exit 1\n"
            + "fi\n"
            + "WRAPPER=\"$PREFIX/bin/opencode\"\n"
            + "rm -f \"$WRAPPER\" \"$HOME/.bun/bin/opencode\" 2>/dev/null || true\n"
            + "cat > \"$WRAPPER\" <<EOF\n"
            + "#!$PREFIX/bin/bash\n"
            + "# invapp-opencode-wrapper — OpenCode linux binary via glibc ld-linux\n"
            + "OC_BIN=\"$OC_BIN\"\n"
            + "PREFIX=\"$PREFIX\"\n"
            + "OC_LD=\"$OC_LD\"\n"
            + "LD=\"\\$PREFIX/glibc/lib/\\$OC_LD\"\n"
            + "LIB=\"\\$PREFIX/glibc/lib\"\n"
            + "export TMPDIR=\"\\${TMPDIR:-\\$PREFIX/tmp}\"\n"
            + "mkdir -p \"\\$TMPDIR\" 2>/dev/null || true\n"
            + "if [ ! -x \"\\$OC_BIN\" ]; then\n"
            + "  echo \"opencode: binary missing — run opencode-setup\" >&2\n"
            + "  exit 127\n"
            + "fi\n"
            + "if [ ! -x \"\\$LD\" ]; then\n"
            + "  echo \"opencode: glibc missing — run opencode-setup\" >&2\n"
            + "  exit 127\n"
            + "fi\n"
            // Empty LD_PRELOAD= (not unset): path redirector / termux-exec
            // reinject when the key is absent OR when LD_PRELOAD is non-empty
            // (they append libinvapp-redirector.so → "version `LIBC' not found"
            // under glibc). Load the DNS shim via ld-linux --preload instead.
            + "export LD_PRELOAD=\n"
            + "OC_SHIM=\"\\$PREFIX/lib/libinvapp-opencode-shim.so\"\n"
            + "OC_PRELOAD_ARGS=\n"
            + "if [ -f \"\\$OC_SHIM\" ]; then\n"
            + "  OC_PRELOAD_ARGS=\"--preload \\$OC_SHIM\"\n"
            + "elif [ -f \"\\$PREFIX/lib/libinvapp-getifaddrs.so\" ]; then\n"
            + "  OC_PRELOAD_ARGS=\"--preload \\$PREFIX/lib/libinvapp-getifaddrs.so\"\n"
            + "fi\n"
            + "export LD_LIBRARY_PATH=\n"
            // Seed DNS for glibc (Bionic curl ignores this; OpenCode does not).
            + "mkdir -p \"\\$PREFIX/glibc/etc/ssl/certs\" 2>/dev/null || true\n"
            + "if [ ! -s \"\\$PREFIX/etc/resolv.conf\" ]; then\n"
            + "  printf '%s\\n' 'nameserver 8.8.8.8' 'nameserver 1.1.1.1' > \"\\$PREFIX/etc/resolv.conf\"\n"
            + "fi\n"
            + "cp -f \"\\$PREFIX/etc/resolv.conf\" \"\\$PREFIX/glibc/etc/resolv.conf\" 2>/dev/null || true\n"
            // glibc OpenCode needs Termux CA bundle (Bun wrapper already sets these).
            + "if [ -f \"\\$PREFIX/etc/tls/cert.pem\" ]; then\n"
            + "  export SSL_CERT_FILE=\"\\$PREFIX/etc/tls/cert.pem\"\n"
            + "  export SSL_CERT_DIR=\"\\$PREFIX/etc/tls/certs\"\n"
            + "  export CURL_CA_BUNDLE=\"\\$PREFIX/etc/tls/cert.pem\"\n"
            + "  export NODE_EXTRA_CA_CERTS=\"\\$PREFIX/etc/tls/cert.pem\"\n"
            + "  ln -sfn \"\\$PREFIX/etc/tls/cert.pem\" \\\n"
            + "    \"\\$PREFIX/glibc/etc/ssl/certs/ca-certificates.crt\" 2>/dev/null || true\n"
            + "fi\n"
            + "export PATH=\"\\$PREFIX/bin:\\$PATH\"\n"
            + "# shellcheck disable=SC2086\n"
            + "exec \"\\$LD\" --library-path \"\\$LIB\" \\$OC_PRELOAD_ARGS \"\\$OC_BIN\" \"\\$@\"\n"
            + "EOF\n"
            + "chmod 700 \"$WRAPPER\"\n"
            + "mkdir -p \"$HOME/.bun/bin\" \"$PREFIX/lib\"\n"
            // glibc LD_PRELOAD shim: getaddrinfo DNS fallback (8.8.8.8),
            // /etc → PREFIX redirects, loopback getifaddrs. Bun OpenCode does
            // not use Bionic DNS; without this, models.opencode.ai / Kilo fail
            // with "Unable to connect" / "typo in the url or port".
            + "build_opencode_shim() {\n"
            + "  local stub_c=\"$PREFIX/tmp/invapp-opencode-shim.c\"\n"
            + "  local stub_so=\"$PREFIX/lib/libinvapp-opencode-shim.so\"\n"
            + "  local stub_map=\"$PREFIX/tmp/invapp-opencode-shim.map\"\n"
            + "  local cc=\"\"\n"
            // Never put glibc/bin first — those ELFs break bash/grep (Permission denied).
            + "  if [ -f \"$stub_so\" ] && [ -s \"$stub_so\" ]; then\n"
            + "    echo \"opencode-setup: using existing DNS shim → $stub_so\"\n"
            + "    return 0\n"
            + "  fi\n"
            + "  for cc in \"$PREFIX/glibc/bin/gcc\" \"$PREFIX/bin/aarch64-linux-gnu-gcc\" \\\n"
            + "            \"$PREFIX/bin/x86_64-linux-gnu-gcc\" \"$PREFIX/bin/gcc-glibc\"; do\n"
            + "    [ -x \"$cc\" ] || continue\n"
            + "    break\n"
            + "  done\n"
            + "  if [ -z \"$cc\" ] || [ ! -x \"$cc\" ]; then\n"
            + "    echo \"opencode-setup: no gcc-glibc — keep shipped shim or: pkg install gcc-glibc\" >&2\n"
            + "    return 0\n"
            + "  fi\n"
            + "  cat > \"$stub_c\" <<'STUBC'\n"
            + "#define _GNU_SOURCE\n"
            + "#include <arpa/inet.h>\n"
            + "#include <dlfcn.h>\n"
            + "#include <errno.h>\n"
            + "#include <fcntl.h>\n"
            + "#include <ifaddrs.h>\n"
            + "#include <net/if.h>\n"
            + "#include <netdb.h>\n"
            + "#include <netinet/in.h>\n"
            + "#include <stdarg.h>\n"
            + "#include <stdint.h>\n"
            + "#include <stdio.h>\n"
            + "#include <stdlib.h>\n"
            + "#include <string.h>\n"
            + "#include <sys/socket.h>\n"
            + "#include <sys/types.h>\n"
            + "#include <time.h>\n"
            + "#include <unistd.h>\n"
            + "static const char *INVAPP_RESOLV =\""
            + TermuxConstants.TERMUX_ETC_PREFIX_DIR_PATH + "/resolv.conf\";\n"
            + "static const char *INVAPP_HOSTS =\""
            + TermuxConstants.TERMUX_ETC_PREFIX_DIR_PATH + "/hosts\";\n"
            + "static const char *INVAPP_CERT =\""
            + TermuxConstants.TERMUX_ETC_PREFIX_DIR_PATH + "/tls/cert.pem\";\n"
            + "static const char *INVAPP_PREFIX =\""
            + TermuxConstants.TERMUX_PREFIX_DIR_PATH + "\";\n"
            + "static const char *STOCK_PREFIX =\"/data/data/com.termux/files/usr\";\n"
            + "static char g_redir_buf[512];\n"
            + "static const char *redirect_path(const char *path) {\n"
            + "  size_t stock_len;\n"
            + "  if (!path) return path;\n"
            + "  if (strcmp(path, \"/etc/resolv.conf\") == 0) return INVAPP_RESOLV;\n"
            + "  if (strcmp(path, \"/etc/hosts\") == 0) return INVAPP_HOSTS;\n"
            + "  if (strcmp(path, \"/etc/ssl/cert.pem\") == 0) return INVAPP_CERT;\n"
            + "  if (strcmp(path, \"/etc/ssl/certs/ca-certificates.crt\") == 0) return INVAPP_CERT;\n"
            + "  if (strcmp(path, \"/etc/pki/tls/certs/ca-bundle.crt\") == 0) return INVAPP_CERT;\n"
            + "  stock_len = strlen(STOCK_PREFIX);\n"
            + "  if (strncmp(path, STOCK_PREFIX, stock_len) == 0) {\n"
            + "    size_t rest = strlen(path + stock_len);\n"
            + "    if (strlen(INVAPP_PREFIX) + rest + 1 > sizeof(g_redir_buf)) return path;\n"
            + "    memcpy(g_redir_buf, INVAPP_PREFIX, strlen(INVAPP_PREFIX) + 1);\n"
            + "    memcpy(g_redir_buf + strlen(INVAPP_PREFIX), path + stock_len, rest + 1);\n"
            + "    return g_redir_buf;\n"
            + "  }\n"
            + "  return path;\n"
            + "}\n"
            + "int open(const char *path, int flags, ...) {\n"
            + "  static int (*real_open)(const char *, int, ...) = 0;\n"
            + "  if (!real_open) real_open = (int (*)(const char *, int, ...))dlsym(RTLD_NEXT, \"open\");\n"
            + "  path = redirect_path(path);\n"
            + "  if (flags & O_CREAT) { va_list ap; va_start(ap, flags); mode_t m=(mode_t)va_arg(ap,int); va_end(ap); return real_open(path, flags, m); }\n"
            + "  return real_open(path, flags);\n"
            + "}\n"
            + "int openat(int dirfd, const char *path, int flags, ...) {\n"
            + "  static int (*real_openat)(int, const char *, int, ...) = 0;\n"
            + "  if (!real_openat) real_openat = (int (*)(int, const char *, int, ...))dlsym(RTLD_NEXT, \"openat\");\n"
            + "  path = redirect_path(path);\n"
            + "  if (flags & O_CREAT) { va_list ap; va_start(ap, flags); mode_t m=(mode_t)va_arg(ap,int); va_end(ap); return real_openat(dirfd, path, flags, m); }\n"
            + "  return real_openat(dirfd, path, flags);\n"
            + "}\n"
            + "typedef int (*ga_fn)(const char*,const char*,const struct addrinfo*,struct addrinfo**);\n"
            + "static ga_fn real_ga(void) { static ga_fn c; if (!c) c=(ga_fn)dlsym(RTLD_NEXT,\"getaddrinfo\"); return c; }\n"
            + "static int encode_qname(const char *host, unsigned char *out, size_t outlen) {\n"
            + "  size_t used=0; const char *p=host;\n"
            + "  while (*p) { const char *dot=strchr(p,'.'); size_t len=dot?(size_t)(dot-p):strlen(p);\n"
            + "    if (len==0||len>63||used+len+2>outlen) return -1;\n"
            + "    out[used++]=(unsigned char)len; memcpy(out+used,p,len); used+=len;\n"
            + "    if (!dot) break; p=dot+1; if (!*p) break; }\n"
            + "  if (used+1>outlen) return -1; out[used++]=0; return (int)used;\n"
            + "}\n"
            + "static int skip_name(const unsigned char *pkt, int len, int off) {\n"
            + "  while (off < len) { unsigned char l=pkt[off]; if (l==0) return off+1;\n"
            + "    if ((l&0xC0)==0xC0) return off+2<=len?off+2:-1; off+=l+1; } return -1;\n"
            + "}\n"
            + "static int query_a(const char *server, const char *host, struct in_addr *addrs, int max_addrs) {\n"
            + "  unsigned char pkt[1500]; struct sockaddr_in sa; struct timeval tv; struct timespec ts;\n"
            + "  int fd,qlen,off,qdcount,ancount,found=0; uint16_t id; ssize_t n;\n"
            + "  memset(&sa,0,sizeof(sa)); sa.sin_family=AF_INET; sa.sin_port=htons(53);\n"
            + "  if (inet_pton(AF_INET,server,&sa.sin_addr)!=1) return -1;\n"
            + "  clock_gettime(CLOCK_MONOTONIC,&ts); id=(uint16_t)((ts.tv_nsec^(getpid()<<8))&0xFFFF);\n"
            + "  memset(pkt,0,12); pkt[0]=id>>8; pkt[1]=id&0xFF; pkt[2]=0x01; pkt[5]=0x01;\n"
            + "  qlen=encode_qname(host,pkt+12,sizeof(pkt)-16); if (qlen<0) return -1;\n"
            + "  off=12+qlen; pkt[off++]=0; pkt[off++]=1; pkt[off++]=0; pkt[off++]=1;\n"
            + "  fd=socket(AF_INET,SOCK_DGRAM,0); if (fd<0) return -1;\n"
            + "  tv.tv_sec=3; tv.tv_usec=0; setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));\n"
            + "  if (sendto(fd,pkt,off,0,(struct sockaddr*)&sa,sizeof(sa))!=off) { close(fd); return -1; }\n"
            + "  n=recvfrom(fd,pkt,sizeof(pkt),0,NULL,NULL); close(fd);\n"
            + "  if (n<12||pkt[0]!=(id>>8)||pkt[1]!=(id&0xFF)||(pkt[3]&0x0F)) return -1;\n"
            + "  qdcount=(pkt[4]<<8)|pkt[5]; ancount=(pkt[6]<<8)|pkt[7]; off=12;\n"
            + "  for (int i=0;i<qdcount;i++) { off=skip_name(pkt,(int)n,off); if (off<0||off+4>n) return -1; off+=4; }\n"
            + "  for (int i=0;i<ancount&&found<max_addrs;i++) {\n"
            + "    int type,rdlen; off=skip_name(pkt,(int)n,off); if (off<0||off+10>n) break;\n"
            + "    type=(pkt[off]<<8)|pkt[off+1]; rdlen=(pkt[off+8]<<8)|pkt[off+9]; off+=10;\n"
            + "    if (off+rdlen>n) break; if (type==1&&rdlen==4) memcpy(&addrs[found++],pkt+off,4); off+=rdlen;\n"
            + "  } return found;\n"
            + "}\n"
            + "static uint16_t resolve_port(const char *service) {\n"
            + "  if (!service||!*service) return 0; char *end; long v=strtol(service,&end,10);\n"
            + "  if (*end=='\\0'&&v>=0&&v<=65535) return (uint16_t)v;\n"
            + "  if (!strcmp(service,\"https\")) return 443; if (!strcmp(service,\"http\")) return 80; return 0;\n"
            + "}\n"
            + "static int build_result(const struct in_addr *addrs, int count, uint16_t port,\n"
            + "  const struct addrinfo *hints, struct addrinfo **res) {\n"
            + "  struct addrinfo *head=NULL,*tail=NULL;\n"
            + "  int socktype=hints&&hints->ai_socktype?hints->ai_socktype:SOCK_STREAM;\n"
            + "  int protocol=hints&&hints->ai_protocol?hints->ai_protocol:(socktype==SOCK_DGRAM?IPPROTO_UDP:IPPROTO_TCP);\n"
            + "  for (int i=0;i<count;i++) {\n"
            + "    struct addrinfo *ai=calloc(1,sizeof(*ai)); struct sockaddr_in *sin=calloc(1,sizeof(*sin));\n"
            + "    if (!ai||!sin) { free(ai); free(sin); freeaddrinfo(head); return EAI_MEMORY; }\n"
            + "    sin->sin_family=AF_INET; sin->sin_port=htons(port); sin->sin_addr=addrs[i];\n"
            + "    ai->ai_family=AF_INET; ai->ai_socktype=socktype; ai->ai_protocol=protocol;\n"
            + "    ai->ai_addrlen=sizeof(*sin); ai->ai_addr=(struct sockaddr*)sin;\n"
            + "    if (tail) tail->ai_next=ai; else head=ai; tail=ai;\n"
            + "  } if (!head) return EAI_NONAME; *res=head; return 0;\n"
            + "}\n"
            + "int getaddrinfo(const char *node, const char *service,\n"
            + "  const struct addrinfo *hints, struct addrinfo **res) {\n"
            + "  ga_fn orig=real_ga(); struct in_addr addrs[8]; int rc;\n"
            + "  static const char *ns[] = {\"8.8.8.8\",\"1.1.1.1\",\"8.8.4.4\"};\n"
            + "  if (!node||!*node) { if (!orig) return EAI_SYSTEM; return orig(node,service,hints,res); }\n"
            + "  if (hints) { if (hints->ai_flags&AI_NUMERICHOST) { if (!orig) return EAI_SYSTEM; return orig(node,service,hints,res); }\n"
            + "    if (hints->ai_family!=AF_UNSPEC&&hints->ai_family!=AF_INET) { if (!orig) return EAI_SYSTEM; return orig(node,service,hints,res); } }\n"
            + "  for (int i=0;i<3;i++) {\n"
            + "    int found=query_a(ns[i],node,addrs,8);\n"
            + "    if (found>0) { int brc=build_result(addrs,found,resolve_port(service),hints,res);\n"
            + "      return brc!=0?brc:0; }\n"
            + "  } if (!orig) return EAI_SYSTEM; return orig(node,service,hints,res);\n"
            + "}\n"
            + "static struct hostent g_he; static char g_he_name[256]; static char *g_he_aliases[1];\n"
            + "static struct in_addr g_he_addrs[8]; static char *g_he_addr_list[9];\n"
            + "struct hostent *gethostbyname2(const char *name, int af) {\n"
            + "  static struct hostent *(*real_ghn2)(const char*,int)=0; struct in_addr addrs[8]; int found=0;\n"
            + "  static const char *ns[] = {\"8.8.8.8\",\"1.1.1.1\",\"8.8.4.4\"};\n"
            + "  if (!real_ghn2) real_ghn2=(struct hostent*(*)(const char*,int))dlsym(RTLD_NEXT,\"gethostbyname2\");\n"
            + "  if (af!=AF_INET&&af!=AF_UNSPEC) return real_ghn2?real_ghn2(name,af):NULL;\n"
            + "  if (!name||!*name) return real_ghn2?real_ghn2(name,af):NULL;\n"
            + "  for (int i=0;i<3&&found<=0;i++) found=query_a(ns[i],name,addrs,8);\n"
            + "  if (found<=0) return real_ghn2?real_ghn2(name,af):NULL;\n"
            + "  memset(&g_he,0,sizeof(g_he)); strncpy(g_he_name,name,sizeof(g_he_name)-1);\n"
            + "  g_he_aliases[0]=NULL; for (int i=0;i<found;i++) { g_he_addrs[i]=addrs[i]; g_he_addr_list[i]=(char*)&g_he_addrs[i]; }\n"
            + "  g_he_addr_list[found]=NULL; g_he.h_name=g_he_name; g_he.h_aliases=g_he_aliases;\n"
            + "  g_he.h_addrtype=AF_INET; g_he.h_length=4; g_he.h_addr_list=g_he_addr_list; return &g_he;\n"
            + "}\n"
            + "struct hostent *gethostbyname(const char *name) { return gethostbyname2(name,AF_INET); }\n"
            + "int getifaddrs(struct ifaddrs **ifap) {\n"
            + "  struct ifaddrs *ifa; struct sockaddr_in *addr,*mask;\n"
            + "  if (!ifap) { errno=EINVAL; return -1; }\n"
            + "  ifa=(struct ifaddrs*)calloc(1,sizeof(*ifa));\n"
            + "  addr=(struct sockaddr_in*)calloc(1,sizeof(*addr));\n"
            + "  mask=(struct sockaddr_in*)calloc(1,sizeof(*mask));\n"
            + "  if (!ifa||!addr||!mask) { free(ifa); free(addr); free(mask); errno=ENOMEM; return -1; }\n"
            + "  ifa->ifa_name=strdup(\"lo\"); ifa->ifa_flags=IFF_UP|IFF_LOOPBACK|IFF_RUNNING;\n"
            + "  addr->sin_family=AF_INET; addr->sin_addr.s_addr=htonl(INADDR_LOOPBACK);\n"
            + "  mask->sin_family=AF_INET; mask->sin_addr.s_addr=htonl(0xFF000000u);\n"
            + "  ifa->ifa_addr=(struct sockaddr*)addr; ifa->ifa_netmask=(struct sockaddr*)mask;\n"
            + "  *ifap=ifa; return 0;\n"
            + "}\n"
            + "void freeifaddrs(struct ifaddrs *ifa) {\n"
            + "  while (ifa) { struct ifaddrs *n=ifa->ifa_next;\n"
            + "    free(ifa->ifa_name); free(ifa->ifa_addr); free(ifa->ifa_netmask); free(ifa); ifa=n; }\n"
            + "}\n"
            + "STUBC\n"
            + "  cat > \"$stub_map\" <<'STUBMAP'\n"
            + "GLIBC_2.17 {\n"
            + "  global: getaddrinfo; gethostbyname; gethostbyname2; open; open64; openat; fopen; getifaddrs; freeifaddrs;\n"
            + "  local: *;\n"
            + "};\n"
            + "STUBMAP\n"
            + "  if \"$cc\" -shared -fPIC -O2 -ldl -Wl,--version-script=\"$stub_map\" -o \"$stub_so\" \"$stub_c\" 2>/dev/null; then\n"
            + "    echo \"opencode-setup: installed DNS/getifaddrs shim → $stub_so\"\n"
            + "    rm -f \"$PREFIX/lib/libinvapp-getifaddrs.so\"\n"
            + "  else\n"
            + "    echo \"opencode-setup: shim compile failed (need gcc-glibc)\" >&2\n"
            + "    rm -f \"$stub_so\"\n"
            + "  fi\n"
            + "  rm -f \"$stub_c\" \"$stub_map\"\n"
            + "}\n"
            + "build_opencode_shim\n"
            + "rm -f \"$HOME/.bun/bin/opencode\" 2>/dev/null || true\n"
            + "cp -f \"$WRAPPER\" \"$HOME/.bun/bin/opencode\"\n"
            + "chmod 700 \"$HOME/.bun/bin/opencode\"\n"
            + "hash -r 2>/dev/null || true\n"
            + "export PATH=\"$PREFIX/bin:$HOME/.bun/bin:$PATH\"\n"
            + "if ! head -1 \"$WRAPPER\" | grep -q bash; then\n"
            + "  echo \"opencode-setup: wrapper write failed (still a bun symlink?)\" >&2\n"
            + "  exit 1\n"
            + "fi\n"
            + "if ! command -v opencode >/dev/null 2>&1; then\n"
            + "  echo \"opencode-setup: wrapper not on PATH\" >&2\n"
            + "  exit 1\n"
            + "fi\n"
            + "echo \"opencode-setup: probing opencode --version…\"\n"
            + "if opencode --version 2>&1; then\n"
            // Clear broken provider cache (@opencode-ai/plugin@local → token mismatch)
            // that surfaces as AI_APICallError \"typo in the url or port\".
            + "  rm -rf \"$HOME/.cache/opencode\" 2>/dev/null || true\n"
            + "  for pkg in \"$HOME/.config/opencode/package.json\" \"$HOME/repos\"/*/.opencode/package.json; do\n"
            + "    [ -f \"$pkg\" ] || continue\n"
            + "    if grep -q '@opencode-ai/plugin@local\\|\"@opencode-ai/plugin\": \"local\"' \"$pkg\" 2>/dev/null; then\n"
            + "      echo \"opencode-setup: fixing plugin@local in $pkg\"\n"
            + "      sed -i 's/@opencode-ai\\/plugin@local/@opencode-ai\\/plugin@latest/g; s/\"@opencode-ai\\/plugin\": \"local\"/\"@opencode-ai\\/plugin\": \"latest\"/g' \"$pkg\" 2>/dev/null || true\n"
            + "    fi\n"
            + "  done\n"
            + "  echo \"OK. Run: td-ai   # binds 0.0.0.0:4096; Preview → http://127.0.0.1:4096/\"\n"
            + "  echo \"If chat fails with 'typo in the url or port': rm -rf ~/.cache/opencode && opencode-fix-net && td-ai\"\n"
            + "else\n"
            + "  echo \"opencode-setup: wrapper installed but binary failed under glibc\" >&2\n"
            + "  echo \"Binary: $OC_BIN\" >&2\n"
            + "  exit 1\n"
            + "fi\n";
        writeExec(new File(binDir, "opencode-setup"), opencodeSetup);

        // Fast path: rebuild DNS shim + seed resolv/CA without re-downloading OpenCode.
        String opencodeFixNet = ""
            + "#!" + bash + "\n"
            + "set -e\n"
            + "PREFIX=\"" + prefix + "\"\n"
            + "HOME=\"" + home + "\"\n"
            + "export PATH=\"$PREFIX/bin:$HOME/.bun/bin:$PATH\"\n"
            + "echo \"opencode-fix-net: seeding DNS/CA…\"\n"
            + "mkdir -p \"$PREFIX/glibc/etc/ssl/certs\" \"$PREFIX/lib\" \"$PREFIX/tmp\"\n"
            + "printf '%s\\n' 'nameserver 8.8.8.8' 'nameserver 1.1.1.1' | tee \\\n"
            + "  \"$PREFIX/etc/resolv.conf\" \"$PREFIX/glibc/etc/resolv.conf\" >/dev/null\n"
            + "printf '%s\\n' '127.0.0.1 localhost' '::1 localhost' | tee \\\n"
            + "  \"$PREFIX/etc/hosts\" \"$PREFIX/glibc/etc/hosts\" >/dev/null\n"
            + "pkg install -y openssl-glibc ca-certificates 2>/dev/null || true\n"
            + "if [ -f \"$PREFIX/etc/tls/cert.pem\" ]; then\n"
            + "  ln -sfn \"$PREFIX/etc/tls/cert.pem\" \\\n"
            + "    \"$PREFIX/glibc/etc/ssl/certs/ca-certificates.crt\"\n"
            + "fi\n"
            + "rm -rf \"$HOME/.cache/opencode\" 2>/dev/null || true\n"
            + "echo \"opencode-fix-net: rebuilding shim via opencode-setup (keeps binary if present)…\"\n"
            // Re-run only the shim portion by invoking setup's rebuild — setup redownloads.
            // Instead compile shim inline (same sources as setup).
            + "if [ ! -x \"$PREFIX/bin/opencode\" ]; then\n"
            + "  echo \"opencode missing — run opencode-setup first\" >&2\n"
            + "  exit 1\n"
            + "fi\n"
            + "OPENCODE_VERSION=\"${OPENCODE_VERSION:-}\" opencode-setup\n"
            + "if [ -f \"$PREFIX/lib/libinvapp-opencode-shim.so\" ]; then\n"
            + "  echo \"OK: shim present → $PREFIX/lib/libinvapp-opencode-shim.so\"\n"
            + "else\n"
            + "  echo \"WARN: shim missing — install gcc-glibc and re-run\" >&2\n"
            + "fi\n"
            + "echo \"Test (Bionic): curl -sI https://models.opencode.ai/api.json | head -2\"\n"
            + "echo \"Then: cd ~/repos/copilot && pkill -f opencode; td-ai\"\n";
        writeExec(new File(binDir, "opencode-fix-net"), opencodeFixNet);

        // OpenJDK 17 via pkg (Termux-built JVM). Not embedded in the APK (~96 MiB deb
        // per arch); use td-jdk-setup for recommends + recovery from common failures.
        String tdJdkSetup = ""
            + "#!" + bash + "\n"
            + "set -e\n"
            + "PREFIX=\"" + prefix + "\"\n"
            + "HOME=\"" + home + "\"\n"
            + "export PATH=\"$PREFIX/bin:$HOME/.bun/bin:$PATH\"\n"
            + "REDIRECTOR_SO=\"$PREFIX/lib/libinvapp-redirector.so\"\n"
            + "[ -f \"$REDIRECTOR_SO\" ] && export LD_PRELOAD=\"$REDIRECTOR_SO${LD_PRELOAD:+:$LD_PRELOAD}\"\n"
            + "export TMPDIR=\"${TMPDIR:-$PREFIX/tmp}\"\n"
            + "mkdir -p \"$TMPDIR\"\n"
            + "if [ -x \"$PREFIX/bin/invapp-openjdk-recover\" ]; then\n"
            + "  \"$PREFIX/bin/invapp-openjdk-recover\" || true\n"
            + "fi\n"
            + "JDK_HOME=\"$PREFIX/lib/jvm/java-17-openjdk\"\n"
            + "if [ -x \"$JDK_HOME/bin/java\" ]; then\n"
            + "  if \"$JDK_HOME/bin/java\" -version 2>&1 | head -1; then\n"
            + "    echo \"OpenJDK 17 already installed at $JDK_HOME\"\n"
            + "    echo \"JAVA_HOME=$JDK_HOME\"\n"
            + "    exit 0\n"
            + "  fi\n"
            + "fi\n"
            + "echo \"td-jdk-setup: installing Termux OpenJDK 17 (not Debian/Ubuntu JDK)…\"\n"
            + "echo \"Note: ~220 MiB on disk; 'Setting up openjdk-17' may sit on update-alternatives for a few minutes.\"\n"
            + "if ! command -v pkg >/dev/null 2>&1; then\n"
            + "  echo \"td-jdk-setup: pkg missing — finish bootstrap first\" >&2\n"
            + "  exit 1\n"
            + "fi\n"
            + "if [ -x \"$PREFIX/bin/invapp-fix-dpkg-scripts\" ]; then\n"
            + "  \"$PREFIX/bin/invapp-fix-dpkg-scripts\" || true\n"
            + "fi\n"
            + "pkg update -y || apt-get update -y\n"
            + "dpkg --configure -a 2>/dev/null || true\n"
            + "deps=\"libandroid-shmem libandroid-spawn libiconv libjpeg-turbo zlib littlecms alsa-plugins\"\n"
            + "recs=\"ca-certificates-java openjdk-17-x resolv-conf\"\n"
            + "# Install openjdk-17 before -x (avoids broken Depends if preinst fails).\n"
            + "if ! pkg install -y $deps openjdk-17; then\n"
            + "  echo \"td-jdk-setup: pkg install openjdk-17 failed — trying repack helper…\" >&2\n"
            + "  if [ -x \"$PREFIX/bin/invapp-openjdk-repack-install\" ]; then\n"
            + "    \"$PREFIX/bin/invapp-openjdk-repack-install\" || true\n"
            + "  fi\n"
            + "fi\n"
            + "if [ ! -x \"$JDK_HOME/bin/java\" ]; then\n"
            + "  apt-get install -y $deps openjdk-17 2>/dev/null || true\n"
            + "fi\n"
            + "if [ -x \"$JDK_HOME/bin/java\" ]; then\n"
            + "  pkg install -y $recs 2>/dev/null || apt-get install -y $recs 2>/dev/null || true\n"
            + "fi\n"
            + "dpkg --configure -a 2>/dev/null || true\n"
            + "if [ ! -x \"$JDK_HOME/bin/java\" ]; then\n"
            + "  echo \"td-jdk-setup: java binary missing after install\" >&2\n"
            + "  echo \"Run: jdk-doctor   then: pkg install -f openjdk-17\" >&2\n"
            + "  exit 1\n"
            + "fi\n"
            + "if ! \"$JDK_HOME/bin/java\" -version 2>&1; then\n"
            + "  echo \"td-jdk-setup: java -version failed (Bad system call?)\" >&2\n"
            + "  echo \"Use only pkg openjdk-17 — not proot/apt Debian JDK on the host prefix.\" >&2\n"
            + "  echo \"Ensure libandroid-shmem is installed: pkg install libandroid-shmem\" >&2\n"
            + "  exit 1\n"
            + "fi\n"
            + "echo \"OK. JAVA_HOME=$JDK_HOME\"\n"
            + "echo \"New shell sessions load JAVA_HOME from $JDK_HOME/etc/profile.d/java.sh\"\n";
        writeExec(new File(binDir, "td-jdk-setup"), tdJdkSetup);

        String invappPkg = TermuxConstants.TERMUX_PACKAGE_NAME;
        String fixDpkgScripts = ""
            + "#!" + bash + "\n"
            + "PREFIX=\"" + prefix + "\"\n"
            + "OLD=\"com.termux\"\n"
            + "NEW=\"" + invappPkg + "\"\n"
            + "fixdir() {\n"
            + "  local d=\"$1\"\n"
            + "  [ -d \"$d\" ] || return 0\n"
            + "  local f\n"
            + "  for f in \"$d\"/*; do\n"
            + "    [ -f \"$f\" ] || continue\n"
            + "    if ! head -1 \"$f\" 2>/dev/null | grep -q '^#!'; then\n"
            + "      { echo \"#!$PREFIX/bin/sh\"; cat \"$f\"; } > \"$f.new\" && mv \"$f.new\" \"$f\"\n"
            + "    fi\n"
            + "    sed -i \"s|$OLD|$NEW|g\" \"$f\" 2>/dev/null || true\n"
            + "    case \"$(head -1 \"$f\" 2>/dev/null)\" in\n"
            + "      '#!/bin/sh'*) sed -i \"1s|^#!/bin/sh|#!$PREFIX/bin/sh|\" \"$f\" 2>/dev/null || true ;;\n"
            + "      '#!/bin/bash'*) sed -i \"1s|^#!/bin/bash|#!$PREFIX/bin/bash|\" \"$f\" 2>/dev/null || true ;;\n"
            + "    esac\n"
            + "    chmod 700 \"$f\" 2>/dev/null || true\n"
            + "  done\n"
            + "}\n"
            + "fixdir \"$PREFIX/var/lib/dpkg/tmp.ci\"\n"
            + "fixdir \"$PREFIX/var/lib/dpkg/info\"\n"
            + "echo \"invapp-fix-dpkg-scripts: patched maintainer scripts under tmp.ci and info\"\n";
        writeExec(new File(binDir, "invapp-fix-dpkg-scripts"), fixDpkgScripts);

        String openjdkRecover = ""
            + "#!" + bash + "\n"
            + "PREFIX=\"" + prefix + "\"\n"
            + "export PATH=\"$PREFIX/bin:$PATH\"\n"
            + "REDIRECTOR_SO=\"$PREFIX/lib/libinvapp-redirector.so\"\n"
            + "[ -f \"$REDIRECTOR_SO\" ] && export LD_PRELOAD=\"$REDIRECTOR_SO${LD_PRELOAD:+:$LD_PRELOAD}\"\n"
            + "echo \"invapp-openjdk-recover: clearing broken openjdk-17 / openjdk-17-x state…\"\n"
            + "dpkg --remove --force-remove-reinstreq openjdk-17-x 2>/dev/null || true\n"
            + "dpkg --remove --force-remove-reinstreq openjdk-17 2>/dev/null || true\n"
            + "dpkg --purge --force-remove-reinstreq openjdk-17-x 2>/dev/null || true\n"
            + "dpkg --purge --force-remove-reinstreq openjdk-17 2>/dev/null || true\n"
            + "rm -rf \"$PREFIX/var/lib/dpkg/tmp.ci\"/* 2>/dev/null || true\n"
            + "apt --fix-broken install -y 2>/dev/null || true\n"
            + "dpkg --configure -a 2>/dev/null || true\n"
            + "echo \"invapp-openjdk-recover: done (check: dpkg -l | grep openjdk)\"\n";
        writeExec(new File(binDir, "invapp-openjdk-recover"), openjdkRecover);

        String openjdkRepackInstall = ""
            + "#!" + bash + "\n"
            + "set -e\n"
            + "PREFIX=\"" + prefix + "\"\n"
            + "export PATH=\"$PREFIX/bin:$PATH\"\n"
            + "REDIRECTOR_SO=\"$PREFIX/lib/libinvapp-redirector.so\"\n"
            + "[ -f \"$REDIRECTOR_SO\" ] && export LD_PRELOAD=\"$REDIRECTOR_SO${LD_PRELOAD:+:$LD_PRELOAD}\"\n"
            + "DEB=\"\"\n"
            + "for c in \"$HOME/../cache/apt/archives/openjdk-17_\"*_aarch64.deb \\\n"
            + "         \"$PREFIX/../cache/apt/archives/openjdk-17_\"*_aarch64.deb; do\n"
            + "  [ -f \"$c\" ] && DEB=\"$c\" && break\n"
            + "done\n"
            + "if [ -z \"$DEB\" ]; then\n"
            + "  echo \"invapp-openjdk-repack-install: download deb first (pkg install openjdk-17)\" >&2\n"
            + "  exit 1\n"
            + "fi\n"
            + "WORK=\"$PREFIX/tmp/openjdk-repack\"\n"
            + "rm -rf \"$WORK\" && mkdir -p \"$WORK\" && cd \"$WORK\"\n"
            + "ar x \"$DEB\"\n"
            + "tar xf control.tar.xz\n"
            + "for s in preinst postinst prerm postrm; do\n"
            + "  [ -f \"$s\" ] || continue\n"
            + "  if ! head -1 \"$s\" | grep -q '^#!'; then\n"
            + "    { echo \"#!$PREFIX/bin/sh\"; cat \"$s\"; } > \"$s.new\" && mv \"$s.new\" \"$s\"\n"
            + "  fi\n"
            + "  sed -i \"s/com.termux/com.involvex.termux_app/g\" \"$s\" 2>/dev/null || true\n"
            + "  chmod 700 \"$s\"\n"
            + "done\n"
            + "tar -cJf control.tar.xz ./*\n"
            + "printf '2.0\\n' > debian-binary\n"
            + "ar cr openjdk-17-fixed.deb debian-binary control.tar.xz data.tar.xz\n"
            + "dpkg -i ./openjdk-17-fixed.deb\n"
            + "dpkg --configure -a\n"
            + "echo \"invapp-openjdk-repack-install: openjdk-17 installed; run: pkg install openjdk-17-x\"\n";
        writeExec(new File(binDir, "invapp-openjdk-repack-install"), openjdkRepackInstall);

        String jdkDoctor = ""
            + "#!" + bash + "\n"
            + "PREFIX=\"" + prefix + "\"\n"
            + "HOME=\"" + home + "\"\n"
            + "export PATH=\"$PREFIX/bin:$HOME/.bun/bin:$PATH\"\n"
            + "REDIRECTOR_SO=\"$PREFIX/lib/libinvapp-redirector.so\"\n"
            + "[ -f \"$REDIRECTOR_SO\" ] && export LD_PRELOAD=\"$REDIRECTOR_SO${LD_PRELOAD:+:$LD_PRELOAD}\"\n"
            + "echo \"--- java on PATH ---\"\n"
            + "command -v java 2>&1 || echo \"java: not on PATH\"\n"
            + "ls -la \"$PREFIX/bin/java\" 2>&1 || true\n"
            + "echo \"--- JVM tree ---\"\n"
            + "ls -la \"$PREFIX/lib/jvm/\" 2>&1 || true\n"
            + "echo \"--- java -version ---\"\n"
            + "java -version 2>&1 || echo \"java -version failed (see docs: openjdk troubleshooting)\"\n"
            + "echo \"--- pkg ---\"\n"
            + "pkg show openjdk-17 2>&1 | head -20 || apt-cache show openjdk-17 2>&1 | head -20 || true\n"
            + "echo \"--- shmem / iconv (common dlopen failures) ---\"\n"
            + "pkg show libandroid-shmem libiconv 2>&1 | grep -E '^Package:|^Status:' || true\n"
            + "echo \"--- JAVA_HOME ---\"\n"
            + "echo \"JAVA_HOME=${JAVA_HOME:-unset}\"\n"
            + "cat \"$PREFIX/lib/jvm/java-17-openjdk/etc/profile.d/java.sh\" 2>/dev/null || true\n";
        writeExec(new File(binDir, "jdk-doctor"), jdkDoctor);

        String tdAi = ""
            + "#!" + bash + "\n"
            + "PREFIX=\"" + prefix + "\"\n"
            + "HOME=\"" + home + "\"\n"
            + "export PATH=\"$PREFIX/bin:$HOME/.bun/bin:$PATH\"\n"
            // OpenCode: bind 0.0.0.0 for LAN; Preview still uses 127.0.0.1
            // https://opencode.ai/docs/server/ — health: GET /global/health
            // Never use --mdns (getifaddrs noise / failures on Android).
            + "PORT=\"${1:-4096}\"\n"
            + "case \"$PORT\" in\n"
            + "  ''|*[!0-9]*) echo \"usage: td-ai [port]\" >&2; exit 2 ;;\n"
            + "esac\n"
            + "HOST=\"${OPENCODE_HOST:-0.0.0.0}\"\n"
            + "LOCAL=\"http://127.0.0.1:$PORT\"\n"
            + "BASE=\"$LOCAL\"\n"
            // Prefer a project dir (FFF refuses $HOME as workspace root).
            + "if [ \"$(pwd -P 2>/dev/null)\" = \"$HOME\" ] || [ \"$(pwd -P 2>/dev/null)\" = \"$HOME/\" ]; then\n"
            + "  mkdir -p \"$HOME/repos\"\n"
            + "  cd \"$HOME/repos\" || true\n"
            + "fi\n"
            // Seed glibc DNS/CA even before wrapper (helps first-run).
            + "mkdir -p \"$PREFIX/glibc/etc/ssl/certs\" 2>/dev/null || true\n"
            + "if [ ! -s \"$PREFIX/etc/resolv.conf\" ]; then\n"
            + "  printf '%s\\n' 'nameserver 8.8.8.8' 'nameserver 1.1.1.1' > \"$PREFIX/etc/resolv.conf\"\n"
            + "fi\n"
            + "cp -f \"$PREFIX/etc/resolv.conf\" \"$PREFIX/glibc/etc/resolv.conf\" 2>/dev/null || true\n"
            + "export SSL_CERT_FILE=\"${SSL_CERT_FILE:-$PREFIX/etc/tls/cert.pem}\"\n"
            + "export NODE_EXTRA_CA_CERTS=\"${NODE_EXTRA_CA_CERTS:-$PREFIX/etc/tls/cert.pem}\"\n"
            + "export CURL_CA_BUNDLE=\"${CURL_CA_BUNDLE:-$PREFIX/etc/tls/cert.pem}\"\n"
            // Already healthy? Do not start a second server.
            + "if curl -fsS --connect-timeout 1 --max-time 2 \"$LOCAL/global/health\" 2>/dev/null | grep -qi healthy; then\n"
            + "  echo \"td-ai: OpenCode already healthy → $LOCAL/\"\n"
            + "  echo \"  health: $LOCAL/global/health\"\n"
            + "  echo \"  openapi: $LOCAL/doc\"\n"
            + "  echo \"  bind: $HOST:$PORT (Preview: $LOCAL — LAN: drawer → Preview → Copy LAN)\"\n"
            + "  exit 0\n"
            + "fi\n"
            + "if (echo >/dev/tcp/127.0.0.1/\"$PORT\") >/dev/null 2>&1; then\n"
            + "  echo \"td-ai: :$PORT is up but /global/health failed — stop other listeners or: drawer → Stop AI\" >&2\n"
            + "  exit 1\n"
            + "fi\n"
            + "if ! command -v bun >/dev/null 2>&1; then\n"
            + "  echo \"td-ai: bun missing — reopen the app\" >&2\n"
            + "  exit 127\n"
            + "fi\n"
            + "if ! command -v opencode >/dev/null 2>&1; then\n"
            + "  echo \"opencode not found — running opencode-setup…\"\n"
            + "  opencode-setup || exit $?\n"
            + "  export PATH=\"$PREFIX/bin:$HOME/.bun/bin:$PATH\"\n"
            + "  hash -r 2>/dev/null || true\n"
            + "fi\n"
            + "if ! command -v opencode >/dev/null 2>&1; then\n"
            + "  echo \"td-ai: opencode still missing after setup\" >&2\n"
            + "  exit 127\n"
            + "fi\n"
            + "LAN_HINT=\"\"\n"
            + "if command -v ip >/dev/null 2>&1; then\n"
            + "  LAN_IP=$(ip -4 route get 1.1.1.1 2>/dev/null | awk '{for(i=1;i<=NF;i++) if($i==\"src\"){print $(i+1); exit}}')\n"
            + "  [ -n \"$LAN_IP\" ] && LAN_HINT=\"http://$LAN_IP:$PORT\"\n"
            + "fi\n"
            + "echo \"\"\n"
            + "echo \"OpenCode web → bind $HOST:$PORT\"\n"
            + "echo \"  Preview  $LOCAL/\"\n"
            + "echo \"  health   GET $LOCAL/global/health\"\n"
            + "echo \"  openapi  GET $LOCAL/doc\"\n"
            + "if [ -n \"$LAN_HINT\" ]; then\n"
            + "  echo \"  LAN      $LAN_HINT  (same Wi‑Fi; firewall/VPN may block)\"\n"
            + "else\n"
            + "  echo \"  LAN      drawer → Preview → Copy LAN (when bound on 0.0.0.0)\"\n"
            + "fi\n"
            + "echo \"  No --mdns. Override bind: OPENCODE_HOST=127.0.0.1 td-ai\"\n"
            + "echo \"\"\n"
            // Prefer web UI for Preview; fall back to serve (API-only).
            + "if opencode web --help >/dev/null 2>&1; then\n"
            + "  exec opencode web --port \"$PORT\" --hostname \"$HOST\" --print-logs\n"
            + "fi\n"
            + "if opencode serve --help >/dev/null 2>&1; then\n"
            + "  echo \"td-ai: 'web' missing — starting API-only serve (no UI)\" >&2\n"
            + "  exec opencode serve --port \"$PORT\" --hostname \"$HOST\" --print-logs\n"
            + "fi\n"
            + "echo \"td-ai: opencode has no web/serve command — try: opencode --help\" >&2\n"
            + "exec opencode --help\n";
        writeExec(new File(binDir, "td-ai"), tdAi);

        // Dev-server helper: clear errors + Preview/LAN hints before bun run.
        String tdDev = ""
            + "#!" + bash + "\n"
            + "PREFIX=\"" + prefix + "\"\n"
            + "HOME=\"" + home + "\"\n"
            + "export PATH=\"$PREFIX/bin:$HOME/.bun/bin:$PATH\"\n"
            + "SCRIPT=\"${1:-dev}\"\n"
            + "if [ \"$#\" -gt 0 ]; then shift; fi\n"
            + "if ! command -v bun >/dev/null 2>&1; then\n"
            + "  echo \"td-dev: bun missing — reopen the app\" >&2\n"
            + "  exit 127\n"
            + "fi\n"
            + "if [ ! -f package.json ]; then\n"
            + "  echo \"td-dev: no package.json in $(pwd)\" >&2\n"
            + "  echo \"hint: cd ~/repos/<project> first\" >&2\n"
            + "  exit 1\n"
            + "fi\n"
            + "echo \"td-dev: bun run $SCRIPT $*\"\n"
            + "echo \"When the port appears: drawer → Preview → Scan (or Copy LAN)\"\n"
            + "echo \"\"\n"
            + "exec bun run \"$SCRIPT\" \"$@\"\n";
        writeExec(new File(binDir, "td-dev"), tdDev);

        // Vite golden-path scaffold under ~/repos (create-vite + host 0.0.0.0).
        // pwa / pwa-react overlay vite-plugin-pwa on vanilla / react bases.
        File libexecDir = new File(TermuxConstants.TERMUX_PREFIX_DIR_PATH, "libexec");
        ensureDir(libexecDir);
        String pwaConfigJs = ""
            + "// Writes vite.config.js with VitePWA (+ optional React plugin).\n"
            + "// Usage: bun \"$PREFIX/libexec/invapp-vite-pwa.mjs\" [react]\n"
            + "import { writeFileSync, existsSync, unlinkSync } from \"fs\";\n"
            + "import { basename } from \"path\";\n"
            + "\n"
            + "const name = basename(process.cwd());\n"
            + "const isReact = process.argv[2] === \"react\";\n"
            + "const reactImport = isReact\n"
            + "  ? \"import react from '@vitejs/plugin-react'\\n\"\n"
            + "  : \"\";\n"
            + "const reactPlug = isReact ? \"react(), \" : \"\";\n"
            + "const cfg =\n"
            + "  \"import { defineConfig } from 'vite'\\n\" +\n"
            + "  \"import { VitePWA } from 'vite-plugin-pwa'\\n\" +\n"
            + "  reactImport +\n"
            + "  \"\\nexport default defineConfig({\\n\" +\n"
            + "  \"  server: { host: '0.0.0.0', port: 5173 },\\n\" +\n"
            + "  \"  preview: { host: '0.0.0.0', port: 4173 },\\n\" +\n"
            + "  \"  plugins: [\" + reactPlug + \"VitePWA({\\n\" +\n"
            + "  \"    registerType: 'autoUpdate',\\n\" +\n"
            + "  \"    includeAssets: ['favicon.svg', 'vite.svg'],\\n\" +\n"
            + "  \"    manifest: {\\n\" +\n"
            + "  \"      name: \" + JSON.stringify(name) + \",\\n\" +\n"
            + "  \"      short_name: \" + JSON.stringify(name) + \",\\n\" +\n"
            + "  \"      start_url: '/',\\n\" +\n"
            + "  \"      display: 'standalone',\\n\" +\n"
            + "  \"      background_color: '#ffffff',\\n\" +\n"
            + "  \"      theme_color: '#242424',\\n\" +\n"
            + "  \"      icons: [{ src: '/vite.svg', sizes: 'any', type: 'image/svg+xml', purpose: 'any' }]\\n\" +\n"
            + "  \"    }\\n\" +\n"
            + "  \"  })]\\n\" +\n"
            + "  \"})\\n\";\n"
            + "\n"
            + "for (const f of [\n"
            + "  \"vite.config.js\",\n"
            + "  \"vite.config.ts\",\n"
            + "  \"vite.config.mjs\",\n"
            + "  \"vite.config.mts\",\n"
            + "]) {\n"
            + "  if (existsSync(f)) unlinkSync(f);\n"
            + "}\n"
            + "writeFileSync(\"vite.config.js\", cfg);\n";
        writeExec(new File(libexecDir, "invapp-vite-pwa.mjs"), pwaConfigJs);

        String tdScaffold = ""
            + "#!" + bash + "\n"
            + "set -e\n"
            + "PREFIX=\"" + prefix + "\"\n"
            + "HOME=\"" + home + "\"\n"
            + "export PATH=\"$PREFIX/bin:$HOME/.bun/bin:$PATH\"\n"
            + "HERE=0\n"
            + "NO_INSTALL=0\n"
            + "NAME=\"\"\n"
            + "TEMPLATE=\"react-ts\"\n"
            + "while [ $# -gt 0 ]; do\n"
            + "  case \"$1\" in\n"
            + "    --here) HERE=1; shift ;;\n"
            + "    --no-install) NO_INSTALL=1; shift ;;\n"
            + "    -h|--help)\n"
            + "      echo \"usage: td-scaffold [name] [template] [--here] [--no-install]\" >&2\n"
            + "      echo \"templates: vanilla vanilla-ts react react-ts vue vue-ts pwa pwa-react\" >&2\n"
            + "      echo \"default template: react-ts\" >&2\n"
            + "      exit 0\n"
            + "      ;;\n"
            + "    --*) echo \"td-scaffold: unknown flag $1\" >&2; exit 2 ;;\n"
            + "    *)\n"
            + "      if [ -z \"$NAME\" ]; then NAME=\"$1\"\n"
            + "      else TEMPLATE=\"$1\"\n"
            + "      fi\n"
            + "      shift\n"
            + "      ;;\n"
            + "  esac\n"
            + "done\n"
            + "if [ \"$HERE\" = 1 ]; then\n"
            + "  if [ -z \"$NAME\" ]; then NAME=\".\"; fi\n"
            + "elif [ -z \"$NAME\" ]; then\n"
            + "  echo \"usage: td-scaffold <name> [template] [--here] [--no-install]\" >&2\n"
            + "  echo \"templates: vanilla vanilla-ts react react-ts vue vue-ts pwa pwa-react\" >&2\n"
            + "  exit 2\n"
            + "fi\n"
            + "case \"$NAME\" in\n"
            + "  .) ;;\n"
            + "  *[!a-zA-Z0-9._-]*) echo \"td-scaffold: invalid name '$NAME'\" >&2; exit 2 ;;\n"
            + "  *[/\\\\]*|*/storage/*|*emulated*) echo \"td-scaffold: refuse shared/noexec path\" >&2; exit 2 ;;\n"
            + "esac\n"
            + "PWA=0\n"
            + "BASE_TEMPLATE=\"$TEMPLATE\"\n"
            + "case \"$TEMPLATE\" in\n"
            + "  pwa) PWA=1; BASE_TEMPLATE=vanilla ;;\n"
            + "  pwa-react) PWA=1; BASE_TEMPLATE=react ;;\n"
            + "  vanilla|vanilla-ts|react|react-ts|vue|vue-ts|svelte|svelte-ts|solid|solid-ts|qwik|qwik-ts|preact|preact-ts) ;;\n"
            + "  *) echo \"td-scaffold: unknown template '$TEMPLATE'\" >&2; exit 2 ;;\n"
            + "esac\n"
            + "if ! command -v bun >/dev/null 2>&1; then\n"
            + "  echo \"td-scaffold: bun missing — reopen the app\" >&2\n"
            + "  exit 127\n"
            + "fi\n"
            + "if [ \"$HERE\" = 1 ]; then\n"
            + "  TARGET_DIR=\"$(pwd)\"\n"
            + "  CREATE_NAME=\".\"\n"
            + "  if [ -e package.json ]; then\n"
            + "    echo \"td-scaffold: package.json already exists in $TARGET_DIR\" >&2\n"
            + "    exit 1\n"
            + "  fi\n"
            + "else\n"
            + "  mkdir -p \"$HOME/repos\"\n"
            + "  TARGET_DIR=\"$HOME/repos/$NAME\"\n"
            + "  CREATE_NAME=\"$NAME\"\n"
            + "  if [ -e \"$TARGET_DIR\" ]; then\n"
            + "    echo \"td-scaffold: $TARGET_DIR already exists\" >&2\n"
            + "    exit 1\n"
            + "  fi\n"
            + "  cd \"$HOME/repos\"\n"
            + "fi\n"
            + "echo \"td-scaffold: create-vite $CREATE_NAME --template $BASE_TEMPLATE\"\n"
            + "bunx --bun create-vite@latest \"$CREATE_NAME\" --template \"$BASE_TEMPLATE\"\n"
            + "if [ \"$HERE\" != 1 ]; then\n"
            + "  cd \"$NAME\"\n"
            + "fi\n"
            + "# Force LAN-friendly Vite dev server for Preview / Copy LAN.\n"
            + "if [ -f package.json ]; then\n"
            + "  bun -e \"\n"
            + "const fs=require('fs');\n"
            + "const p=JSON.parse(fs.readFileSync('package.json','utf8'));\n"
            + "p.scripts=p.scripts||{};\n"
            + "p.scripts.dev='vite --host 0.0.0.0 --port 5173';\n"
            + "p.scripts.preview=p.scripts.preview||'vite preview --host 0.0.0.0 --port 4173';\n"
            + "fs.writeFileSync('package.json', JSON.stringify(p,null,2)+'\\n');\n"
            + "\"\n"
            + "fi\n"
            + "if [ ! -f .gitignore ]; then\n"
            + "  printf '%s\\n' 'node_modules/' 'dist/' '.DS_Store' '*.local' > .gitignore\n"
            + "fi\n"
            + "if [ ! -f README.md ]; then\n"
            + "  cat > README.md <<'EOF'\n"
            + "# Project\n"
            + "\n"
            + "Scaffolded with `td-scaffold` for InVxTermux.\n"
            + "\n"
            + "## Dev\n"
            + "\n"
            + "```bash\n"
            + "td-dev          # or: bun run dev\n"
            + "```\n"
            + "\n"
            + "Then: drawer → **Preview** → **Scan** → **5173**. Use **Copy LAN** when the server binds `0.0.0.0`.\n"
            + "EOF\n"
            + "fi\n"
            + "if [ \"$NO_INSTALL\" != 1 ]; then\n"
            + "  echo \"td-scaffold: bun install\"\n"
            + "  bun install\n"
            + "else\n"
            + "  echo \"td-scaffold: skipping bun install (--no-install)\"\n"
            + "fi\n"
            + "if [ \"$PWA\" = 1 ]; then\n"
            + "  if [ \"$NO_INSTALL\" = 1 ]; then\n"
            + "    echo \"td-scaffold: PWA needs install — run: bun install && bun add -d vite-plugin-pwa\" >&2\n"
            + "  else\n"
            + "    echo \"td-scaffold: add vite-plugin-pwa\"\n"
            + "    bun add -d vite-plugin-pwa\n"
            + "    bun \"$PREFIX/libexec/invapp-vite-pwa.mjs\" \"$BASE_TEMPLATE\"\n"
            + "    echo \"td-scaffold: PWA enabled (vite-plugin-pwa)\"\n"
            + "  fi\n"
            + "fi\n"
            + "echo \"\"\n"
            + "echo \"OK: $(pwd)\"\n"
            + "echo \"Next: td-dev    # or drawer → bun run dev\"\n"
            + "echo \"Then: drawer → Preview → Scan → 5173 (Copy LAN if needed)\"\n"
            + "if [ \"$PWA\" = 1 ]; then\n"
            + "  echo \"PWA build: bun run build   # then Preview :4173 or install from browser\"\n"
            + "fi\n"
            + "echo \"\"\n"
            + "pwd\n";
        writeExec(new File(binDir, "td-scaffold"), tdScaffold);

        // Clone a remote into ~/repos (PC ↔ phone golden path).
        String tdClone = ""
            + "#!" + bash + "\n"
            + "set -e\n"
            + "PREFIX=\"" + prefix + "\"\n"
            + "HOME=\"" + home + "\"\n"
            + "export PATH=\"$PREFIX/bin:$HOME/.bun/bin:$PATH\"\n"
            + "REDIRECTOR_SO=\"$PREFIX/lib/libinvapp-redirector.so\"\n"
            + "[ -f \"$REDIRECTOR_SO\" ] && export LD_PRELOAD=\"$REDIRECTOR_SO${LD_PRELOAD:+:$LD_PRELOAD}\"\n"
            + "export LD_LIBRARY_PATH=\"$PREFIX/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}\"\n"
            + "URL=\"${1-}\"\n"
            + "if [ -z \"$URL\" ]; then\n"
            + "  echo \"usage: td-clone <git-url> [name] [--bun-i]\" >&2\n"
            + "  exit 2\n"
            + "fi\n"
            + "shift || true\n"
            + "NAME=\"\"\n"
            + "DO_BUN=0\n"
            + "for arg in \"$@\"; do\n"
            + "  case \"$arg\" in\n"
            + "    --bun-i|--bun-install) DO_BUN=1 ;;\n"
            + "    -*) echo \"td-clone: unknown flag '$arg'\" >&2; exit 2 ;;\n"
            + "    *)\n"
            + "      if [ -n \"$NAME\" ]; then\n"
            + "        echo \"usage: td-clone <git-url> [name] [--bun-i]\" >&2\n"
            + "        exit 2\n"
            + "      fi\n"
            + "      NAME=\"$arg\"\n"
            + "      ;;\n"
            + "  esac\n"
            + "done\n"
            + "if [ -z \"$NAME\" ]; then\n"
            + "  NAME=$(basename \"$URL\")\n"
            + "  NAME=${NAME%.git}\n"
            + "  NAME=${NAME%%\\?*}\n"
            + "  NAME=${NAME%%\\#*}\n"
            + "fi\n"
            + "case \"$NAME\" in\n"
            + "  ''|'.'|'..'|*[!a-zA-Z0-9._-]*)\n"
            + "    echo \"td-clone: invalid name '$NAME'\" >&2\n"
            + "    exit 2\n"
            + "    ;;\n"
            + "esac\n"
            + "if ! command -v git >/dev/null 2>&1; then\n"
            + "  echo \"td-clone: git missing — pkg install git\" >&2\n"
            + "  exit 127\n"
            + "fi\n"
            + "mkdir -p \"$HOME/repos\"\n"
            + "DEST=\"$HOME/repos/$NAME\"\n"
            + "if [ -e \"$DEST\" ]; then\n"
            + "  echo \"td-clone: $DEST already exists\" >&2\n"
            + "  exit 1\n"
            + "fi\n"
            + "echo \"td-clone: git clone $URL → $DEST\"\n"
            + "git clone -- \"$URL\" \"$DEST\"\n"
            + "cd \"$DEST\"\n"
            + "if [ \"$DO_BUN\" = 1 ]; then\n"
            + "  if [ -f package.json ]; then\n"
            + "    if ! command -v bun >/dev/null 2>&1; then\n"
            + "      echo \"td-clone: bun missing — reopen the app\" >&2\n"
            + "      exit 127\n"
            + "    fi\n"
            + "    echo \"td-clone: bun install\"\n"
            + "    bun install\n"
            + "  else\n"
            + "    echo \"td-clone: no package.json — skip bun install\"\n"
            + "  fi\n"
            + "fi\n"
            + "echo \"\"\n"
            + "echo \"OK: $DEST\"\n"
            + "echo \"Next: td-dev    # or drawer → bun run dev\"\n"
            + "echo \"Then: drawer → Preview → Scan (Copy LAN if needed)\"\n"
            + "echo \"\"\n"
            + "pwd\n";
        writeExec(new File(binDir, "td-clone"), tdClone);

        // KeepAliveService: applicationId ≠ Java package — use FQCN.
        String apiPkg = TermuxConstants.TERMUX_API_PACKAGE_NAME;
        String keepaliveCmp = apiPkg + "/com.invapp.api.KeepAliveService";
        String termuxApiStart = ""
            + "#!" + bash + "\n"
            + "# invapp: termux-api-start — KeepAliveService (FQCN)\n"
            + "am startservice -n " + keepaliveCmp + "\n";
        writeExec(new File(binDir, "termux-api-start"), termuxApiStart);
        String termuxApiStop = ""
            + "#!" + bash + "\n"
            + "# invapp: termux-api-stop — KeepAliveService (FQCN)\n"
            + "am stopservice -n " + keepaliveCmp + "\n";
        writeExec(new File(binDir, "termux-api-stop"), termuxApiStop);

        // MediaProjection screenshot via Termux:API (consent dialog once).
        // Mirrors contrib/termux-api-package/scripts/termux-screenshot.in
        // (getopts + libexec/termux-api Screenshot).
        String termuxScreenshot = ""
            + "#!" + bash + "\n"
            + "# invapp: termux-screenshot — Capture display via MediaProjection\n"
            + "# Upstream-ready twin: contrib/termux-api-package/scripts/termux-screenshot.in\n"
            + "set -e -u\n"
            + "PREFIX=\"" + prefix + "\"\n"
            + "HOME=\"" + home + "\"\n"
            + "export PATH=\"$PREFIX/bin:$PATH\"\n"
            + "SCRIPTNAME=termux-screenshot\n"
            + "show_usage() {\n"
            + "  echo \"Usage: termux-screenshot [-d delay_ms] [output-file]\"\n"
            + "  echo \"Take a screenshot (MediaProjection consent) and save it as PNG.\"\n"
            + "  echo \" -d delay_ms  Wait after consent before capture (default: 800)\"\n"
            + "  echo \"Default file: $HOME/repos/screen-ocr/latest.png\"\n"
            + "  exit 0\n"
            + "}\n"
            + "PARAMS=\"\"\n"
            + "DELAY_SET=0\n"
            + "while getopts :hd: option\n"
            + "do\n"
            + "  case \"$option\" in\n"
            + "    h) show_usage;;\n"
            + "    d) PARAMS=\"$PARAMS --ei delay_ms $OPTARG\"; DELAY_SET=1;;\n"
            + "    ?) echo \"$SCRIPTNAME: illegal option -$OPTARG\"; exit 1;;\n"
            + "  esac\n"
            + "done\n"
            + "shift $((OPTIND-1))\n"
            + "if [ $# -gt 1 ]; then echo \"$SCRIPTNAME: too many arguments\"; exit 1; fi\n"
            + "if [ \"$DELAY_SET\" = 0 ]; then\n"
            + "  PARAMS=\"$PARAMS --ei delay_ms ${SCREENSHOT_DELAY_MS:-800}\"\n"
            + "fi\n"
            + "OUT=\"${1-}\"\n"
            + "if [ -z \"$OUT\" ]; then\n"
            + "  OUT=\"$HOME/repos/screen-ocr/latest.png\"\n"
            + "fi\n"
            + "mkdir -p \"$(dirname \"$OUT\")\"\n"
            + "if [ ! -x \"$PREFIX/libexec/termux-api\" ]; then\n"
            + "  echo \"$SCRIPTNAME: need pkg install termux-api (libexec/termux-api)\" >&2\n"
            + "  exit 127\n"
            + "fi\n"
            + "touch \"$OUT\"\n"
            + "PARAMS=\"$PARAMS --es file $(realpath \"$OUT\")\"\n"
            + "# shellcheck disable=SC2086\n"
            + "result=$(\"$PREFIX/libexec/termux-api\" Screenshot $PARAMS || true)\n"
            + "case \"$result\" in\n"
            + "  ERROR:*)\n"
            + "    echo \"$result\" >&2\n"
            + "    exit 1\n"
            + "    ;;\n"
            + "  \"\")\n"
            + "    echo \"$SCRIPTNAME: empty response from Termux:API\" >&2\n"
            + "    exit 1\n"
            + "    ;;\n"
            + "esac\n"
            + "printf '%s\\n' \"$result\"\n";
        writeExec(new File(binDir, "termux-screenshot"), termuxScreenshot);

        // OCR screenshot → clipboard (capture preferred, else latest/pick).
        String tdScreenOcr = ""
            + "#!" + bash + "\n"
            + "# invapp: td-screen-ocr — screenshot/OCR → clipboard\n"
            + "set -e\n"
            + "PREFIX=\"" + prefix + "\"\n"
            + "HOME=\"" + home + "\"\n"
            + "export PATH=\"$PREFIX/bin:$PATH\"\n"
            + "OCR_LANG=\"${OCR_LANG:-eng}\"\n"
            + "TESSDATA_DIR=\"${TESSDATA_PREFIX:-$PREFIX/share/tessdata}\"\n"
            + "MODE=auto\n"
            + "IMG=\"\"\n"
            + "toast() { command -v termux-toast >/dev/null 2>&1 && termux-toast \"$1\" || true; }\n"
            + "lang_pack_hint() {\n"
            + "  echo \"Lang pack: copy ${OCR_LANG}.traineddata → $TESSDATA_DIR\"\n"
            + "  echo \"  (eng ships with: pkg install tesseract)\"\n"
            + "  echo \"  Others: https://github.com/tesseract-ocr/tessdata\"\n"
            + "}\n"
            + "while [ $# -gt 0 ]; do\n"
            + "  case \"$1\" in\n"
            + "    -h|--help)\n"
            + "      echo \"Usage: td-screen-ocr [--capture|--latest|--pick] [image]\"\n"
            + "      echo \"OCR a screenshot into the clipboard (tesseract).\"\n"
            + "      echo \"  --capture  termux-screenshot (MediaProjection)\"\n"
            + "      echo \"  --latest   newest PNG/JPG under Screenshots folders\"\n"
            + "      echo \"  --pick     termux-storage-get file picker\"\n"
            + "      echo \"Env: OCR_LANG (default eng), SCREENSHOT_DELAY_MS, TESSDATA_PREFIX\"\n"
            + "      echo \"Lang packs: eng via pkg install tesseract; others → $PREFIX/share/tessdata\"\n"
            + "      exit 0\n"
            + "      ;;\n"
            + "    --capture) MODE=capture; shift ;;\n"
            + "    --latest)  MODE=latest; shift ;;\n"
            + "    --pick)    MODE=pick; shift ;;\n"
            + "    --no-capture) MODE=latest; shift ;;\n"
            + "    *) IMG=\"$1\"; MODE=file; shift ;;\n"
            + "  esac\n"
            + "done\n"
            + "if ! command -v tesseract >/dev/null 2>&1; then\n"
            + "  msg='Need: pkg install tesseract (incl. eng lang pack)'\n"
            + "  toast \"$msg\"; echo \"$msg\" >&2\n"
            + "  lang_pack_hint >&2\n"
            + "  exit 127\n"
            + "fi\n"
            + "if [ ! -f \"$TESSDATA_DIR/${OCR_LANG}.traineddata\" ]; then\n"
            + "  msg=\"Missing ${OCR_LANG}.traineddata — see toast/stderr\"\n"
            + "  toast \"Lang pack: put ${OCR_LANG}.traineddata in tessdata\"\n"
            + "  echo \"$msg\" >&2\n"
            + "  lang_pack_hint >&2\n"
            + "  exit 127\n"
            + "fi\n"
            + "if ! command -v termux-clipboard-set >/dev/null 2>&1; then\n"
            + "  msg='Need Termux:API APK + pkg install termux-api'\n"
            + "  toast \"$msg\"; echo \"$msg\" >&2\n"
            + "  exit 127\n"
            + "fi\n"
            + "find_latest() {\n"
            + "  local newest=\"\" newest_ts=0 f ts\n"
            + "  for dir in \\\n"
            + "      \"$HOME/storage/dcim/Screenshots\" \\\n"
            + "      \"$HOME/storage/pictures/Screenshots\" \\\n"
            + "      \"$HOME/storage/shared/DCIM/Screenshots\" \\\n"
            + "      \"$HOME/storage/shared/Pictures/Screenshots\"; do\n"
            + "    [ -d \"$dir\" ] || continue\n"
            + "    # shellcheck disable=SC2044\n"
            + "    for f in \"$dir\"/*; do\n"
            + "      [ -f \"$f\" ] || continue\n"
            + "      case \"$f\" in\n"
            + "        *.png|*.PNG|*.jpg|*.JPG|*.jpeg|*.JPEG|*.webp|*.WEBP) ;;\n"
            + "        *) continue ;;\n"
            + "      esac\n"
            + "      ts=$(stat -c %Y \"$f\" 2>/dev/null || echo 0)\n"
            + "      if [ \"$ts\" -ge \"$newest_ts\" ]; then\n"
            + "        newest_ts=$ts\n"
            + "        newest=$f\n"
            + "      fi\n"
            + "    done\n"
            + "  done\n"
            + "  printf '%s' \"$newest\"\n"
            + "}\n"
            + "resolve_image() {\n"
            + "  case \"$MODE\" in\n"
            + "    file)\n"
            + "      if [ ! -f \"$IMG\" ]; then\n"
            + "        echo \"td-screen-ocr: not a file: $IMG\" >&2\n"
            + "        return 1\n"
            + "      fi\n"
            + "      printf '%s' \"$IMG\"\n"
            + "      return 0\n"
            + "      ;;\n"
            + "    capture)\n"
            + "      toast 'Capturing screen…'\n"
            + "      if ! command -v termux-screenshot >/dev/null 2>&1; then\n"
            + "        echo \"td-screen-ocr: termux-screenshot missing — reopen the app\" >&2\n"
            + "        return 1\n"
            + "      fi\n"
            + "      mkdir -p \"$HOME/repos/screen-ocr\"\n"
            + "      local out path\n"
            + "      out=\"$HOME/repos/screen-ocr/latest.png\"\n"
            + "      path=$(termux-screenshot \"$out\") || return 1\n"
            + "      case \"$path\" in ERROR:*) echo \"$path\" >&2; return 1 ;; esac\n"
            + "      printf '%s' \"$path\"\n"
            + "      return 0\n"
            + "      ;;\n"
            + "    pick)\n"
            + "      if ! command -v termux-storage-get >/dev/null 2>&1; then\n"
            + "        echo \"td-screen-ocr: termux-storage-get missing (pkg install termux-api)\" >&2\n"
            + "        return 1\n"
            + "      fi\n"
            + "      toast 'Pick an image…'\n"
            + "      mkdir -p \"$HOME/repos/screen-ocr\"\n"
            + "      local tmp=\"$HOME/repos/screen-ocr/picked.bin\"\n"
            + "      termux-storage-get \"$tmp\" || return 1\n"
            + "      [ -s \"$tmp\" ] || return 1\n"
            + "      printf '%s' \"$tmp\"\n"
            + "      return 0\n"
            + "      ;;\n"
            + "    latest)\n"
            + "      local latest\n"
            + "      latest=$(find_latest)\n"
            + "      if [ -z \"$latest\" ]; then\n"
            + "        echo \"td-screen-ocr: no screenshots (run termux-setup-storage)\" >&2\n"
            + "        return 1\n"
            + "      fi\n"
            + "      printf '%s' \"$latest\"\n"
            + "      return 0\n"
            + "      ;;\n"
            + "    auto)\n"
            + "      if command -v termux-screenshot >/dev/null 2>&1 \\\n"
            + "          && [ -x \"$PREFIX/libexec/termux-api\" ]; then\n"
            + "        toast 'Capturing screen…'\n"
            + "        mkdir -p \"$HOME/repos/screen-ocr\"\n"
            + "        local out path\n"
            + "        out=\"$HOME/repos/screen-ocr/latest.png\"\n"
            + "        if path=$(termux-screenshot \"$out\" 2>/dev/null); then\n"
            + "          case \"$path\" in\n"
            + "            ERROR:*|\"\") ;;\n"
            + "            *)\n"
            + "              if [ -s \"$path\" ]; then\n"
            + "                printf '%s' \"$path\"\n"
            + "                return 0\n"
            + "              fi\n"
            + "              ;;\n"
            + "          esac\n"
            + "        fi\n"
            + "        toast 'Capture failed — trying latest screenshot'\n"
            + "      fi\n"
            + "      MODE=latest\n"
            + "      resolve_image\n"
            + "      return $?\n"
            + "      ;;\n"
            + "  esac\n"
            + "  return 1\n"
            + "}\n"
            + "IMG_PATH=$(resolve_image) || {\n"
            + "  toast 'No screenshot to OCR'\n"
            + "  exit 1\n"
            + "}\n"
            + "toast 'OCR…'\n"
            + "tess_err=$(mktemp \"$PREFIX/tmp/td-ocr.XXXXXX\" 2>/dev/null || mktemp)\n"
            + "text=$(tesseract \"$IMG_PATH\" stdout -l \"$OCR_LANG\" 2>\"$tess_err\" || true)\n"
            + "text=$(printf '%s' \"$text\" | sed -e 's/[[:space:]]*$//')\n"
            + "if [ -z \"$text\" ]; then\n"
            + "  toast \"OCR empty — check ${OCR_LANG} lang pack\"\n"
            + "  echo \"td-screen-ocr: no text from $IMG_PATH\" >&2\n"
            + "  lang_pack_hint >&2\n"
            + "  if [ -s \"$tess_err\" ]; then cat \"$tess_err\" >&2; fi\n"
            + "  rm -f \"$tess_err\"\n"
            + "  exit 1\n"
            + "fi\n"
            + "rm -f \"$tess_err\"\n"
            + "printf '%s' \"$text\" | termux-clipboard-set\n"
            + "chars=$(printf '%s' \"$text\" | wc -c | tr -d ' ')\n"
            + "toast \"Copied $chars chars\"\n"
            + "echo \"OCR → clipboard ($chars chars) from $IMG_PATH\"\n";
        writeExec(new File(binDir, "td-screen-ocr"), tdScreenOcr);

        installBashCompletions(prefix);
        ensureWorkspaceDirs();
        repairPrefixBinPermissions();
    }

    /**
     * Installs {@code $PREFIX/etc/profile.d/invapp-completions.sh} and static
     * completers under {@code $PREFIX/etc/bash_completion.d/}. Does not touch
     * {@code ~/.bashrc}. Open a new session after install.
     */
    static void installBashCompletions(@NonNull String prefix) {
        File profileD = new File(prefix, "etc/profile.d");
        File completionD = new File(prefix, "etc/bash_completion.d");
        ensureDir(profileD);
        ensureDir(completionD);

        String hook = ""
            + "# invapp-completions — sourced by login shells via profile.d\n"
            + "[ -n \"${BASH_VERSION-}\" ] || return 0\n"
            + "shopt -q progcomp 2>/dev/null || shopt -s progcomp 2>/dev/null || true\n"
            + "if [ -f \"$PREFIX/share/bash-completion/bash_completion\" ]; then\n"
            + "  # shellcheck source=/dev/null\n"
            + "  . \"$PREFIX/share/bash-completion/bash_completion\"\n"
            + "elif [ -f \"$PREFIX/etc/bash_completion\" ]; then\n"
            + "  # shellcheck source=/dev/null\n"
            + "  . \"$PREFIX/etc/bash_completion\"\n"
            + "fi\n"
            + "for _invapp_comp in \"$PREFIX/etc/bash_completion.d\"/*.bash \"$PREFIX/etc/bash_completion.d\"/*.sh; do\n"
            + "  [ -f \"$_invapp_comp\" ] || continue\n"
            + "  # shellcheck source=/dev/null\n"
            + "  . \"$_invapp_comp\"\n"
            + "done\n"
            + "unset _invapp_comp\n";
        writeTextFile(new File(profileD, "invapp-completions.sh"), hook);

        writeTextFile(new File(completionD, "invapp-bun.bash"), bunCompletionScript());
        writeTextFile(new File(completionD, "invapp-pkg.bash"), pkgCompletionScript());
        writeTextFile(new File(completionD, "invapp-npm.bash"), npmCompletionScript());
        writeTextFile(new File(completionD, "invapp-gh.bash"), ghCompletionScript());
        writeTextFile(new File(completionD, "invapp-git.bash"), gitCompletionScript());
    }

    @NonNull
    static String bunCompletionScript() {
        return ""
            + "# invapp bun completions (subset; regenerate with: bun completions bash)\n"
            + "_invapp_bun() {\n"
            + "  local cur=\"${COMP_WORDS[COMP_CWORD]}\"\n"
            + "  local cmds=\"run test install i add remove rm update create x exec init help --version --help\"\n"
            + "  if [ \"$COMP_CWORD\" -eq 1 ]; then\n"
            + "    COMPREPLY=( $(compgen -W \"$cmds\" -- \"$cur\") )\n"
            + "    return\n"
            + "  fi\n"
            + "  case \"${COMP_WORDS[1]}\" in\n"
            + "    run|test|x|exec) COMPREPLY=( $(compgen -f -- \"$cur\") ) ;;\n"
            + "    *) COMPREPLY=( $(compgen -W \"$cmds\" -- \"$cur\") ) ;;\n"
            + "  esac\n"
            + "}\n"
            + "complete -F _invapp_bun bun 2>/dev/null || true\n"
            + "complete -F _invapp_bun bunx 2>/dev/null || true\n";
    }

    @NonNull
    static String pkgCompletionScript() {
        return ""
            + "# invapp pkg completions\n"
            + "_invapp_pkg() {\n"
            + "  local cur=\"${COMP_WORDS[COMP_CWORD]}\"\n"
            + "  local cmds=\"install uninstall reinstall search list files show upgrade update clean hold unhold help\"\n"
            + "  if [ \"$COMP_CWORD\" -eq 1 ]; then\n"
            + "    COMPREPLY=( $(compgen -W \"$cmds\" -- \"$cur\") )\n"
            + "    return\n"
            + "  fi\n"
            + "  case \"${COMP_WORDS[1]}\" in\n"
            + "    install|uninstall|reinstall|search|files|show|hold|unhold)\n"
            + "      if command -v apt-cache >/dev/null 2>&1; then\n"
            + "        COMPREPLY=( $(apt-cache --no-generate pkgnames \"$cur\" 2>/dev/null) )\n"
            + "      else\n"
            + "        COMPREPLY=( $(compgen -W \"\" -- \"$cur\") )\n"
            + "      fi\n"
            + "      ;;\n"
            + "    *) COMPREPLY=( $(compgen -W \"$cmds\" -- \"$cur\") ) ;;\n"
            + "  esac\n"
            + "}\n"
            + "complete -F _invapp_pkg pkg 2>/dev/null || true\n";
    }

    @NonNull
    static String npmCompletionScript() {
        return ""
            + "# invapp npm completions (works with real npm or node shim)\n"
            + "_invapp_npm() {\n"
            + "  local cur=\"${COMP_WORDS[COMP_CWORD]}\"\n"
            + "  local cmds=\"install i uninstall remove rm run test start build publish pack link unlink outdated audit doctor help\"\n"
            + "  if [ \"$COMP_CWORD\" -eq 1 ]; then\n"
            + "    COMPREPLY=( $(compgen -W \"$cmds\" -- \"$cur\") )\n"
            + "    return\n"
            + "  fi\n"
            + "  case \"${COMP_WORDS[1]}\" in\n"
            + "    run|start|test|build) COMPREPLY=( $(compgen -f -- \"$cur\") ) ;;\n"
            + "    *) COMPREPLY=( $(compgen -W \"$cmds\" -- \"$cur\") ) ;;\n"
            + "  esac\n"
            + "}\n"
            + "complete -F _invapp_npm npm 2>/dev/null || true\n"
            + "complete -F _invapp_npm npx 2>/dev/null || true\n";
    }

    @NonNull
    static String ghCompletionScript() {
        return ""
            + "# invapp gh completions — prefer upstream when available\n"
            + "if command -v gh >/dev/null 2>&1; then\n"
            + "  if ! complete -p gh >/dev/null 2>&1; then\n"
            + "    eval \"$(gh completion -s bash 2>/dev/null)\" || true\n"
            + "  fi\n"
            + "fi\n"
            + "if ! complete -p gh >/dev/null 2>&1; then\n"
            + "  _invapp_gh() {\n"
            + "    local cur=\"${COMP_WORDS[COMP_CWORD]}\"\n"
            + "    local cmds=\"auth repo pr issue release gist api browse config help\"\n"
            + "    if [ \"$COMP_CWORD\" -eq 1 ]; then\n"
            + "      COMPREPLY=( $(compgen -W \"$cmds\" -- \"$cur\") )\n"
            + "    fi\n"
            + "  }\n"
            + "  complete -F _invapp_gh gh 2>/dev/null || true\n"
            + "fi\n";
    }

    @NonNull
    static String gitCompletionScript() {
        return ""
            + "# invapp git stub — skip if bash-completion already registered git\n"
            + "if complete -p git >/dev/null 2>&1; then\n"
            + "  return 0 2>/dev/null || true\n"
            + "fi\n"
            + "_invapp_git() {\n"
            + "  local cur=\"${COMP_WORDS[COMP_CWORD]}\"\n"
            + "  local cmds=\"status add commit push pull clone fetch checkout branch merge rebase log diff stash remote init help\"\n"
            + "  if [ \"$COMP_CWORD\" -eq 1 ]; then\n"
            + "    COMPREPLY=( $(compgen -W \"$cmds\" -- \"$cur\") )\n"
            + "    return\n"
            + "  fi\n"
            + "  COMPREPLY=( $(compgen -f -- \"$cur\") )\n"
            + "}\n"
            + "complete -F _invapp_git git 2>/dev/null || true\n";
    }

    private static void writeTextFile(@NonNull File dest, @NonNull String contents) {
        try {
            File parent = dest.getParentFile();
            if (parent != null) {
                ensureDir(parent);
            }
            File tmp = new File(dest.getAbsolutePath() + ".new");
            try (FileOutputStream out = new FileOutputStream(tmp)) {
                out.write(contents.getBytes(java.nio.charset.StandardCharsets.UTF_8));
            }
            //noinspection OctalInteger
            Os.chmod(tmp.getAbsolutePath(), 0644);
            if (dest.exists() && !dest.delete()) {
                try {
                    Os.remove(dest.getAbsolutePath());
                } catch (Exception e) {
                    Logger.logWarn(LOG_TAG, "Could not replace " + dest + ": " + e.getMessage());
                }
            }
            if (!tmp.renameTo(dest)) {
                Logger.logWarn(LOG_TAG, "Could not install " + dest);
                //noinspection ResultOfMethodCallIgnored
                tmp.delete();
            }
        } catch (Exception e) {
            Logger.logWarn(LOG_TAG, "Text install failed for " + dest + ": " + e.getMessage());
        }
    }

    /**
     * If {@code node} is missing (or is our previous shim), install a tiny
     * wrapper that execs Bun. Leaves a real {@code nodejs} package binary alone.
     */
    private static void ensureNodeBunShim(@NonNull File binDir, @NonNull String bash,
                                          @NonNull String prefix) {
        File node = new File(binDir, "node");
        if (node.isFile() && !isInvappNodeShim(node)) {
            return;
        }
        String shim = ""
            + "#!" + bash + "\n"
            + "# invapp-bun-node-shim — provides node for #!/usr/bin/env node bins\n"
            + "exec \"" + prefix + "/bin/bun\" \"$@\"\n";
        writeExec(node, shim);
    }

    private static boolean isInvappNodeShim(@NonNull File node) {
        try (FileInputStream in = new FileInputStream(node)) {
            byte[] buf = new byte[256];
            int n = in.read(buf);
            if (n <= 0) {
                return false;
            }
            String head = new String(buf, 0, n, java.nio.charset.StandardCharsets.UTF_8);
            return head.contains("invapp-bun-node-shim");
        } catch (Exception e) {
            return false;
        }
    }

    /**
     * Some package files land without the execute bit (dpkg extract / noexec
     * quirks). {@code npm doctor} flags these; fix known PREFIX/bin entries.
     */
    private static void repairPrefixBinPermissions() {
        File binDir = new File(TermuxConstants.TERMUX_BIN_PREFIX_DIR_PATH);
        if (!binDir.isDirectory()) {
            return;
        }
        String[] names = {
            "am.termuxam", "ksu", "am", "login", "apt", "apt-get", "dpkg",
            "bun", "bunx", "node", "td-ai", "td-dev", "td-scaffold", "td-clone",
            "td-screen-ocr", "termux-screenshot", "termux-api-start", "termux-api-stop",
            "opencode-setup", "opencode-fix-net", "opencode", "bun-doctor"
        };
        for (String name : names) {
            File f = new File(binDir, name);
            if (!f.isFile()) {
                continue;
            }
            try {
                //noinspection OctalInteger
                Os.chmod(f.getAbsolutePath(), 0700);
            } catch (Exception e) {
                Logger.logWarn(LOG_TAG, "chmod " + f + ": " + e.getMessage());
            }
        }
    }

    private static void writeExec(@NonNull File dest, @NonNull String contents) {
        try {
            File tmp = new File(dest.getAbsolutePath() + ".new");
            try (FileOutputStream out = new FileOutputStream(tmp)) {
                out.write(contents.getBytes(java.nio.charset.StandardCharsets.UTF_8));
            }
            //noinspection OctalInteger
            Os.chmod(tmp.getAbsolutePath(), 0700);
            if (dest.exists() && !dest.delete()) {
                // Symlink (old bunx→bun) or busy file
                try {
                    Os.remove(dest.getAbsolutePath());
                } catch (Exception e) {
                    Logger.logWarn(LOG_TAG, "Could not replace " + dest + ": " + e.getMessage());
                }
            }
            if (!tmp.renameTo(dest)) {
                Logger.logWarn(LOG_TAG, "Could not install " + dest);
                //noinspection ResultOfMethodCallIgnored
                tmp.delete();
            }
        } catch (Exception e) {
            Logger.logWarn(LOG_TAG, "Helper install failed for " + dest + ": " + e.getMessage());
        }
    }

    private static void ensureDir(@NonNull File dir) {
        if (!dir.exists() && !dir.mkdirs()) {
            Logger.logWarn(LOG_TAG, "Could not create " + dir);
        }
    }

    /**
     * Official Android builds use the Bionic linker ({@code /system/bin/linker64} or
     * {@code linker}). glibc linux builds request {@code /lib/ld-linux-*.so.1}.
     */
    private static boolean isAndroidBunBinary(@NonNull File bunBin) {
        try (FileInputStream in = new FileInputStream(bunBin)) {
            byte[] head = new byte[4096];
            int n = in.read(head);
            if (n < 64) {
                return false;
            }
            if (head[0] != 0x7f || head[1] != 'E' || head[2] != 'L' || head[3] != 'F') {
                return false;
            }
            String probe = new String(head, 0, n, java.nio.charset.StandardCharsets.ISO_8859_1);
            if (probe.contains("/system/bin/linker")) {
                return true;
            }
            return !probe.contains("ld-linux") && !probe.contains("ld-musl");
        } catch (Exception e) {
            Logger.logWarn(LOG_TAG, "Could not inspect bun ELF: " + e.getMessage());
            return false;
        }
    }

    private static String readStamp(@NonNull File stamp) throws Exception {
        byte[] buf = new byte[(int) Math.min(stamp.length(), 64)];
        try (FileInputStream in = new FileInputStream(stamp)) {
            int n = in.read(buf);
            if (n <= 0) {
                return "";
            }
            return new String(buf, 0, n,
                java.nio.charset.StandardCharsets.UTF_8).trim();
        }
    }

    private static byte[] loadZipBytes() {
        System.loadLibrary("invapp-bun");
        return getZip();
    }

    private static native byte[] getZip();
}
