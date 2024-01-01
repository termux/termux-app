package com.termux.shared.interact;

import android.annotation.TargetApi;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.os.Build;

import java.util.ArrayList;
import java.util.List;

public class ProcessTextUtils {

    /**
     * Data required to interact with an activity that supports the PROCESS_TEXT intent.
     */
    public static class ProcessTextActivityData {
        /**
         * The label of the external app activity.
         */
        public final CharSequence label;

        /**
         * The intent to launch the activity.
         */
        public final Intent intent;

        private ProcessTextActivityData(CharSequence label, Intent intent) {
            this.label = label;
            this.intent = intent;
        }
    }

    /**
     * Retrieve a list of activities (including outside of this app) that support the
     * {@link Intent#ACTION_PROCESS_TEXT} intent action. For each item in the returned
     * list, its Intent can be used to launch the activity with the given text.
     *
     * @param context The context for operations
     * @param text    The text to send in the Intents for the supported activities.
     * @return a List of {@link ProcessTextActivityData} for supported activities.
     */
    @TargetApi(Build.VERSION_CODES.M)
    public static List<ProcessTextActivityData> getProcessTextActivities(Context context, String text) {
        PackageManager packageManager = context.getPackageManager();
        Intent processTextIntent = new Intent(Intent.ACTION_PROCESS_TEXT);
        processTextIntent.setType("text/plain");
        List<ResolveInfo> activities = packageManager.queryIntentActivities(processTextIntent, PackageManager.MATCH_ALL);
        List<ProcessTextActivityData> result = new ArrayList<>();
        for (ResolveInfo resolveInfo : activities) {
            CharSequence label = resolveInfo.loadLabel(packageManager);
            result.add(new ProcessTextActivityData(label, createProcessTextIntentForPackage(resolveInfo.activityInfo, text)));
        }
        return result;
    }

    @TargetApi(Build.VERSION_CODES.M)
    private static Intent createProcessTextIntentForPackage(ActivityInfo activityInfo, String text) {
        Intent intent = new Intent(Intent.ACTION_PROCESS_TEXT);
        intent.setComponent(new ComponentName(activityInfo.packageName, activityInfo.name));
        intent.setType("text/plain");
        intent.putExtra(Intent.EXTRA_PROCESS_TEXT, text);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        return intent;
    }
}
