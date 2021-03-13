package com.termux.view;

import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;

import com.termux.terminal.TerminalSession;

/**
 * Input and scale listener which may be set on a {@link TerminalView} through
 * {@link TerminalView#setTerminalViewClient(TerminalViewClient)}.
 * <p/>
 */
public interface TerminalViewClient {

    /**
     * Callback function on scale events according to {@link ScaleGestureDetector#getScaleFactor()}.
     */
    float onScale(float scale);



    /**
     * On a single tap on the terminal if terminal mouse reporting not enabled.
     */
    void onSingleTapUp(MotionEvent e);

    boolean shouldBackButtonBeMappedToEscape();

    boolean shouldEnforeCharBasedInput();

    boolean shouldUseCtrlSpaceWorkaround();



    void copyModeChanged(boolean copyMode);



    boolean onKeyDown(int keyCode, KeyEvent e, TerminalSession session);

    boolean onKeyUp(int keyCode, KeyEvent e);

    boolean onLongPress(MotionEvent event);



    boolean readControlKey();

    boolean readAltKey();


    boolean onCodePoint(int codePoint, boolean ctrlDown, TerminalSession session);



    void logError(String tag, String message);

    void logWarn(String tag, String message);

    void logInfo(String tag, String message);

    void logDebug(String tag, String message);

    void logVerbose(String tag, String message);

    void logStackTraceWithMessage(String tag, String message, Exception e);

    void logStackTrace(String tag, Exception e);

}
