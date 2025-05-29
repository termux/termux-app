// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.termux.x11.input;

import android.content.Context;
import android.graphics.PointF;
import android.os.SystemClock;
import android.view.MotionEvent;
import android.view.ViewConfiguration;

/**
 * This interface defines the methods used to customize input handling for
 * a particular strategy.  The implementing class is responsible for sending
 * remote input events and defining implementation specific behavior.
 */
public interface InputStrategyInterface {
    /**
     * Called when a user tap has been detected.
     *
     * @param button The button value for the tap event.
     */
    void onTap(int button);

    /**
     * Called when the user has put one or more fingers down on the screen for a period of time.
     *
     * @param button The button value for the tap event.
     * @return A boolean representing whether the event was handled.
     */
    boolean onPressAndHold(int button, boolean force);

    /**
     * Called when a MotionEvent is received.  This method allows the input strategy to store or
     * react to specific MotionEvents as needed.
     *
     * @param event The original event for the current touch motion.
     */
    void onMotionEvent(MotionEvent event);

    /**
     * Called when the user is attempting to scroll/pan the remote UI.
     *
     * @param distanceX The distance moved along the x-axis.
     * @param distanceY The distance moved along the y-axis.
     */
    void onScroll(float distanceX, float distanceY);

    class NullInputStrategy implements InputStrategyInterface {
        @Override public void onTap(int button) {}
        @Override public boolean onPressAndHold(int button, boolean force) { return false; }
        @Override public void onScroll(float distanceX, float distanceY) {}
        @Override public void onMotionEvent(MotionEvent event) {}
    }

    /**
     * This class receives local touch events and translates them into the appropriate mouse based
     * events for the remote host.  The net result is that the local input method feels like a touch
     * interface but the remote host will be given mouse events to inject.
     */
    class SimulatedTouchInputStrategy implements InputStrategyInterface {
        /** Used to adjust the size of the region used for double tap detection. */
        private static final float DOUBLE_TAP_SLOP_SCALE_FACTOR = 0.25f;

        private final RenderData mRenderData;
        private final InputEventSender mInjector;

        /**
         * Stores the time of the most recent left button single tap processed.
         */
        private long mLastTapTimeInMs;

        /**
         * Stores the position of the last left button single tap processed.
         */
        private PointF mLastTapPoint;

        /**
         * The maximum distance, in pixels, between two points in order for them to be considered a
         * double tap gesture.
         */
        private final int mDoubleTapSlopSquareInPx;

        /**
         * The interval, measured in milliseconds, in which two consecutive left button taps must
         * occur in order to be considered a double tap gesture.
         */
        private final long mDoubleTapDurationInMs;

        /** Mouse-button currently held down, or BUTTON_UNDEFINED otherwise. */
        private int mHeldButton = InputStub.BUTTON_UNDEFINED;

        public SimulatedTouchInputStrategy(
            RenderData renderData, InputEventSender injector, Context context) {
            if (injector == null)
                throw new NullPointerException();
            mRenderData = renderData;
            mInjector = injector;

            mDoubleTapDurationInMs = ViewConfiguration.getDoubleTapTimeout();

            // In order to detect whether the user is attempting to double tap a target, we define a
            // region around the first point within which the second tap must occur.  The standard way
            // to do this in an Android UI (meaning a UI comprised of UI elements which conform to the
            // visual guidelines for the platform which are 'Touch Friendly') is to use the
            // getScaledDoubleTapSlop() value for checking this distance (or use a GestureDetector).
            // Our scenario is a bit different as our UI consists of an image of a remote machine where
            // the UI elements were probably designed for mouse and keyboard (meaning smaller targets)
            // and the image itself which can be zoomed to change the size of the targets.  Ths adds up
            // to the target to be invoked often being either larger or much smaller than a standard
            // Android UI element.  Our approach to this problem is to make double-tap detection
            // consistent regardless of the zoom level or remote target size so that the user can rely
            // on their muscle memory when interacting with our UI.  With respect to the original
            // problem, getScaledDoubleTapSlop() gives a value which is optimized for an Android based
            // UI however this value is too large for interacting with remote elements in our app.
            // Our solution is to use the original value from getScaledDoubleTapSlop() (which includes
            // scaling to account for display differences between devices) and apply a fudge/scale
            // factor to make the interaction more intuitive and useful for our scenario.
            ViewConfiguration config = ViewConfiguration.get(context);
            int scaledDoubleTapSlopInPx = config.getScaledDoubleTapSlop();
            scaledDoubleTapSlopInPx = (int) (scaledDoubleTapSlopInPx * DOUBLE_TAP_SLOP_SCALE_FACTOR);
            mDoubleTapSlopSquareInPx = scaledDoubleTapSlopInPx * scaledDoubleTapSlopInPx;
        }

