package com.termux.dom.api;


import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Environment;
import android.os.Handler;
import android.system.Os;
import android.util.Log;

import java.io.File;

import com.termux.app.TermuxService;
import com.termux.dom.AisCoreUtils;
import com.termux.dom.AisPanelService;


public class MountReceiver extends BroadcastReceiver {
    public static final String TAG = "MountReceiver";

    @Override
    public void onReceive(final Context context, Intent intent) {
        Log.i(TAG, intent.getAction());
        if (AisCoreUtils.onBox()) {
            boolean coverActivity = false;

            try {
                if (intent.getAction().equals("android.intent.action.MEDIA_MOUNTED")) {
                    AisPanelService.publishSpeechText("Dysk zewnętrzny, podłączony.");
                    coverActivity = true;
                } else if (intent.getAction().equals("android.intent.action.MEDIA_UNMOUNTED")) {
                    AisPanelService.publishSpeechText("Dysk zewnętrzny, odłączony.");
                    coverActivity = true;
                }

                final Context c = context;
                // to cover the TvSettings mount disc activity
                if (coverActivity) {
                    setupStorageSymlinks(c);
                    Handler handler = new Handler();
                    handler.postDelayed(new Runnable() {
                        public void run() {
                            startAisActivity(c);
                        }
                    }, 3000);   //3 seconds
                }
            } catch (Exception e) {
                Log.e(TAG, e.toString());
            }
        }
    }


    private void startAisActivity(Context context) {
        Log.d(TAG, "BrowserActivityNative Called");
        try {
            Intent i = new Intent();
            i.setClassName("pl.sviete.dom", "pl.sviete.dom.BrowserActivityNative");
            i.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            context.startActivity(i);
        } catch (Exception e) {
            e.printStackTrace();
        }

    }


    public static void setupStorageSymlinks(final Context context) {
        final String LOG_TAG = "ais-storage";
        new Thread() {
            public void run() {
                try {
                    File storageDir = new File(TermuxService.HOME_PATH, "dom");
                    if (storageDir.exists() && !storageDir.delete()) {
                        Log.i(LOG_TAG, "$HOME/dom exists and it is not empty");
                    } else {
                        if (!storageDir.mkdirs()) {
                            Log.e(LOG_TAG, "Unable to mkdirs() for $HOME/dom return");
                            return;
                        }
                    }
                    try {
                        File domDir = Environment.getExternalStorageDirectory();
                        Os.symlink(domDir.getAbsolutePath() + "/dom", new File(storageDir, "dysk-wewnętrzny").getAbsolutePath());
                    } catch (Exception e) {
                        Log.e(LOG_TAG, "Error setting up link to dysk-wewnętrzny", e);
                    }

                    File externalDir = new File(TermuxService.HOME_PATH + "/dom", "dyski-zewnętrzne");
                    if (externalDir.exists() && !externalDir.delete()) {
                        Log.i(LOG_TAG, "$HOME/dom/dyski-zewnętrzne exists and it is not empty");
                    } else {
                        if (!externalDir.mkdirs()) {
                            Log.e(LOG_TAG, "Unable to mkdirs() for $HOME/dom/dyski-zewnętrzne");
                            return;
                        }
                    }

                    // delete all the symbolic links to external disks in folder
                    try {
                        for(File file: new java.io.File(externalDir.getAbsolutePath()).listFiles()) {
                            file.delete();
                        }
                    } catch (Exception e) {
                        Log.e(LOG_TAG, "Error deliting the symbolic links", e);
                    }

                    // delete all the symbolic links to external disks in folder
                    final File[] dirs = context.getExternalFilesDirs(null);
                    if (dirs != null && dirs.length > 1) {
                        for (int i = 1; i < dirs.length; i++) {
                            File dir = dirs[i];
                            if (dir == null) continue;
                            String symlinkName = "dysk_" + i;
                            int iend = dir.getAbsolutePath().indexOf("/Android/");
                            // we have -> /storage/66E3-0FE6/Android/data/pl.sviete.dom/files
                            // but we want to have /storage/66E3-0FE6
                            Log.i("DYSK", dir.getAbsolutePath().substring(0 , iend));
                            Os.symlink(dir.getAbsolutePath().substring(0 , iend), new File(externalDir, symlinkName).getAbsolutePath());
                        }
                    }

                } catch (Exception e) {
                    Log.e(LOG_TAG, "Error setting up link", e);
                }
            }
        }.start();
    }
}
