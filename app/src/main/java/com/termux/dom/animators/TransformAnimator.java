package com.termux.dom.animators;

/**
 * Created by andrzej on 29.01.18.
 */

import android.graphics.Point;

import java.util.ArrayList;
import java.util.List;

import com.termux.dom.RecognitionBar;
import com.termux.dom.RecognitionProgressView;


public class TransformAnimator implements com.termux.dom.animators.BarParamsAnimator {

    private static final long DURATION = 300;

    private long startTimestamp;
    private boolean isPlaying;


    private com.termux.dom.animators.TransformAnimator.OnInterpolationFinishedListener listener;

    private final int radius;
    private final int centerX, centerY;
    private final List<Point> finalPositions = new ArrayList<>();
    private final List<RecognitionBar> bars;

    public TransformAnimator(List<RecognitionBar> bars, int centerX, int centerY, int radius) {
        this.centerX = centerX;
        this.centerY = centerY;
        this.bars = bars;
        this.radius = radius;
    }

    @Override
    public void start() {
        isPlaying = true;
        startTimestamp = System.currentTimeMillis();
        initFinalPositions();
    }

    @Override
    public void stop() {
        isPlaying = false;
        if (listener != null) {
            listener.onFinished();
        }
    }

    @Override
    public void animate() {
        if (!isPlaying) return;

        long currTimestamp = System.currentTimeMillis();
        long delta = currTimestamp - startTimestamp;
        if (delta > DURATION) {
            delta = DURATION;
        }

        for (int i = 0; i < bars.size(); i++) {
            RecognitionBar bar = bars.get(i);

            int x = bar.getStartX() + (int) ((finalPositions.get(i).x - bar.getStartX()) * ((float) delta / DURATION));
            int y = bar.getStartY() + (int) ((finalPositions.get(i).y - bar.getStartY()) * ((float) delta / DURATION));

            bar.setX(x);
            bar.setY(y);
            bar.update();
        }


        if (delta == DURATION) {
            stop();
        }
    }

    private void initFinalPositions() {
        Point startPoint = new Point();
        startPoint.x = centerX;
        startPoint.y = centerY - radius;
        for (int i = 0; i < RecognitionProgressView.BARS_COUNT; i++) {
            Point point = new Point(startPoint);
            rotate((360d / RecognitionProgressView.BARS_COUNT) * i, point);
            finalPositions.add(point);
        }
    }

    /**
     * X = x0 + (x - x0) * cos(a) - (y - y0) * sin(a);
     * Y = y0 + (y - y0) * cos(a) + (x - x0) * sin(a);
     **/
    private void rotate(double degrees, Point point) {

        double angle = Math.toRadians(degrees);

        int x = centerX + (int) ((point.x - centerX) * Math.cos(angle) -
                (point.y - centerY) * Math.sin(angle));

        int y = centerY + (int) ((point.x - centerX) * Math.sin(angle) +
                (point.y - centerY) * Math.cos(angle));

        point.x = x;
        point.y = y;
    }

    public void setOnInterpolationFinishedListener(com.termux.dom.animators.TransformAnimator.OnInterpolationFinishedListener listener) {
        this.listener = listener;
    }

    public interface OnInterpolationFinishedListener {
        void onFinished();
    }
}
