package com.termux.shared.shell;

import android.app.Application;
import android.net.LocalServerSocket;
import android.net.LocalSocket;

import androidx.annotation.NonNull;

import com.termux.shared.logger.Logger;

import java.io.BufferedWriter;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintStream;

public class LocalSocketListener
{
    public interface LocalSocketHandler {
        int handle(String[] args, PrintStream out, PrintStream err);
    }
    
    
    private static final String LOG_TAG = "LocalSocketListener";
    
    private final Thread thread;
    private final LocalServerSocket server;
    private final int timeoutMillis;
    
    public LocalSocketListener(@NonNull Application a, @NonNull LocalSocketHandler h, String address, int timeoutMillis) throws IOException {
        this.timeoutMillis = timeoutMillis;
        server = new LocalServerSocket(address);
        thread = new Thread(new LocalSocketListenerRunnable(a, h));
        thread.setUncaughtExceptionHandler((t, e) -> Logger.logStackTraceWithMessage(LOG_TAG, "Uncaught exception in LocalSocketListenerRunnable", e));
        thread.start();
    }
    
    @SuppressWarnings("unused")
    public void stop() {
        try {
            thread.interrupt();
            server.close();
        } catch (Exception ignored) {}
    }
    
    private class LocalSocketListenerRunnable implements Runnable {
        private final Application a;
        private final TimeoutWatcher timeoutWatcher;
        private final Thread timeoutWatcherThread;
        private final LocalSocketHandler h;
        public LocalSocketListenerRunnable(@NonNull Application a, @NonNull LocalSocketHandler h) {
            this.a = a;
            this.h = h;
            timeoutWatcher = new TimeoutWatcher();
            timeoutWatcherThread = new Thread(timeoutWatcher);
            timeoutWatcherThread.start();
        }
        
        // the socket timeout for LocalSocket doesn't seem to work, so close the socket if the timeout is over, so the processing Thread doesn't get blocked.
        private class TimeoutWatcher implements Runnable {
            private final Object lock = new Object();
            private LocalSocket current = null;
            @Override
            public void run() {
                while (! Thread.currentThread().isInterrupted()) {
                    LocalSocket watch = current;
                    synchronized (lock) {
                        while (watch == null) {
                            try {
                                lock.wait();
                            }
                            catch (InterruptedException ignored) {
                                Thread.currentThread().interrupt();
                            }
                            watch = current;
                        }
                    }
                    try {
                        //noinspection BusyWait
                        Thread.sleep(timeoutMillis);
                    }
                    catch (InterruptedException e) {
                        Thread.currentThread().interrupt();
                    }
                    try {
                        watch.shutdownInput();
                    } catch (Exception ignored) {}
                    try {
                        watch.shutdownOutput();
                    } catch (Exception ignored) {}
                    try {
                        watch.close();
                    } catch (Exception ignored) {}
                }
            }
        }
        
        @Override
        public void run() {
            try {
                while (! Thread.currentThread().isInterrupted()) {
                    try (LocalSocket s = server.accept();
                         OutputStream sockout = s.getOutputStream();
                         InputStreamReader r = new InputStreamReader(s.getInputStream())) {
                        timeoutWatcher.current = s;
                        synchronized (timeoutWatcher.lock) {
                            timeoutWatcher.lock.notifyAll();
                        }
                        // ensure only Termux programs can connect
                        if (s.getPeerCredentials().getUid() != a.getApplicationInfo().uid) {
                            Logger.logDebug(LOG_TAG, "A program with another UID tried to connect");
                            continue;
                        }
                        StringBuilder b = new StringBuilder();
                        int c;
                        while ((c = r.read()) > 0) {
                            b.append((char) c);
                        }
                        String outString;
                        String errString;
                        int ret;
                        try (ByteArrayOutputStream out = new ByteArrayOutputStream();
                             PrintStream outp = new PrintStream(out);
                             ByteArrayOutputStream err = new ByteArrayOutputStream();
                             PrintStream errp = new PrintStream(err)) {
                            
                            ret = h.handle(ArgumentTokenizer.tokenize(b.toString()).toArray(new String[0]), outp, errp);
                            
                            outp.flush();
                            outString = out.toString("UTF-8");
                            
                            errp.flush();
                            errString = err.toString("UTF-8");
                        }
                        try (BufferedWriter w = new BufferedWriter(new OutputStreamWriter(sockout))) {
                            w.write(Integer.toString(ret));
                            w.write('\0');
                            w.write(outString);
                            w.write('\0');
                            w.write(errString);
                            w.flush();
                        }
                    } catch (Exception e) {
                        Logger.logStackTraceWithMessage(LOG_TAG, "Exception while handling connection", e);
                    }
                }
            }
            finally {
                try {
                    server.close();
                } catch (Exception ignored) {}
                if (timeoutWatcherThread.isAlive()) {
                    timeoutWatcherThread.interrupt();
                }
            }
            Logger.logDebug(LOG_TAG, "LocalSocketListenerRunnable returned");
        }
    }
}
