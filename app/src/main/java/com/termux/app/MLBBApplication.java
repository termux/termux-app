package com.termux.app;

import android.app.Application;

/** Standalone GUI app. Avoids Termux runtime initialization until a matching bootstrap exists. */
public class MLBBApplication extends Application {
    @Override public void onCreate() { super.onCreate(); }
}
