package com.termux.shared.net.socket.local;

import androidx.annotation.NonNull;

import com.termux.shared.data.DataUtils;
import com.termux.shared.errors.Error;
import com.termux.shared.jni.models.JniResult;
import com.termux.shared.logger.Logger;
import com.termux.shared.markdown.MarkdownUtils;

import java.io.BufferedWriter;
import java.io.Closeable;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.OutputStreamWriter;

/** The client socket for {@link LocalSocketManager}. */
public class LocalClientSocket implements Closeable {

    public static final String LOG_TAG = "LocalClientSocket";

    /** The {@link LocalSocketManager} instance for the local socket. */
    @NonNull protected final LocalSocketManager mLocalSocketManager;

    /** The {@link LocalSocketRunConfig} containing run config for the {@link LocalClientSocket}. */
    @NonNull protected final LocalSocketRunConfig mLocalSocketRunConfig;

    /**
     * The {@link LocalClientSocket} file descriptor.
     * Value will be `>= 0` if socket has been connected and `-1` if closed.
     */
    protected int mFD;

    /** The creation time of {@link LocalClientSocket}. This is also used for deadline. */
    protected final long mCreationTime;

    /** The {@link PeerCred} of the {@link LocalClientSocket} containing info of client/peer. */
    @NonNull protected final PeerCred mPeerCred;

    /** The {@link OutputStream} implementation for the {@link LocalClientSocket}. */
    @NonNull protected final SocketOutputStream mOutputStream;

    /** The {@link InputStream} implementation for the {@link LocalClientSocket}. */
    @NonNull protected final SocketInputStream mInputStream;

    /**
     * Create an new instance of {@link LocalClientSocket}.
     *
     * @param localSocketManager The {@link #mLocalSocketManager} value.
     * @param fd The {@link #mFD} value.
     * @param peerCred The {@link #mPeerCred} value.
     */
    LocalClientSocket(@NonNull LocalSocketManager localSocketManager, int fd, @NonNull PeerCred peerCred) {
        mLocalSocketManager = localSocketManager;
        mLocalSocketRunConfig = localSocketManager.getLocalSocketRunConfig();
        mCreationTime = System.currentTimeMillis();
        mOutputStream = new SocketOutputStream();
        mInputStream = new SocketInputStream();
        mPeerCred = peerCred;

        setFD(fd);
        mPeerCred.fillPeerCred(localSocketManager.getContext());
    }


    /** Close client socket. */
    public synchronized Error closeClientSocket(boolean logErrorMessage) {
        try {
            close();
        } catch (IOException e) {
            Error error = LocalSocketErrno.ERRNO_CLOSE_CLIENT_SOCKET_FAILED_WITH_EXCEPTION.getError(e, mLocalSocketRunConfig.getTitle(), e.getMessage());
            if (logErrorMessage)
                Logger.logErrorExtended(LOG_TAG, error.getErrorLogString());
            return error;
        }

        return null;
    }

    /** Close client socket that exists at fd. */
    public static void closeClientSocket(@NonNull LocalSocketManager localSocketManager, int fd) {
        new LocalClientSocket(localSocketManager, fd, new PeerCred()).closeClientSocket(true);
    }

    /** Implementation for {@link Closeable#close()} to close client socket. */
    @Override
    public void close() throws IOException {
        if (mFD >= 0) {
            Logger.logVerbose(LOG_TAG, "Client socket close for \"" + mLocalSocketRunConfig.getTitle() + "\" server: " + getPeerCred().getMinimalString());
            JniResult result = LocalSocketManager.closeSocket(mLocalSocketRunConfig.getLogTitle() + " (client)", mFD);
            if (result == null || result.retval != 0) {
                throw new IOException(JniResult.getErrorString(result));
            }
            // Update fd to signify that client socket has been closed
            setFD(-1);
        }
    }


