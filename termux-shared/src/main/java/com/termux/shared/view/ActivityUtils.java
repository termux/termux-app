package com.termux.shared.view;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;

import androidx.activity.result.ActivityResultLauncher;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import com.termux.shared.R;
import com.termux.shared.logger.Logger;
import com.termux.shared.errors.Error;
import com.termux.shared.errors.FunctionErrno;


public class ActivityUtils {

    private static final String LOG_TAG = "ActivityUtils";

    /**
     * Wrapper for {@link #startActivityForResult(Context, int, Intent, boolean, boolean, ActivityResultLauncher)}.
     */
    public static boolean startActivityForResult(Context context, int requestCode, @NonNull Intent intent) {
        return startActivityForResult(context, requestCode, intent, true, true, null);
    }

    /**
     * Wrapper for {@link #startActivityForResult(Context, int, Intent, boolean, boolean, ActivityResultLauncher)}.
     */
    public static boolean startActivityForResult(Context context, int requestCode, @NonNull Intent intent, boolean logErrorMessage, boolean showErrorMessage) {
        return startActivityForResult(context, requestCode, intent, logErrorMessage, showErrorMessage, null);
    }

    /**
     * Start an {@link Activity} for result.
     *
     * @param context The context for operations. It must be an instance of {@link Activity} or
     *               {@link AppCompatActivity}. It is ignored if {@code activityResultLauncher}
     *                is not {@code null}.
     * @param requestCode The request code to use while sending intent. This must be >= 0, otherwise
     *                    exception will be raised. This is ignored if {@code activityResultLauncher}
     *                    is {@code null}.
     * @param intent The {@link Intent} to send to start the activity.
     * @param logErrorMessage If an error message should be logged if failed to start activity.
     * @param showErrorMessage If an error message toast should be shown if failed to start activity
     *                         in addition to logging a message.
     * @param activityResultLauncher The {@link ActivityResultLauncher<Intent>} to use for start the
     *                               activity. If this is {@code null}, then
     *                               {@link Activity#startActivity(Intent)} will be used instead.
     *                               Note that later is deprecated.
     * @return Returns {@code true} if starting activity was successful, otherwise {@code false}.
     */
    public static boolean startActivityForResult(@NonNull Context context, int requestCode, @NonNull Intent intent,
                                                 boolean logErrorMessage, boolean showErrorMessage, @Nullable ActivityResultLauncher<Intent> activityResultLauncher) {
        try {
            if (activityResultLauncher != null) {
                activityResultLauncher.launch(intent);
            } else {
                if (context instanceof AppCompatActivity)
                    ((AppCompatActivity) context).startActivityForResult(intent, requestCode);
                else if (context instanceof Activity)
                    ((Activity) context).startActivityForResult(intent, requestCode);
                else {
                    if (logErrorMessage)
                        Error.logErrorAndShowToast(showErrorMessage ? context : null, LOG_TAG,
                            FunctionErrno.ERRNO_PARAMETER_NOT_INSTANCE_OF.getError("context", "startActivityForResult", "Activity or AppCompatActivity"));
                    return false;
                }
            }
        } catch (Exception e) {
            if (logErrorMessage) {
                String activityName = intent.getComponent() != null ? intent.getComponent().getShortClassName() : "Unknown";
                String errmsg = context.getString(R.string.error_failed_to_start_activity_for_result, activityName);
                Logger.logStackTraceWithMessage(LOG_TAG, errmsg, e);
                if (showErrorMessage)
                    Logger.showToast(context, errmsg + ": " + e.getMessage(), true);
            }
            return false;
        }

        return true;
    }

}
