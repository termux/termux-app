package com.termux.shared.terminal;

import android.view.KeyEvent;
import android.view.MotionEvent;

import com.termux.shared.logger.Logger;
import com.termux.terminal.TerminalSession;
import com.termux.view.TerminalViewClient;

public class TermuxTerminalViewClientBase implements TerminalViewClient {

    public TermuxTerminalViewClientBase() {
    }

    @Override
    public float onScale(float scale) {
        return 1.0f;
    }

    @Override
    public void onSingleTapUp(MotionEvent e) {
    }

    public boolean shouldBackButtonBeMappedToEscape() {
        return false;
    }

    public boolean shouldEnforceCharBasedInput() {
        return false;
    }

    public boolean shouldUseCtrlSpaceWorkaround() {
        return false;
    }

    @Override
    public boolean isTerminalViewSelected() {
        return true;
    }

    @Override
    public void copyModeChanged(boolean copyMode) {
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent e, TerminalSession session) {
        return false;
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent e) {
        return false;
    }

    @Override
    public boolean onLongPress(MotionEvent event) {
        return false;
    }

    @Override
    public boolean readControlKey() {
        return false;
    }

    @Override
    public boolean readAltKey() {
        return false;
    }

    @Override
    public boolean readShiftKey() {
        return false;
    }

    @Override
    public boolean readFnKey() {
        return false;
    }



    @Override
    public boolean onCodePoint(int codePoint, boolean ctrlDown, TerminalSession session) {
        return false;
    }

    @Override
    public void onEmulatorSet() {

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