    /**
     * Attempts to read up to data buffer length bytes from file descriptor into the data buffer.
     * On success, the number of bytes read is returned (zero indicates end of file) in bytesRead.
     * It is not an error if bytesRead is smaller than the number of bytes requested; this may happen
     * for example because fewer bytes are actually available right now (maybe because we were close
     * to end-of-file, or because we are reading from a pipe), or because read() was interrupted by
     * a signal.
     *
     * If while reading the {@link #mCreationTime} + the milliseconds returned by
     * {@link LocalSocketRunConfig#getDeadline()} elapses but all the data has not been read, an
     * error would be returned.
     *
     * This is a wrapper for {@link LocalSocketManager#read(String, int, byte[], long)}, which can
     * be called instead if you want to get access to errno int value instead of {@link JniResult}
     * error {@link String}.
     *
     * @param data The data buffer to read bytes into.
     * @param bytesRead The actual bytes read.
     * @return Returns the {@code error} if reading was not successful containing {@link JniResult}
     * error {@link String}, otherwise {@code null}.
     */
    public Error read(@NonNull byte[] data, MutableInt bytesRead) {
        bytesRead.value = 0;

        if (mFD < 0) {
            return LocalSocketErrno.ERRNO_USING_CLIENT_SOCKET_WITH_INVALID_FD.getError(mFD,
                mLocalSocketRunConfig.getTitle());
        }

        JniResult result = LocalSocketManager.read(mLocalSocketRunConfig.getLogTitle() + " (client)",
            mFD, data,
            mLocalSocketRunConfig.getDeadline() > 0 ? mCreationTime + mLocalSocketRunConfig.getDeadline() : 0);
        if (result == null || result.retval != 0) {
            return LocalSocketErrno.ERRNO_READ_DATA_FROM_CLIENT_SOCKET_FAILED.getError(
                mLocalSocketRunConfig.getTitle(), JniResult.getErrorString(result));
        }

        bytesRead.value = result.intData;
        return null;
    }

    /**
     * Attempts to send data buffer to the file descriptor.
     *
     * If while sending the {@link #mCreationTime} + the milliseconds returned by
     * {@link LocalSocketRunConfig#getDeadline()} elapses but all the data has not been sent, an
     * error would be returned.
     *
     * This is a wrapper for {@link LocalSocketManager#send(String, int, byte[], long)}, which can
     * be called instead if you want to get access to errno int value instead of {@link JniResult}
     * error {@link String}.
     *
     * @param data The data buffer containing bytes to send.
     * @return Returns the {@code error} if sending was not successful containing {@link JniResult}
     * error {@link String}, otherwise {@code null}.
     */
    public Error send(@NonNull byte[] data) {
        if (mFD < 0) {
            return LocalSocketErrno.ERRNO_USING_CLIENT_SOCKET_WITH_INVALID_FD.getError(mFD,
                mLocalSocketRunConfig.getTitle());
        }

        JniResult result = LocalSocketManager.send(mLocalSocketRunConfig.getLogTitle() + " (client)",
            mFD, data,
            mLocalSocketRunConfig.getDeadline() > 0 ? mCreationTime + mLocalSocketRunConfig.getDeadline() : 0);
        if (result == null || result.retval != 0) {
            return LocalSocketErrno.ERRNO_SEND_DATA_TO_CLIENT_SOCKET_FAILED.getError(
                mLocalSocketRunConfig.getTitle(), JniResult.getErrorString(result));
        }

        return null;
    }

    /**
     * Attempts to read all the bytes available on {@link SocketInputStream} and appends them to
     * {@code data} {@link StringBuilder}.
     *
     * This is a wrapper for {@link #read(byte[], MutableInt)} called via {@link SocketInputStream#read()}.
     *
     * @param data The data {@link StringBuilder} to append the bytes read into.
     * @param closeStreamOnFinish If set to {@code true}, then underlying input stream will closed
     *                            and further attempts to read from socket will fail.
     * @return Returns the {@code error} if reading was not successful containing {@link JniResult}
     * error {@link String}, otherwise {@code null}.
     */
    public Error readDataOnInputStream(@NonNull StringBuilder data, boolean closeStreamOnFinish) {
        int c;
        InputStreamReader inputStreamReader = getInputStreamReader();
        try {
            while ((c = inputStreamReader.read()) > 0) {
                data.append((char) c);
            }
        } catch (IOException e) {
            // The SocketInputStream.read() throws the Error message in an IOException,
            // so just read the exception message and not the stack trace, otherwise it would result
            // in a messy nested error message.
            return LocalSocketErrno.ERRNO_READ_DATA_FROM_INPUT_STREAM_OF_CLIENT_SOCKET_FAILED_WITH_EXCEPTION.getError(
                mLocalSocketRunConfig.getTitle(), DataUtils.getSpaceIndentedString(e.getMessage(), 1));
        } catch (Exception e) {
            return LocalSocketErrno.ERRNO_READ_DATA_FROM_INPUT_STREAM_OF_CLIENT_SOCKET_FAILED_WITH_EXCEPTION.getError(
                e, mLocalSocketRunConfig.getTitle(), e.getMessage());
        } finally {
            if (closeStreamOnFinish) {
                try { inputStreamReader.close();
                } catch (IOException e) {
                    // Ignore
                }
            }
        }

        return null;
    }