        @Override
        public void onTap(int button) {
            PointF currentTapPoint = mRenderData.getCursorPosition();
            if (button == InputStub.BUTTON_LEFT) {
                // Left clicks are handled a little differently than the events for other buttons.
                // This is needed because translating touch events to mouse events has a problem with
                // location consistency for double clicks.  If you take the center location of each tap
                // and inject them as mouse clicks, the distance between those two points will often
                // cause the remote OS to recognize the gesture as two distinct clicks instead of a
                // double click.  In order to increase the success rate of double taps/clicks, we
                // squirrel away the time and coordinates of each single tap and if we detect the user
                // attempting a double tap, we use the original event's location for that second tap.
                long tapInterval = SystemClock.uptimeMillis() - mLastTapTimeInMs;
                if (isDoubleTap(currentTapPoint.x, currentTapPoint.y, tapInterval)) {
                    mLastTapPoint = null;
                    mLastTapTimeInMs = 0;
                } else {
                    mLastTapPoint = currentTapPoint;
                    mLastTapTimeInMs = SystemClock.uptimeMillis();
                }
            } else {
                mLastTapPoint = null;
                mLastTapTimeInMs = 0;
            }

            mInjector.sendMouseClick(button, false);
        }

        @Override
        public boolean onPressAndHold(int button, boolean force) {
            mInjector.sendMouseDown(button, false);
            mHeldButton = button;
            return true;
        }

        @Override
        public void onScroll(float distanceX, float distanceY) {
            mInjector.sendMouseWheelEvent(distanceX, distanceY);
        }

        @Override
        public void onMotionEvent(MotionEvent event) {
            if (event.getActionMasked() == MotionEvent.ACTION_UP && mHeldButton != InputStub.BUTTON_UNDEFINED) {
                mInjector.sendMouseUp(mHeldButton, false);
                mHeldButton = InputStub.BUTTON_UNDEFINED;
            }
        }

        private boolean isDoubleTap(float currentX, float currentY, long tapInterval) {
            if (tapInterval > mDoubleTapDurationInMs || mLastTapPoint == null) {
                return false;
            }

            // Convert the image based coordinates back to screen coordinates so the user experiences
            // consistent double tap behavior regardless of zoom level.
            //
            float[] currentValues = {currentX * mRenderData.scale.x, currentY * mRenderData.scale.y};
            float[] previousValues = {mLastTapPoint.x * mRenderData.scale.x, mLastTapPoint.y * mRenderData.scale.y};

            int deltaX = (int) (currentValues[0] - previousValues[0]);
            int deltaY = (int) (currentValues[1] - previousValues[1]);
            return ((deltaX * deltaX + deltaY * deltaY) <= mDoubleTapSlopSquareInPx);
        }
    }

    /**
     * Defines a set of behavior and methods to simulate trackpad behavior when responding to
     * local input event data.  This class is also responsible for forwarding input event data
     * to the remote host for injection there.
     */
    class TrackpadInputStrategy implements InputStrategyInterface {
        private final InputEventSender mInjector;

        /** Mouse-button currently held down, or BUTTON_UNDEFINED otherwise. */
        private int mHeldButton = InputStub.BUTTON_UNDEFINED;

        public TrackpadInputStrategy(InputEventSender injector) {
            if ((mInjector = injector) == null)
                throw new NullPointerException();
        }

        @Override
        public void onTap(int button) {
            mInjector.sendMouseClick(button, true);
        }

        @Override
        public boolean onPressAndHold(int button, boolean force) {
            if (mInjector.tapToMove && !force)
                return false;

            mInjector.sendMouseDown(button, true);
            mHeldButton = button;
            return true;
        }

        @Override
        public void onScroll(float distanceX, float distanceY) {
            mInjector.sendMouseWheelEvent(distanceX, distanceY);
        }

        @Override
        public void onMotionEvent(MotionEvent event) {
            if (event.getActionMasked() == MotionEvent.ACTION_UP && mHeldButton != InputStub.BUTTON_UNDEFINED) {
                mInjector.sendMouseUp(mHeldButton, true);
                mHeldButton = InputStub.BUTTON_UNDEFINED;
            }
        }
    }
}
