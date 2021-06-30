package com.termux.shared.crash;

import android.content.Context;

import androidx.annotation.NonNull;

import com.termux.shared.termux.TermuxConstants;
import com.termux.shared.termux.TermuxUtils;

public class TermuxCrashUtils implements CrashHandler.CrashHandlerClient {

    /**
     * Set default uncaught crash handler of current thread to {@link CrashHandler} for Termux app
     * and its plugin to log crashes at {@link TermuxConstants#TERMUX_CRASH_LOG_FILE_PATH}.
     */
    public static void setCrashHandler(@NonNull final Context context) {
        CrashHandler.setCrashHandler(context, new TermuxCrashUtils());
    }

    @NonNull
    @Override
    public String getCrashLogFilePath(Context context) {
        return TermuxConstants.TERMUX_CRASH_LOG_FILE_PATH;
    }

    @Override
    public String getAppInfoMarkdownString(Context context) {
        return TermuxUtils.getAppInfoMarkdownString(context, true);
    }

}
