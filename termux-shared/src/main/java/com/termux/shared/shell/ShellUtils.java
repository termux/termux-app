package com.termux.shared.shell;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.termux.shared.file.FileUtils;
import com.termux.terminal.TerminalBuffer;
import com.termux.terminal.TerminalEmulator;
import com.termux.terminal.TerminalSession;

import java.lang.reflect.Field;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class ShellUtils {

    /** Get process id of {@link Process}. */
    public static int getPid(Process p) {
        try {
            Field f = p.getClass().getDeclaredField("pid");
            f.setAccessible(true);
            try {
                return f.getInt(p);
            } finally {
                f.setAccessible(false);
            }
        } catch (Throwable e) {
            return -1;
        }
    }

    /** Setup shell command arguments for the execute. */
    @NonNull
    public static String[] setupShellCommandArguments(@NonNull String executable, @Nullable String[] arguments) {
        List<String> result = new ArrayList<>();
        result.add(executable);
        if (arguments != null) Collections.addAll(result, arguments);
        return result.toArray(new String[0]);
    }

    /** Get basename for executable. */
    @Nullable
    public static String getExecutableBasename(@Nullable String executable) {
        return FileUtils.getFileBasename(executable);
    }



    /** Get transcript for {@link TerminalSession}. */
    public static String getTerminalSessionTranscriptText(TerminalSession terminalSession, boolean linesJoined, boolean trim) {
        if (terminalSession == null) return null;

        TerminalEmulator terminalEmulator = terminalSession.getEmulator();
        if (terminalEmulator == null) return null;

        TerminalBuffer terminalBuffer = terminalEmulator.getScreen();
        if (terminalBuffer == null) return null;

        String transcriptText;

        if (linesJoined)
            transcriptText = terminalBuffer.getTranscriptTextWithFullLinesJoined();
        else
            transcriptText = terminalBuffer.getTranscriptTextWithoutJoinedLines();

        if (transcriptText == null) return null;

        if (trim)
            transcriptText = transcriptText.trim();

        return transcriptText;
    }

}
