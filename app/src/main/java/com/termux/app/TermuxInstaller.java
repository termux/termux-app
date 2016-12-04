package com.termux.app;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.DialogInterface.OnDismissListener;
import android.os.Build;
import android.os.Environment;
import android.os.UserManager;
import android.system.Os;
import android.util.Log;
import android.util.Pair;
import android.view.WindowManager;

import com.termux.R;
import com.termux.terminal.EmulatorDebug;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStreamReader;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

/**
 * Install the Termux bootstrap packages if necessary by following the below steps:
 * <p/>
 * (1) If $PREFIX already exist, assume that it is correct and be done. Note that this relies on that we do not create a
 * broken $PREFIX folder below.
 * <p/>
 * (2) A progress dialog is shown with "Installing..." message and a spinner.
 * <p/>
 * (3) A staging folder, $STAGING_PREFIX, is {@link #deleteFolder(File)} if left over from broken installation below.
 * <p/>
 * (4) The architecture is determined and an appropriate bootstrap zip url is determined in {@link #determineZipUrl()}.
 * <p/>
 * (5) The zip, containing entries relative to the $PREFIX, is is downloaded and extracted by a zip input stream
 * continously encountering zip file entries:
 * <p/>
 * (5.1) If the zip entry encountered is SYMLINKS.txt, go through it and remember all symlinks to setup.
 * <p/>
 * (5.2) For every other zip entry, extract it into $STAGING_PREFIX and set execute permissions if necessary.
 */
final class TermuxInstaller {

