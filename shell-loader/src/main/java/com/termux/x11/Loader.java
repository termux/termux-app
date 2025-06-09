package com.termux.x11;

import com.termux.shell.BuildConfig;

public class Loader {
    /**
     * Command-line entry point.
     * It is pretty simple.
     * 1. Check if application is installed.
     * 2. Check if target apk's signature matches stored hash to prevent running code of potentially replaced malicious apk.
     * 3. Load target apk code with `PathClassLoader` and start target's main function.
     * <p>
     * This way we can make this loader version-agnostic and keep it secure. All application logic is located in target apk.
     *
     * @param args The command-line arguments
     */
        public static void main(String[] args) {
            String cls = System.getenv("TERMUX_X11_LOADER_OVERRIDE_CMDENTRYPOINT_CLASS");
            cls = cls != null ? cls : BuildConfig.CLASS_ID;
            try {
                android.content.pm.PackageInfo targetInfo = (android.os.Build.VERSION.SDK_INT <= 32) ?
                    android.app.ActivityThread.getPackageManager().getPackageInfo(BuildConfig.APPLICATION_ID, android.content.pm.PackageManager.GET_SIGNATURES, 0) :
                    android.app.ActivityThread.getPackageManager().getPackageInfo(BuildConfig.APPLICATION_ID, (long) android.content.pm.PackageManager.GET_SIGNATURES, 0);
                assert targetInfo != null : BuildConfig.packageNotInstalledErrorText.replace("ARCH", android.os.Build.SUPPORTED_ABIS[0]);
                assert targetInfo.signatures.length == 1 && BuildConfig.SIGNATURE == targetInfo.signatures[0].hashCode() : BuildConfig.packageSignatureMismatchErrorText;

                android.util.Log.i(BuildConfig.logTag, "loading " + targetInfo.applicationInfo.sourceDir + "::" + BuildConfig.CLASS_ID + "::main of " + BuildConfig.APPLICATION_ID + " application (commit " + BuildConfig.COMMIT + ")");
                Class<?> targetClass = Class.forName(cls, true,
                    new dalvik.system.PathClassLoader(targetInfo.applicationInfo.sourceDir, null, ClassLoader.getSystemClassLoader()));
                targetClass.getMethod("main", String[].class).invoke(null, (Object) args);
            } catch (AssertionError e) {
                System.err.println(e.getMessage());
            } catch (java.lang.reflect.InvocationTargetException e) {
                e.getCause().printStackTrace(System.err);
            } catch (Throwable e) {
                android.util.Log.e(BuildConfig.logTag, "Loader error", e);
                e.printStackTrace(System.err);
            }
        }

}
