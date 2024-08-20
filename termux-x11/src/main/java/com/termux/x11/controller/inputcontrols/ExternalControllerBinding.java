package com.termux.x11.controller.inputcontrols;

import android.view.KeyEvent;
import android.view.MotionEvent;

import androidx.annotation.NonNull;

import org.json.JSONException;
import org.json.JSONObject;

public class ExternalControllerBinding {
    public static final byte AXIS_X_NEGATIVE = -1;
    public static final byte AXIS_X_POSITIVE = -2;
    public static final byte AXIS_Y_NEGATIVE = -3;
    public static final byte AXIS_Y_POSITIVE = -4;
    public static final byte AXIS_Z_NEGATIVE = -5;
    public static final byte AXIS_Z_POSITIVE = -6;
    public static final byte AXIS_RZ_NEGATIVE = -7;
    public static final byte AXIS_RZ_POSITIVE = -8;
    private short keyCode;
    private Binding binding = Binding.NONE;

    public int getKeyCodeForAxis() {
        return keyCode;
    }

    public void setKeyCode(int keyCode) {
        this.keyCode = (short)keyCode;
    }

    public Binding getBinding() {
        return binding;
    }

    public void setBinding(Binding binding) {
        this.binding = binding;
    }

    public JSONObject toJSONObject() {
        try {
            JSONObject controllerBindingJSONObject = new JSONObject();
            controllerBindingJSONObject.put("keyCode", keyCode);
            controllerBindingJSONObject.put("binding", binding.name());
            return controllerBindingJSONObject;
        }
        catch (JSONException e) {
            return null;
        }
    }

    @NonNull
    @Override
    public String toString() {
        switch (keyCode) {
            case AXIS_X_NEGATIVE:
                return "AXIS X-";
            case AXIS_X_POSITIVE:
                return "AXIS X+";
            case AXIS_Y_NEGATIVE:
                return "AXIS Y-";
            case AXIS_Y_POSITIVE:
                return "AXIS Y+";
            case AXIS_Z_NEGATIVE:
                return "AXIS Z-";
            case AXIS_Z_POSITIVE:
                return "AXIS Z+";
            case AXIS_RZ_NEGATIVE:
                return "AXIS RZ-";
            case AXIS_RZ_POSITIVE:
                return "AXIS RZ+";
            default:
                return KeyEvent.keyCodeToString(keyCode).replace("KEYCODE_", "").replace("_", " ");
        }
    }

    public static int getKeyCodeForAxis(int axis, byte sign) {
        switch (axis) {
            case MotionEvent.AXIS_X:
                return sign > 0 ? AXIS_X_POSITIVE : AXIS_X_NEGATIVE;
            case MotionEvent.AXIS_Y:
                return sign > 0 ? AXIS_Y_NEGATIVE : AXIS_Y_POSITIVE;
            case MotionEvent.AXIS_Z:
                return sign > 0 ? AXIS_Z_POSITIVE : AXIS_Z_NEGATIVE;
            case MotionEvent.AXIS_RZ:
                return sign > 0 ? AXIS_RZ_NEGATIVE : AXIS_RZ_POSITIVE;
            case MotionEvent.AXIS_HAT_X:
                return sign > 0 ? KeyEvent.KEYCODE_DPAD_RIGHT : KeyEvent.KEYCODE_DPAD_LEFT;
            case MotionEvent.AXIS_HAT_Y:
                return sign > 0 ? KeyEvent.KEYCODE_DPAD_DOWN : KeyEvent.KEYCODE_DPAD_UP;
            default:
                return KeyEvent.KEYCODE_UNKNOWN;
        }
    }
}
