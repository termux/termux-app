package com.termux.app;

import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Environment;
import android.util.Log;

import com.termux.terminal.EmulatorDebug;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.net.URL;

public class ApkInstaller {
    static void installPackageApk(String packageName, Context context) {
        // FIXME: Different file names in case of multiple packages being installed at same time.
        new Thread() {
            @Override
            public void run() {
                try {
                    String urlString = "https://termux.net/apks/" + packageName + ".apk";
                    Log.e(EmulatorDebug.LOG_TAG, "Installing " + packageName + ", url is " + urlString);
                    File downloadFile = new File(Environment.getExternalStorageDirectory(), "tmp.apk");
                    URL url = new URL(urlString);
                    try (FileOutputStream out = new FileOutputStream(downloadFile)) {
                        try (InputStream in = url.openStream()) {
                            byte[] buffer = new byte[8196];
                            int read;
                            while ((read = in.read(buffer)) >= 0) {
                                out.write(buffer, 0, read);
                            }
                        }
                    }

                    Intent installIntent = new Intent(Intent.ACTION_INSTALL_PACKAGE);
                    installIntent.setData(Uri.parse("content://com.termux.files" + downloadFile.getAbsolutePath()));
                    installIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_GRANT_READ_URI_PERMISSION);
                    context.startActivity(installIntent);
                } catch (Exception e) {
                    Log.e("termux", "Error installing " + packageName, e);
                }
            }
        }.start();

    }
}