    /**
     * Attempts to send all the bytes passed to {@link SocketOutputStream} .
     *
     * This is a wrapper for {@link #send(byte[])} called via {@link SocketOutputStream#write(int)}.
     *
     * @param data The {@link String} bytes to send.
     * @param closeStreamOnFinish If set to {@code true}, then underlying output stream will closed
     *                            and further attempts to send to socket will fail.
     * @return Returns the {@code error} if sending was not successful containing {@link JniResult}
     * error {@link String}, otherwise {@code null}.
     */
    public Error sendDataToOutputStream(@NonNull String data, boolean closeStreamOnFinish) {

        OutputStreamWriter outputStreamWriter = getOutputStreamWriter();

        try (BufferedWriter byteStreamWriter = new BufferedWriter(outputStreamWriter)) {
            byteStreamWriter.write(data);
            byteStreamWriter.flush();
        } catch (IOException e) {
            // The SocketOutputStream.write() throws the Error message in an IOException,
            // so just read the exception message and not the stack trace, otherwise it would result
            // in a messy nested error message.
            return LocalSocketErrno.ERRNO_SEND_DATA_TO_OUTPUT_STREAM_OF_CLIENT_SOCKET_FAILED_WITH_EXCEPTION.getError(
                mLocalSocketRunConfig.getTitle(), DataUtils.getSpaceIndentedString(e.getMessage(), 1));
        } catch (Exception e) {
            return LocalSocketErrno.ERRNO_SEND_DATA_TO_OUTPUT_STREAM_OF_CLIENT_SOCKET_FAILED_WITH_EXCEPTION.getError(
                e, mLocalSocketRunConfig.getTitle(), e.getMessage());
        } finally {
            if (closeStreamOnFinish) {
                try {
                    outputStreamWriter.close();
                } catch (IOException e) {
                    // Ignore
                }
            }
        }

        return null;
    }

    /** Wrapper for {@link #available(MutableInt, boolean)} that checks deadline. The
     * {@link SocketInputStream} calls this. */
    public Error available(MutableInt available) {
        return available(available, true);
    }

    /**
     * Get available bytes on {@link #mInputStream} and optionally check if value returned by
     * {@link LocalSocketRunConfig#getDeadline()} has passed.
     */
    public Error available(MutableInt available, boolean checkDeadline) {
        available.value = 0;

        if (mFD < 0) {
            return LocalSocketErrno.ERRNO_USING_CLIENT_SOCKET_WITH_INVALID_FD.getError(mFD,
                mLocalSocketRunConfig.getTitle());
        }

        if (checkDeadline && mLocalSocketRunConfig.getDeadline() > 0 && System.currentTimeMillis() > (mCreationTime + mLocalSocketRunConfig.getDeadline())) {
            return null;
        }

        JniResult result = LocalSocketManager.available(mLocalSocketRunConfig.getLogTitle() + " (client)", mLocalSocketRunConfig.getFD());
        if (result == null || result.retval != 0) {
            return LocalSocketErrno.ERRNO_CHECK_AVAILABLE_DATA_ON_CLIENT_SOCKET_FAILED.getError(
                mLocalSocketRunConfig.getTitle(), JniResult.getErrorString(result));
        }

        available.value = result.intData;
        return null;
    }



    /** Set {@link LocalClientSocket} receiving (SO_RCVTIMEO) timeout to value returned by {@link LocalSocketRunConfig#getReceiveTimeout()}. */
    public Error setReadTimeout() {
        if (mFD >= 0) {
            JniResult result = LocalSocketManager.setSocketReadTimeout(mLocalSocketRunConfig.getLogTitle() + " (client)",
                mFD, mLocalSocketRunConfig.getReceiveTimeout());
            if (result == null || result.retval != 0) {
                return LocalSocketErrno.ERRNO_SET_CLIENT_SOCKET_READ_TIMEOUT_FAILED.getError(
                    mLocalSocketRunConfig.getTitle(), mLocalSocketRunConfig.getReceiveTimeout(), JniResult.getErrorString(result));
            }
        }
        return null;
    }

    /** Set {@link LocalClientSocket} sending (SO_SNDTIMEO) timeout to value returned by {@link LocalSocketRunConfig#getSendTimeout()}. */
    public Error setWriteTimeout() {
        if (mFD >= 0) {
            JniResult result = LocalSocketManager.setSocketSendTimeout(mLocalSocketRunConfig.getLogTitle() + " (client)",
                mFD, mLocalSocketRunConfig.getSendTimeout());
            if (result == null || result.retval != 0) {
                return LocalSocketErrno.ERRNO_SET_CLIENT_SOCKET_SEND_TIMEOUT_FAILED.getError(
                    mLocalSocketRunConfig.getTitle(), mLocalSocketRunConfig.getSendTimeout(), JniResult.getErrorString(result));
            }
        }
        return null;
    }



