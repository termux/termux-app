package com.termux.app.activities;

import android.app.AlertDialog;
import android.content.ContentResolver;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.provider.DocumentsContract;
import android.provider.OpenableColumns;
import android.view.ContextMenu;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import com.termux.R;
import com.termux.app.ssh.CodeMirrorMode;
import com.termux.app.ssh.ImageFileType;
import com.termux.app.ssh.RemoteFile;
import com.termux.app.ssh.RemoteFileListAdapter;
import com.termux.app.ssh.RemoteFileLister;
import com.termux.app.ssh.RemoteFileOperator;
import com.termux.app.ssh.RemoteFileTransfer;
import com.termux.app.ssh.SSHConnectionInfo;
import com.termux.shared.interact.MessageDialogUtils;
import com.termux.shared.logger.Logger;
import com.termux.shared.theme.NightMode;
import com.termux.shared.activity.media.AppCompatActivityUtils;
import com.termux.shared.termux.interact.TextInputDialogUtils;

import java.io.InputStream;
import java.io.OutputStream;
import java.io.Serializable;
import java.util.ArrayList;
import java.util.List;
import java.util.Stack;

/**
 * Activity for browsing remote files via SSH ControlMaster connection.
 *
 * Displays a directory listing from a remote SSH server with:
 * - Breadcrumb navigation showing current path
 * - Click to enter subdirectories
 * - Back button to navigate to parent directories
 * - Refresh button to reload current directory
 * - Upload button to send local files to remote server
 * - Download context menu to retrieve remote files
 *
 * Requires SSHConnectionInfo passed via Intent extras to establish
 * connection through existing ControlMaster socket.
 */
public class RemoteFileBrowserActivity extends AppCompatActivity {

    private static final String LOG_TAG = "RemoteFileBrowserActivity";

    /** Intent extra key for SSH connection info */
    public static final String EXTRA_CONNECTION_INFO = "connection_info";

    /** Intent extra key for initial remote path */
    public static final String EXTRA_INITIAL_PATH = "initial_path";

    /** Default initial path if not specified */
    private static final String DEFAULT_INITIAL_PATH = "/";

    /** Request code for SAF upload file picker */
    private static final int REQUEST_CODE_UPLOAD = 1001;

    /** Request code for SAF download file saver */
    private static final int REQUEST_CODE_DOWNLOAD = 1002;

    /** Minimum progress update interval in milliseconds (throttle UI updates) */
    private static final long PROGRESS_UPDATE_INTERVAL_MS = 100;

    /** Minimum bytes change for progress update (throttle UI updates) */
    private static final long PROGRESS_UPDATE_MIN_BYTES = 1024;

    /** Current SSH connection info */
    private SSHConnectionInfo mConnectionInfo;

    /** Current remote directory path */
    private String mCurrentPath;

    /** Stack of visited paths for back navigation */
    private final Stack<String> mPathStack = new Stack<>();

    /** ListView for displaying files */
    private ListView mFileListView;

    /** Adapter for file list */
    private RemoteFileListAdapter mAdapter;

    /** Empty state view */
    private TextView mEmptyView;

    /** Breadcrumb path layout container */
    private LinearLayout mBreadcrumbPathLayout;

    /** Back button */
    private ImageButton mBackButton;

    /** Refresh button */
    private View mRefreshButton;

    /** New folder button */
    private View mNewFolderButton;

    /** Upload button */
    private View mUploadButton;

    /** Loading indicator */
    private ProgressBar mLoadingIndicator;

    /** Main content view (hidden during loading) */
    private View mContentContainer;

    /** Handler for UI updates from background threads */
    private final Handler mMainThreadHandler = new Handler(Looper.getMainLooper());

    /** Flag indicating if a load operation is in progress */
    private boolean mIsLoading = false;

    /** Flag indicating if activity is still active */
    private boolean mIsActive = false;

    /** Currently selected file for context menu operations */
    private RemoteFile mSelectedFile = null;

    /** Progress dialog for file transfers */
    private AlertDialog mProgressDialog = null;

    /** Progress bar in transfer dialog */
    private ProgressBar mTransferProgressBar = null;

    /** Progress text in transfer dialog */
    private TextView mTransferProgressText = null;

    /** Last progress update timestamp for throttling */
    private long mLastProgressUpdateMs = 0;

    /** Last reported bytes for throttling */
    private long mLastReportedBytes = 0;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Apply night mode theme
        AppCompatActivityUtils.setNightMode(this, NightMode.getAppNightMode().getName(), true);

        setContentView(R.layout.activity_remote_file_browser);

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

        // Set up click listeners
        setupClickListeners();

        // Mark activity as active
        mIsActive = true;

        // Load initial directory
        loadDirectory(mCurrentPath);

        Logger.logDebug(LOG_TAG, "Activity created for connection: " + mConnectionInfo.toString());
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

