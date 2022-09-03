package com.termux.shared.shell.am;

import android.Manifest;
import android.app.Application;
import android.content.Context;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.termux.am.Am;
import com.termux.shared.R;
import com.termux.shared.android.PackageUtils;
import com.termux.shared.android.PermissionUtils;
import com.termux.shared.errors.Error;
import com.termux.shared.logger.Logger;
import com.termux.shared.net.socket.local.ILocalSocketManager;
import com.termux.shared.net.socket.local.LocalClientSocket;
import com.termux.shared.net.socket.local.LocalServerSocket;
import com.termux.shared.net.socket.local.LocalSocketManager;
import com.termux.shared.net.socket.local.LocalSocketManagerClientBase;
import com.termux.shared.net.socket.local.LocalSocketRunConfig;
import com.termux.shared.shell.ArgumentTokenizer;
import com.termux.shared.shell.command.ExecutionCommand;

import java.io.ByteArrayOutputStream;
import java.io.PrintStream;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * A AF_UNIX/SOCK_STREAM local server managed with {@link LocalSocketManager} whose
 * {@link LocalServerSocket} receives android activity manager (am) commands from {@link LocalClientSocket}
 * and runs them with termux-am-library. It would normally only allow processes belonging to the
 * server app's user and root user to connect to it.
 *
 * The client must send the am command as a string without the initial "am" arg on its output stream
 * and then wait for the result on its input stream. The result of the execution or error is sent
 * back in the format `exit_code\0stdout\0stderr\0` where `\0` represents a null character.
 * Check termux/termux-am-socket for implementation of a native c client.
 *
 * Usage:
 * 1. Optionally extend {@link AmSocketServerClient}, the implementation for
 *    {@link ILocalSocketManager} that will receive call backs from the server including
 *    when client connects via {@link ILocalSocketManager#onClientAccepted(LocalSocketManager, LocalClientSocket)}.
 * 2. Create a {@link AmSocketServerRunConfig} instance which extends from {@link LocalSocketRunConfig}
 *    with the run config of the am server. It would  be better to use a filesystem socket instead
 *    of abstract namespace socket for security reasons.
 * 3. Call {@link #start(Context, LocalSocketRunConfig)} to start the server and store the {@link LocalSocketManager}
 *    instance returned.
 * 4. Stop server if needed with a call to {@link LocalSocketManager#stop()} on the
 *    {@link LocalSocketManager} instance returned by start call.
 *
 * https://github.com/termux/termux-am-library/blob/main/termux-am-library/src/main/java/com/termux/am/Am.java
 * https://github.com/termux/termux-am-socket
 * https://developer.android.com/studio/command-line/adb#am
 * https://cs.android.com/android/platform/superproject/+/android-12.0.0_r32:frameworks/base/services/core/java/com/android/server/am/ActivityManagerShellCommand.java
 */
public class AmSocketServer {

    public static final String LOG_TAG = "AmSocketServer";

    /**
     * Create the {@link AmSocketServer} {@link LocalServerSocket} and start listening for new {@link LocalClientSocket}.
     *
     * @param context The {@link Context} for {@link LocalSocketManager}.
     * @param localSocketRunConfig The {@link LocalSocketRunConfig} for {@link LocalSocketManager}.
     */
    public static synchronized LocalSocketManager start(@NonNull Context context,
                                                        @NonNull LocalSocketRunConfig localSocketRunConfig) {
        LocalSocketManager localSocketManager = new LocalSocketManager(context, localSocketRunConfig);
        Error error = localSocketManager.start();
        if (error != null) {
            localSocketManager.onError(error);
            return null;
        }

        return localSocketManager;
    }

    public static void processAmClient(@NonNull LocalSocketManager localSocketManager,
                                       @NonNull LocalClientSocket clientSocket) {
        Error error;

        // Read amCommandString client sent and close input stream
        StringBuilder data = new StringBuilder();
        error = clientSocket.readDataOnInputStream(data, true);
        if (error != null) {
            sendResultToClient(localSocketManager, clientSocket, 1, null, error.toString());
            return;
        }

        String amCommandString = data.toString();

        Logger.logVerbose(LOG_TAG, "am command received from peer " + clientSocket.getPeerCred().getMinimalString() +
            "\nam command: `" + amCommandString + "`");

        // Parse am command string and convert it to a list of arguments
        List<String> amCommandList = new ArrayList<>();
        error = parseAmCommand(amCommandString, amCommandList);
        if (error != null) {
            sendResultToClient(localSocketManager, clientSocket, 1, null, error.toString());
            return;
        }

        String[] amCommandArray = amCommandList.toArray(new String[0]);

        Logger.logDebug(LOG_TAG, "am command received from peer " + clientSocket.getPeerCred().getMinimalString() +
            "\n" + ExecutionCommand.getArgumentsLogString("am command", amCommandArray));

        AmSocketServerRunConfig amSocketServerRunConfig = (AmSocketServerRunConfig) localSocketManager.getLocalSocketRunConfig();

        // Run am command and send its result to the client
        StringBuilder stdout = new StringBuilder();
        StringBuilder stderr = new StringBuilder();
        error = runAmCommand(localSocketManager.getContext(), amCommandArray, stdout, stderr,
            amSocketServerRunConfig.shouldCheckDisplayOverAppsPermission());
        if (error != null) {
            sendResultToClient(localSocketManager, clientSocket, 1, stdout.toString(),
                !stderr.toString().isEmpty() ? stderr + "\n\n" + error : error.toString());
        }

        sendResultToClient(localSocketManager, clientSocket, 0, stdout.toString(), stderr.toString());
    }

    /**
     * Send result to {@link LocalClientSocket} that requested the am command to be run.
     *
     * @param localSocketManager The {@link LocalSocketManager} instance for the local socket.
     * @param clientSocket The {@link LocalClientSocket} to which the result is to be sent.
     * @param exitCode The exit code value to send.
     * @param stdout The stdout value to send.
     * @param stderr The stderr value to send.
     */
    public static void sendResultToClient(@NonNull LocalSocketManager localSocketManager,
                                          @NonNull LocalClientSocket clientSocket,
                                          int exitCode,
                                          @Nullable String stdout, @Nullable String stderr) {
        StringBuilder result = new StringBuilder();
        result.append(sanitizeExitCode(clientSocket, exitCode));
        result.append('\0');
        result.append(stdout != null ? stdout : "");
        result.append('\0');
        result.append(stderr != null ? stderr : "");

        // Send result to client and close output stream
        Error error = clientSocket.sendDataToOutputStream(result.toString(), true);
        if (error != null) {
            localSocketManager.onError(clientSocket, error);
        }
    }

    /**
     * Sanitize exitCode to between 0-255, otherwise it may be considered invalid.
     * Out of bound exit codes would return with exit code `44` `Channel number out of range` in shell.
     *
     * @param clientSocket The {@link LocalClientSocket} to which the exit code will be sent.
     * @param exitCode The current exit code.
     * @return Returns the sanitized exit code.
     */
    public static int sanitizeExitCode(@NonNull LocalClientSocket clientSocket, int exitCode) {
        if (exitCode < 0 || exitCode > 255) {
            Logger.logWarn(LOG_TAG, "Ignoring invalid peer "  + clientSocket.getPeerCred().getMinimalString() + " result value \"" + exitCode + "\" and force setting it to \"" + 1 + "\"");
            exitCode = 1;
        }

        return exitCode;
    }


    /**
     * Parse amCommandString into a list of arguments like normally done on shells like bourne shell.
     * Arguments are split on whitespaces unless quoted with single or double quotes.
     * Double quotes and backslashes can be escaped with backslashes in arguments surrounded.
     * Double quotes and backslashes can be escaped with backslashes in arguments surrounded with
     * double quotes.
     *
     * @param amCommandString The am command {@link String}.
     * @param amCommandList The {@link List<String>} to set list of arguments in.
     * @return Returns the {@code error} if parsing am command failed, otherwise {@code null}.
     */
    public static Error parseAmCommand(String amCommandString, List<String> amCommandList) {

        if (amCommandString == null || amCommandString.isEmpty()) {
            return null;
        }

        try {
            amCommandList.addAll(ArgumentTokenizer.tokenize(amCommandString));
        } catch (Exception e) {
            return AmSocketServerErrno.ERRNO_PARSE_AM_COMMAND_FAILED_WITH_EXCEPTION.getError(e, amCommandString, e.getMessage());
        }

        return null;
    }

    /**
     * Call termux-am-library to run the am command.
     *
     * @param context The {@link Context} to run am command with.
     * @param amCommandArray The am command array.
     * @param stdout The {@link StringBuilder} to set stdout in that is returned by the am command.
     * @param stderr The {@link StringBuilder} to set stderr in that is returned by the am command.
     * @param checkDisplayOverAppsPermission Check if {@link Manifest.permission#SYSTEM_ALERT_WINDOW}
     *                                       has been granted if running on Android `>= 10` and
     *                                       starting activity or service.
     * @return Returns the {@code error} if am command failed, otherwise {@code null}.
     */
    public static Error runAmCommand(@NonNull Context context,
                                     String[] amCommandArray,
                                     @NonNull StringBuilder stdout, @NonNull StringBuilder stderr,
                                     boolean checkDisplayOverAppsPermission) {
        try (ByteArrayOutputStream stdoutByteStream = new ByteArrayOutputStream();
             PrintStream stdoutPrintStream = new PrintStream(stdoutByteStream);
             ByteArrayOutputStream stderrByteStream = new ByteArrayOutputStream();
             PrintStream stderrPrintStream = new PrintStream(stderrByteStream)) {

            if (checkDisplayOverAppsPermission && amCommandArray.length >= 1 &&
                (amCommandArray[0].equals("start") || amCommandArray[0].equals("startservice")) &&
                !PermissionUtils.validateDisplayOverOtherAppsPermissionForPostAndroid10(context, true)) {
                throw new IllegalStateException(context.getString(R.string.error_display_over_other_apps_permission_not_granted,
                    PackageUtils.getAppNameForPackage(context)));
            }

            new Am(stdoutPrintStream, stderrPrintStream, (Application) context.getApplicationContext()).run(amCommandArray);

            // Set stdout to value set by am command in stdoutPrintStream
            stdoutPrintStream.flush();
            stdout.append(stdoutByteStream.toString(StandardCharsets.UTF_8.name()));

            // Set stderr to value set by am command in stderrPrintStream
            stderrPrintStream.flush();
            stderr.append(stderrByteStream.toString(StandardCharsets.UTF_8.name()));
        } catch (Exception e) {
            return AmSocketServerErrno.ERRNO_RUN_AM_COMMAND_FAILED_WITH_EXCEPTION.getError(e, Arrays.toString(amCommandArray), e.getMessage());
        }

        return null;
    }





    /** Implementation for {@link ILocalSocketManager} for {@link AmSocketServer}. */
    public abstract static class AmSocketServerClient extends LocalSocketManagerClientBase {

        @Override
        public void onClientAccepted(@NonNull LocalSocketManager localSocketManager,
                                     @NonNull LocalClientSocket clientSocket) {
            AmSocketServer.processAmClient(localSocketManager, clientSocket);
            super.onClientAccepted(localSocketManager, clientSocket);
        }

    }

}
