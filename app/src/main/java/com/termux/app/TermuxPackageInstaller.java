package com.termux.app;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.os.Process;
import android.system.Os;
import android.util.Log;

import com.termux.terminal.EmulatorDebug;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;

public class TermuxPackageInstaller extends BroadcastReceiver {

    @Override
    public void onReceive(Context context, Intent intent) {
        try {
            String packageName = intent.getData().getSchemeSpecificPart();
            String action = intent.getAction();
            PackageManager packageManager = context.getPackageManager();

            if (Intent.ACTION_PACKAGE_ADDED.equals(action)) {
                ApplicationInfo info = packageManager.getApplicationInfo(packageName, 0);
                if (Process.myUid() == info.uid) {
                    installPackage(info);
                }
            } else if (Intent.ACTION_PACKAGE_REMOVED.equals(action)) {
                if (Process.myUid() == intent.getIntExtra(Intent.EXTRA_UID, -1)) {
                    uninstallPackage(packageName);
                }

            }
        } catch (Exception e) {
            Log.e("termux", "Error in package management: " + e);
        }
    }

    private static void installPackage(ApplicationInfo info) throws Exception {
        File filesMappingFile = new File(info.nativeLibraryDir, "files.so");
        if (!filesMappingFile.exists()) {
            Log.e("termux", "No file mapping at " + filesMappingFile.getAbsolutePath());
            return;
        }

        Log.e("termux", "Installing: " + info.packageName);
        BufferedReader reader = new BufferedReader(new FileReader(filesMappingFile));
        String line;
        while ((line = reader.readLine()) != null) {
            String[] parts = line.split("←");
            if (parts.length != 2) {
                Log.e(EmulatorDebug.LOG_TAG, "Malformed line " + line + " in " + filesMappingFile.getAbsolutePath());
                continue;
            }

            String oldPath = info.nativeLibraryDir + "/" + parts[0];
            String newPath = TermuxService.PREFIX_PATH + "/" + parts[1];

            TermuxInstaller.ensureDirectoryExists(new File(newPath).getParentFile());

            Log.e(EmulatorDebug.LOG_TAG, "About to setup link: " + oldPath + " ← " + newPath);
            new File(newPath).delete();
            Os.symlink(oldPath, newPath);
        }

        File symlinksFile = new File(info.nativeLibraryDir, "symlinks.so");
        if (!symlinksFile.exists()) {
            Log.e("termux", "No symlinks mapping at " + symlinksFile.getAbsolutePath());
        }

        reader = new BufferedReader(new FileReader(symlinksFile));
        while ((line = reader.readLine()) != null) {
            String[] parts = line.split("←");
            if (parts.length != 2) {
                Log.e(EmulatorDebug.LOG_TAG, "Malformed line " + line + " in " + symlinksFile.getAbsolutePath());
                continue;
            }

            String oldPath = parts[0];
            String newPath = TermuxService.PREFIX_PATH + "/" + parts[1];

            TermuxInstaller.ensureDirectoryExists(new File(newPath).getParentFile());

            Log.e(EmulatorDebug.LOG_TAG, "About to setup link: " + oldPath + " ← " + newPath);
            new File(newPath).delete();
            Os.symlink(oldPath, newPath);
        }
    }

    private static void uninstallPackage(String packageName) throws IOException {
        Log.e("termux", "Uninstalling: " + packageName);
        // We're currently visiting the whole $PREFIX.
        // If we store installed symlinks in installPackage() we could just visit those,
        // at the cost of increased complexity and risk for errors.
        File prefixDir = new File(TermuxService.PREFIX_PATH);
        removeBrokenSymlinks(prefixDir);
    }

    private static void removeBrokenSymlinks(File parentDir) throws IOException {
        for (File child : parentDir.listFiles()) {
            if (!child.exists()) {
                Log.e("termux", "Removing broken symlink: " + child.getAbsolutePath());
                child.delete();
            } else if (child.isDirectory()) {
                removeBrokenSymlinks(child);
            }
        }
    }

    public static void setupAllInstalledPackages(Context context) {
        try {
            removeBrokenSymlinks(new File(TermuxService.PREFIX_PATH));

            PackageManager packageManager = context.getPackageManager();
            for (PackageInfo info : packageManager.getInstalledPackages(0)) {
                if (info.sharedUserId != null && info.sharedUserId.equals("com.termux")) {
                    installPackage(info.applicationInfo);
                }
            }
        } catch (Exception e) {
            Log.e("termux", "Error setting up all packages", e);
        }

    }
}
