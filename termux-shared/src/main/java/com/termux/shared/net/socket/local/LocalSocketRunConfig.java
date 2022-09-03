package com.termux.shared.net.socket.local;

import androidx.annotation.NonNull;

import com.termux.shared.file.FileUtils;
import com.termux.shared.logger.Logger;
import com.termux.shared.markdown.MarkdownUtils;

import java.io.Serializable;
import java.nio.charset.StandardCharsets;


/**
 * Run config for {@link LocalSocketManager}.
 */
public class LocalSocketRunConfig implements Serializable {

    /** The {@link LocalSocketManager} title. */
    protected final String mTitle;

    /**
     * The {@link LocalServerSocket} path.
     *
     * For a filesystem socket, this must be an absolute path to the socket file. Creation of a new
     * socket will fail if the server starter app process does not have write and search (execute)
     * permission on the directory in which the socket is created. The client process must have write
     * permission on the socket to connect to it. Other app will not be able to connect to socket
     * if its created in private app data directory.
     *
     * For an abstract namespace socket, the first byte must be a null `\0` character. Note that on
     * Android 9+, if server app is using `targetSdkVersion` `28`, then other apps will not be able
     * to connect to it due to selinux restrictions.
     * > Per-app SELinux domains
     * > Apps that target Android 9 or higher cannot share data with other apps using world-accessible
     * Unix permissions. This change improves the integrity of the Android Application Sandbox,
     * particularly the requirement that an app's private data is accessible only by that app.
     * https://developer.android.com/about/versions/pie/android-9.0-changes-28
     * https://github.com/android/ndk/issues/1469
     * https://stackoverflow.com/questions/63806516/avc-denied-connectto-when-using-uds-on-android-10
     *
     * Max allowed length is 108 bytes as per sun_path size (UNIX_PATH_MAX) on Linux.
     */
    protected final String mPath;

    /** If abstract namespace {@link LocalServerSocket} instead of filesystem. */
    protected final boolean mAbstractNamespaceSocket;

    /** The {@link ILocalSocketManager} client for the {@link LocalSocketManager}. */
    protected final ILocalSocketManager mLocalSocketManagerClient;

    /**
     * The {@link LocalServerSocket} file descriptor.
     * Value will be `>= 0` if socket has been created successfully and `-1` if not created or closed.
     */
    protected int mFD = -1;

    /**
     * The {@link LocalClientSocket} receiving (SO_RCVTIMEO) timeout in milliseconds.
     *
     * https://manpages.debian.org/testing/manpages/socket.7.en.html
     * https://cs.android.com/android/platform/superproject/+/android-12.0.0_r32:frameworks/base/services/core/java/com/android/server/am/NativeCrashListener.java;l=55
     * Defaults to {@link #DEFAULT_RECEIVE_TIMEOUT}.
     */
    protected Integer mReceiveTimeout;
    public static final int DEFAULT_RECEIVE_TIMEOUT = 10000;

    /**
     * The {@link LocalClientSocket} sending (SO_SNDTIMEO) timeout in milliseconds.
     *
     * https://manpages.debian.org/testing/manpages/socket.7.en.html
     * https://cs.android.com/android/platform/superproject/+/android-12.0.0_r32:frameworks/base/services/core/java/com/android/server/am/NativeCrashListener.java;l=55
     * Defaults to {@link #DEFAULT_SEND_TIMEOUT}.
     */
    protected Integer mSendTimeout;
    public static final int DEFAULT_SEND_TIMEOUT = 10000;

    /**
     * The {@link LocalClientSocket} deadline in milliseconds. When the deadline has elapsed after
     * creation time of client socket, all reads and writes will error out. Set to 0, for no
     * deadline.
     * Defaults to {@link #DEFAULT_DEADLINE}.
     */
    protected Long mDeadline;
    public static final int DEFAULT_DEADLINE = 0;

    /**
     * The {@link LocalServerSocket} backlog for the maximum length to which the queue of pending connections
     * for the socket may grow. This value may be ignored or may not have one-to-one mapping
     * in kernel implementation. Value must be greater than 0.
     *
     * https://cs.android.com/android/platform/superproject/+/android-12.0.0_r32:frameworks/base/core/java/android/net/LocalSocketManager.java;l=31
     * Defaults to {@link #DEFAULT_BACKLOG}.
     */
    protected Integer mBacklog;
    public static final int DEFAULT_BACKLOG = 50;


    /**
     * Create an new instance of {@link LocalSocketRunConfig}.
     *
     * @param title The {@link #mTitle} value.
     * @param path The {@link #mPath} value.
     * @param localSocketManagerClient The {@link #mLocalSocketManagerClient} value.
     */
    public LocalSocketRunConfig(@NonNull String title, @NonNull String path, @NonNull ILocalSocketManager localSocketManagerClient) {
        mTitle = title;
        mLocalSocketManagerClient = localSocketManagerClient;
        mAbstractNamespaceSocket = path.getBytes(StandardCharsets.UTF_8)[0] == 0;

        if (mAbstractNamespaceSocket)
            mPath = path;
        else
            mPath = FileUtils.getCanonicalPath(path, null);
    }



    /** Get {@link #mTitle}. */
    public String getTitle() {
        return mTitle;
    }

    /** Get log title that should be used for {@link LocalSocketManager}. */
    public String getLogTitle() {
        return Logger.getDefaultLogTag() + "." + mTitle;
    }

    /** Get {@link #mPath}. */
    public String getPath() {
        return mPath;
    }

    /** Get {@link #mAbstractNamespaceSocket}. */
    public boolean isAbstractNamespaceSocket() {
        return mAbstractNamespaceSocket;
    }

