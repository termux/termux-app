package com.termux.app.activities;

import android.graphics.Bitmap;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.View;
import android.widget.ProgressBar;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import com.github.chrisbanes.photoview.PhotoView;
import com.termux.R;
import com.termux.app.ssh.ImageFileType;
import com.termux.app.ssh.RemoteImageLoader;
import com.termux.app.ssh.SSHConnectionInfo;
import com.termux.shared.logger.Logger;
import com.termux.shared.theme.NightMode;
import com.termux.shared.activity.media.AppCompatActivityUtils;

import java.io.Serializable;

/**
 * Activity for previewing remote image files via SSH ControlMaster connection.
 *
 * Displays an image in a PhotoView component with:
 * - Pinch-to-zoom gesture support
 * - Pan/drag gesture support
 * - Double-tap quick zoom
 *
 * Requires SSHConnectionInfo and file path passed via Intent extras.
 * Image data is retrieved via RemoteImageLoader which executes base64
 * command through SSH ControlMaster and decodes the result.
 *
 * Handles lifecycle safely with mIsActive flag to prevent callbacks
 * after activity is destroyed.
 */
public class RemoteImagePreviewActivity extends AppCompatActivity {

    private static final String LOG_TAG = "RemoteImagePreviewActivity";

    /** Intent extra key for SSH connection info */
    public static final String EXTRA_CONNECTION_INFO = "connection_info";

    /** Intent extra key for remote file path */
    public static final String EXTRA_FILE_PATH = "file_path";

    /** Intent extra key for file name (optional, derived from path if missing) */
    public static final String EXTRA_FILE_NAME = "file_name";

    /** Current SSH connection info */
    private SSHConnectionInfo mConnectionInfo;

    /** Current remote file path */
    private String mFilePath;

    /** Current file name (displayed in title) */
    private String mFileName;

    /** PhotoView for zoomable/pannable image display */
    private PhotoView mImageView;

    /** Loading indicator */
    private ProgressBar mLoadingIndicator;

    /** Error view (shown on load failure) */
    private TextView mErrorView;

    /** Handler for UI updates from background threads */
    private final Handler mMainThreadHandler = new Handler(Looper.getMainLooper());

    /** Flag indicating if activity is active */
    private boolean mIsActive = false;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Apply night mode theme
        AppCompatActivityUtils.setNightMode(this, NightMode.getAppNightMode().getName(), true);

        setContentView(R.layout.activity_remote_image_preview);

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

        // Mark activity as active
        mIsActive = true;

        // Load image from remote server
        loadImage();

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
        Logger.logDebug(LOG_TAG, "Activity destroyed");
    }

    @Override
    public boolean onSupportNavigateUp() {
        onBackPressed();
        return true;
    }

    /**
     * Initialize view references from layout.
     */
    private void initializeViews() {
        mImageView = findViewById(R.id.image_view);
        mLoadingIndicator = findViewById(R.id.loading_indicator);
        mErrorView = findViewById(R.id.error_view);
    }

    /**
     * Parse SSH connection info and file path from Intent extras.
     *
     * @return true if parsing succeeded, false if required extras missing
     */
    private boolean parseIntentExtras() {
        android.content.Intent intent = getIntent();
        if (intent == null) {
            Logger.logError(LOG_TAG, "No intent provided");
            showError(getString(R.string.error_image_load_failed));
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
                showError(getString(R.string.error_image_load_failed));
                return false;
            }
        }

        // Get file path
        mFilePath = intent.getStringExtra(EXTRA_FILE_PATH);
        if (mFilePath == null || mFilePath.isEmpty()) {
            Logger.logError(LOG_TAG, "File path not provided in intent");
            showError(getString(R.string.error_image_load_failed));
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

        // Validate file is an image
        if (!ImageFileType.isImageFile(mFileName)) {
            Logger.logError(LOG_TAG, "File is not a supported image format: " + mFileName);
            showError(getString(R.string.error_image_not_supported));
            return false;
        }

        Logger.logDebug(LOG_TAG, "Parsed intent: " + mConnectionInfo.toString() +
                       ", path: " + mFilePath);
        return true;
    }

    /**
     * Load image from remote server asynchronously.
     */
    private void loadImage() {
        Logger.logDebug(LOG_TAG, "Loading image: " + mFilePath);

        showLoading(true);

        new Thread(() -> {
            RemoteImageLoader.ImageLoadResult result = RemoteImageLoader.loadImage(
                this,
                mConnectionInfo,
                mFilePath
            );

            mMainThreadHandler.post(() -> {
                if (!mIsActive) {
                    Logger.logDebug(LOG_TAG, "Activity no longer active, discarding load result");
                    return;
                }

                handleLoadResult(result);
            });
        }).start();
    }

    /**
     * Handle image load result.
     *
     * @param result Result from RemoteImageLoader
     */
    private void handleLoadResult(@NonNull RemoteImageLoader.ImageLoadResult result) {
        Logger.logDebug(LOG_TAG, "Image load result: " + result);

        if (result.success) {
            Bitmap bitmap = result.bitmap;
            if (bitmap != null) {
                // Display bitmap in PhotoView
                mImageView.setImageBitmap(bitmap);
                showLoading(false);

                // Show warning if downsampling was applied
                if (result.warning != null && !result.warning.isEmpty()) {
                    android.widget.Toast.makeText(this, result.warning,
                        android.widget.Toast.LENGTH_SHORT).show();
                }

                Logger.logDebug(LOG_TAG, "Image displayed: " + result.width + "x" + result.height +
                               " (" + result.fileSize + " bytes)");
            } else {
                showError(getString(R.string.error_image_load_failed));
            }
        } else {
            // Show error
            String errorMsg = result.errorMessage;
            if (errorMsg == null || errorMsg.isEmpty()) {
                errorMsg = getString(R.string.error_image_load_failed);
            }

            // Add dimension info if too large
            if (result.width > 0 && result.height > 0) {
                errorMsg += " (" + result.width + "x" + result.height + ")";
            }

            showError(errorMsg);
            Logger.logError(LOG_TAG, "Image load failed: " + errorMsg);
        }
    }

    /**
     * Show or hide loading indicator.
     *
     * @param isLoading true to show loading, false to hide
     */
    private void showLoading(boolean isLoading) {
        mLoadingIndicator.setVisibility(isLoading ? View.VISIBLE : View.GONE);
        mImageView.setVisibility(isLoading ? View.GONE : View.VISIBLE);
        mErrorView.setVisibility(View.GONE);
    }

    /**
     * Show error message in error view.
     *
     * @param message Error message to display
     */
    private void showError(@NonNull String message) {
        mLoadingIndicator.setVisibility(View.GONE);
        mImageView.setVisibility(View.GONE);
        mErrorView.setText(message);
        mErrorView.setVisibility(View.VISIBLE);
        Logger.logError(LOG_TAG, "Error shown: " + message);
    }
}