        // Close progress dialog to prevent memory leak
        if (mProgressDialog != null && mProgressDialog.isShowing()) {
            mProgressDialog.dismiss();
            mProgressDialog = null;
        }
    }

    @Override
    public boolean onSupportNavigateUp() {
        onBackPressed();
        return true;
    }

    @Override
    public void onBackPressed() {
        if (!mPathStack.isEmpty()) {
            // Navigate to parent directory instead of exiting
            navigateToParent();
        } else {
            // At root level - exit the activity
            super.onBackPressed();
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        Logger.logDebug(LOG_TAG, "onActivityResult: requestCode=" + requestCode +
                       " resultCode=" + resultCode + " data=" + (data != null ? data.getData() : "null"));

        if (resultCode != RESULT_OK || data == null) {
            // User cancelled or no data - just close progress dialog if showing
            if (mProgressDialog != null && mProgressDialog.isShowing()) {
                mProgressDialog.dismiss();
            }
            return;
        }

        if (requestCode == REQUEST_CODE_UPLOAD) {
            handleUploadResult(requestCode, resultCode, data);
        } else if (requestCode == REQUEST_CODE_DOWNLOAD) {
            handleDownloadResult(requestCode, resultCode, data);
        }
    }

    /**
     * Initialize view references from layout.
     */
    private void initializeViews() {
        mFileListView = findViewById(R.id.file_list);
        mEmptyView = findViewById(R.id.empty_view);
        mBreadcrumbPathLayout = findViewById(R.id.breadcrumb_path_layout);
        mBackButton = findViewById(R.id.back_button);
        mRefreshButton = findViewById(R.id.refresh_button);
        mNewFolderButton = findViewById(R.id.new_folder_button);
        mUploadButton = findViewById(R.id.upload_button);

        // Create loading indicator programmatically
        mLoadingIndicator = findViewById(R.id.loading_indicator);
        mContentContainer = findViewById(R.id.root_layout);

        // Set empty view for ListView
        mFileListView.setEmptyView(mEmptyView);

        // Create adapter with empty list
        mAdapter = new RemoteFileListAdapter(this, new ArrayList<>());
        mFileListView.setAdapter(mAdapter);

        // Register for context menu (long press)
        registerForContextMenu(mFileListView);
    }

    /**
     * Parse SSH connection info and initial path from Intent extras.
     *
     * @return true if parsing succeeded, false if required extras missing
     */
    private boolean parseIntentExtras() {
        Intent intent = getIntent();
        if (intent == null) {
            Logger.logError(LOG_TAG, "No intent provided");
            showError(getString(R.string.error_not_connected));
            return false;
        }

        // Get SSH connection info (must be passed from SSH session activity)
        // The connection info should be provided as a Serializable or Parcelable
        // For now, we expect socket path components to be passed separately
        String socketPath = intent.getStringExtra("socket_path");
        String user = intent.getStringExtra("user");
        String host = intent.getStringExtra("host");
        int port = intent.getIntExtra("port", 22);

        if (socketPath == null || socketPath.isEmpty()) {
            // Try to get serialized connection info object
            Serializable connectionInfoSerial = intent.getSerializableExtra(EXTRA_CONNECTION_INFO);
            if (connectionInfoSerial instanceof SSHConnectionInfo) {
                mConnectionInfo = (SSHConnectionInfo) connectionInfoSerial;
            } else {
                Logger.logError(LOG_TAG, "SSH connection info not provided in intent");
                showError(getString(R.string.error_not_connected));
                return false;
            }
        } else {
            // Build connection info from individual components
            if (user == null || user.isEmpty()) {
                user = "root"; // Default user
            }
            if (host == null || host.isEmpty()) {
                Logger.logError(LOG_TAG, "Host not provided in intent");
                showError(getString(R.string.error_not_connected));
                return false;
            }
            mConnectionInfo = new SSHConnectionInfo(user, host, port, socketPath);
        }

        // Get initial path
        mCurrentPath = intent.getStringExtra(EXTRA_INITIAL_PATH);
        if (mCurrentPath == null || mCurrentPath.isEmpty()) {
            mCurrentPath = DEFAULT_INITIAL_PATH;
        }

        Logger.logDebug(LOG_TAG, "Parsed intent: " + mConnectionInfo.toString() + ", path: " + mCurrentPath);
        return true;
    }

    /**
     * Set up click listeners for navigation and actions.
     */
    private void setupClickListeners() {
        // Back button - navigate to parent directory
        mBackButton.setOnClickListener(v -> navigateToParent());

        // Refresh button - reload current directory
        mRefreshButton.setOnClickListener(v -> loadDirectory(mCurrentPath));

        // New folder button - show new folder dialog
        mNewFolderButton.setOnClickListener(v -> showNewFolderDialog());

        // Upload button - trigger SAF file picker
        mUploadButton.setOnClickListener(v -> startUploadFilePicker());

        // File item click - enter directory or show file info
        mFileListView.setOnItemClickListener((parent, view, position, id) -> {
            RemoteFile file = mAdapter.getItem(position);
            if (file != null) {
                onFileItemClick(file);
            }
        });
    }

    /**
     * Start SAF file picker for upload.
     *
     * Opens ACTION_OPEN_DOCUMENT Intent allowing user to select any file
     * from local storage to upload to current remote directory.
     */
    private void startUploadFilePicker() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");  // Allow any file type
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);

        Logger.logDebug(LOG_TAG, "Starting SAF upload file picker");

        startActivityForResult(intent, REQUEST_CODE_UPLOAD);
    }

    /**
     * Start SAF file saver for download.
     *
     * Opens ACTION_CREATE_DOCUMENT Intent allowing user to choose where
     * to save the downloaded file on local storage.
     *
     * @param fileName Default file name to suggest
     */
    private void startDownloadFileSaver(@NonNull String fileName) {
        Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");  // Allow any file type
        intent.putExtra(Intent.EXTRA_TITLE, fileName);
        intent.addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION);

        Logger.logDebug(LOG_TAG, "Starting SAF download file saver for: " + fileName);

        startActivityForResult(intent, REQUEST_CODE_DOWNLOAD);
    }

    /**
     * Show progress dialog for file transfer.
     *
     * Creates an AlertDialog with ProgressBar and progress text.
     * Dialog is non-cancellable to prevent interrupting transfer.
     *
     * @param title Dialog title (e.g., "Uploading..." or "Downloading...")
     * @return The created AlertDialog
     */
    @NonNull
    private AlertDialog showTransferProgressDialog(@NonNull String title) {
        // Reset throttling state
        mLastProgressUpdateMs = 0;
        mLastReportedBytes = 0;

        // Inflate dialog layout
        LayoutInflater inflater = LayoutInflater.from(this);
        View dialogView = inflater.inflate(R.layout.dialog_transfer_progress, null);

        // Find views
        mTransferProgressBar = dialogView.findViewById(R.id.transfer_progress_bar);
        mTransferProgressText = dialogView.findViewById(R.id.transfer_progress_text);

        // Set initial state
        mTransferProgressBar.setProgress(0);
        mTransferProgressText.setText(getString(R.string.transfer_progress, 0, 0));

        // Create non-cancellable dialog
        AlertDialog dialog = new AlertDialog.Builder(this)
            .setTitle(title)
            .setView(dialogView)
            .setCancelable(false)  // Prevent user cancelling mid-transfer
            .create();

        dialog.show();
        mProgressDialog = dialog;

        return dialog;
    }

    /**
     * Handle SAF upload result.
     *
     * Gets InputStream from content URI, retrieves file name and size,
     * then executes upload via RemoteFileTransfer.
     *
     * @param requestCode Request code (unused, for consistency)
     * @param resultCode Result code (should be RESULT_OK)
     * @param data Intent containing content URI
     */
    private void handleUploadResult(int requestCode, int resultCode, @Nullable Intent data) {
        if (data == null || data.getData() == null) {
            Logger.logError(LOG_TAG, "Upload result has no data");
            showError(getString(R.string.error_upload_failed));
            return;
        }

        Uri uri = data.getData();
        Logger.logDebug(LOG_TAG, "Upload URI: " + uri.toString());

        // Take persistable URI permission
        try {
            getContentResolver().takePersistableUriPermission(
                uri,
                Intent.FLAG_GRANT_READ_URI_PERMISSION
            );
        } catch (SecurityException e) {
            Logger.logError(LOG_TAG, "Failed to take persistable permission: " + e.getMessage());
            // Continue anyway - permission may already exist
        }

        // Get file info from URI
        String fileNameRaw = getFileNameFromUri(uri);
        long fileSize = getFileSizeFromUri(uri);

        // Use default name if not available
        final String fileName = (fileNameRaw == null || fileNameRaw.isEmpty()) ? "uploaded_file" : fileNameRaw;

        Logger.logDebug(LOG_TAG, "Upload file: " + fileName + " size: " + fileSize);

        // Show progress dialog
        showTransferProgressDialog(getString(R.string.title_uploading));

        // Compute remote destination path
        String basePath = mCurrentPath.endsWith("/") ? mCurrentPath : mCurrentPath + "/";
        final String remotePath = basePath + fileName;

        // Execute upload in background thread
        new Thread(() -> {
            ContentResolver resolver = getContentResolver();
            InputStream inputStream = null;

            try {
                inputStream = resolver.openInputStream(uri);
                if (inputStream == null) {
                    String errorMsg = "Failed to open local file";
                    Logger.logError(LOG_TAG, errorMsg);
                    mMainThreadHandler.post(() -> {
                        closeProgressDialog();
                        showError(getString(R.string.error_upload_failed) + ": " + errorMsg);
                    });
                    return;
                }

                // Execute upload with progress callback (using chunked streaming for large files)
                RemoteFileTransfer.TransferResult result = RemoteFileTransfer.uploadChunked(
                    this,
                    mConnectionInfo,
                    inputStream,
                    fileName,
                    fileSize,
                    remotePath,
                    new RemoteFileTransfer.ProgressCallback() {
                        @Override
                        public void onProgress(long bytesTransferred, long totalBytes) {
                            updateTransferProgress(bytesTransferred, totalBytes);
                        }

                        @Override
                        public void onComplete(@NonNull RemoteFileTransfer.TransferResult result) {
                            // Handled after upload returns
                        }
                    }
                );

                // Close input stream
                try {
                    inputStream.close();
                } catch (Exception e) {
                    Logger.logError(LOG_TAG, "Failed to close input stream: " + e.getMessage());
                }

                // Handle result on main thread
                mMainThreadHandler.post(() -> {
                    closeProgressDialog();

                    if (result.success) {
                        Toast.makeText(this, getString(R.string.success_file_uploaded), Toast.LENGTH_SHORT).show();
                        // Refresh directory to show uploaded file
                        loadDirectory(mCurrentPath);
                    } else {
                        String errorMsg = result.errorMessage != null
                            ? result.errorMessage
                            : getString(R.string.error_upload_failed);
                        showError(getString(R.string.error_upload_failed) + ": " + errorMsg);
                        Logger.logError(LOG_TAG, "Upload failed: " + result);
                    }
                });

            } catch (SecurityException e) {
                Logger.logError(LOG_TAG, "Security exception reading file: " + e.getMessage());
                mMainThreadHandler.post(() -> {
                    closeProgressDialog();
                    showError(getString(R.string.error_upload_failed) + ": Permission denied");
                });
            } catch (Exception e) {
                Logger.logError(LOG_TAG, "Exception during upload: " + e.getMessage());
                mMainThreadHandler.post(() -> {
                    closeProgressDialog();
                    showError(getString(R.string.error_upload_failed) + ": " + e.getMessage());
                });
            }
        }).start();
    }

    /**
     * Handle SAF download result.
     *
     * Gets OutputStream from content URI, then executes download
     * via RemoteFileTransfer for the selected file.
     *
     * @param requestCode Request code (unused, for consistency)
     * @param resultCode Result code (should be RESULT_OK)
     * @param data Intent containing content URI
     */
    private void handleDownloadResult(int requestCode, int resultCode, @Nullable Intent data) {
        if (data == null || data.getData() == null) {
            Logger.logError(LOG_TAG, "Download result has no data");
            showError(getString(R.string.error_download_failed));
            return;
        }

        if (mSelectedFile == null) {
            Logger.logError(LOG_TAG, "No file selected for download");
            showError(getString(R.string.error_download_failed));
            return;
        }

        Uri uri = data.getData();
        Logger.logDebug(LOG_TAG, "Download URI: " + uri.toString());

        // Take persistable URI permission
        try {
            getContentResolver().takePersistableUriPermission(
                uri,
                Intent.FLAG_GRANT_WRITE_URI_PERMISSION
            );
        } catch (SecurityException e) {
            Logger.logError(LOG_TAG, "Failed to take persistable permission: " + e.getMessage());
            // Continue anyway - permission may already exist
        }

        // Show progress dialog
        showTransferProgressDialog(getString(R.string.title_downloading));

        // Execute download in background thread
        new Thread(() -> {
            ContentResolver resolver = getContentResolver();
            OutputStream outputStream = null;

            try {
                outputStream = resolver.openOutputStream(uri);
                if (outputStream == null) {
                    String errorMsg = "Failed to open local file for writing";
                    Logger.logError(LOG_TAG, errorMsg);
                    mMainThreadHandler.post(() -> {
                        closeProgressDialog();
                        showError(getString(R.string.error_download_failed) + ": " + errorMsg);
                    });
                    return;
                }

                // Execute download with progress callback (using chunked streaming for large files)
                RemoteFileTransfer.TransferResult result = RemoteFileTransfer.downloadChunked(
                    this,
                    mConnectionInfo,
                    mSelectedFile.getPath(),
                    outputStream,
                    new RemoteFileTransfer.ProgressCallback() {
                        @Override
                        public void onProgress(long bytesTransferred, long totalBytes) {
                            updateTransferProgress(bytesTransferred, totalBytes);
                        }

                        @Override
                        public void onComplete(@NonNull RemoteFileTransfer.TransferResult result) {
                            // Handled after download returns
                        }
                    }
                );

                // Close output stream
                try {
                    outputStream.close();
                } catch (Exception e) {
                    Logger.logError(LOG_TAG, "Failed to close output stream: " + e.getMessage());
                }

                // Handle result on main thread
                mMainThreadHandler.post(() -> {
                    closeProgressDialog();

                    if (result.success) {
                        Toast.makeText(this, getString(R.string.success_file_downloaded), Toast.LENGTH_SHORT).show();
                    } else {
                        String errorMsg = result.errorMessage != null
                            ? result.errorMessage
                            : getString(R.string.error_download_failed);
                        showError(getString(R.string.error_download_failed) + ": " + errorMsg);
                        Logger.logError(LOG_TAG, "Download failed: " + result);
                    }
                });

            } catch (SecurityException e) {
                Logger.logError(LOG_TAG, "Security exception writing file: " + e.getMessage());
                mMainThreadHandler.post(() -> {
                    closeProgressDialog();
                    showError(getString(R.string.error_download_failed) + ": Permission denied");
                });
            } catch (Exception e) {
                Logger.logError(LOG_TAG, "Exception during download: " + e.getMessage());
                mMainThreadHandler.post(() -> {
                    closeProgressDialog();
                    showError(getString(R.string.error_download_failed) + ": " + e.getMessage());
                });
            }
        }).start();
    }

    /**
     * Update progress dialog UI with throttling.
     *
     * Throttles updates to avoid UI flooding: max one update per
     * PROGRESS_UPDATE_INTERVAL_MS or per PROGRESS_UPDATE_MIN_BYTES change.
     *
     * @param bytesTransferred Bytes transferred so far
     * @param totalBytes Total bytes to transfer
     */
    private void updateTransferProgress(long bytesTransferred, long totalBytes) {
        long now = System.currentTimeMillis();
        long bytesDelta = Math.abs(bytesTransferred - mLastReportedBytes);

        // Throttle: skip update if too soon and not enough byte change
        if ((now - mLastProgressUpdateMs < PROGRESS_UPDATE_INTERVAL_MS) &&
            (bytesDelta < PROGRESS_UPDATE_MIN_BYTES) &&
            (bytesTransferred > 0 && bytesTransferred < totalBytes)) {
            return;
        }

        mLastProgressUpdateMs = now;
        mLastReportedBytes = bytesTransferred;

        mMainThreadHandler.post(() -> {
            if (mTransferProgressBar != null && mTransferProgressText != null) {
                int progressPercent = 0;
                if (totalBytes > 0) {
                    progressPercent = (int) ((bytesTransferred * 100) / totalBytes);
                }
                mTransferProgressBar.setProgress(progressPercent);
                mTransferProgressText.setText(getString(R.string.transfer_progress, bytesTransferred, totalBytes));
            }
        });
    }

    /**
     * Close progress dialog safely.
     */
    private void closeProgressDialog() {
        if (mProgressDialog != null && mProgressDialog.isShowing()) {
            mProgressDialog.dismiss();
        }
        mProgressDialog = null;
        mTransferProgressBar = null;
        mTransferProgressText = null;
    }

    /**
     * Get file name from content URI.
     *
     * Queries ContentResolver for OpenableColumns.DISPLAY_NAME.
     *
     * @param uri Content URI to query
     * @return File name or null if not available
     */
    @Nullable
    private String getFileNameFromUri(@NonNull Uri uri) {
        String fileName = null;

        try {
            Cursor cursor = getContentResolver().query(uri, new String[]{OpenableColumns.DISPLAY_NAME}, null, null, null);
            if (cursor != null) {
                try {
                    if (cursor.moveToFirst()) {
                        int nameIndex = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                        if (nameIndex >= 0) {
                            fileName = cursor.getString(nameIndex);
                        }
                    }
                } finally {
                    cursor.close();
                }
            }
        } catch (Exception e) {
            Logger.logError(LOG_TAG, "Failed to get file name: " + e.getMessage());
        }

        // Fallback: use last path segment
        if (fileName == null || fileName.isEmpty()) {
            fileName = uri.getLastPathSegment();
        }

        return fileName;
    }

    /**
     * Get file size from content URI.
     *
     * Queries ContentResolver for OpenableColumns.SIZE.
     *
     * @param uri Content URI to query
     * @return File size in bytes, or 0 if not available
     */
    private long getFileSizeFromUri(@NonNull Uri uri) {
        long fileSize = 0;

        try {
            Cursor cursor = getContentResolver().query(uri, new String[]{OpenableColumns.SIZE}, null, null, null);
            if (cursor != null) {
                try {
                    if (cursor.moveToFirst()) {
                        int sizeIndex = cursor.getColumnIndex(OpenableColumns.SIZE);
                        if (sizeIndex >= 0) {
                            fileSize = cursor.getLong(sizeIndex);
                        }
                    }
                } finally {
                    cursor.close();
                }
            }
        } catch (Exception e) {
            Logger.logError(LOG_TAG, "Failed to get file size: " + e.getMessage());
        }

        return fileSize;
    }

    /**
     * Load directory contents asynchronously.
     *
     * Shows loading indicator while fetching, updates list on success,
     * shows error toast on failure.
     *
     * @param path Remote directory path to load
     */
    private void loadDirectory(@NonNull String path) {
        if (mIsLoading) {
            Logger.logDebug(LOG_TAG, "Already loading, skipping duplicate request");
            return;
        }

        if (!mIsActive) {
            Logger.logDebug(LOG_TAG, "Activity not active, skipping load");
            return;
        }

        mIsLoading = true;
        mCurrentPath = path;

        Logger.logDebug(LOG_TAG, "Loading directory: " + path);

        // Show loading state
        showLoading(true);

        // Run listing in background thread
        new Thread(() -> {
            try {
                List<RemoteFile> files = RemoteFileLister.listDirectory(
                    this, mConnectionInfo, path);

                // Update UI on main thread
                mMainThreadHandler.post(() -> {
                    if (!mIsActive) {
                        Logger.logDebug(LOG_TAG, "Activity no longer active, discarding result");
                        return;
                    }

                    mIsLoading = false;
                    showLoading(false);

                    if (files.isEmpty()) {
                        // Empty directory - show empty view
                        mAdapter.updateFiles(files);
                        mEmptyView.setText(getString(R.string.empty_directory));
                        mEmptyView.setVisibility(View.VISIBLE);
                        mFileListView.setVisibility(View.GONE);
                        Logger.logDebug(LOG_TAG, "Directory is empty: " + path);
                    } else {
                        // Update adapter with new files
                        mAdapter.updateFiles(files);
                        mEmptyView.setVisibility(View.GONE);
                        mFileListView.setVisibility(View.VISIBLE);
                        Logger.logDebug(LOG_TAG, "Loaded " + files.size() + " files from " + path);
                    }

                    // Update breadcrumb navigation
                    updateBreadcrumb(path);
                });

            } catch (Exception e) {
                Logger.logError(LOG_TAG, "Exception loading directory: " + e.getMessage());

                mMainThreadHandler.post(() -> {
                    if (!mIsActive) return;

                    mIsLoading = false;
                    showLoading(false);
                    showError(getString(R.string.error_listing_directory) + ": " + e.getMessage());

                    // Show empty view with error
                    mAdapter.updateFiles(new ArrayList<>());
                    mEmptyView.setText(getString(R.string.error_listing_directory));
                    mEmptyView.setVisibility(View.VISIBLE);
                    mFileListView.setVisibility(View.GONE);
                });
            }
        }).start();
    }

    /**
     * Show or hide loading indicator.
     *
     * @param isLoading true to show loading, false to hide
     */
    private void showLoading(boolean isLoading) {
        if (mLoadingIndicator != null) {
            mLoadingIndicator.setVisibility(isLoading ? View.VISIBLE : View.GONE);
        }
        if (mContentContainer != null && isLoading) {
            // Keep content visible but dimmed/semi-transparent during loading
            // Or hide completely if loading indicator replaces content
            // For now, just show loading overlay
        }
        // Disable interactions during loading
        mFileListView.setEnabled(!isLoading);
        mBackButton.setEnabled(!isLoading && !mPathStack.isEmpty());
        mRefreshButton.setEnabled(!isLoading);
        mNewFolderButton.setEnabled(!isLoading);
        mUploadButton.setEnabled(!isLoading);
    }

    /**
     * Show error message as Toast.
     *
     * @param message Error message to display
     */
    private void showError(@NonNull String message) {
        Toast.makeText(this, message, Toast.LENGTH_LONG).show();
    }

    /**
     * Handle file item click.
     *
     * If clicked item is a directory or symlink to a directory, navigate into it.
     * If it's a file or symlink, show info (future: download/open).
     *
     * @param file Clicked RemoteFile item
     */
    private void onFileItemClick(@NonNull RemoteFile file) {
        Logger.logDebug(LOG_TAG, "File clicked: " + file.getName() + " (type: " + file.getType() +
                       ", isDirectory: " + file.isDirectory() +
                       ", symlinkTargetIsDirectory: " + file.isSymlinkTargetDirectory() + ")");

        if (file.isDirectoryOrSymlinkToDirectory()) {
            // Navigate into subdirectory (including symlink-to-directory)
            String newPath = file.getPath();
            mPathStack.push(mCurrentPath);
            loadDirectory(newPath);
        } else {
            // Check if file is editable code file
            if (CodeMirrorMode.isEditableFile(file.getName())) {
                // Launch code editor
                Intent intent = new Intent(this, RemoteCodeEditorActivity.class);
                intent.putExtra(RemoteCodeEditorActivity.EXTRA_CONNECTION_INFO, mConnectionInfo);
                intent.putExtra(RemoteCodeEditorActivity.EXTRA_FILE_PATH, file.getPath());
                startActivity(intent);
            } else if (ImageFileType.isImageFile(file.getName())) {
                // Launch image preview
                Intent intent = new Intent(this, RemoteImagePreviewActivity.class);
                intent.putExtra(RemoteImagePreviewActivity.EXTRA_CONNECTION_INFO, mConnectionInfo);
                intent.putExtra(RemoteImagePreviewActivity.EXTRA_FILE_PATH, file.getPath());
                startActivity(intent);
            } else {
                // Non-code/non-image file: show info toast
                String info = file.getName() + " (" + file.getSizeFormatted() + ")";
                Toast.makeText(this, info, Toast.LENGTH_SHORT).show();
            }
        }
    }

    /**
     * Navigate to parent directory.
     *
     * If path stack is empty, shows root or stays at current level.
     */
    private void navigateToParent() {
        if (mPathStack.isEmpty()) {
            // Already at root level, can't go back further
            Logger.logDebug(LOG_TAG, "Path stack empty, already at root level");
            Toast.makeText(this, "Already at root directory", Toast.LENGTH_SHORT).show();
            return;
        }

        String parentPath = mPathStack.pop();
        Logger.logDebug(LOG_TAG, "Navigating to parent: " + parentPath);
        loadDirectory(parentPath);
    }

    /**
     * Update breadcrumb navigation display.
     *
     * Creates clickable path segments for each directory level.
     *
     * @param path Current full path
     */
    private void updateBreadcrumb(@NonNull String path) {
        mBreadcrumbPathLayout.removeAllViews();

        // Split path into segments
        String normalizedPath = path;
        if (normalizedPath.startsWith("/") && normalizedPath.length() > 1) {
            normalizedPath = normalizedPath.substring(1);
        }
        if (normalizedPath.endsWith("/") && normalizedPath.length() > 1) {
            normalizedPath = normalizedPath.substring(0, normalizedPath.length() - 1);
        }

        String[] segments = normalizedPath.split("/");
        if (segments.length == 0 || (segments.length == 1 && segments[0].isEmpty())) {
            // Root directory
            addBreadcrumbSegment("/", "/", true);
            return;
        }

        // Build breadcrumb: root -> ... -> current
        StringBuilder accumulatedPath = new StringBuilder();

        // Add root segment
        addBreadcrumbSegment("/", "/", segments.length == 0);
        accumulatedPath.append("/");

        // Add each path segment
        for (int i = 0; i < segments.length; i++) {
            if (!segments[i].isEmpty()) {
                accumulatedPath.append(segments[i]);
                boolean isLast = (i == segments.length - 1);
                addBreadcrumbSegment(segments[i], accumulatedPath.toString(), isLast);

                if (!isLast) {
                    accumulatedPath.append("/");
                }
            }
        }
    }

    /**
     * Add a breadcrumb segment TextView to the navigation.
     *
     * @param text Display text for segment
     * @param path Full path this segment represents
     * @param isCurrent true if this is the current (last) segment
     */
    private void addBreadcrumbSegment(@NonNull String text, @NonNull String path, boolean isCurrent) {
        TextView segmentView = new TextView(this);
        segmentView.setText(isCurrent ? text : text + "/");
        segmentView.setTextSize(14);

        if (isCurrent) {
            // Current segment: bold style
            segmentView.setTextColor(getColor(com.termux.shared.R.color.white));
            segmentView.setTypeface(segmentView.getTypeface(), android.graphics.Typeface.BOLD);
        } else {
            // Clickable segment: link style
            segmentView.setTextColor(getColor(com.termux.shared.R.color.blue_link_dark));
            segmentView.setClickable(true);
            segmentView.setOnClickListener(v -> {
                Logger.logDebug(LOG_TAG, "Breadcrumb clicked: " + path);

                // Calculate how many levels to pop from stack
                // Pop until we reach the clicked path
                while (!mPathStack.isEmpty() && !mPathStack.peek().equals(path)) {
                    mPathStack.pop();
                }

                // Add current path to stack before navigating
                if (!mCurrentPath.equals(path)) {
                    mPathStack.push(mCurrentPath);
                }

                loadDirectory(path);
            });
        }

        segmentView.setPadding(8, 8, 8, 8);
        mBreadcrumbPathLayout.addView(segmentView);
    }

    @Override
    public void onCreateContextMenu(ContextMenu menu, View v, ContextMenu.ContextMenuInfo menuInfo) {
        super.onCreateContextMenu(menu, v, menuInfo);

        // Get the selected file from the adapter
        android.widget.AdapterView.AdapterContextMenuInfo info =
            (android.widget.AdapterView.AdapterContextMenuInfo) menuInfo;
        int position = info.position;

        mSelectedFile = mAdapter.getItem(position);
        if (mSelectedFile == null) {
            Logger.logError(LOG_TAG, "Selected file is null at position " + position);
            return;
        }

        // Set context menu header with file name
        menu.setHeaderTitle(mSelectedFile.getName());

        // Inflate the context menu
        getMenuInflater().inflate(R.menu.context_menu_remote_file, menu);

        // Hide download menu for directories and symlinks to directories (only files can be downloaded)
        MenuItem downloadItem = menu.findItem(R.id.menu_download);
        if (downloadItem != null) {
            downloadItem.setVisible(!mSelectedFile.isDirectoryOrSymlinkToDirectory());
        }

        Logger.logDebug(LOG_TAG, "Context menu created for: " + mSelectedFile.getName());
    }

    @Override
    public boolean onContextItemSelected(@NonNull MenuItem item) {
        if (mSelectedFile == null) {
            Logger.logError(LOG_TAG, "No file selected for context menu action");
            return super.onContextItemSelected(item);
        }

        int itemId = item.getItemId();
        Logger.logDebug(LOG_TAG, "Context menu item selected: " + itemId + " for file: " + mSelectedFile.getName());

        if (itemId == R.id.menu_download) {
            // Only download files, not directories
            if (!mSelectedFile.isDirectory()) {
                startDownloadFileSaver(mSelectedFile.getName());
            }
            return true;
        } else if (itemId == R.id.menu_rename) {
            showRenameDialog(mSelectedFile);
            return true;
        } else if (itemId == R.id.menu_delete) {
            showDeleteConfirmationDialog(mSelectedFile);
            return true;
        } else if (itemId == R.id.menu_new_folder) {
            showNewFolderDialog();
            return true;
        }
        // Copy and Move will be implemented in later slices

        return super.onContextItemSelected(item);
    }

    /**
     * Show delete confirmation dialog for a file or directory.
     *
     * Distinguishes three cases:
     * - Regular directory: warn about deleting all contents
     * - Symlink directory: warn only link is deleted, target unaffected
     * - File or symlink file: simple delete confirmation
     *
     * @param file File to delete
     */
    private void showDeleteConfirmationDialog(@NonNull RemoteFile file) {
        String title = getString(R.string.title_confirm_delete);
        String message;

        if (file.isDirectory()) {
            // Regular directory - warn about deleting all contents
            message = getString(R.string.message_confirm_delete_directory, file.getName());
        } else if (file.isSymlink() && file.isSymlinkTargetDirectory()) {
            // Symlink to directory - special message: only delete link, target unaffected
            message = getString(R.string.message_confirm_delete_symlink_directory, file.getName());
        } else {
            // Regular file or symlink to file - simple delete confirmation
            message = getString(R.string.message_confirm_delete_file, file.getName());
        }

        MessageDialogUtils.showMessage(
            this,
            title,
            message,
            getString(android.R.string.yes),
            (dialog, which) -> executeDelete(file),
            getString(android.R.string.no),
            null,
            null
        );
    }

    /**
     * Execute delete operation on a file or directory.
     *
     * @param file File to delete
     */
    private void executeDelete(@NonNull RemoteFile file) {
        Logger.logDebug(LOG_TAG, "Executing delete for: " + file.getPath());

        new Thread(() -> {
            RemoteFileOperator.OperationResult result = RemoteFileOperator.delete(
                this,
                mConnectionInfo,
                file.getPath(),
                file.isDirectory(),  // recursive if directory
                true                  // force
            );

            mMainThreadHandler.post(() -> {
                if (result.success) {
                    String message = file.isDirectory()
                        ? getString(R.string.success_directory_deleted)
                        : getString(R.string.success_file_deleted);
                    Toast.makeText(this, message, Toast.LENGTH_SHORT).show();
                    // Refresh the directory listing
                    loadDirectory(mCurrentPath);
                } else {
                    String error = getString(R.string.error_delete_failed);
                    if (result.errorMessage != null) {
                        error = error + ": " + result.errorMessage;
                    }
                    Toast.makeText(this, error, Toast.LENGTH_LONG).show();
                    Logger.logError(LOG_TAG, "Delete failed: " + result);
                }
            });
        }).start();
    }

    /**
     * Show rename dialog for a file or directory.
     *
     * @param file File to rename
     */
    private void showRenameDialog(@NonNull RemoteFile file) {
        // Use same title for both files and directories - TextInputDialogUtils requires resource ID
        TextInputDialogUtils.textInput(
            this,
            R.string.action_rename,
            file.getName(),
            android.R.string.ok,
            newText -> executeRename(file, newText),
            0,  // neutralButtonText (not used)
            null,
            android.R.string.cancel,
            null,
            null
        );
    }

    /**
     * Execute rename operation on a file or directory.
     *
     * @param file File to rename
     * @param newName New name for the file
     */
    private void executeRename(@NonNull RemoteFile file, @NonNull String newName) {
        if (newName == null || newName.trim().isEmpty()) {
            Toast.makeText(this, "Name cannot be empty", Toast.LENGTH_SHORT).show();
            return;
        }

        if (newName.equals(file.getName())) {
            Toast.makeText(this, "Name unchanged", Toast.LENGTH_SHORT).show();
            return;
        }

        Logger.logDebug(LOG_TAG, "Executing rename for: " + file.getPath() + " to: " + newName);

        new Thread(() -> {
            RemoteFileOperator.OperationResult result = RemoteFileOperator.rename(
                this,
                mConnectionInfo,
                file.getPath(),
                newName.trim()
            );

            mMainThreadHandler.post(() -> {
                if (result.success) {
                    String message = file.isDirectory()
                        ? getString(R.string.success_directory_renamed)
                        : getString(R.string.success_file_renamed);
                    Toast.makeText(this, message, Toast.LENGTH_SHORT).show();
                    // Refresh the directory listing
                    loadDirectory(mCurrentPath);
                } else {
                    String error = getString(R.string.error_rename_failed);
                    if (result.errorMessage != null) {
                        error = error + ": " + result.errorMessage;
                    }
                    Toast.makeText(this, error, Toast.LENGTH_LONG).show();
                    Logger.logError(LOG_TAG, "Rename failed: " + result);
                }
            });
        }).start();
    }

    /**
     * Show new folder dialog to create a directory in current path.
     */
    private void showNewFolderDialog() {
        TextInputDialogUtils.textInput(
            this,
            R.string.title_new_folder,
            "",
            android.R.string.ok,
            folderName -> executeMkdir(folderName),
            0,  // neutralButtonText (not used)
            null,
            android.R.string.cancel,
            null,
            null
        );
    }

    /**
     * Execute mkdir operation to create a new folder.
     *
     * @param folderName Name of the folder to create
     */
    private void executeMkdir(@NonNull String folderName) {
        if (folderName == null || folderName.trim().isEmpty()) {
            Toast.makeText(this, "Folder name cannot be empty", Toast.LENGTH_SHORT).show();
            return;
        }

        // Compute full path - must be final for lambda
        String basePath = mCurrentPath.endsWith("/") ? mCurrentPath : mCurrentPath + "/";
        final String newPath = basePath + folderName.trim();

        Logger.logDebug(LOG_TAG, "Executing mkdir at: " + newPath);

        new Thread(() -> {
            RemoteFileOperator.OperationResult result = RemoteFileOperator.mkdir(
                this,
                mConnectionInfo,
                newPath,
                false  // createParents - just create single directory
            );

            mMainThreadHandler.post(() -> {
                if (result.success) {
                    Toast.makeText(this, getString(R.string.success_folder_created), Toast.LENGTH_SHORT).show();
                    // Refresh the directory listing
                    loadDirectory(mCurrentPath);
                } else {
                    String error = getString(R.string.error_mkdir_failed);
                    if (result.errorMessage != null) {
                        error = error + ": " + result.errorMessage;
                    }
                    Toast.makeText(this, error, Toast.LENGTH_LONG).show();
                    Logger.logError(LOG_TAG, "Mkdir failed: " + result);
                }
            });
        }).start();
    }
}