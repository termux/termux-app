package com.termux.view;

import android.content.Context;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;

/** A combination of {@link GestureDetector} and {@link ScaleGestureDetector}. */
public class GestureAndScaleRecognizer {

	public interface Listener {
		boolean onSingleTapUp(MotionEvent e);

		boolean onDoubleTap(MotionEvent e);

		boolean onScroll(MotionEvent e2, float dx, float dy);

		boolean onFling(MotionEvent e, float velocityX, float velocityY);

		boolean onScale(float focusX, float focusY, float scale);

		boolean onDown(float x, float y);

		boolean onUp(MotionEvent e);

		void onLongPress(MotionEvent e);
	}

	private final GestureDetector mGestureDetector;
	private final ScaleGestureDetector mScaleDetector;
	final Listener mListener;

	public GestureAndScaleRecognizer(Context context, Listener listener) {
		mListener = listener;

		mGestureDetector = new GestureDetector(context, new GestureDetector.SimpleOnGestureListener() {
			@Override
			public boolean onScroll(MotionEvent e1, MotionEvent e2, float dx, float dy) {
				return mListener.onScroll(e2, dx, dy);
			}

			@Override
			public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {
				return mListener.onFling(e2, velocityX, velocityY);
			}

			@Override
			public boolean onDown(MotionEvent e) {
				return mListener.onDown(e.getX(), e.getY());
			}

			@Override
			public void onLongPress(MotionEvent e) {
				mListener.onLongPress(e);
			}
		}, null, true /* ignoreMultitouch */);

		mGestureDetector.setOnDoubleTapListener(new GestureDetector.OnDoubleTapListener() {
			@Override
			public boolean onSingleTapConfirmed(MotionEvent e) {
				return mListener.onSingleTapUp(e);
			}

			@Override
			public boolean onDoubleTap(MotionEvent e) {
				return mListener.onDoubleTap(e);
			}

			@Override
			public boolean onDoubleTapEvent(MotionEvent e) {
				return true;
			}
		});

		mScaleDetector = new ScaleGestureDetector(context, new ScaleGestureDetector.SimpleOnScaleGestureListener() {
			@Override
			public boolean onScaleBegin(ScaleGestureDetector detector) {
				return true;
			}

			@Override
			public boolean onScale(ScaleGestureDetector detector) {
				return mListener.onScale(detector.getFocusX(), detector.getFocusY(), detector.getScaleFactor());
			}
		});
	}

	public void onTouchEvent(MotionEvent event) {
		mGestureDetector.onTouchEvent(event);
		mScaleDetector.onTouchEvent(event);
		if (event.getAction() == MotionEvent.ACTION_UP) {
			mListener.onUp(event);
		}
	}

	public boolean isInProgress() {
		return mScaleDetector.isInProgress();
	}

}