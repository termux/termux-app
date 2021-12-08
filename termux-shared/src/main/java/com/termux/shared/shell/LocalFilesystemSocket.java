package com.termux.shared.shell;

import android.content.Context;

import androidx.annotation.NonNull;

import com.termux.shared.logger.Logger;

import java.io.Closeable;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;

public class LocalFilesystemSocket
{
    static {
        System.loadLibrary("local-filesystem-socket");
    }
    
    /**
     * Creates as UNIX server socket at {@code path}, with a backlog of {@code backlog}.
     * @return The file descriptor of the server socket or -1 if an error occurred.
     */
    private static native int createserversocket(@NonNull byte[] path, int backlog);
    
    /**
     * Accepts a connection on the supplied server socket.
     * @return The file descriptor of the connection socket or -1 in case of an error.
     */
    private static native int accept(int fd);
    
    private static native void closefd(int fd);
    
    
    /**
     * Returns the UID of the socket peer, or -1 in case of an error.
     */
    private static native int getpeeruid(int fd);
    
    
    /**
     * Sends {@code data} over the socket and decrements the timeout by the elapsed time. If the timeout hits or an error occurs, false is returned, true otherwise.
     */
    private static native boolean send(@NonNull byte[] data, int fd, long deadline);
    
    /**
     * Receives data from the socket and decrements the timeout by the elapsed time. If the timeout hits or an error occurs, -1 is returned, otherwise the number of received bytes.
     */
    private static native int recv(@NonNull byte[] data, int fd, long deadline);
    
    /**
     * Sets the send and receive timeout for the socket in milliseconds.
     */
    private static native void settimeout(int fd, int timeout);
    
    /**
     * Gets the number of bytes available to read on the socket.
     */
    private static native int available(int fd);
    
    
    
    
    static class Socket implements Closeable {
        private int fd;
        private long deadline = 0;
        private final SocketOutputStream out = new SocketOutputStream();
        private final SocketInputStream in = new SocketInputStream();
        
        class SocketInputStream extends InputStream {
            private final byte[] readb = new byte[1];
            
            @Override
            public int read() throws IOException {
                int ret = recv(readb);
                if (ret == -1) {
                    throw new IOException("Could not read from socket");
                }
                if (ret == 0) {
                    return -1;
                }
                return readb[0];
            }
    
            @Override
            public int read(byte[] b) throws IOException {
                if (b == null) {
                    throw new NullPointerException("Read buffer  can't be null");
                }
                int ret = recv(b);
                if (ret == -1) {
                    throw new IOException("Could not read from socket");
                }
                if (ret == 0) {
                    return -1;
                }
                return ret;
            }
    
            @Override
            public int available() {
                return Socket.this.available();
            }
        }
        
        class SocketOutputStream extends OutputStream {
            private final byte[] writeb = new byte[1];
            
            @Override
            public void write(int b) throws IOException {
                writeb[0] = (byte) b;
                if (! send(writeb)) {
                    throw new IOException("Could not write to socket");
                }
            }
    
            @Override
            public void write(byte[] b) throws IOException {
                if (! send(b)) {
                    throw new IOException("Could not write to socket");
                }
            }
        }
        
        private Socket(int fd) {
            this.fd = fd;
        }
    
    
        /**
         * Sets the socket timeout, that makes a single send/recv error out if it triggers. For a deadline after which the socket should be finished see setDeadline()
         */
        public void setTimeout(int timeout) {
            if (fd != -1) {
                settimeout(fd, timeout);
            }
        }
    
    
        /**
         * Sets the deadline in unix milliseconds. When the deadline has elapsed and the socket timeout triggers, all reads and writes will error.
         */
        public void setDeadline(long deadline) {
            this.deadline = deadline;
        }
        
        public boolean send(@NonNull byte[] data) {
            if (fd == -1) {
                return false;
            }
            return LocalFilesystemSocket.send(data, fd, deadline);
        }
    
        public int recv(@NonNull byte[] data) {
            if (fd == -1) {
                return -1;
            }
            return LocalFilesystemSocket.recv(data, fd, deadline);
        }
        
        public int available() {
            if (fd == -1 || System.currentTimeMillis() > deadline) {
                return 0;
            }
            return LocalFilesystemSocket.available(fd);
        }
        
        @Override
        public void close() throws IOException {
            if (fd != -1) {
                closefd(fd);
                fd = -1;
            }
        }
    
        /**
         * Returns the UID of the socket peer, or -1 in case of an error.
         */
        public int getPeerUID() {
            return getpeeruid(fd); 
        }
        
        /**
         * Returns an {@link OutputStream} for the socket. You don't need to close the stream, it's automatically closed when closing the socket.
         */
        public OutputStream getOutputStream() {
            return out;
        }
    
        /**
         * Returns an {@link InputStream} for the socket. You don't need to close the stream, it's automatically closed when closing the socket.
         */
        public InputStream getInputStream() {
            return  in;
        }
    }
    
    
    static class ServerSocket implements Closeable
    {
        private final String path;
        private final Context app;
        private int fd;
        
        public ServerSocket(Context c, String path, int backlog) throws IOException {
            app = c.getApplicationContext();
            if (backlog <= 0) {
                throw new IllegalArgumentException("Backlog has to be at least 1");
            }
            if (path == null || path.length() == 0) {
                throw new IllegalArgumentException("path cannot be null or empty");
            }
            this.path = path;
            if (path.getBytes(StandardCharsets.UTF_8)[0] != 0) {
                // not a socket in the abstract linux namespace, make sure the path is accessible and clear
                File f = new File(path);
                File parent = f.getParentFile();
                if (parent != null) {
                    parent.mkdirs();
                }
                f.delete();
            }
    
            fd = createserversocket(path.getBytes(StandardCharsets.UTF_8), backlog);
            if (fd == -1) {
                throw new IOException("Could not create UNIX server socket at \""+path+"\"");
            }
        }
        
        public ServerSocket(Context c, String path) throws IOException {
            this(c, path, 50); // 50 is the default value for the Android LocalSocket implementation, so this should be good here, too
        }
        
        public Socket accept() {
            if (fd == -1) {
                return null;
            }
            int c = -1;
            while (true) {
                while (c == -1) {
                    c = LocalFilesystemSocket.accept(fd);
                }
                int peeruid = getpeeruid(c);
                if (peeruid == -1) {
                    Logger.logWarn("LocalFilesystemSocket.ServerSocket", "Could not verify peer uid, closing socket");
                    closefd(c);
                    c = -1;
                    continue;
                }
    
                // if the peer has the same uid or is root, allow the connection
                if (peeruid == app.getApplicationInfo().uid || peeruid == 0) {
                    break;
                } else {
                    Logger.logWarn("LocalFilesystemSocket.ServerSocket", "WARNING: An app with the uid of "+peeruid+" tried to connect to the socket at \""+path+"\", closing connection.");
                    closefd(c);
                    c = -1;
                }
            }
            return new Socket(c);
        }
        
        @Override
        public void close() throws IOException {
            if (fd != -1) {
                closefd(fd);
                fd = -1;
            }
        }
    }
}
