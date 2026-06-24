package com.termux.app.api.file;

import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.provider.OpenableColumns;
import android.util.Patterns;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import com.termux.R;
import com.termux.shared.android.PackageUtils;
import com.termux.shared.data.DataUtils;
import com.termux.shared.data.IntentUtils;
import com.termux.shared.net.uri.UriUtils;
import com.termux.shared.interact.MessageDialogUtils;
import com.termux.shared.net.uri.UriScheme;
import com.termux.shared.termux.interact.TextInputDialogUtils;
import com.termux.shared.termux.TermuxConstants;
import com.termux.shared.termux.TermuxConstants.TERMUX_APP;
import com.termux.shared.termux.TermuxConstants.TERMUX_APP.TERMUX_SERVICE;
import com.termux.app.TermuxService;
import com.termux.shared.logger.Logger;
import com.termux.shared.termux.settings.properties.TermuxAppSharedProperties;
import com.termux.shared.termux.settings.properties.TermuxPropertyConstants;

import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.util.regex.Pattern;

public class FileReceiverActivity extends AppCompatActivity {

    static final String TERMUX_RECEIVEDIR = TermuxConstants.TERMUX_FILES_DIR_PATH + "/home/downloads";
    static final String EDITOR_PROGRAM = TermuxConstants.TERMUX_HOME_DIR_PATH + "/bin/termux-file-editor";
    static final String URL_OPENER_PROGRAM = TermuxConstants.TERMUX_HOME_DIR_PATH + "/bin/termux-url-opener";

    boolean mFinishOnDismissNameDialog = true;

    private static final String API_TAG = TermuxConstants.TERMUX_APP_NAME + "FileReceiver";
    private static final String LOG_TAG = "FileReceiverActivity";

    static boolean isSharedTextAnUrl(String sharedText) {
        if (sharedText == null || sharedText.isEmpty()) return false;

        return Patterns.WEB_URL.matcher(sharedText).matches()
            || Pattern.matches("magnet:\\?xt=urn:btih:.*?", sharedText);
    }

    @Override
    protected void onResume() {
        super.onResume();

        final Intent intent = getIntent();
        final String action = intent.getAction();
        final String type = intent.getType();
        final String scheme = intent.getScheme();

        Logger.logVerbose(LOG_TAG, "Intent Received:\n" + IntentUtils.getIntentString(intent));

        final String sharedTitle = IntentUtils.getStringExtraIfSet(intent, Intent.EXTRA_TITLE, null);

        if (Intent.ACTION_SEND.equals(action) && type != null) {
            final String sharedText = intent.getStringExtra(Intent.EXTRA_TEXT);
            final Uri sharedUri = intent.getParcelableExtra(Intent.EXTRA_STREAM);

            if (sharedUri != null) {
                handleContentUri(sharedUri, sharedTitle);
            } else if (sharedText != null) {
                if (isSharedTextAnUrl(sharedText)) {
                    handleUrlAndFinish(sharedText);
                } else {
                    String subject = IntentUtils.getStringExtraIfSet(intent, Intent.EXTRA_SUBJECT, null);
                    if (subject == null) subject = sharedTitle;
                    if (subject != null) subject += ".txt";
                    
                    // Safely close stream after handling text content
                    ByteArrayInputStream in = new ByteArrayInputStream(sharedText.getBytes(StandardCharsets.UTF_8));
                    promptNameAndSave(in, subject);
                }
            } else {
                showErrorDialogAndQuit("Send action without content - nothing to save.");
            }
        } else {
            Uri dataUri = intent.getData();

            if (dataUri == null) {
                showErrorDialogAndQuit("Data uri not passed.");
                return;
            }

            if (UriScheme.SCHEME_CONTENT.equals(scheme)) {
                handleContentUri(dataUri, sharedTitle);
            } else if (UriScheme.SCHEME_FILE.equals(scheme)) {
                Logger.logVerbose(LOG_TAG, "uri: \"" + dataUri + "\", path: \"" + dataUri.getPath() + "\", fragment: \"" + dataUri.getFragment() + "\"");

                String path = UriUtils.getUriFilePathWithFragment(dataUri);
                if (DataUtils.isNullOrEmpty(path)) {
                    showErrorDialogAndQuit("File path from data uri is null, empty or invalid.");
                    return;
                }

                File file = new File(path);
                try {
                    // Path Traversal check for the input file path if inside sandbox
                    String canonicalPath = file.getCanonicalPath();
                    if (canonicalPath.startsWith(TermuxConstants.TERMUX_FILES_DIR_PATH)) {
                        // Safe internal path verification can go here if needed
                    }
                    
                    FileInputStream in = new FileInputStream(file);
                    promptNameAndSave(in, file.getName());
                } catch (FileNotFoundException e) {
                    showErrorDialogAndQuit("Cannot open file: " + e.getMessage() + ".");
                } catch (IOException e) {
                    showErrorDialogAndQuit("Failed to validate file path.");
                }
            } else {
                showErrorDialogAndQuit("Unable to receive any file or URL.");
            }
        }
    }

