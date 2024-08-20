package com.termux.x11;

import android.app.Activity;
import android.app.ActivityOptions;
import android.content.Context;
import android.content.Intent;
import android.graphics.Rect;
import android.graphics.SurfaceTexture;
import android.hardware.display.DisplayManager;
import android.opengl.GLES11Ext;
import android.opengl.GLSurfaceView;
import android.os.Build;
import android.os.Bundle;
import android.os.RemoteException;
import android.util.Log;
import android.view.Display;
import android.view.KeyEvent;
import android.view.Surface;
import android.view.inputmethod.InputMethodManager;

import com.termux.x11.input.InputStub;
import com.termux.x11.utils.GLUtility;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class XrActivity extends MainActivity implements GLSurfaceView.Renderer {
    // Order of the enum has to be the same as in xrio/android.c
    public enum ControllerAxis {
        L_PITCH, L_YAW, L_ROLL, L_THUMBSTICK_X, L_THUMBSTICK_Y, L_X, L_Y, L_Z,
        R_PITCH, R_YAW, R_ROLL, R_THUMBSTICK_X, R_THUMBSTICK_Y, R_X, R_Y, R_Z,
        HMD_PITCH, HMD_YAW, HMD_ROLL, HMD_X, HMD_Y, HMD_Z, HMD_IPD
    }

    // Order of the enum has to be the same as in xrio/android.c
    public enum ControllerButton {
        L_GRIP,  L_MENU, L_THUMBSTICK_PRESS, L_THUMBSTICK_LEFT, L_THUMBSTICK_RIGHT, L_THUMBSTICK_UP, L_THUMBSTICK_DOWN, L_TRIGGER, L_X, L_Y,
        R_A, R_B, R_GRIP, R_THUMBSTICK_PRESS, R_THUMBSTICK_LEFT, R_THUMBSTICK_RIGHT, R_THUMBSTICK_UP, R_THUMBSTICK_DOWN, R_TRIGGER,
    }

    // Order of the enum has to be the same as in xrio/android.c
    public enum RenderParam {
        CANVAS_DISTANCE, IMMERSIVE, PASSTHROUGH, SBS, VIEWPORT_WIDTH, VIEWPORT_HEIGHT,
    }

    private static boolean isDeviceDetectionFinished = false;
    private static boolean isDeviceSupported = false;

    private boolean isImmersive = false;
    private boolean isSBS = false;
    private long lastEnter;
    private final float[] lastAxes = new float[ControllerAxis.values().length];
    private final boolean[] lastButtons = new boolean[ControllerButton.values().length];
    private final float[] mouse = new float[2];

    private int program;
    private int texture;
    private SurfaceTexture surface;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        GLSurfaceView view = new GLSurfaceView(this);
        view.setEGLContextClientVersion(2);
        view.setRenderer(this);
        frm.addView(view);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        teardown();

        // Going back to the Android 2D rendering isn't supported.
        // Kill the app to ensure there is no unexpected behaviour.
        System.exit(0);
    }

    public static boolean isEnabled() {
        return isSupported();
    }

    public static boolean isSupported() {
        if (!isDeviceDetectionFinished) {
            if (Build.MANUFACTURER.compareToIgnoreCase("META") == 0) {
                isDeviceSupported = true;
            }
            if (Build.MANUFACTURER.compareToIgnoreCase("OCULUS") == 0) {
                isDeviceSupported = true;
            }
            isDeviceDetectionFinished = true;
        }
        return isDeviceSupported;
    }

    public static void openIntent(Activity context) {
        // 0. Create the launch intent
        Intent intent = new Intent(context, XrActivity.class);

        // 1. Locate the main x11 ID and add that to the intent
        final int mainDisplayId = getMainDisplay(context);
        ActivityOptions options = ActivityOptions.makeBasic().setLaunchDisplayId(mainDisplayId);

        // 2. Set the flags: start in a new task
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);

        // 3. Launch the activity.
        // Don't use the container's ContextWrapper, which is adding arguments
        context.getBaseContext().startActivity(intent, options.toBundle());

        // 4. Finish the previous activity: this avoids an audio bug
        context.finish();
    }

    private static int getMainDisplay(Context context) {
        final DisplayManager displayManager =
                (DisplayManager) context.getSystemService(Context.DISPLAY_SERVICE);
        for (Display display : displayManager.getDisplays()) {
            if (display.getDisplayId() == Display.DEFAULT_DISPLAY) {
                return display.getDisplayId();
            }
        }
        return -1;
    }

    @Override
    void clientConnectedStateChanged(boolean connected) {
        if (!connected && (surface != null)) {
            teardown();

            // Going back to the Android 2D rendering isn't supported.
            // Kill the app to ensure there is no unexpected behaviour.
            System.exit(0);
        } else {
            super.clientConnectedStateChanged(connected);
        }
    }

    @Override
    public void onSurfaceCreated(GL10 gl10, EGLConfig eglConfig) {
        System.loadLibrary("XRio");
        if (isSupported()) {
            init();
        }
    }

    @Override
    public void onSurfaceChanged(GL10 gl10, int w, int h) {
        texture = GLUtility.createTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES);
        program = GLUtility.createProgram(true);
        surface = new SurfaceTexture(texture);

        runOnUiThread(() -> getLorieView().setCallback((sfc, surfaceWidth, surfaceHeight, screenWidth, screenHeight) -> {
            LorieView.sendWindowChange(screenWidth, screenHeight, 60);
            surface.setDefaultBufferSize(screenWidth, screenHeight);

            if (service != null) {
                try {
                    service.windowChanged(new Surface(surface), "OpenXR");
                } catch (RemoteException e) {
                    Log.e("XrActivity", "failed to send windowChanged request", e);
                }
            }
        }));
    }

    @Override
    public void onDrawFrame(GL10 gl10) {
        if (isSupported()) {
            setRenderParam(RenderParam.CANVAS_DISTANCE.ordinal(), 5);
            setRenderParam(RenderParam.IMMERSIVE.ordinal(), isImmersive ? 1 : 0);
            setRenderParam(RenderParam.PASSTHROUGH.ordinal(), isImmersive ? 0 : 1);
            setRenderParam(RenderParam.SBS.ordinal(), isSBS ? 1 : 0);

            if (beginFrame()) {
                renderFrame(gl10);
                finishFrame();
                processInput();
            }
        } else {
            renderFrame(gl10);
        }
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        // Meta HorizonOS sends up event only (as for in v66)
        if (event.getAction() == KeyEvent.ACTION_UP) {

            // The OS sends 4x enter event, filter it
            if (event.getKeyCode() == KeyEvent.KEYCODE_ENTER) {
                if (event.getDownTime() - lastEnter < 250) {
                    return true;
                }
                lastEnter = event.getDownTime();
            }

            // Send key press, give system chance to notice it and send key release
            getLorieView().sendKeyEvent(0, event.getKeyCode(), true);
            try {
                Thread.sleep(50);
            } catch (Exception e) {
                e.printStackTrace();
            }
            getLorieView().sendKeyEvent(0, event.getKeyCode(), false);
        }
        return true;
    }

    private void processInput() {
        float[] axes = getAxes();
        boolean[] buttons = getButtons();
        LorieView view = getLorieView();
        Rect frame = view.getHolder().getSurfaceFrame();

        // Mouse control with hand
        float meter2px = frame.width() * 10.0f;
        float dx = (axes[ControllerAxis.R_X.ordinal()] - lastAxes[ControllerAxis.R_X.ordinal()]) * meter2px;
        float dy = (axes[ControllerAxis.R_Y.ordinal()] - lastAxes[ControllerAxis.R_Y.ordinal()]) * meter2px;

        // Mouse control with head
        if (isImmersive) {
            float angle2px = frame.width() * 0.05f;
            dx = getAngleDiff(lastAxes[ControllerAxis.HMD_YAW.ordinal()], axes[ControllerAxis.HMD_YAW.ordinal()]) * angle2px;
            dy = getAngleDiff(lastAxes[ControllerAxis.HMD_PITCH.ordinal()], axes[ControllerAxis.HMD_PITCH.ordinal()]) * angle2px;
            if (Float.isNaN(dy)) {
                dy = 0;
            }
        }

        // Mouse speed adjust
        float mouseSpeed = isImmersive ? 1 : 0.25f;
        dx *= mouseSpeed;
        dy *= mouseSpeed;

        // Mouse "snap turn"
        int snapturn = 125;
        if (getButtonClicked(buttons, ControllerButton.R_THUMBSTICK_LEFT)) {
            dx = -snapturn;
        }
        if (getButtonClicked(buttons, ControllerButton.R_THUMBSTICK_RIGHT)) {
            dx = snapturn;
        }

        // Mouse smoothing
        if (isImmersive) {
            mouse[0] = dx;
            mouse[1] = dy;
        } else {
            float smoothFactor = 0.75f;
            mouse[0] = mouse[0] * smoothFactor + dx * (1 - smoothFactor);
            mouse[1] = mouse[1] * smoothFactor + dy * (1 - smoothFactor);
        }

        // Set mouse status
        int scrollStep = 150;
        view.sendMouseEvent(mouse[0], -mouse[1], 0, false, true);
        mapMouse(view, buttons, ControllerButton.R_TRIGGER, InputStub.BUTTON_LEFT);
        mapMouse(view, buttons, ControllerButton.R_GRIP, InputStub.BUTTON_RIGHT);
        mapScroll(view, buttons, ControllerButton.R_THUMBSTICK_UP, 0, -scrollStep);
        mapScroll(view, buttons, ControllerButton.R_THUMBSTICK_DOWN, 0, scrollStep);

        // Switch immersive/SBS mode
        if (getButtonClicked(buttons, ControllerButton.L_THUMBSTICK_PRESS)) {
            if (buttons[ControllerButton.R_GRIP.ordinal()]) {
                isSBS = !isSBS;
            }
            else {
                isImmersive = !isImmersive;
            }
        }

        // Show system keyboard
        if (getButtonClicked(buttons, ControllerButton.R_THUMBSTICK_PRESS)) {
            getInstance().runOnUiThread(() -> {
                isSBS = false;
                isImmersive = false;
                getWindow().getDecorView().postDelayed(() -> {
                    InputMethodManager imm = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
                    imm.toggleSoftInput(InputMethodManager.SHOW_FORCED, 0);
                }, 500L);
            });
        }

        // Update keyboard
        mapKey(view, buttons, ControllerButton.R_A, KeyEvent.KEYCODE_A);
        mapKey(view, buttons, ControllerButton.R_B, KeyEvent.KEYCODE_B);
        mapKey(view, buttons, ControllerButton.L_X, KeyEvent.KEYCODE_X);
        mapKey(view, buttons, ControllerButton.L_Y, KeyEvent.KEYCODE_Y);
        mapKey(view, buttons, ControllerButton.L_GRIP, KeyEvent.KEYCODE_SPACE);
        mapKey(view, buttons, ControllerButton.L_MENU, KeyEvent.KEYCODE_BACK);
        mapKey(view, buttons, ControllerButton.L_TRIGGER, KeyEvent.KEYCODE_ENTER);
        mapKey(view, buttons, ControllerButton.L_THUMBSTICK_LEFT, KeyEvent.KEYCODE_DPAD_LEFT);
        mapKey(view, buttons, ControllerButton.L_THUMBSTICK_RIGHT, KeyEvent.KEYCODE_DPAD_RIGHT);
        mapKey(view, buttons, ControllerButton.L_THUMBSTICK_UP, KeyEvent.KEYCODE_DPAD_UP);
        mapKey(view, buttons, ControllerButton.L_THUMBSTICK_DOWN, KeyEvent.KEYCODE_DPAD_DOWN);

        // Store the OpenXR data
        System.arraycopy(axes, 0, lastAxes, 0, axes.length);
        System.arraycopy(buttons, 0, lastButtons, 0, buttons.length);
    }

    private float getAngleDiff(float oldAngle, float newAngle) {
        float diff = oldAngle - newAngle;
        while (diff > 180) {
            diff -= 360;
        }
        while (diff < -180) {
            diff += 360;
        }
        return diff;
    }

    private boolean getButtonClicked(boolean[] buttons, ControllerButton button) {
        return buttons[button.ordinal()] && !lastButtons[button.ordinal()];
    }

    private void mapKey(LorieView v, boolean[] buttons, ControllerButton b, int keycode) {
        if (buttons[b.ordinal()] != lastButtons[b.ordinal()]) {
            v.sendKeyEvent(0, keycode, buttons[b.ordinal()]);
        }
    }

    private void mapMouse(LorieView v, boolean[] buttons, ControllerButton b, int button) {
        if (buttons[b.ordinal()] != lastButtons[b.ordinal()]) {
            v.sendMouseEvent(0, 0, button, buttons[b.ordinal()], true);
        }
    }

    private void mapScroll(LorieView v, boolean[] buttons, ControllerButton b, float x, float y) {
        if (getButtonClicked(buttons, b)) {
            v.sendMouseWheelEvent(x, y);
        }
    }

    private void renderFrame(GL10 gl10) {
        if (isSupported()) {
            int w = getRenderParam(RenderParam.VIEWPORT_WIDTH.ordinal());
            int h = getRenderParam(RenderParam.VIEWPORT_HEIGHT.ordinal());
            gl10.glViewport(0, 0, w, h);
        }

        surface.updateTexImage();
        float aspect = 1;
        if (isSupported()) {
            LorieView view = getLorieView();
            int w = view.getWidth();
            int h = view.getHeight();
            aspect = Math.min(w, h) / (float)Math.max(w, h);
        }
        GLUtility.drawTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, program, texture, -aspect);
    }

    private native void init();
    private native void teardown();
    private native boolean beginFrame();
    private native void finishFrame();
    public native float[] getAxes();
    public native boolean[] getButtons();
    public native int getRenderParam(int param);
    public native void setRenderParam(int param, int value);
}
