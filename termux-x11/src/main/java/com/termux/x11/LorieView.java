package com.termux.x11;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.ClipData;
import android.content.ClipDescription;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.graphics.Point;
import android.graphics.Rect;
import android.graphics.drawable.ColorDrawable;
import android.preference.PreferenceManager;
import android.util.AttributeSet;
import android.util.Log;
import android.view.KeyEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

import com.termux.x11.controller.core.CursorLocker;
import com.termux.x11.controller.winhandler.WinHandler;
import com.termux.x11.controller.xserver.InputDeviceManager;
import com.termux.x11.controller.xserver.Keyboard;
import com.termux.x11.controller.xserver.Pointer;
import com.termux.x11.controller.xserver.XKeycode;
import com.termux.x11.input.InputStub;

import java.nio.charset.StandardCharsets;
import java.util.regex.PatternSyntaxException;

@Keep
@SuppressLint("WrongConstant")
@SuppressWarnings("deprecation")
public class LorieView extends SurfaceView implements InputStub {
    public final Keyboard keyboard = Keyboard.createKeyboard(this);
    public final Pointer pointer = new Pointer(this);
    final public InputDeviceManager inputDeviceManager = new InputDeviceManager(this);
    public ScreenInfo screenInfo;
    public CursorLocker cursorLocker;
    public WinHandler winHandler;

    public void setWinHandler(WinHandler handler) {
        this.winHandler = handler;
    }

    public WinHandler getWinHandler() {
        return winHandler;
    }


    public boolean isFullscreen() {
        return true;
    }

    public interface Callback {
        void changed(Surface sfc, int surfaceWidth, int surfaceHeight, int screenWidth, int screenHeight);
    }

    interface PixelFormat {
        int BGRA_8888 = 5; // Stands for HAL_PIXEL_FORMAT_BGRA_8888
    }

    private ClipboardManager clipboard;
    private long lastClipboardTimestamp = System.currentTimeMillis();
    private static boolean clipboardSyncEnabled = false;
    private Callback mCallback;
    private final Point p = new Point();
    private final SurfaceHolder.Callback mSurfaceCallback = new SurfaceHolder.Callback() {
        @Override
        public void surfaceCreated(@NonNull SurfaceHolder holder) {
            holder.setFormat(PixelFormat.BGRA_8888);
        }

        @Override
        public void surfaceChanged(@NonNull SurfaceHolder holder, int f, int width, int height) {
            width = getMeasuredWidth();
            height = getMeasuredHeight();

//            Log.d("SurfaceChangedListener", "Surface was changed: " + width + "x" + height);
            if (mCallback == null)
                return;

            getDimensionsFromSettings();
            mCallback.changed(holder.getSurface(), width, height, p.x, p.y);
        }

        @Override
        public void surfaceDestroyed(@NonNull SurfaceHolder holder) {
            if (mCallback != null)
                mCallback.changed(holder.getSurface(), 0, 0, 0, 0);
        }
    };

    public LorieView(Context context) {
        super(context);
        init();
    }