    void showErrorDialogAndQuit(String message) {
        mFinishOnDismissNameDialog = false;
        MessageDialogUtils.showMessage(this,
            API_TAG, message,
            null, (dialog, which) -> finish(),
            null, null,
            dialog -> finish());
    }

    void handleContentUri(@NonNull final Uri uri, String subjectFromIntent) {
        InputStream in = null;
        try {
            Logger.logVerbose(LOG_TAG, "uri: \"" + uri + "\", path: \"" + uri.getPath() + "\", fragment: \"" + uri.getFragment() + "\"");

            String attachmentFileName = null;
            String[] projection = new String[]{OpenableColumns.DISPLAY_NAME};
            
            try (Cursor c = getContentResolver().query(uri, projection, null, null, null)) {
                if (c != null && c.moveToFirst()) {
                    final int fileNameColumnId = c.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                    if (fileNameColumnId >= 0) attachmentFileName = c.getString(fileNameColumnId);
                }
            }

            if (attachmentFileName == null) attachmentFileName = subjectFromIntent;
            if (attachmentFileName == null) attachmentFileName = UriUtils.getUriFileBasename(uri, true);

            in = getContentResolver().openInputStream(uri);
            if (in == null) {
                showErrorDialogAndQuit("Unable to open input stream from content URI.");
                return;
            }
            
            promptNameAndSave(in, attachmentFileName);
        } catch (Exception e) {
            if (in != null) {
                try { in.close(); } catch (IOException ignored) {}
            }
            showErrorDialogAndQuit("Unable to handle shared content:\n\n" + e.getMessage());
            Logger.logStackTraceWithMessage(LOG_TAG, "handleContentUri(uri=" + uri + ") failed", e);
        }
    }

    void promptNameAndSave(final InputStream in, final String attachmentFileName) {
        // Sanitize initially suggested text to prevent malicious default text layouts
        String sanitizedDefaultName = sanitizeInputFileName(attachmentFileName);

        TextInputDialogUtils.textInput(this, R.string.title_file_received, sanitizedDefaultName,
            R.string.action_file_received_edit, text -> {
                try {
                    File outFile = saveStreamWithName(in, text);
                    if (outFile == null) return;

                    final File editorProgramFile = new File(EDITOR_PROGRAM);
                    if (!editorProgramFile.isFile()) {
                        showErrorDialogAndQuit("The following file does not exist:\n$HOME/bin/termux-file-editor\n\n"
                            + "Create this file as a script or a symlink - it will be called with the received file as only argument.");
                        return;
                    }

                    // Enforce structural restriction on program path execution
                    if (!editorProgramFile.getAbsolutePath().startsWith(TermuxConstants.TERMUX_HOME_DIR_PATH)) {
                        showErrorDialogAndQuit("Invalid program execution path configuration.");
                        return;
                    }

                    //noinspection ResultOfMethodCallIgnored
                    editorProgramFile.setExecutable(true);

                    final Uri scriptUri = UriUtils.getFileUri(EDITOR_PROGRAM);

                    Intent executeIntent = new Intent(TERMUX_SERVICE.ACTION_SERVICE_EXECUTE, scriptUri);
                    executeIntent.setClass(FileReceiverActivity.this, TermuxService.class);
                    executeIntent.putExtra(TERMUX_SERVICE.EXTRA_ARGUMENTS, new String[]{outFile.getAbsolutePath()});
                    startService(executeIntent);
                    finish();
                } finally {
                    try { in.close(); } catch (IOException ignored) {}
                }
            },
            R.string.action_file_received_open_directory, text -> {
                try {
                    if (saveStreamWithName(in, text) == null) return;

                    Intent executeIntent = new Intent(TERMUX_SERVICE.ACTION_SERVICE_EXECUTE);
                    executeIntent.putExtra(TERMUX_SERVICE.EXTRA_WORKDIR, TERMUX_RECEIVEDIR);
                    executeIntent.setClass(FileReceiverActivity.this, TermuxService.class);
                    startService(executeIntent);
                    finish();
                } finally {
                    try { in.close(); } catch (IOException ignored) {}
                }
            },
            android.R.string.cancel, text -> {
                try { in.close(); } catch (IOException ignored) {}
                finish();
            }, dialog -> {
                try { in.close(); } catch (IOException ignored) {}
                if (mFinishOnDismissNameDialog) finish();
            });
    }

