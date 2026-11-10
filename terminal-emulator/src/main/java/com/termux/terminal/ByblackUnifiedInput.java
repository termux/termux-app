package com.termux.terminal;

import java.nio.charset.StandardCharsets;
import java.util.ArrayDeque;
import java.util.Arrays;

/** A single FIFO producer queue; never waits on the Android UI thread. */
final class ByblackUnifiedInput {
    static final int MAX_PENDING = 4 * 1024 * 1024;
    // Leave capacity for typing after a paste has filled its allowed share.
    static final int INPUT_RESERVE = 64 * 1024;
    // Account for queue nodes and job objects as well as payloads.
    private static final int JOB_OVERHEAD = 64;
    enum Result { ACCEPTED, FULL, CLOSED }
    private static final class Job {
        final byte[] bytes;
        final String paste;
        final boolean bracketed;
        final int charge;
        Job(byte[] bytes, String paste, boolean bracketed, int charge) {
            this.bytes = bytes; this.paste = paste;
            this.bracketed = bracketed; this.charge = charge;
        }
    }
    private final ByteQueue output;
    private final ArrayDeque<Job> pending = new ArrayDeque<>();
    private final Thread worker;
    private int pendingBytes;
    private boolean closed;

    ByblackUnifiedInput(ByteQueue output) {
        this.output = output;
        worker = new Thread(this::runWorker, "ByblackUnifiedInput");
        worker.setDaemon(true);
        worker.start();
    }
    synchronized Result offer(byte[] data, int offset, int count) {
        if (data == null) throw new NullPointerException("data");
        if (offset < 0 || count < 0 || offset > data.length - count)
            throw new IllegalArgumentException("Invalid range");
        if (closed) return Result.CLOSED;
        if (count == 0) return Result.ACCEPTED;
        long charge = (long) count + JOB_OVERHEAD;
        if (charge > MAX_PENDING - pendingBytes) return Result.FULL;
        pending.addLast(new Job(Arrays.copyOfRange(data, offset, offset + count), null, false, (int) charge));
        pendingBytes += (int) charge;
        notifyAll();
        return Result.ACCEPTED;
    }
    synchronized Result offerPaste(String text, boolean bracketed) {
        if (closed) return Result.CLOSED;
        if (text == null || text.isEmpty()) return Result.ACCEPTED;
        // Count UTF-16 storage pessimistically; no UI-thread UTF-8 encoding.
        long charge = (long) text.length() * 2 + JOB_OVERHEAD;
        if (charge > MAX_PENDING - INPUT_RESERVE - pendingBytes) return Result.FULL;
        pending.addLast(new Job(null, text, bracketed, (int) charge));
        pendingBytes += (int) charge;
        notifyAll();
        return Result.ACCEPTED;
    }
    private void runWorker() {
        try {
            while (true) {
                Job job;
                synchronized (this) {
                    while (pending.isEmpty() && !closed) wait();
                    if (closed) return;
                    job = pending.removeFirst();
                }
                boolean ok;
                if (job.paste == null) {
                    ok = send(job.bytes);
                } else {
                    String clean = job.paste.replaceAll("(\\u001B|[\\u0080-\\u009F])", "");
                    clean = clean.replace("\r\n", "\r").replace('\n', '\r');
                    byte[] data = ((job.bracketed ? "\033[200~" : "") + clean +
                        (job.bracketed ? "\033[201~" : "")).getBytes(StandardCharsets.UTF_8);
                    ok = send(data);
                }
                synchronized (this) {
                    if (!closed) pendingBytes -= job.charge;
                    notifyAll();
                }
                if (!ok) return;
            }
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        } finally {
            synchronized (this) {
                closed = true;
                pending.clear();
                pendingBytes = 0;
                notifyAll();
            }
        }
    }
    private boolean send(byte[] data) {
        // ByteQueue.write may block, but only on our worker.
        for (int i = 0; i < data.length; i += 4096) {
            if (Thread.currentThread().isInterrupted()) return false;
            int n = Math.min(4096, data.length - i);
            if (!output.write(data, i, n)) return false;
        }
        return true;
    }
    // Package-private lifecycle probe for tests; callers must close before joining.
    boolean awaitTermination(long timeoutMillis) throws InterruptedException {
        worker.join(timeoutMillis);
        return !worker.isAlive();
    }
    synchronized int pendingBytes() { return pendingBytes; }
    synchronized void close() {
        closed = true;
        pending.clear();
        pendingBytes = 0;
        notifyAll();
        output.close();
        worker.interrupt();
    }
}
