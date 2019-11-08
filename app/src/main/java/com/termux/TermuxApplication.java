package com.termux;

import android.app.Application;
import android.content.Context;
import android.util.Log;

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
    }
}
