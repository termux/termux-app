package com.termux;

import android.app.Application;
import android.content.Context;
import android.util.Log;

import com.termux.service.TermuxConfig;

import java.io.File;

public class TermuxApplication extends Application {
    private static final String TAG = "TermuxApplication";
    private static TermuxApplication sApplication;

    @Override
    protected void attachBaseContext(Context context) {
        super.attachBaseContext(context);
        Log.e(TAG, "Show Application attachBaseContext");
    }

    @Override
    public void onCreate() {
        super.onCreate();
        sApplication = this;
        final File rootDataDir = sApplication.getFilesDir();
        TermuxConfig.FILES_PATH = rootDataDir.toString();
        TermuxConfig.PREFIX_PATH = TermuxConfig.FILES_PATH + "/usr";
        TermuxConfig.HOME_PATH = TermuxConfig.FILES_PATH + "/home";
    }
}
