package com.termux;

import android.content.Context;

public class TermuxApplication extends TermuxServiceApplication {
    private static final String TAG = "TermuxApplication";

    @Override
    protected void attachBaseContext(Context context) {
        super.attachBaseContext(context);

    }

    @Override
    public void onCreate() {
        super.onCreate();
    }
}
