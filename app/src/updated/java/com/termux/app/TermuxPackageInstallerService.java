package com.termux.app;

import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.os.Binder;
import android.os.IBinder;
import android.os.Process;
import android.system.Os;
import android.util.Log;

import androidx.annotation.Nullable;

import com.termux.shared.termux.TermuxConstants;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;

public class TermuxPackageInstallerService extends Service {

    private static final String LOG_TAG = "termux-package-installer";

    class LocalBinder extends Binder {
        public final TermuxPackageInstallerService service = TermuxPackageInstallerService.this;
    }

    private final IBinder mBinder = new LocalBinder();

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    @Override
    public void onCreate() {
        super.onCreate();

        TermuxPackageInstaller packageInstaller = new TermuxPackageInstaller();
        TermuxPackageInstaller.setupAllInstalledPackages(this);

        IntentFilter addedFilter = new IntentFilter(Intent.ACTION_PACKAGE_ADDED);
        addedFilter.addDataScheme("package");
        IntentFilter removedFilter = new IntentFilter(Intent.ACTION_PACKAGE_REMOVED);
        removedFilter.addDataScheme("package");
        this.registerReceiver(packageInstaller, addedFilter);
        this.registerReceiver(packageInstaller, removedFilter);
    }
}
