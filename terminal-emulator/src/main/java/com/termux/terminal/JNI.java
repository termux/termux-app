package com.termux.terminal;

/**
 * Native methods for creating and managing pseudoterminal subprocesses. C code is in jni/termux.c.
 */
public final class JNI {

    static {
        System.loadLibrary("termux");
    }

    /**
     * Create a subprocess. Differs from {@link ProcessBuilder} in that a pseudoterminal is used to communicate with the
     * subprocess.
     * <p/>
     * Callers are responsible for calling {@link #close(int)} on the returned file descriptor.
     *
     * @param cmd       The command to execute
     * @param cwd       The current working directory for the executed command
     * @param args      An array of arguments to the command
     * @param envVars   An array of strings of the form "VAR=value" to be added to the environment of the process
     * @param processId A one-element array to which the process ID of the started process will be written.
     * @return the file descriptor resulting from opening /dev/ptmx master device. The sub process will have opened the
     * slave device counterpart (/dev/pts/$N) and have it as stdint, stdout and stderr.
     */
    public static native int createSubprocess(String cmd, String cwd, String[] args, String[] envVars, int[] processId, int rows, int columns);
    
    /**
     * Create a subprocess. Differs from {@link #createSubprocess(String, String, String[], String[], int[], int, int)} in that there is no 
     * pseudoterminal, but all input and output is redirected through the given file descriptors without the need for {@link com.termux.shared.shell.StreamGobbler}
     * or additional threads to do the IO operations for that. Because file descriptors are used, this can also transmit more data than the normal Binder
     * transaction size limit for Intents.
     * 
     * @param cmd       The command to execute
     * @param cwd       The current working directory for the executed command
     * @param args      An array of arguments to the command
     * @param envVars   An array of strings of the form "VAR=value" to be added to the environment of the process
     * @param stdin     The file descriptor that should be used for stdin for the process
     * @param stdout    The file descriptor that should be used for stdout for the process
     * @param stderr    The file descriptor that should be used for stderr for the process
     * @return The pid of the created subprocess.
     */
    public static native int createTask(String cmd, String cwd, String[] args, String[] envVars, int stdin, int stdout, int stderr);

    /** Set the window size for a given pty, which allows connected programs to learn how large their screen is. */
    public static native void setPtyWindowSize(int fd, int rows, int cols);

    /**
     * Causes the calling thread to wait for the process associated with the receiver to finish executing.
     *
     * @return if >= 0, the exit status of the process. If < 0, the signal causing the process to stop negated.
     */
    public static native int waitFor(int processId);

    /** Close a file descriptor through the close(2) system call. */
    public static native void close(int fileDescriptor);

}
