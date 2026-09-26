package com.termux.app.activities;

import android.app.AlertDialog;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.webkit.WebChromeClient;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import com.termux.R;
import com.termux.app.ssh.CodeMirrorMode;
import com.termux.app.ssh.RemoteFileReader;
import com.termux.app.ssh.RemoteFileWriter;
import com.termux.app.ssh.SSHConnectionInfo;
import com.termux.shared.logger.Logger;
import com.termux.shared.theme.NightMode;
import com.termux.shared.activity.media.AppCompatActivityUtils;

import java.io.Serializable;

/**
 * Activity for editing remote code files via SSH ControlMaster connection.
 *
 * Displays a CodeMirror 5 editor in a WebView with:
 * - Syntax highlighting based on file extension
 * - Line numbers display
 * - Search and replace functionality via toolbar menu
 * - Save functionality syncing back to remote server
 * - Unsaved changes confirmation on exit
 *
 * Requires SSHConnectionInfo and file path passed via Intent extras.
 */
public class RemoteCodeEditorActivity extends AppCompatActivity {

    private static final String LOG_TAG = "RemoteCodeEditorActivity";

    /** Intent extra key for SSH connection info */
    public static final String EXTRA_CONNECTION_INFO = "connection_info";

    /** Intent extra key for remote file path */
    public static final String EXTRA_FILE_PATH = "file_path";

    /** Intent extra key for file name (optional, derived from path if missing) */
    public static final String EXTRA_FILE_NAME = "file_name";

    /** JavaScript interface name for Android-to-JS communication */
    private static final String JS_INTERFACE_NAME = "AndroidInterface";

    /** Current SSH connection info */
    private SSHConnectionInfo mConnectionInfo;

    /** Current remote file path */
    private String mFilePath;

    /** Current file name (displayed in title) */
    private String mFileName;

    /** CodeMirror mode for syntax highlighting */
    private CodeMirrorMode mEditorMode;

    /** WebView containing CodeMirror editor */
    private WebView mWebView;

    /** Loading indicator */
    private ProgressBar mLoadingIndicator;

    /** Error view (shown on load failure) */
    private TextView mErrorView;

    /** Root layout (for controlling visibility) */
    private View mRootLayout;

    /** Handler for UI updates from background threads */
    private final Handler mMainThreadHandler = new Handler(Looper.getMainLooper());

    /** Flag indicating if editor is ready (JavaScript loaded) */
    private boolean mEditorReady = false;

    /** Flag indicating if there are unsaved changes */
    private boolean mHasUnsavedChanges = false;

    /** Original file content (for change detection) */
    private String mOriginalContent = null;

    /** Current file content (for save operations) */
    private String mCurrentContent = null;

    /** Flag indicating if activity is active */
    private boolean mIsActive = false;

    /** Flag indicating if save operation is in progress */
    private boolean mIsSaving = false;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Apply night mode theme
        AppCompatActivityUtils.setNightMode(this, NightMode.getAppNightMode().getName(), true);

        setContentView(R.layout.activity_remote_code_editor);

        // Set up toolbar with back button
        AppCompatActivityUtils.setToolbar(this, com.termux.shared.R.id.toolbar);
        AppCompatActivityUtils.setShowBackButtonInActionBar(this, true);

        // Initialize views
        initializeViews();

        // Parse intent extras
        if (!parseIntentExtras()) {
            finish();
            return;
        }

        // Set toolbar title to file name
        if (getSupportActionBar() != null) {
            getSupportActionBar().setTitle(mFileName);
        }

        // Configure WebView
        configureWebView();

        // Mark activity as active
        mIsActive = true;

        // Load editor HTML (file content will be loaded after editor is ready)
        loadEditorHtml();

