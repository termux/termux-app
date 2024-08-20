package com.termux.x11.controller.core;

import android.app.Activity;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.Executors;
import java.util.concurrent.atomic.AtomicBoolean;

public abstract class HttpUtils {
    private static void downloadAsync(String url, Callback<String> onDownloadComplete) {
        try {
            HttpURLConnection connection = (HttpURLConnection)(new URL(url)).openConnection();
            if (connection.getResponseCode() != HttpURLConnection.HTTP_OK) {
                onDownloadComplete.call(null);
                return;
            }

            byte[] bytes;
            try (InputStream inStream = connection.getInputStream()) {
                bytes = StreamUtils.copyToByteArray(inStream);
            }
            onDownloadComplete.call(new String(bytes, StandardCharsets.UTF_8));
        }
        catch (Exception e) {
            onDownloadComplete.call(null);
        }
    }

    public static void download(final String url, final Callback<String> onDownloadComplete) {
        Executors.newSingleThreadExecutor().execute(() -> downloadAsync(url, onDownloadComplete));
    }

    private static void downloadAsync(String url, File destination, AtomicBoolean interruptRef, Callback<Integer> onPublishProgress, Callback<Boolean> onDownloadComplete) {
        try {
            interruptRef.set(false);
            HttpURLConnection connection = (HttpURLConnection)(new URL(url)).openConnection();
            if (connection.getResponseCode() != HttpURLConnection.HTTP_OK) {
                onDownloadComplete.call(false);
                return;
            }

            int contentLength = connection.getContentLength();
            try (InputStream inStream = new BufferedInputStream(connection.getInputStream(), StreamUtils.BUFFER_SIZE);
                 OutputStream outStream = new FileOutputStream(destination)) {

                byte[] buffer = new byte[1024];
                int totalSize = 0;
                int bytesRead;
                while ((bytesRead = inStream.read(buffer)) != -1 && !interruptRef.get()) {
                    totalSize += bytesRead;
                    if (onPublishProgress != null) {
                        int progress = (int)(((float)totalSize / contentLength) * 100);
                        onPublishProgress.call(progress);
                    }
                    outStream.write(buffer, 0, bytesRead);
                }

            }

            onDownloadComplete.call(!interruptRef.get());
        }
        catch (Exception e) {
            onDownloadComplete.call(false);
        }
    }

    public static void download(final Activity activity, final String url, final File destination, final Callback<Boolean> onDownloadComplete) {
        final DownloadProgressDialog dialog = new DownloadProgressDialog(activity);
        final AtomicBoolean interruptRef = new AtomicBoolean();
        dialog.show(() -> interruptRef.set(true));
        Executors.newSingleThreadExecutor().execute(() -> {
            downloadAsync(url, destination, interruptRef, (progress) -> {
                activity.runOnUiThread(() -> {
                    dialog.setProgress(progress);
                });
            }, (success) -> {
                if (!success && destination.isFile()) destination.delete();
                activity.runOnUiThread(() -> {
                    dialog.close();
                    onDownloadComplete.call(success);
                });
            });
        });
    }
}