    /** Get {@link #mLocalSocketManagerClient}. */
    public ILocalSocketManager getLocalSocketManagerClient() {
        return mLocalSocketManagerClient;
    }

    /** Get {@link #mFD}. */
    public Integer getFD() {
        return mFD;
    }

    /** Set {@link #mFD}. Value must be greater than 0 or -1. */
    public void setFD(int fd) {
        if (fd >= 0)
            mFD = fd;
        else
            mFD = -1;
    }

    /** Get {@link #mReceiveTimeout} if set, otherwise {@link #DEFAULT_RECEIVE_TIMEOUT}. */
    public Integer getReceiveTimeout() {
        return mReceiveTimeout != null ? mReceiveTimeout : DEFAULT_RECEIVE_TIMEOUT;
    }

    /** Set {@link #mReceiveTimeout}. */
    public void setReceiveTimeout(Integer receiveTimeout) {
        mReceiveTimeout = receiveTimeout;
    }

    /** Get {@link #mSendTimeout} if set, otherwise {@link #DEFAULT_SEND_TIMEOUT}. */
    public Integer getSendTimeout() {
        return mSendTimeout != null ? mSendTimeout : DEFAULT_SEND_TIMEOUT;
    }

    /** Set {@link #mSendTimeout}. */
    public void setSendTimeout(Integer sendTimeout) {
        mSendTimeout = sendTimeout;
    }

    /** Get {@link #mDeadline} if set, otherwise {@link #DEFAULT_DEADLINE}. */
    public Long getDeadline() {
        return mDeadline != null ? mDeadline : DEFAULT_DEADLINE;
    }

    /** Set {@link #mDeadline}. */
    public void setDeadline(Long deadline) {
        mDeadline = deadline;
    }

    /** Get {@link #mBacklog} if set, otherwise {@link #DEFAULT_BACKLOG}. */
    public Integer getBacklog() {
        return mBacklog != null ? mBacklog : DEFAULT_BACKLOG;
    }

    /** Set {@link #mBacklog}. Value must be greater than 0. */
    public void setBacklog(Integer backlog) {
        if (backlog > 0)
            mBacklog = backlog;
    }


    /**
     * Get a log {@link String} for {@link LocalSocketRunConfig}.
     *
     * @param config The {@link LocalSocketRunConfig} to get info of.
     * @return Returns the log {@link String}.
     */
    @NonNull
    public static String getRunConfigLogString(final LocalSocketRunConfig config) {
        if (config == null) return "null";
        return config.getLogString();
    }

    /** Get a log {@link String} for the {@link LocalSocketRunConfig}. */
    @NonNull
    public String getLogString() {
        StringBuilder logString = new StringBuilder();

        logString.append(mTitle).append(" Socket Server Run Config:");
        logString.append("\n").append(Logger.getSingleLineLogStringEntry("Path", mPath, "-"));
        logString.append("\n").append(Logger.getSingleLineLogStringEntry("AbstractNamespaceSocket", mAbstractNamespaceSocket, "-"));
        logString.append("\n").append(Logger.getSingleLineLogStringEntry("LocalSocketManagerClient", mLocalSocketManagerClient.getClass().getName(), "-"));
        logString.append("\n").append(Logger.getSingleLineLogStringEntry("FD", mFD, "-"));
        logString.append("\n").append(Logger.getSingleLineLogStringEntry("ReceiveTimeout", getReceiveTimeout(), "-"));
        logString.append("\n").append(Logger.getSingleLineLogStringEntry("SendTimeout", getSendTimeout(), "-"));
        logString.append("\n").append(Logger.getSingleLineLogStringEntry("Deadline", getDeadline(), "-"));
        logString.append("\n").append(Logger.getSingleLineLogStringEntry("Backlog", getBacklog(), "-"));

        return logString.toString();
    }

    /**
     * Get a markdown {@link String} for {@link LocalSocketRunConfig}.
     *
     * @param config The {@link LocalSocketRunConfig} to get info of.
     * @return Returns the markdown {@link String}.
     */
    public static String getRunConfigMarkdownString(final LocalSocketRunConfig config) {
        if (config == null) return "null";
        return config.getMarkdownString();
    }

    /** Get a markdown {@link String} for the {@link LocalSocketRunConfig}. */
    @NonNull
    public String getMarkdownString() {
        StringBuilder markdownString = new StringBuilder();

        markdownString.append("## ").append(mTitle).append(" Socket Server Run Config");
        markdownString.append("\n").append(MarkdownUtils.getSingleLineMarkdownStringEntry("Path", mPath, "-"));
        markdownString.append("\n").append(MarkdownUtils.getSingleLineMarkdownStringEntry("AbstractNamespaceSocket", mAbstractNamespaceSocket, "-"));
        markdownString.append("\n").append(MarkdownUtils.getSingleLineMarkdownStringEntry("LocalSocketManagerClient", mLocalSocketManagerClient.getClass().getName(), "-"));
        markdownString.append("\n").append(MarkdownUtils.getSingleLineMarkdownStringEntry("FD", mFD, "-"));
        markdownString.append("\n").append(MarkdownUtils.getSingleLineMarkdownStringEntry("ReceiveTimeout", getReceiveTimeout(), "-"));
        markdownString.append("\n").append(MarkdownUtils.getSingleLineMarkdownStringEntry("SendTimeout", getSendTimeout(), "-"));
        markdownString.append("\n").append(MarkdownUtils.getSingleLineMarkdownStringEntry("Deadline", getDeadline(), "-"));
        markdownString.append("\n").append(MarkdownUtils.getSingleLineMarkdownStringEntry("Backlog", getBacklog(), "-"));

        return markdownString.toString();
    }



    @NonNull
    @Override
    public String toString() {
        return getLogString();
    }

}