    /** Get {@link #mFD} for the client socket. */
    public int getFD() {
        return mFD;
    }

    /** Set {@link #mFD}. Value must be greater than 0 or -1. */
    private void setFD(int fd) {
        if (fd >= 0)
            mFD = fd;
        else
            mFD = -1;
    }

    /** Get {@link #mPeerCred} for the client socket. */
    public PeerCred getPeerCred() {
        return mPeerCred;
    }

    /** Get {@link #mCreationTime} for the client socket. */
    public long getCreationTime() {
        return mCreationTime;
    }

    /** Get {@link #mOutputStream} for the client socket. The stream will automatically close when client socket is closed. */
    public OutputStream getOutputStream() {
        return mOutputStream;
    }

    /** Get {@link OutputStreamWriter} for {@link #mOutputStream} for the client socket. The stream will automatically close when client socket is closed. */
    @NonNull
    public OutputStreamWriter getOutputStreamWriter() {
        return new OutputStreamWriter(getOutputStream());
    }

    /** Get {@link #mInputStream} for the client socket. The stream will automatically close when client socket is closed. */
    public InputStream getInputStream() {
        return mInputStream;
    }

    /** Get {@link InputStreamReader} for {@link #mInputStream} for the client socket. The stream will automatically close when client socket is closed. */
    @NonNull
    public InputStreamReader getInputStreamReader() {
        return new InputStreamReader(getInputStream());
    }



    /** Get a log {@link String} for the {@link LocalClientSocket}. */
    @NonNull
    public String getLogString() {
        StringBuilder logString = new StringBuilder();

        logString.append("Client Socket:");
        logString.append("\n").append(Logger.getSingleLineLogStringEntry("FD", mFD, "-"));
        logString.append("\n").append(Logger.getSingleLineLogStringEntry("Creation Time", mCreationTime, "-"));
        logString.append("\n\n\n");

        logString.append(mPeerCred.getLogString());

        return logString.toString();
    }

    /** Get a markdown {@link String} for the {@link LocalClientSocket}. */
    @NonNull
    public String getMarkdownString() {
        StringBuilder markdownString = new StringBuilder();

        markdownString.append("## ").append("Client Socket");
        markdownString.append("\n").append(MarkdownUtils.getSingleLineMarkdownStringEntry("FD", mFD, "-"));
        markdownString.append("\n").append(MarkdownUtils.getSingleLineMarkdownStringEntry("Creation Time", mCreationTime, "-"));
        markdownString.append("\n\n\n");

        markdownString.append(mPeerCred.getMarkdownString());

        return markdownString.toString();
    }





    /** Wrapper class to allow pass by reference of int values. */
    public static final class MutableInt {
        public int value;

        public MutableInt(int value) {
            this.value = value;
        }
    }



    /** The {@link InputStream} implementation for the {@link LocalClientSocket}. */
    protected class SocketInputStream extends InputStream {
        private final byte[] mBytes = new byte[1];

        @Override
        public int read() throws IOException {
            MutableInt bytesRead = new MutableInt(0);
            Error error = LocalClientSocket.this.read(mBytes, bytesRead);
            if (error != null) {
                throw new IOException(error.getErrorMarkdownString());
            }

            if (bytesRead.value == 0) {
                return -1;
            }

            return mBytes[0];
        }

        @Override
        public int read(byte[] bytes) throws IOException {
            if (bytes == null) {
                throw new NullPointerException("Read buffer can't be null");
            }

            MutableInt bytesRead = new MutableInt(0);
            Error error = LocalClientSocket.this.read(bytes, bytesRead);
            if (error != null) {
                throw new IOException(error.getErrorMarkdownString());
            }

            if (bytesRead.value == 0) {
                return -1;
            }

            return bytesRead.value;
        }

        @Override
        public int available() throws IOException {
            MutableInt available = new MutableInt(0);
            Error error = LocalClientSocket.this.available(available);
            if (error != null) {
                throw new IOException(error.getErrorMarkdownString());
            }
            return available.value;
        }
    }



    /** The {@link OutputStream} implementation for the {@link LocalClientSocket}. */
    protected class SocketOutputStream extends OutputStream {
        private final byte[] mBytes = new byte[1];

        @Override
        public void write(int b) throws IOException {
            mBytes[0] = (byte) b;

            Error error = LocalClientSocket.this.send(mBytes);
            if (error != null) {
                throw new IOException(error.getErrorMarkdownString());
            }
        }

        @Override
        public void write(byte[] bytes) throws IOException {
            Error error = LocalClientSocket.this.send(bytes);
            if (error != null) {
                throw new IOException(error.getErrorMarkdownString());
            }
        }
    }

}