        Logger.logDebug(LOG_TAG, "Activity created for: " + mFilePath);
    }

    @Override
    protected void onStart() {
        super.onStart();
        mIsActive = true;
    }

    @Override
    protected void onStop() {
        super.onStop();
        mIsActive = false;
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        mIsActive = false;
        mMainThreadHandler.removeCallbacksAndMessages(null);

        // Clean up WebView
        if (mWebView != null) {
            mWebView.removeJavascriptInterface(JS_INTERFACE_NAME);
            mWebView.destroy();
        }
    }

    @Override
    public boolean onSupportNavigateUp() {
        onBackPressed();
        return true;
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.menu_code_editor, menu);
        return true;
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        // Disable save menu item while saving is in progress
        MenuItem saveItem = menu.findItem(R.id.menu_save);
        if (saveItem != null) {
            saveItem.setEnabled(!mIsSaving);
        }
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(@NonNull MenuItem item) {
        int itemId = item.getItemId();

        if (itemId == R.id.menu_save) {
            saveFile();
            return true;
        } else if (itemId == R.id.menu_search) {
            triggerSearch();
            return true;
        } else if (itemId == R.id.menu_replace) {
            triggerReplace();
            return true;
        }

        return super.onOptionsItemSelected(item);
    }

    @Override
    public void onBackPressed() {
        if (mHasUnsavedChanges) {
            showUnsavedChangesDialog();
        } else {
            super.onBackPressed();
        }
    }

    /**
     * Initialize view references from layout.
     */
    private void initializeViews() {
        mWebView = findViewById(R.id.editor_webview);
        mLoadingIndicator = findViewById(R.id.loading_indicator);
        mErrorView = findViewById(R.id.error_view);
        mRootLayout = findViewById(R.id.root_layout);
    }

    /**
     * Parse SSH connection info and file path from Intent extras.
     *
     * @return true if parsing succeeded, false if required extras missing
     */
    private boolean parseIntentExtras() {
        Intent intent = getIntent();
        if (intent == null) {
            Logger.logError(LOG_TAG, "No intent provided");
            showError(getString(R.string.error_file_load_failed));
            return false;
        }

        // Get SSH connection info
        Serializable connectionInfoSerial = intent.getSerializableExtra(EXTRA_CONNECTION_INFO);
        if (connectionInfoSerial instanceof SSHConnectionInfo) {
            mConnectionInfo = (SSHConnectionInfo) connectionInfoSerial;
        } else {
            // Try alternative: individual components
            String socketPath = intent.getStringExtra("socket_path");
            String user = intent.getStringExtra("user");
            String host = intent.getStringExtra("host");
            int port = intent.getIntExtra("port", 22);

            if (socketPath != null && host != null) {
                if (user == null || user.isEmpty()) {
                    user = "root";
                }
                mConnectionInfo = new SSHConnectionInfo(user, host, port, socketPath);
            } else {
                Logger.logError(LOG_TAG, "SSH connection info not provided in intent");
                showError(getString(R.string.error_file_load_failed));
                return false;
            }
        }

        // Get file path
        mFilePath = intent.getStringExtra(EXTRA_FILE_PATH);
        if (mFilePath == null || mFilePath.isEmpty()) {
            Logger.logError(LOG_TAG, "File path not provided in intent");
            showError(getString(R.string.error_file_load_failed));
            return false;
        }

        // Get or derive file name
        mFileName = intent.getStringExtra(EXTRA_FILE_NAME);
        if (mFileName == null || mFileName.isEmpty()) {
            // Derive from path
            int lastSlash = mFilePath.lastIndexOf('/');
            if (lastSlash >= 0 && lastSlash < mFilePath.length() - 1) {
                mFileName = mFilePath.substring(lastSlash + 1);
            } else {
                mFileName = mFilePath;
            }
        }

        // Determine editor mode from file extension
        mEditorMode = CodeMirrorMode.getModeFromFilename(mFileName);

        Logger.logDebug(LOG_TAG, "Parsed intent: " + mConnectionInfo.toString() +
                       ", path: " + mFilePath + ", mode: " + mEditorMode.getModeName());
        return true;
    }

    /**
     * Configure WebView settings for CodeMirror editor.
     */
    private void configureWebView() {
        WebSettings settings = mWebView.getSettings();

        // Enable JavaScript (required for CodeMirror)
        settings.setJavaScriptEnabled(true);

        // Enable DOM storage (for editor state)
        settings.setDomStorageEnabled(true);

        // Allow file access for assets
        settings.setAllowFileAccess(true);
        settings.setAllowContentAccess(true);

        // Disable zoom (editor handles this internally)
        settings.setSupportZoom(false);
        settings.setBuiltInZoomControls(false);
        settings.setDisplayZoomControls(false);

        // Set text zoom to 100% (respect font settings)
        settings.setTextZoom(100);

        // Use wide viewport for better editing experience
        settings.setUseWideViewPort(true);
        settings.setLoadWithOverviewMode(false);

        // Set WebViewClient for page load callbacks
        mWebView.setWebViewClient(new EditorWebViewClient());

        // Set WebChromeClient for progress updates
        mWebView.setWebChromeClient(new EditorWebChromeClient());

        // Add JavaScript interface for native-to-JS communication
        mWebView.addJavascriptInterface(new CodeEditorJSInterface(), JS_INTERFACE_NAME);
    }

    /**
     * Load CodeMirror HTML template from assets.
     */
    private void loadEditorHtml() {
        Logger.logDebug(LOG_TAG, "Loading editor HTML from assets");

        showLoading(true);
        mWebView.setVisibility(View.VISIBLE);
        mErrorView.setVisibility(View.GONE);

        // Load HTML from assets
        mWebView.loadUrl("file:///android_asset/code_editor.html");
    }

    /**
     * Load file content from remote server and set in editor.
     */
    private void loadFileContent() {
        Logger.logDebug(LOG_TAG, "Loading file content: " + mFilePath);

        new Thread(() -> {
            RemoteFileReader.ReadResult result = RemoteFileReader.readFile(
                this,
                mConnectionInfo,
                mFilePath
            );

            mMainThreadHandler.post(() -> {
                if (!mIsActive) {
                    Logger.logDebug(LOG_TAG, "Activity no longer active, discarding load result");
                    return;
                }

                if (result.isSuccess()) {
                    String content = result.getContent();
                    Logger.logDebug(LOG_TAG, "File loaded successfully: " +
                                   (content != null ? content.length() : 0) + " chars");

                    mOriginalContent = content != null ? content : "";
                    mCurrentContent = mOriginalContent;
                    mHasUnsavedChanges = false;

                    setEditorContent(mOriginalContent, mEditorMode.getModeName(), mFileName);
                    showLoading(false);
                } else {
                    Logger.logError(LOG_TAG, "File load failed: " + result);
                    showError(getString(R.string.error_file_load_failed) +
                             (result.getErrorMessage() != null ? ": " + result.getErrorMessage() : ""));
                }
            });
        }).start();
    }

    /**
     * Set content in CodeMirror editor via JavaScript.
     *
     * @param content File content to display
     * @param mode CodeMirror mode name (MIME type)
     * @param filename File name for display
     */
    private void setEditorContent(@NonNull String content, @NonNull String mode, @NonNull String filename) {
        if (!mEditorReady) {
            Logger.logError(LOG_TAG, "Editor not ready, cannot set content");
            return;
        }

        // Escape content for JavaScript string
        String escapedContent = escapeForJavaScript(content);
        String escapedFilename = escapeForJavaScript(filename);

        String js = String.format("setContent('%s', '%s', '%s')",
                                  escapedContent, mode, escapedFilename);

        Logger.logDebug(LOG_TAG, "Setting editor content via JavaScript: " +
                       content.length() + " chars, mode: " + mode);

        mWebView.evaluateJavascript(js, result -> {
            Logger.logDebug(LOG_TAG, "setContent result: " + result);
        });
    }

    /**
     * Get content from CodeMirror editor via JavaScript.
     *
     * @return Current editor content, or null if not available
     */
    @Nullable
    private String getEditorContent() {
        if (!mEditorReady) {
            Logger.logError(LOG_TAG, "Editor not ready, cannot get content");
            return null;
        }

        // This is synchronous via evaluateJavascript, but we need async
        // For now, we rely on the content being tracked via onContentChanged
        return mCurrentContent;
    }

    /**
     * Request content from editor asynchronously.
     */
    private void requestEditorContent() {
        if (!mEditorReady) {
            Logger.logError(LOG_TAG, "Editor not ready, cannot request content");
            return;
        }

        mWebView.evaluateJavascript("getContent()", result -> {
            // Result is a quoted JSON string, need to unescape
            if (result != null && !result.equals("null")) {
                mCurrentContent = unescapeJavaScriptString(result);
                Logger.logDebug(LOG_TAG, "getContent returned: " +
                               (mCurrentContent != null ? mCurrentContent.length() : 0) + " chars");
            }
        });
    }

    /**
     * Save file content to remote server.
     */
    private void saveFile() {
        if (mIsSaving) {
            Logger.logDebug(LOG_TAG, "Save already in progress");
            return;
        }

        if (!mEditorReady) {
            Logger.logError(LOG_TAG, "Editor not ready, cannot save");
            Toast.makeText(this, getString(R.string.error_file_save_failed), Toast.LENGTH_SHORT).show();
            return;
        }

        // Get current content from editor
        mWebView.evaluateJavascript("getContent()", result -> {
            if (result != null && !result.equals("null")) {
                mCurrentContent = unescapeJavaScriptString(result);

                Logger.logDebug(LOG_TAG, "Saving file: " + mFilePath +
                               ", content: " + (mCurrentContent != null ? mCurrentContent.length() : 0) + " chars");

                executeSave(mCurrentContent);
            } else {
                Logger.logError(LOG_TAG, "getContent returned null");
                Toast.makeText(this, getString(R.string.error_file_save_failed), Toast.LENGTH_SHORT).show();
            }
        });
    }

    /**
     * Execute save operation in background thread.
     *
     * @param content Content to save
     */
    private void executeSave(@NonNull String content) {
        mIsSaving = true;
        invalidateOptionsMenu();  // Disable save button

        new Thread(() -> {
            RemoteFileWriter.WriteResult result = RemoteFileWriter.writeFile(
                this,
                mConnectionInfo,
                mFilePath,
                content
            );

            mMainThreadHandler.post(() -> {
                mIsSaving = false;
                invalidateOptionsMenu();  // Re-enable save button

                if (!mIsActive) {
                    Logger.logDebug(LOG_TAG, "Activity no longer active, discarding save result");
                    return;
                }

                if (result.isSuccess()) {
                    Logger.logDebug(LOG_TAG, "File saved successfully: " + content.length() + " chars");

                    mOriginalContent = content;
                    mHasUnsavedChanges = false;

                    // Mark editor as clean
                    mWebView.evaluateJavascript("markClean()", null);

                    Toast.makeText(this, getString(R.string.success_file_saved), Toast.LENGTH_SHORT).show();
                } else {
                    Logger.logError(LOG_TAG, "File save failed: " + result);
                    Toast.makeText(this, getString(R.string.error_file_save_failed) +
                                  (result.getErrorMessage() != null ? ": " + result.getErrorMessage() : ""),
                                  Toast.LENGTH_LONG).show();
                }
            });
        }).start();
    }

    /**
     * Trigger search dialog in CodeMirror editor.
     */
    private void triggerSearch() {
        if (!mEditorReady) {
            Logger.logError(LOG_TAG, "Editor not ready, cannot search");
            return;
        }

        Logger.logDebug(LOG_TAG, "Triggering search dialog");
        mWebView.evaluateJavascript("triggerSearch()", null);
    }

    /**
     * Trigger replace dialog in CodeMirror editor.
     */
    private void triggerReplace() {
        if (!mEditorReady) {
            Logger.logError(LOG_TAG, "Editor not ready, cannot replace");
            return;
        }

        Logger.logDebug(LOG_TAG, "Triggering replace dialog");
        mWebView.evaluateJavascript("triggerReplace()", null);
    }

    /**
     * Show or hide loading indicator.
     *
     * @param isLoading true to show loading, false to hide
     */
    private void showLoading(boolean isLoading) {
        mLoadingIndicator.setVisibility(isLoading ? View.VISIBLE : View.GONE);
        mWebView.setVisibility(isLoading ? View.GONE : View.VISIBLE);
    }

    /**
     * Show error message in error view.
     *
     * @param message Error message to display
     */
    private void showError(@NonNull String message) {
        mLoadingIndicator.setVisibility(View.GONE);
        mWebView.setVisibility(View.GONE);
        mErrorView.setText(message);
        mErrorView.setVisibility(View.VISIBLE);
        Logger.logError(LOG_TAG, "Error shown: " + message);
    }

    /**
     * Show unsaved changes confirmation dialog.
     */
    private void showUnsavedChangesDialog() {
        new AlertDialog.Builder(this)
            .setTitle(R.string.title_unsaved_changes)
            .setMessage(R.string.message_unsaved_changes)
            .setPositiveButton(R.string.action_save, (dialog, which) -> {
                // Save and then exit
                saveFileAndExit();
            })
            .setNegativeButton(R.string.action_discard, (dialog, which) -> {
                // Discard changes and exit
                mHasUnsavedChanges = false;
                finish();
            })
            .setNeutralButton(android.R.string.cancel, (dialog, which) -> {
                // Stay in editor
            })
            .setCancelable(true)
            .show();
    }

    /**
     * Save file and exit activity after save completes.
     */
    private void saveFileAndExit() {
        if (mIsSaving) {
            return;
        }

        // Set flag to exit after save
        mIsSaving = true;
        invalidateOptionsMenu();

        mWebView.evaluateJavascript("getContent()", result -> {
            if (result != null && !result.equals("null")) {
                mCurrentContent = unescapeJavaScriptString(result);

                new Thread(() -> {
                    RemoteFileWriter.WriteResult saveResult = RemoteFileWriter.writeFile(
                        this,
                        mConnectionInfo,
                        mFilePath,
                        mCurrentContent
                    );

                    mMainThreadHandler.post(() -> {
                        mIsSaving = false;
                        mHasUnsavedChanges = false;

                        if (saveResult.isSuccess()) {
                            Toast.makeText(this, getString(R.string.success_file_saved), Toast.LENGTH_SHORT).show();
                        } else {
                            Toast.makeText(this, getString(R.string.error_file_save_failed), Toast.LENGTH_SHORT).show();
                        }

                        finish();
                    });
                }).start();
            } else {
                mMainThreadHandler.post(() -> {
                    mIsSaving = false;
                    finish();
                });
            }
        });
    }

    /**
     * Escape string for use in JavaScript string literal.
     *
     * Handles quotes, backslashes, and newlines.
     *
     * @param str String to escape
     * @return Escaped string safe for JS
     */
    @NonNull
    private String escapeForJavaScript(@NonNull String str) {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < str.length(); i++) {
            char c = str.charAt(i);
            switch (c) {
                case '\'':
                    sb.append("\\'");
                    break;
                case '\"':
                    sb.append("\\\"");
                    break;
                case '\\':
                    sb.append("\\\\");
                    break;
                case '\n':
                    sb.append("\\n");
                    break;
                case '\r':
                    sb.append("\\r");
                    break;
                case '\t':
                    sb.append("\\t");
                    break;
                default:
                    sb.append(c);
            }
        }
        return sb.toString();
    }

    /**
     * Unescape a JavaScript-quoted string returned by evaluateJavascript.
     *
     * evaluateJavascript returns the value as a JSON-like quoted string.
     *
     * @param str Quoted string from evaluateJavascript
     * @return Unescaped string content
     */
    @Nullable
    private String unescapeJavaScriptString(@Nullable String str) {
        if (str == null || str.equals("null")) {
            return null;
        }

        // Remove surrounding quotes if present
        if (str.startsWith("\"") && str.endsWith("\"")) {
            str = str.substring(1, str.length() - 1);
        } else if (str.startsWith("'") && str.endsWith("'")) {
            str = str.substring(1, str.length() - 1);
        }

        // Unescape common sequences
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < str.length(); i++) {
            char c = str.charAt(i);
            if (c == '\\' && i + 1 < str.length()) {
                char next = str.charAt(i + 1);
                switch (next) {
                    case 'n':
                        sb.append('\n');
                        i++;
                        break;
                    case 'r':
                        sb.append('\r');
                        i++;
                        break;
                    case 't':
                        sb.append('\t');
                        i++;
                        break;
                    case '\\':
                        sb.append('\\');
                        i++;
                        break;
                    case '"':
                        sb.append('"');
                        i++;
                        break;
                    case '\'':
                        sb.append('\'');
                        i++;
                        break;
                    case 'u':
                        // Unicode escape \\uXXXX
                        if (i + 5 < str.length()) {
                            try {
                                int code = Integer.parseInt(str.substring(i + 2, i + 6), 16);
                                sb.append((char) code);
                                i += 5;
                            } catch (NumberFormatException e) {
                                sb.append(c);
                            }
                        } else {
                            sb.append(c);
                        }
                        break;
                    default:
                        sb.append(c);
                }
            } else {
                sb.append(c);
            }
        }
        return sb.toString();
    }

    /**
     * WebViewClient for handling page load events.
     */
    private class EditorWebViewClient extends WebViewClient {

        @Override
        public void onPageFinished(WebView view, String url) {
            Logger.logDebug(LOG_TAG, "WebView page finished: " + url);

            // Page loaded, but editor may not be ready yet
            // Wait for onEditorReady callback from JavaScript
        }

        @Override
        public void onReceivedError(WebView view, int errorCode, String description, String failingUrl) {
            Logger.logError(LOG_TAG, "WebView error: " + errorCode + " - " + description +
                           " for URL: " + failingUrl);
            showError(getString(R.string.error_file_load_failed) + ": " + description);
        }
    }

    /**
     * WebChromeClient for handling progress updates.
     */
    private class EditorWebChromeClient extends WebChromeClient {

        @Override
        public void onProgressChanged(WebView view, int newProgress) {
            Logger.logDebug(LOG_TAG, "WebView progress: " + newProgress + "%");
        }
    }

    /**
     * JavaScript interface for native-to-JS communication.
     *
     * Methods annotated with @JavascriptInterface are callable from JavaScript.
     */
    private class CodeEditorJSInterface {

        /**
         * Called from JavaScript when CodeMirror editor is fully initialized.
         */
        @android.webkit.JavascriptInterface
        public void onEditorReady() {
            Logger.logDebug(LOG_TAG, "Editor ready callback from JavaScript");

            mMainThreadHandler.post(() -> {
                if (!mIsActive) {
                    Logger.logDebug(LOG_TAG, "Activity no longer active, ignoring editor ready");
                    return;
                }

                mEditorReady = true;

                // Load file content now that editor is ready
                loadFileContent();
            });
        }

        /**
         * Called from JavaScript when editor content changes.
         *
         * @param content Current editor content (may be large)
         */
        @android.webkit.JavascriptInterface
        public void onContentChanged(String content) {
            // This is called frequently during typing - be careful with performance
            // We track the current content but don't do heavy processing here

            mCurrentContent = content;

            // Check if content differs from original
            boolean hasChanges = (mOriginalContent != null && !content.equals(mOriginalContent));

            if (hasChanges != mHasUnsavedChanges) {
                mHasUnsavedChanges = hasChanges;
                Logger.logDebug(LOG_TAG, "Unsaved changes state changed: " + mHasUnsavedChanges);
            }
        }
    }
}