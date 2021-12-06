package com.termux.shared.shell;

import android.app.Application;

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
    private final LocalFilesystemSocket.ServerSocket server;
    private final int timeoutMillis;
    private final int deadlineMillis;
    
    
    private LocalSocketListener(@NonNull Application a, @NonNull LocalSocketHandler h, @NonNull String path, int timeoutMillis, int deadlineMillis) throws IOException {
        this.timeoutMillis = timeoutMillis;
        this.deadlineMillis = deadlineMillis;
        server = new LocalFilesystemSocket.ServerSocket(a, path);
        thread = new Thread(new LocalSocketListenerRunnable(h));
        thread.setUncaughtExceptionHandler((t, e) -> Logger.logStackTraceWithMessage(LOG_TAG, "Uncaught exception in LocalSocketListenerRunnable", e));
        thread.start();
    }
    
    public static LocalSocketListener tryEstablishLocalSocketListener(@NonNull Application a, @NonNull LocalSocketHandler h, @NonNull String address, int timeoutMillis, int deadlineMillis) {
        try {
            return new LocalSocketListener(a, h, address, timeoutMillis, deadlineMillis);
        } catch (IOException e) {
            return null;
        }
    }
    
    @SuppressWarnings("unused")
    public void stop() {
        try {
            thread.interrupt();
            server.close();
        } catch (Exception ignored) {}
    }
    
    private class LocalSocketListenerRunnable implements Runnable {
        private final LocalSocketHandler h;
        public LocalSocketListenerRunnable(@NonNull LocalSocketHandler h) {
            this.h = h;
        }
        
        @Override
        public void run() {
            try {
                while (! Thread.currentThread().isInterrupted()) {
                    try (LocalFilesystemSocket.Socket s = server.accept();
                         OutputStream sockout = s.getOutputStream();
                         InputStreamReader r = new InputStreamReader(s.getInputStream())) {
                        
                        
                        
                        s.setTimeout(timeoutMillis);
                        s.setDeadline(System.currentTimeMillis()+deadlineMillis);
                        
                        StringBuilder b = new StringBuilder();
                        int c;
                        while ((c = r.read()) > 0) {
                            b.append((char) c);
                        }
                        Logger.logDebug(LOG_TAG, b.toString());
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
            }
            Logger.logDebug(LOG_TAG, "LocalSocketListenerRunnable returned");
        }
    }
}