    /** Performs setup if necessary. */
    static void setupIfNeeded(final Activity activity, final Runnable whenDone) {
        // Termux can only be run as the primary user (device owner) since only that
        // account has the expected file system paths. Verify that:
        UserManager um = (UserManager) activity.getSystemService(Context.USER_SERVICE);
        boolean isPrimaryUser = um.getSerialNumberForUser(android.os.Process.myUserHandle()) == 0;
        if (!isPrimaryUser) {
            new AlertDialog.Builder(activity).setTitle(R.string.bootstrap_error_title).setMessage(R.string.bootstrap_error_not_primary_user_message)
                .setOnDismissListener(new OnDismissListener() {
                    @Override
                    public void onDismiss(DialogInterface dialog) {
                        System.exit(0);
                    }
                }).setPositiveButton(android.R.string.ok, null).show();
            return;
        }

        final File PREFIX_FILE = new File(TermuxService.PREFIX_PATH);
        if (PREFIX_FILE.isDirectory()) {
            whenDone.run();
            return;
        }

        final ProgressDialog progress = ProgressDialog.show(activity, null, activity.getString(R.string.bootstrap_installer_body), true, false);
        new Thread() {
            @Override
            public void run() {
                try {
                    final String STAGING_PREFIX_PATH = TermuxService.FILES_PATH + "/usr-staging";
                    final File STAGING_PREFIX_FILE = new File(STAGING_PREFIX_PATH);

                    if (STAGING_PREFIX_FILE.exists()) {
                        deleteFolder(STAGING_PREFIX_FILE);
                    }

                    final byte[] buffer = new byte[8096];
                    final List<Pair<String, String>> symlinks = new ArrayList<>(50);

                    final URL zipUrl = determineZipUrl();
                    try (ZipInputStream zipInput = new ZipInputStream(zipUrl.openStream())) {
                        ZipEntry zipEntry;
                        while ((zipEntry = zipInput.getNextEntry()) != null) {
                            if (zipEntry.getName().equals("SYMLINKS.txt")) {
                                BufferedReader symlinksReader = new BufferedReader(new InputStreamReader(zipInput));
                                String line;
                                while ((line = symlinksReader.readLine()) != null) {
                                    String[] parts = line.split("←");
                                    if (parts.length != 2)
                                        throw new RuntimeException("Malformed symlink line: " + line);
                                    String oldPath = parts[0];
                                    String newPath = STAGING_PREFIX_PATH + "/" + parts[1];
                                    symlinks.add(Pair.create(oldPath, newPath));
                                }
                            } else {
                                String zipEntryName = zipEntry.getName();
                                File targetFile = new File(STAGING_PREFIX_PATH, zipEntryName);
                                if (zipEntry.isDirectory()) {
                                    if (!targetFile.mkdirs())
                                        throw new RuntimeException("Failed to create directory: " + targetFile.getAbsolutePath());
                                } else {
                                    try (FileOutputStream outStream = new FileOutputStream(targetFile)) {
                                        int readBytes;
                                        while ((readBytes = zipInput.read(buffer)) != -1)
                                            outStream.write(buffer, 0, readBytes);
                                    }
                                    if (zipEntryName.startsWith("bin/") || zipEntryName.startsWith("libexec") || zipEntryName.startsWith("lib/apt/methods")) {
                                        //noinspection OctalInteger
                                        Os.chmod(targetFile.getAbsolutePath(), 0700);
                                    }
                                }
                            }
                        }
                    }

                    if (symlinks.isEmpty())
                        throw new RuntimeException("No SYMLINKS.txt encountered");
                    for (Pair<String, String> symlink : symlinks) {
                        Os.symlink(symlink.first, symlink.second);
                    }

                    if (!STAGING_PREFIX_FILE.renameTo(PREFIX_FILE)) {
                        throw new RuntimeException("Unable to rename staging folder");
                    }

                    activity.runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            whenDone.run();
                        }
                    });
                } catch (final Exception e) {
                    Log.e(EmulatorDebug.LOG_TAG, "Bootstrap error", e);
                    activity.runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            try {
                                new AlertDialog.Builder(activity).setTitle(R.string.bootstrap_error_title).setMessage(R.string.bootstrap_error_body)
                                    .setNegativeButton(R.string.bootstrap_error_abort, new OnClickListener() {
                                        @Override
                                        public void onClick(DialogInterface dialog, int which) {
                                            dialog.dismiss();
                                            activity.finish();
                                        }
                                    }).setPositiveButton(R.string.bootstrap_error_try_again, new OnClickListener() {
                                    @Override
                                    public void onClick(DialogInterface dialog, int which) {
                                        dialog.dismiss();
                                        TermuxInstaller.setupIfNeeded(activity, whenDone);
                                    }
                                }).show();
                            } catch (WindowManager.BadTokenException e) {
                                // Activity already dismissed - ignore.
                            }
                        }
                    });
                } finally {
                    activity.runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            try {
                                progress.dismiss();
                            } catch (RuntimeException e) {
                                // Activity already dismissed - ignore.
                            }
                        }
                    });
                }
            }
        }.start();
    }

    /** Get bootstrap zip url for this systems cpu architecture. */
    static URL determineZipUrl() throws MalformedURLException {
        String archName = determineTermuxArchName();
        return new URL("https://termux.net/bootstrap/bootstrap-" + archName + ".zip");
    }

    private static String determineTermuxArchName() {
        // Note that we cannot use System.getProperty("os.arch") since that may give e.g. "aarch64"
        // while a 64-bit runtime may not be installed (like on the Samsung Galaxy S5 Neo).
        // Instead we search through the supported abi:s on the device, see:
        // http://developer.android.com/ndk/guides/abis.html
        // Note that we search for abi:s in preferred order (the ordering of the
        // Build.SUPPORTED_ABIS list) to avoid e.g. installing arm on an x86 system where arm
        // emulation is available.
        for (String androidArch : Build.SUPPORTED_ABIS) {
            switch (androidArch) {
                case "arm64-v8a": return "aarch64";
                case "armeabi-v7a": return "arm";
                case "x86_64": return "x86_64";
                case "x86": return "i686";
            }
        }
        throw new RuntimeException("Unable to determine arch from Build.SUPPORTED_ABIS =  " +
            Arrays.toString(Build.SUPPORTED_ABIS));
    }

    /** Delete a folder and all its content or throw. */
    static void deleteFolder(File fileOrDirectory) {
        File[] children = fileOrDirectory.listFiles();
        if (children != null) {
            for (File child : children) {
                deleteFolder(child);
            }
        }
        if (!fileOrDirectory.delete()) {
            throw new RuntimeException("Unable to delete " + (fileOrDirectory.isDirectory() ? "directory " : "file ") + fileOrDirectory.getAbsolutePath());
        }
    }

    public static void setupStorageSymlinks(final Context context) {
        final String LOG_TAG = "termux-storage";
        new Thread() {
            public void run() {
                try {
                    File storageDir = new File(TermuxService.HOME_PATH, "storage");

                    if (storageDir.exists() && !storageDir.delete()) {
                        Log.e(LOG_TAG, "Could not delete old $HOME/storage");
                        return;
                    }

                    if (!storageDir.mkdirs()) {
                        Log.e(LOG_TAG, "Unable to mkdirs() for $HOME/storage");
                        return;
                    }

                    File sharedDir = Environment.getExternalStorageDirectory();
                    Os.symlink(sharedDir.getAbsolutePath(), new File(storageDir, "shared").getAbsolutePath());

                    File downloadsDir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS);
                    Os.symlink(downloadsDir.getAbsolutePath(), new File(storageDir, "downloads").getAbsolutePath());

                    File dcimDir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DCIM);
                    Os.symlink(dcimDir.getAbsolutePath(), new File(storageDir, "dcim").getAbsolutePath());

                    File picturesDir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_PICTURES);
                    Os.symlink(picturesDir.getAbsolutePath(), new File(storageDir, "pictures").getAbsolutePath());

                    File musicDir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_MUSIC);
                    Os.symlink(musicDir.getAbsolutePath(), new File(storageDir, "music").getAbsolutePath());

                    File moviesDir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_MOVIES);
                    Os.symlink(moviesDir.getAbsolutePath(), new File(storageDir, "movies").getAbsolutePath());

                    final File[] dirs = context.getExternalFilesDirs(null);
                    if (dirs != null && dirs.length > 1) {
                        for (int i = 1; i < dirs.length; i++) {
                            File dir = dirs[i];
                            if (dir == null) continue;
                            String symlinkName = "external-" + i;
                            Os.symlink(dir.getAbsolutePath(), new File(storageDir, symlinkName).getAbsolutePath());
                        }
                    }
                } catch (Exception e) {
                    Log.e(LOG_TAG, "Error setting up link", e);
                }
            }
        }.start();
    }

}
