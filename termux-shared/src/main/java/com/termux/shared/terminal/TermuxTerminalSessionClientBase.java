package com.termux.shared.terminal;

import com.termux.shared.logger.Logger;
import com.termux.terminal.TerminalSession;
import com.termux.terminal.TerminalSessionClient;

public class TermuxTerminalSessionClientBase implements TerminalSessionClient {

    public TermuxTerminalSessionClientBase() {
    }

    @Override
    public void onTextChanged(TerminalSession changedSession) {
    }

    @Override
    public void onTitleChanged(TerminalSession updatedSession) {
    }

    @Override
    public void onSessionFinished(final TerminalSession finishedSession) {
    }

    @Override
    public void onCopyTextToClipboard(TerminalSession session, String text) {
    }

    @Override
    public void onPasteTextFromClipboard(TerminalSession session) {
    }

    @Override
    public void onBell(TerminalSession session) {
    }

    @Override
    public void onColorsChanged(TerminalSession changedSession) {
    }

    @Override
    public void onTerminalCursorStateChange(boolean state) {
    }



    @Override
    public Integer getTerminalCursorStyle() {
        return null;
    }



    @Override
    public void logError(String tag, String message) {
        Logger.logError(tag, message);
    }

    @Override
    public void logWarn(String tag, String message) {
        Logger.logWarn(tag, message);
    }

    @Override
    public void logInfo(String tag, String message) {
        Logger.logInfo(tag, message);
    }

    @Override
    public void logDebug(String tag, String message) {
        Logger.logDebug(tag, message);
    }

    @Override
    public void logVerbose(String tag, String message) {
        Logger.logVerbose(tag, message);
    }

    @Override
    public void logStackTraceWithMessage(String tag, String message, Exception e) {
        Logger.logStackTraceWithMessage(tag, message, e);
    }

    @Override
    public void logStackTrace(String tag, Exception e) {
        Logger.logStackTrace(tag, e);
    }

}
