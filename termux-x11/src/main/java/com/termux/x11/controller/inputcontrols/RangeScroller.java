package com.termux.x11.controller.inputcontrols;

import android.graphics.Rect;


import com.termux.x11.controller.widget.InputControlsView;

import java.util.Timer;
import java.util.TimerTask;

public class RangeScroller {
    private final InputControlsView inputControlsView;
    private final ControlElement element;
    private float scrollOffset;
    private float currentOffset;
    private float lastPosition;
    private long touchTime;
    private Binding binding = Binding.NONE;
    private boolean isActionDown = false;
    private boolean scrolling = false;
    private Timer timer;

    public RangeScroller(InputControlsView inputControlsView, ControlElement element) {
        this.inputControlsView = inputControlsView;
        this.element = element;
    }

    public float getElementSize() {
        Rect boundingBox = element.getBoundingBox();
        return (float)Math.max(boundingBox.width(), boundingBox.height()) / element.getBindingCount();
    }

    public float getScrollSize() {
        return getElementSize() * element.getRange().max;
    }

    public float getScrollOffset() {
        return scrollOffset;
    }

    public byte[] getRangeIndex() {
        ControlElement.Range range = element.getRange();
        byte from = (byte)Math.floor((scrollOffset / getElementSize()) % range.max);
        if (from < 0) from = (byte)(range.max + from);
        byte to = (byte)(from + element.getBindingCount() + 1);
        return new byte[]{from, to};
    }

    private Binding getBindingByPosition(float x, float y) {
        Rect boundingBox = element.getBoundingBox();
        ControlElement.Range range = element.getRange();
        float offset = element.getOrientation() == 0 ? x - boundingBox.left - currentOffset : y - boundingBox.top - currentOffset;
        int index = (int)Math.floor((offset / getElementSize()) % range.max);
        if (index < 0) index = range.max + index;

        switch (range) {
            case FROM_A_TO_Z:
                return Binding.valueOf("KEY_"+((char)(65 + index)));
            case FROM_0_TO_9:
                return Binding.valueOf("KEY_"+((index + 1) % 10));
            case FROM_F1_TO_F12:
                return Binding.valueOf("KEY_F"+(index + 1));
            case FROM_NP0_TO_NP9:
                return Binding.valueOf("KEY_KP_"+((index + 1) % 10));
            default:
                return Binding.NONE;
        }
    }

    private boolean isTap() {
        return (System.currentTimeMillis() - touchTime) < inputControlsView.MAX_TAP_MILLISECONDS;
    }

    private void destroyTimer() {
        if (timer != null) {
            timer.cancel();
            timer = null;
        }
    }

    public void handleTouchDown(float x, float y) {
        destroyTimer();

        scrolling = false;
        isActionDown = true;
        binding = getBindingByPosition(x, y);
        touchTime = System.currentTimeMillis();
        lastPosition = element.getOrientation() == 0 ? x : y;
        element.setBinding(Binding.NONE);

        timer = new Timer(true);
        timer.schedule(new TimerTask() {
            @Override
            public void run() {
                if (!scrolling) inputControlsView.post(() -> inputControlsView.handleInputEvent(binding, true));
            }
        }, inputControlsView.MAX_TAP_MILLISECONDS);
    }

    public void handleTouchMove(float x, float y) {
        if (isActionDown) {
            float position = element.getOrientation() == 0 ? x : y;
            float deltaPosition = position - lastPosition;

            if (Math.abs(deltaPosition) >= inputControlsView.MAX_TAP_TRAVEL_DISTANCE) {
                scrolling = true;
                destroyTimer();
            }

            if (scrolling) {
                currentOffset += deltaPosition;

                float scrollSize = getScrollSize();
                scrollOffset = -currentOffset % scrollSize;
                if (scrollOffset < 0) scrollOffset = scrollSize + scrollOffset;

                lastPosition = position;
                inputControlsView.invalidate();
            }
        }
    }

    public void handleTouchUp() {
        if (isActionDown) {
            destroyTimer();
            if (isTap() && !scrolling) {
                inputControlsView.handleInputEvent(binding, true);
                final Binding finalBinding = binding;
                inputControlsView.postDelayed(() -> inputControlsView.handleInputEvent(finalBinding, false), 30);
            }
            else inputControlsView.handleInputEvent(binding, false);
        }
        isActionDown = false;
    }
}
