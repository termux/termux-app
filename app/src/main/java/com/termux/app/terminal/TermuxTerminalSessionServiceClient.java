package com.termux.app.terminal;

import android.app.Service;
import android.content.pm.PackageManager;

import androidx.annotation.NonNull;

import com.termux.app.TermuxService;
import com.termux.shared.termux.shell.command.runner.terminal.TermuxSession;
import com.termux.shared.termux.terminal.TermuxTerminalSessionClientBase;
import com.termux.terminal.TerminalSession;
import com.termux.terminal.TerminalSessionClient;

/** The {@link TerminalSessionClient} implementation that may require a {@link Service} for its interface methods. */
public class TermuxTerminalSessionServiceClient extends TermuxTerminalSessionClientBase {

    private static final String LOG_TAG = "TermuxTerminalSessionServiceClient";

    private final TermuxService mService;

    public TermuxTerminalSessionServiceClient(TermuxService service) {
        this.mService = service;
    }

    @Override
    public void setTerminalShellPid(@NonNull TerminalSession terminalSession, int pid) {
        TermuxSession termuxSession = mService.getTermuxSessionForTerminalSession(terminalSession);
        if (termuxSession != null)
            termuxSession.getExecutionCommand().mPid = pid;
    }

    @Override
    public void onSessionFinished(@NonNull TerminalSession finishedSession) {
        // If the {@link TermuxActivity} is not bound to the {@link TermuxService}, then this client
        // is used instead of the {@link TermuxTerminalSessionActivityClient}, whose
        // {@link TermuxTerminalSessionActivityClient#onSessionFinished(TerminalSession)} implementation
        // is responsible for removing finished sessions. Without this, finished sessions would never
        // be removed, plugin results would never be processed and the notification would never be
        // updated, so the service would not stop.
        //
        // Mirror the conditions of {@link TermuxTerminalSessionActivityClient#onSessionFinished(TerminalSession)}
        // except for the activity related actions like toasts and switching the current session.
        TermuxSession termuxSession = mService.getTermuxSessionForTerminalSession(finishedSession);
        boolean isPluginExecutionCommandWithPendingResult = termuxSession != null &&
            termuxSession.getExecutionCommand().isPluginExecutionCommandWithPendingResult();

        boolean shouldRemoveSession;
        if (mService.getPackageManager().hasSystemFeature(PackageManager.FEATURE_LEANBACK)) {
            // On Android TV devices we need to use older behaviour because we may
            // not be able to have multiple launcher icons.
            shouldRemoveSession = mService.getTermuxSessionsSize() > 1 || isPluginExecutionCommandWithPendingResult;
        } else {
            shouldRemoveSession = finishedSession.getExitStatus() == 0 || finishedSession.getExitStatus() == 130 || isPluginExecutionCommandWithPendingResult;
        }

        if (shouldRemoveSession)
            mService.removeTermuxSession(finishedSession);
    }

}
