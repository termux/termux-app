package com.termux.shared.shell.command.runner.nativerunner;


import android.os.Process;

import com.termux.shared.shell.command.ExecutionCommand;
import com.termux.terminal.JNI;

import java.io.IOException;

/**
 * This Runner is only available over Binder IPC, because it requires transferring file descriptors to Termux and back
 * to the client, which is not possible over Intents.
 *
 */
public final class NativeShell
{
    private final ExecutionCommand exe;
    private final Client client;
    private final String[] env;
    private int pid = -1;
    
    public NativeShell(ExecutionCommand exe, Client client, String[] env) {
        this.exe = exe;
        if (exe.executable == null) throw new IllegalArgumentException("NativeShell: Command cannot be null");
        if (exe.workingDirectory == null) throw new IllegalArgumentException("NativeShell: Working directory cannot be null");
        if (exe.stdinFD == null) throw new IllegalArgumentException("NativeShell: stdin cannot be null");
        if (exe.stdoutFD == null) throw new IllegalArgumentException("NativeShell: stdout cannot be null");
        if (exe.stderrFD == null) throw new IllegalArgumentException("NativeShell: stderr cannot be null");
        this.client = client;
        this.env = env;
    }
    
    public interface Client {
        /**
         * @param exitCode The exit code of the process. Undefined if error is not null. Negative numbers mean a signal terminated the process.
         * @param error An exception that was thrown while trying to execute the command. Can be null.
         */
        void terminated(int exitCode, Exception error);
    }
    
    public synchronized boolean execute() {
        try {
            pid = JNI.createTask(exe.executable, exe.workingDirectory, exe.arguments, env, exe.stdinFD.getFd(), exe.stdoutFD.getFd(), exe.stderrFD.getFd());
            new Thread(() -> {
                int exit = JNI.waitFor(pid);
                client.terminated(exit, null);
                pid = -1;
            }).start();
            return true;
        } catch (Exception e) {
            client.terminated(0, e);
        } finally {
            // close the ParcelFileDescriptors
            try {
                exe.stdinFD.close();
            } catch (IOException ignored) {}
            try {
                exe.stdoutFD.close();
            } catch (IOException ignored) {}
            try {
                exe.stderrFD.close();
            } catch (IOException ignored) {}
        }
        return false;
    }
    
    public synchronized void kill(int signal) {
        if (pid != -1)
            Process.sendSignal(pid, signal);
    }
    
    public synchronized int getPid() {
        return pid;
    }
}
