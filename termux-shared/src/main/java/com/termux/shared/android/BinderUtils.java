package com.termux.shared.android;

import android.content.Context;
import android.os.Binder;
import android.os.Process;

import com.termux.shared.termux.TermuxConstants;

public class BinderUtils
{
    /**
     * Get the package name of the calling process for the current Binder transaction of this thread.
     * Returns null if no Binder transaction is being processed.
     * 
     * @param c The context used to get the {@link android.content.pm.PackageManager}.
     * @return The calling package name, or null if the thread isn't processing a Binder transaction.
     */
    public static String getCallerPackageNameOrNull(Context c) {
        if (c.getApplicationInfo().uid == Binder.getCallingUid()) {
            if (Process.myPid() == Binder.getCallingPid()) {
                return null; // same process as Termux
            } else {
                // calling process has the same uid: search all processes with the same uid for the pid
                for (String pack : c.getPackageManager().getPackagesForUid(Binder.getCallingUid())) {
                    if (String.valueOf(Binder.getCallingPid()).equals(PackageUtils.getPackagePID(c, pack))) {
                        return pack;
                    }
                }
                return c.getPackageName(); // process could not be found under shared uid processes, user Termux as default
            }
        } else {
            return c.getPackageManager().getNameForUid(Binder.getCallingUid()); // foreign uid, getNameForUid is correct
        }
    }
    
    /**
     * Get the package name of the calling process for the current Binder transaction of this thread.
     * Returns {@link Context#getPackageName()} if no Binder transaction is being processed.
     *
     * @param c The context used to get the {@link android.content.pm.PackageManager}.
     * @return The calling package name, or {@link Context#getPackageName()} if the thread isn't processing a Binder transaction.
     */
    public static String getCallerPackageName(Context c) {
        String packageName = getCallerPackageNameOrNull(c);
        if (packageName == null) {
            return c.getPackageName();
        } else {
            return packageName;
        }
    }
    
    /**
     * @param c The context used to get the {@link android.content.pm.PackageManager}.
     * @return The plugin directory of the calling package name under {@link TermuxConstants#TERMUX_APPS_DIR_PATH}, or null if {@link #getCallerPackageNameOrNull(Context)} returns null.
     */
    public static String getCallingPluginDir(Context c) {
        String packageName = getCallerPackageNameOrNull(c);
        if (packageName == null) {
            return null;
        }
        return TermuxConstants.TERMUX_PLUGINS_DIR_PATH + "/" + packageName;
    }
    
    
    /**
     * Throws a {@link SecurityException} if the calling process of the current Binder transaction doesn't have the RUN_COMMAND permission
     * or the current thread isn't processing a Binder transaction.
     * 
     * @param c The context used to get the {@link android.content.pm.PackageManager}.
     */
    public static void enforceRunCommandPermission(Context c) {
        c.enforceCallingPermission(TermuxConstants.PERMISSION_RUN_COMMAND, "Calling package "+ getCallerPackageName(c)+" does not have the" + TermuxConstants.PERMISSION_RUN_COMMAND + " permission.");
    }
    
    
    
    
    
    
}
