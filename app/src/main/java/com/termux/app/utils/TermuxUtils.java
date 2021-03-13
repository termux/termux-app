package com.termux.app.utils;

import android.content.Context;

import com.termux.app.TermuxConstants;

public class TermuxUtils {

    public static Context getTermuxPackageContext(Context context) {
        try {
            return context.createPackageContext(TermuxConstants.TERMUX_PACKAGE_NAME, Context.CONTEXT_RESTRICTED);
        } catch (Exception e) {
            Logger.logStackTraceWithMessage("Failed to get \"" + TermuxConstants.TERMUX_PACKAGE_NAME + "\" package context. Force using current context.", e);
            Logger.logError("Force using current context");
            return context;
        }
    }
}
