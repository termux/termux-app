package com.termux.dom.animators;

/**
 * Created by andrzej on 29.01.18.
 */

import java.util.List;

import com.termux.dom.RecognitionBar;

public class IdleAnimator implements com.termux.dom.animators.BarParamsAnimator {

    private static final long IDLE_DURATION = 8500;

    private long startTimestamp;
    private boolean isPlaying;

    private final int floatingAmplitude;
    private final List<RecognitionBar> bars;

    public IdleAnimator(List<RecognitionBar> bars, int floatingAmplitude) {
        this.floatingAmplitude = floatingAmplitude;
        this.bars = bars;
    }

    @Override
    public void start() {
        isPlaying = true;
        startTimestamp = System.currentTimeMillis();
    }

    @Override
    public void stop() {
        isPlaying = false;
    }

    @Override
    public void animate() {
        if (isPlaying) {
            update(bars);
        }
    }

    public void update(List<RecognitionBar> bars) {

        long currTimestamp = System.currentTimeMillis();
        if (currTimestamp - startTimestamp > IDLE_DURATION) {
            startTimestamp += IDLE_DURATION;
        }
        long delta = currTimestamp - startTimestamp;

        int i = 0;
        for (RecognitionBar bar : bars) {
            updateCirclePosition(bar, delta, i);
            i++;
        }
    }

    private void updateCirclePosition(RecognitionBar bar, long delta, int num) {
        float angle = ((float) delta / IDLE_DURATION) * 360f + 120f * num;
        int y = (int) (Math.sin(Math.toRadians(angle)) * floatingAmplitude) + bar.getStartY();
        bar.setY(y);
        bar.update();
    }
}
