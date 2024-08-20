package com.termux.x11.controller.math;

public class XForm {
    private static final float[] tmpXForm = XForm.getInstance();

    public static float[] getInstance() {
        return identity(new float[6]);
    }

    public static float[] identity(float[] xform) {
        return set(xform,
            1, 0,
            0, 1,
            0, 0
        );
    }

    public static float[] set(float[] xform, float n11, float n12, float n21, float n22, float dx, float dy) {
        xform[0] = n11; xform[1] = n12;
        xform[2] = n21; xform[3] = n22;
        xform[4] =  dx; xform[5] =  dy;
        return xform;
    }

    public static float[] set(float[] xform, float tx, float ty, float sx, float sy) {
        xform[0] = sx; xform[1] =  0;
        xform[2] =  0; xform[3] = sy;
        xform[4] = tx; xform[5] = ty;
        return xform;
    }

    public static float[] makeTransform(float[] xform, float tx, float ty, float sx, float sy, float angle) {
        float c = (float)Math.cos(angle);
        float s = (float)Math.sin(angle);
        return set(xform,
            sx *  c,  sy * s,
            sx * -s,  sy * c,
            tx,  ty
        );
    }

    public static float[] makeTranslation(float[] xform, float x, float y) {
        return set(xform,
            1, 0,
            0, 1,
            x, y
        );
    }

    public static float[] makeScale(float[] xform, float x, float y) {
        return set(xform,
            x, 0,
            0, y,
            0, 0
        );
    }

    public static float[] makeRotation(float[] xform, float angle) {
        float c = (float)Math.cos(angle);
        float s = (float)Math.sin(angle);

        return set(xform,
             c,  s,
            -s,  c,
             0,  0
        );
    }

    public static synchronized float[] translate(float[] xform, float x, float y) {
        return multiply(xform, xform, makeTranslation(tmpXForm, x, y));
    }

    public static synchronized float[] scale(float[] xform, float x, float y) {
        return multiply(xform, xform, makeScale(tmpXForm, x, y));
    }

    public static synchronized float[] rotate(float[] xform, float angle) {
        return multiply(xform, xform, makeRotation(tmpXForm, angle));
    }

    public static float[] multiply(float[] result, float[] ta, float[] tb) {
        float a0 = ta[0], a3 = ta[3],
              a1 = ta[1], a4 = ta[4],
              a2 = ta[2], a5 = ta[5],
              b0 = tb[0], b3 = tb[3],
              b1 = tb[1], b4 = tb[4],
              b2 = tb[2], b5 = tb[5];
        result[0] = a0 * b0 + a1 * b2;
        result[1] = a0 * b1 + a1 * b3;
        result[2] = a2 * b0 + a3 * b2;
        result[3] = a2 * b1 + a3 * b3;
        result[4] = a4 * b0 + a5 * b2 + b4;
        result[5] = a4 * b1 + a5 * b3 + b5;
        return result;
    }

    public static float[] transformPoint(float[] xform, float x, float y) {
        return transformPoint(xform, x, y, new float[2]);
    }

    public static float[] transformPoint(float[] xform, float x, float y, float[] result) {
        result[0] = xform[0] * x + xform[2] * y + xform[4];
        result[1] = xform[1] * x + xform[3] * y + xform[5];
        return result;
    }
}