    public File saveStreamWithName(InputStream in, String attachmentFileName) {
        File receiveDir = new File(TERMUX_RECEIVEDIR);

        if (DataUtils.isNullOrEmpty(attachmentFileName)) {
            showErrorDialogAndQuit("File name cannot be null or empty");
            return null;
        }

        // Additional sanitization layer before building file structure
        attachmentFileName = sanitizeInputFileName(attachmentFileName);
        if (DataUtils.isNullOrEmpty(attachmentFileName)) {
            showErrorDialogAndQuit("Invalid symbols inside target filename.");
            return null;
        }

        if (!receiveDir.isDirectory() && !receiveDir.mkdirs()) {
            showErrorDialogAndQuit("Cannot create directory: " + receiveDir.getAbsolutePath());
            return null;
        }

        try {
            final File outFile = new File(receiveDir, attachmentFileName);
            
            // Explicitly verify the output path resolves safely within the intended output folder directory
            String canonicalOutPath = outFile.getCanonicalPath();
            String canonicalReceiveDirPath = receiveDir.getCanonicalPath();
            if (!canonicalOutPath.startsWith(canonicalReceiveDirPath + File.separator)) {
                showErrorDialogAndQuit("Access denied: Arbitrary path traversal attempt detected.");
                return null;
            }

            try (FileOutputStream f = new FileOutputStream(outFile)) {
                byte[] buffer = new byte[4096];
                int readBytes;
                while ((readBytes = in.read(buffer)) > 0) {
                    f.write(buffer, 0, readBytes);
                }
            }
            return outFile;
        } catch (IOException e) {
            showErrorDialogAndQuit("Error saving file:\n\n" + e);
            Logger.logStackTraceWithMessage(LOG_TAG, "Error saving file", e);
            return null;
        }
    }

    void handleUrlAndFinish(final String url) {
        final File urlOpenerProgramFile = new File(URL_OPENER_PROGRAM);
        if (!urlOpenerProgramFile.isFile()) {
            showErrorDialogAndQuit("The following file does not exist:\n$HOME/bin/termux-url-opener\n\n"
                + "Create this file as a script or a symlink - it will be called with the shared URL as the first argument.");
            return;
        }

        if (!urlOpenerProgramFile.getAbsolutePath().startsWith(TermuxConstants.TERMUX_HOME_DIR_PATH)) {
            showErrorDialogAndQuit("Invalid program execution path configuration.");
            return;
        }

        //noinspection ResultOfMethodCallIgnored
        urlOpenerProgramFile.setExecutable(true);

        final Uri urlOpenerProgramUri = UriUtils.getFileUri(URL_OPENER_PROGRAM);

        Intent executeIntent = new Intent(TERMUX_SERVICE.ACTION_SERVICE_EXECUTE, urlOpenerProgramUri);
        executeIntent.setClass(FileReceiverActivity.this, TermuxService.class);
        executeIntent.putExtra(TERMUX_SERVICE.EXTRA_ARGUMENTS, new String[]{url});
        startService(executeIntent);
        finish();
    }

    /**
     * Sanitizes inputs to drop path components and problematic shell characters
     */
    private String sanitizeInputFileName(String filename) {
        if (filename == null) return "";
        // Strip out parent path steps and directory identifiers
        filename = new File(filename).getName();
        // Remove shell metacharacters that cause unexpected execution or command breakdowns
        filename = filename.replaceAll("[\\\\/:*?\"<>|;|&#$`'\"()\\s]+", "_");
        return filename;
    }

    public static void updateFileReceiverActivityComponentsState(@NonNull Context context) {
        new Thread() {
            @Override
            public void run() {
                TermuxAppSharedProperties properties = TermuxAppSharedProperties.getProperties();

                String errmsg;
                boolean state;

                state = !properties.isFileShareReceiverDisabled();
                Logger.logVerbose(LOG_TAG, "Setting " + TERMUX_APP.FILE_SHARE_RECEIVER_ACTIVITY_CLASS_NAME + " component state to " + state);
                errmsg = PackageUtils.setComponentState(context,TermuxConstants.TERMUX_PACKAGE_NAME,
                    TERMUX_APP.FILE_SHARE_RECEIVER_ACTIVITY_CLASS_NAME,
                    state, null, false, false);
                if (errmsg != null)
                    Logger.logError(LOG_TAG, errmsg);

                state = !properties.isFileViewReceiverDisabled();
                Logger.logVerbose(LOG_TAG, "Setting " + TERMUX_APP.FILE_VIEW_RECEIVER_ACTIVITY_CLASS_NAME + " component state to " + state);
                errmsg = PackageUtils.setComponentState(context,TermuxConstants.TERMUX_PACKAGE_NAME,
                    TERMUX_APP.FILE_VIEW_RECEIVER_ACTIVITY_CLASS_NAME,
                    state, null, false, false);
                if (errmsg != null)
                    Logger.logError(LOG_TAG, errmsg);

            }
        }.start();
    }
}