    public LorieView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    public LorieView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init();
    }

    @SuppressWarnings("unused")
    public LorieView(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
        init();
    }

    private void init() {
        getHolder().addCallback(mSurfaceCallback);
        clipboard = (ClipboardManager) getContext().getSystemService(Context.CLIPBOARD_SERVICE);
        screenInfo = new ScreenInfo(this);
        cursorLocker = new CursorLocker(this);
    }

    public void setCallback(Callback callback) {
        mCallback = callback;
        triggerCallback();
    }

    public void regenerate() {
        Callback callback = mCallback;
        mCallback = null;
        getHolder().setFormat(android.graphics.PixelFormat.RGBA_8888);
        mCallback = callback;

        triggerCallback();
    }

    public void triggerCallback() {
        setFocusable(true);
        setFocusableInTouchMode(true);
        requestFocus();

        setBackground(new ColorDrawable(Color.TRANSPARENT) {
            public boolean isStateful() {
                return true;
            }

            public boolean hasFocusStateSpecified() {
                return true;
            }
        });

        Rect r = getHolder().getSurfaceFrame();
        getActivity().runOnUiThread(() -> mSurfaceCallback.surfaceChanged(getHolder(), PixelFormat.BGRA_8888, r.width(), r.height()));
    }

    private Activity getActivity() {
        Context context = getContext();
        while (context instanceof ContextWrapper) {
            if (context instanceof Activity) {
                return (Activity) context;
            }
            context = ((ContextWrapper) context).getBaseContext();
        }

        throw new NullPointerException();
    }

    void getDimensionsFromSettings() {
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(getContext());
        int width = getMeasuredWidth();
        int height = getMeasuredHeight();
        int w = width;
        int h = height;
        switch (preferences.getString("displayResolutionMode", "native")) {
            case "scaled": {
                int scale = preferences.getInt("displayScale", 100);
                w = width * 100 / scale;
                h = height * 100 / scale;
                break;
            }
            case "exact": {
                String[] resolution = preferences.getString("displayResolutionExact", "1280x1024").split("x");
                w = Integer.parseInt(resolution[0]);
                h = Integer.parseInt(resolution[1]);
                break;
            }
            case "custom": {
                try {
                    String[] resolution = preferences.getString("displayResolutionCustom", "1280x1024").split("x");
                    w = Integer.parseInt(resolution[0]);
                    h = Integer.parseInt(resolution[1]);
                } catch (NumberFormatException | PatternSyntaxException ignored) {
                    w = 1280;
                    h = 1024;
                }
                break;
            }
        }

        if ((width < height && w > h) || (width > height && w < h)) {
            p.set(h, w);
            screenInfo.screenWidth = (short) h;
            screenInfo.screenHeight = (short) w;
        } else {
            p.set(w, h);
            screenInfo.screenWidth = (short) w;
            screenInfo.screenHeight = (short) h;
        }
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);

        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(getContext());
        if (preferences.getBoolean("displayStretch", false)
            || "native".equals(preferences.getString("displayResolutionMode", "native"))
            || "scaled".equals(preferences.getString("displayResolutionMode", "native"))) {
            getHolder().setSizeFromLayout();
            return;
        }

        getDimensionsFromSettings();

        if (p.x <= 0 || p.y <= 0)
            return;

        int width = getMeasuredWidth();
        int height = getMeasuredHeight();

        if ((width < height && p.x > p.y) || (width > height && p.x < p.y))
            //noinspection SuspiciousNameCombination
            p.set(p.y, p.x);

        if (width > height * p.x / p.y)
            width = height * p.x / p.y;
        else
            height = width * p.y / p.x;

        getHolder().setFixedSize(p.x, p.y);
        setMeasuredDimension(width, height);

        // In the case if old fixed surface size equals new fixed surface size surfaceChanged will not be called.
        // We should force it.
        //regenerate();
    }

    @Override
    public void sendMouseWheelEvent(float deltaX, float deltaY) {
        sendMouseEvent(deltaX, deltaY, BUTTON_SCROLL, false, true);
    }

    @Override
    public boolean dispatchKeyEventPreIme(KeyEvent event) {
        Activity a = getActivity();
        return (a instanceof MainActivity) && ((MainActivity) a).handleKey(event);
    }

    ClipboardManager.OnPrimaryClipChangedListener clipboardListener = this::handleClipboardChange;

    static void setClipboardSyncEnabled(boolean enabled) {
        clipboardSyncEnabled = enabled;
        setClipboardSyncEnabled(enabled, enabled);
    }

    // It is used in native code
    void setClipboardText(String text) {
        clipboard.setPrimaryClip(ClipData.newPlainText("X11 clipboard", text));

        // Android does not send PrimaryClipChanged event to the window which posted event
        // But in the case we are owning focus and clipboard is unchanged it will be replaced by the same value on X server side.
        // Not cool in the case if user installed some clipboard manager, clipboard content will be doubled.
        lastClipboardTimestamp = System.currentTimeMillis() + 150;
    }

    // It is used in native code
    void requestClipboard() {
        if (!clipboardSyncEnabled) {
            sendClipboardEvent("".getBytes(StandardCharsets.UTF_8));
            return;
        }

        CharSequence clip = clipboard.getText();
        if (clip != null) {
            String text = String.valueOf(clipboard.getText());
            sendClipboardEvent(text.getBytes(StandardCharsets.UTF_8));
            Log.d("CLIP", "sending clipboard contents: " + text);
        }
    }

    public void handleClipboardChange() {
        checkForClipboardChange();
    }

    public void checkForClipboardChange() {
        ClipDescription desc = clipboard.getPrimaryClipDescription();
        if (clipboardSyncEnabled && desc != null &&
            lastClipboardTimestamp < desc.getTimestamp() &&
            desc.getMimeTypeCount() == 1 &&
            desc.hasMimeType(ClipDescription.MIMETYPE_TEXT_PLAIN)) {
            lastClipboardTimestamp = desc.getTimestamp();
            sendClipboardAnnounce();
            Log.d("CLIP", "sending clipboard announce");
        }
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus)
            regenerate();

        requestFocus();

        if (clipboardSyncEnabled && hasFocus) {
            clipboard.addPrimaryClipChangedListener(clipboardListener);
            checkForClipboardChange();
        } else
            clipboard.removePrimaryClipChangedListener(clipboardListener);
    }


    public void injectPointerMove(int x, int y) {
        pointer.moveTo(x, y);
    }

    public void injectPointerMoveDelta(int dx, int dy) {
        pointer.moveDelta(dx, dy);
    }

    public void injectPointerButtonPress(Pointer.Button buttonCode) {
        pointer.setButton(buttonCode, true);
    }

    public void injectPointerButtonRelease(Pointer.Button buttonCode) {
        pointer.setButton(buttonCode, false);
    }

    public void injectKeyPress(XKeycode xKeycode) {
        injectKeyPress(xKeycode, 0);
    }

    public void injectKeyPress(XKeycode xKeycode, int keysym) {
        keyboard.setKeyPress(xKeycode.id, keysym);
    }

    public void injectKeyRelease(XKeycode xKeycode) {
        keyboard.setKeyRelease(xKeycode.id);
    }

    public void injectText(String text) {
        if (text.isEmpty()) {
            return;
        }
        sendTextEvent(text.getBytes());
    }

    static native void connect(int fd);

    native void handleXEvents();

    static native void startLogcat(int fd);

    static native void setClipboardSyncEnabled(boolean enabled, boolean ignored);

    public native void sendClipboardAnnounce();

    public native void sendClipboardEvent(byte[] text);

    static native void sendWindowChange(int width, int height, int framerate);

    public native void sendMouseEvent(float x, float y, int whichButton, boolean buttonDown, boolean relative);

    public native void sendTouchEvent(int action, int id, int x, int y);

    public native void sendStylusEvent(float x, float y, int pressure, int tiltX, int tiltY, int orientation, int buttons, boolean eraser, boolean mouseMode);

    static public native void requestStylusEnabled(boolean enabled);

    public native boolean sendKeyEvent(int scanCode, int keyCode, boolean keyDown);

    public native void sendTextEvent(byte[] text);

    public native void sendUnicodeEvent(int code);

    static {
        System.loadLibrary("Xlorie");
    }
}
