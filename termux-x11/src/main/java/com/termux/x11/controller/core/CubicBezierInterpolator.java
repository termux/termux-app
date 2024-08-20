package com.termux.x11.controller.core;

import android.graphics.PointF;
import android.view.animation.Interpolator;

public class CubicBezierInterpolator implements Interpolator {
    public final PointF start;
    public final PointF end;
    private float ax, bx, cx;

    public CubicBezierInterpolator() {
        this(new PointF(0, 0), new PointF(0, 0));
    }

    public CubicBezierInterpolator(PointF start, PointF end) {
        this.start = start;
        this.end = end;
    }

    public CubicBezierInterpolator(float x1, float y1, float x2, float y2) {
        this(new PointF(x1, y1), new PointF(x2, y2));
    }

    public void set(float x1, float y1, float x2, float y2) {
        start.set(x1, y1);
        end.set(x2, y2);
    }

    @Override
    public float getInterpolation(float time) {
        return getBezierCoordinateY(getXForTime(time));
    }

    private float getBezierCoordinateY(float time) {
        float cy = 3 * start.y;
        float by = 3 * (end.y - start.y) - cy;
        float ay = 1 - cy - by;
        return time * (cy + time * (by + time * ay));
    }

    private float getXForTime(float time) {
        float x = time;
        float z;
        for (int i = 1; i < 14; i++) {
            z = getBezierCoordinateX(x) - time;
            if (Math.abs(z) < 1e-3) break;
            x -= z / getXDerivate(x);
        }
        return x;
    }

    private float getXDerivate(float t) {
        return cx + t * (2 * bx + 3 * ax * t);
    }

    private float getBezierCoordinateX(float time) {
        cx = 3 * start.x;
        bx = 3 * (end.x - start.x) - cx;
        ax = 1 - cx - bx;
        return time * (cx + time * (bx + time * ax));
    }
}