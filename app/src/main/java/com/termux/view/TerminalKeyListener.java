package com.termux.view;

import android.view.MotionEvent;
import android.view.ScaleGestureDetector;

/**
 * Input and scale listener which may be set on a {@link TerminalView} through
 * {@link TerminalView#setOnKeyListener(TerminalKeyListener)}.
 *
 * TODO: Rename to TerminalViewClient.
 */
public interface TerminalKeyListener {

	/** Callback function on scale events according to {@link ScaleGestureDetector#getScaleFactor()}. */
	float onScale(float scale);

	/** On a single tap on the terminal if terminal mouse reporting not enabled. */
	void onSingleTapUp(MotionEvent e);

	boolean shouldBackButtonBeMappedToEscape();

	void copyModeChanged(boolean copyMode);

}
