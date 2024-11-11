package com.termux.x11;

import static android.Manifest.permission.WRITE_SECURE_SETTINGS;
import static android.content.pm.PackageManager.PERMISSION_GRANTED;
import static android.os.Build.VERSION.SDK_INT;
import static android.view.InputDevice.KEYBOARD_TYPE_ALPHABETIC;
import static android.view.KeyEvent.ACTION_UP;
import static android.view.KeyEvent.KEYCODE_BACK;
import static android.view.KeyEvent.KEYCODE_META_LEFT;
import static android.view.KeyEvent.KEYCODE_META_RIGHT;
import static android.view.KeyEvent.KEYCODE_VOLUME_DOWN;
import static android.view.WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS;
import static android.view.WindowManager.LayoutParams.FLAG_FULLSCREEN;
import static android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON;
import static android.view.WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS;
import static android.view.WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS;
import static android.view.WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_DEFAULT;
import static android.view.WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
import static android.view.WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN;
import static android.view.WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE;
import static android.view.WindowManager.LayoutParams.SOFT_INPUT_STATE_HIDDEN;
import static com.termux.x11.CmdEntryPoint.ACTION_START;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.AppOpsManager;
import android.content.BroadcastReceiver;
import android.content.ClipData;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.graphics.Canvas;
import android.graphics.Color;
import android.os.Build;
import android.os.Build.VERSION_CODES;
import android.os.Bundle;
import android.os.IBinder;
import android.os.ParcelFileDescriptor;
import android.os.RemoteException;
import android.os.SystemClock;
import android.preference.PreferenceManager;
import android.provider.Settings;
import android.util.Log;
import android.view.Display;
import android.view.DragEvent;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.PointerIcon;
import android.view.Surface;
import android.view.View;
import android.view.ViewConfiguration;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.ImageButton;
import android.widget.LinearLayout;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AlertDialog;
import androidx.core.math.MathUtils;
import androidx.viewpager.widget.ViewPager;

import com.termux.x11.controller.container.Container;
import com.termux.x11.controller.container.Shortcut;
import com.termux.x11.controller.inputcontrols.InputControlsManager;
import com.termux.x11.controller.widget.InputControlsView;
import com.termux.x11.controller.widget.TouchpadView;
import com.termux.x11.controller.winhandler.TaskManagerDialog;
import com.termux.x11.controller.winhandler.WinHandler;
import com.termux.x11.input.InputEventSender;
import com.termux.x11.input.InputStub;
import com.termux.x11.input.TouchInputHandler;
import com.termux.x11.input.TouchInputHandler.RenderStub;
import com.termux.x11.utils.FullscreenWorkaround;
import com.termux.x11.utils.KeyInterceptor;
import com.termux.x11.utils.SamsungDexUtils;
import com.termux.x11.utils.TermuxX11ExtraKeys;
import com.termux.x11.utils.X11ToolbarViewPager;

import java.io.File;
import java.util.Map;
import java.util.Objects;
import java.util.concurrent.Executors;

@SuppressLint("ApplySharedPref")
@SuppressWarnings({"deprecation", "unused"})
public class MainActivity extends LoriePreferences implements View.OnApplyWindowInsetsListener {
    static final String ACTION_STOP = "com.termux.x11.ACTION_STOP";
    static final String REQUEST_LAUNCH_EXTERNAL_DISPLAY = "request_launch_external_display";
    public TermuxX11ExtraKeys mExtraKeys;
    protected boolean inputControllerViewHandled = false;
    protected FrameLayout frm;
    protected View lorieContentView;
    protected TouchInputHandler mInputHandler;
    protected ICmdEntryInterface service = null;
    private final int mNotificationId = 7893;
    private boolean mClientConnected = false;
    private View.OnKeyListener mLorieKeyListener;
    private boolean filterOutWinKey = false;
    private static final int KEY_BACK = 158;
    protected static boolean hasInit = false;
    protected boolean mEnableFloatBallMenu = false;

    private final BroadcastReceiver receiver = new BroadcastReceiver() {
        @SuppressLint("UnspecifiedRegisterReceiverFlag")
        @Override
        public void onReceive(Context context, Intent intent) {
            if (ACTION_START.equals(intent.getAction())) {
//                mClientStartedFromShell = true;
                try {
                    Log.v("LorieBroadcastReceiver", "Got new ACTION_START intent");
                    IBinder b = Objects.requireNonNull(intent.getBundleExtra("")).getBinder("");
                    service = ICmdEntryInterface.Stub.asInterface(b);
                    Objects.requireNonNull(service).asBinder().linkToDeath(() -> {
                        service = null;
                        CmdEntryPoint.requestConnection();

                        Log.v("Lorie", "Disconnected");
                        runOnUiThread(() -> clientConnectedStateChanged(false)); //recreate()); //onPreferencesChanged(""));
                    }, 0);
                    onReceiveConnection();
                } catch (Exception e) {
                    Log.e("MainActivity", "Something went wrong while we extracted connection details from binder.", e);
                }
            } else if (ACTION_STOP.equals(intent.getAction())) {
                finishAffinity();
            } else if (ACTION_PREFERENCES_CHANGED.equals(intent.getAction())) {
//                Log.d("MainActivity", "preference: " + intent.getStringExtra("key"));
                if (!"showAdditionalKbd".equals(intent.getStringExtra("key"))) {
                    onPreferencesChanged("");
                } else {
                    toggleExtraKeys(true, false);
                }
            }
        }
    };

    @SuppressLint("StaticFieldLeak")
    private static MainActivity instance;
    protected boolean mRaiseSoftKeyBoard = false;


    public MainActivity() {
        instance = this;
    }

    public static MainActivity getInstance() {
        return instance;
    }

    @Override
    @SuppressLint({"AppCompatMethod", "ObsoleteSdkInt", "ClickableViewAccessibility", "WrongConstant", "UnspecifiedRegisterReceiverFlag", "ResourceType", "MissingInflatedId"})
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(this);
        int modeValue = Integer.parseInt(preferences.getString("touchMode", "1")) - 1;
        if (modeValue > 2) {
            SharedPreferences.Editor e = Objects.requireNonNull(preferences).edit();
            e.putString("touchMode", "1");
            e.apply();
        }

//        preferences.registerOnSharedPreferenceChangeListener((sharedPreferences, key) -> onPreferencesChanged(key));
        setContentView(R.layout.main_activity);
        lorieContentView = findViewById(R.id.id_display_window);

        frm = findViewById(R.id.frame);
        findViewById(R.id.preferences_button).setOnClickListener((l) -> {
            if (null != termuxActivityListener) {
                termuxActivityListener.onX11PreferenceSwitchChange(true);
            }
        });
        LorieView lorieView = findViewById(R.id.lorieView);
        View lorieParent = (View) lorieView.getParent();
//        Log.d("Mainactivity","frm==lorieParent:"+String.valueOf(frm==lorieParent));

        mInputHandler = new TouchInputHandler(this, new RenderStub.NullStub() {
            @Override
            public void swipeDown() {
            }
        }, new InputEventSender(lorieView));
        int touch_sensitivity = preferences.getInt("touch_sensitivity", 1);
        mInputHandler.setLongPressedDelay(touch_sensitivity);
//        Log.d("MainActivity","touch_sensitivity:"+touch_sensitivity);
        mLorieKeyListener = (v, k, e) -> {
            if (k == KEYCODE_VOLUME_DOWN && preferences.getBoolean("hideEKOnVolDown", false)) {
                if (e.getAction() == ACTION_UP) {
                    toggleExtraKeys();
                }
                return true;
            }

            if (k == KEYCODE_BACK) {
                if (!e.isFromSource(InputDevice.SOURCE_MOUSE)) {
                    if (mEnableFloatBallMenu && mRaiseSoftKeyBoard) {
                        switchSoftKeyboard(false);
                    } else if (null != termuxActivityListener && !mEnableFloatBallMenu) {
                        termuxActivityListener.releaseSlider(true);
                        handler.postDelayed(new Runnable() {
                            @Override
                            public void run() {
                                loriePreferenceFragment.updatePreferencesLayout();
                            }
                        }, 500);
                    }
                }

                if (e.isFromSource(InputDevice.SOURCE_MOUSE) || e.isFromSource(InputDevice.SOURCE_MOUSE_RELATIVE)) {
                    if (e.getRepeatCount() != 0) // ignore auto-repeat
                        return true;
                    if (e.getAction() == KeyEvent.ACTION_UP || e.getAction() == KeyEvent.ACTION_DOWN)
                        lorieView.sendMouseEvent(-1, -1, InputStub.BUTTON_RIGHT, e.getAction() == KeyEvent.ACTION_DOWN, true);
                    return true;
                }

                if (e.getScanCode() == KEY_BACK && e.getDevice().getKeyboardType() != KEYBOARD_TYPE_ALPHABETIC || e.getScanCode() == 0) {
//                    if (e.getAction() == ACTION_UP)
//                        toggleKeyboardVisibility(MainActivity.this);
                    return true;
                }
            }
            return mInputHandler.sendKeyEvent(v, e);
        };
        lorieParent.setOnTouchListener((v, event) -> true);
//        lorieParent.setOnTouchListener((v, e) -> mInputHandler.handleTouchEvent(lorieParent, lorieView, e));
//        lorieParent.setOnHoverListener((v, e) -> mInputHandler.handleTouchEvent(lorieParent, lorieView, e));
//        lorieParent.setOnGenericMotionListener((v, e) -> mInputHandler.handleTouchEvent(lorieParent, lorieView, e));
//        lorieView.setOnCapturedPointerListener((v, e) -> mInputHandler.handleTouchEvent(lorieView, lorieView, e));
//        lorieParent.setOnCapturedPointerListener((v, e) -> mInputHandler.handleTouchEvent(lorieView, lorieView, e));
        lorieView.setOnHoverListener((v, e) -> mInputHandler.handleTouchEvent(lorieParent, lorieView, e));
        lorieView.setOnKeyListener(mLorieKeyListener);

        lorieView.setCallback((sfc, surfaceWidth, surfaceHeight, screenWidth, screenHeight) -> {
            int framerate = (int) ((lorieView.getDisplay() != null) ? lorieView.getDisplay().getRefreshRate() : 30);

            mInputHandler.handleHostSizeChanged(surfaceWidth, surfaceHeight);
            mInputHandler.handleClientSizeChanged(screenWidth, screenHeight);
            lorieView.screenInfo.handleHostSizeChanged(surfaceWidth, surfaceHeight);
            lorieView.screenInfo.handleClientSizeChanged(screenWidth, screenHeight);
            LorieView.sendWindowChange(screenWidth, screenHeight, framerate);

            if (service != null) {
                try {
                    String name;
                    if (lorieView.getDisplay() == null || lorieView.getDisplay().getDisplayId() == Display.DEFAULT_DISPLAY)
                        name = "Builtin Display";
                    else if (SamsungDexUtils.checkDeXEnabled(this))
                        name = "Dex Display";
                    else
                        name = "External Display";
                    service.windowChanged(sfc, name);
                } catch (RemoteException e) {
                    e.printStackTrace();
                }
            }
        });

        registerReceiver(receiver, new IntentFilter(ACTION_START) {{
            addAction(ACTION_PREFERENCES_CHANGED);
            addAction(ACTION_STOP);
        }}, SDK_INT >= VERSION_CODES.TIRAMISU ? RECEIVER_EXPORTED : 0);

        // Taken from Stackoverflow answer https://stackoverflow.com/questions/7417123/android-how-to-adjust-layout-in-full-screen-mode-when-softkeyboard-is-visible/7509285#
        FullscreenWorkaround.assistActivity(this);

        CmdEntryPoint.requestConnection();
        onPreferencesChanged("");

        toggleExtraKeys(false, false);
        checkXEvents();

        initStylusAuxButtons();
        initMouseAuxButtons();
        setupInputController();
//        inputControlsView.setOnHoverListener((v, e) -> {
//            int[] view0Location = new int[2];
//            int[] viewLocation = new int[2];
//
//            lorieParent.getLocationOnScreen(view0Location);
//            lorieView.getLocationOnScreen(viewLocation);
//
//            int offsetX = viewLocation[0] - view0Location[0];
//            int offsetY = viewLocation[1] - view0Location[1];
//            xServer.pointer.moveTo((int) (e.getRawX()-offsetX), (int) (e.getRawY()-offsetY));
//            return false;
//        });

        if (SDK_INT >= VERSION_CODES.TIRAMISU
            && checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS) != PERMISSION_GRANTED
            && !shouldShowRequestPermissionRationale(Manifest.permission.POST_NOTIFICATIONS)) {
            requestPermissions(new String[]{Manifest.permission.POST_NOTIFICATIONS}, 0);
        }
        winHandler = new WinHandler(this);
        lorieView.setWinHandler(winHandler);
        Executors.newSingleThreadExecutor().execute(() -> {
            winHandler.start();
        });
    }

    @Override
    protected void onDestroy() {
        winHandler.stop();
        unregisterReceiver(receiver);
        super.onDestroy();
    }

    private void setupInputController() {
        xServer = getLorieView();
        globalCursorSpeed = 1.0f;
        touchpadView = new TouchpadView(this, xServer);
        touchpadView.setSensitivity(globalCursorSpeed);
        touchpadView.setVisibility(View.GONE);
//        touchpadView.setBackground(getDrawable(R.drawable.touchpad_background));
        frm.addView(touchpadView);

        inputControlsView = new InputControlsView(this);
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(getBaseContext());
        inputControlsView.setOverlayOpacity(preferences.getFloat("overlay_opacity", InputControlsView.DEFAULT_OVERLAY_OPACITY));
        inputControlsView.setTouchpadView(touchpadView);
        inputControlsView.setXServer(xServer);
        inputControlsView.setVisibility(View.GONE);
        frm.addView(inputControlsView);
        inputControlsManager = new InputControlsManager(this);
        String shortcutPath = getIntent().getStringExtra("shortcut_path");
        container = new Container(0);
        if (shortcutPath != null && !shortcutPath.isEmpty())
            shortcut = new Shortcut(container, new File(shortcutPath));

    }

    //Register the needed events to handle stylus as left, middle and right click
    @SuppressLint("ClickableViewAccessibility")
    private void initStylusAuxButtons() {
        SharedPreferences p = PreferenceManager.getDefaultSharedPreferences(this);
        boolean stylusMenuEnabled = p.getBoolean("showStylusClickOverride", false);
        final float menuUnselectedTrasparency = 0.66f;
        final float menuSelectedTrasparency = 1.0f;
        Button left = findViewById(R.id.button_left_click);
        Button right = findViewById(R.id.button_right_click);
        Button middle = findViewById(R.id.button_middle_click);
        Button visibility = findViewById(R.id.button_visibility);
        LinearLayout overlay = findViewById(R.id.mouse_helper_visibility);
        LinearLayout buttons = findViewById(R.id.mouse_helper_secondary_layer);
        overlay.setOnTouchListener((v, e) -> true);
        overlay.setOnHoverListener((v, e) -> true);
        overlay.setOnGenericMotionListener((v, e) -> true);
        overlay.setOnCapturedPointerListener((v, e) -> true);
        overlay.setVisibility(stylusMenuEnabled ? View.VISIBLE : View.GONE);
        View.OnClickListener listener = view -> {
            TouchInputHandler.STYLUS_INPUT_HELPER_MODE = (view.equals(left) ? 1 : (view.equals(middle) ? 2 : (view.equals(right) ? 3 : 0)));
            left.setAlpha((TouchInputHandler.STYLUS_INPUT_HELPER_MODE == 1) ? menuSelectedTrasparency : menuUnselectedTrasparency);
            middle.setAlpha((TouchInputHandler.STYLUS_INPUT_HELPER_MODE == 2) ? menuSelectedTrasparency : menuUnselectedTrasparency);
            right.setAlpha((TouchInputHandler.STYLUS_INPUT_HELPER_MODE == 3) ? menuSelectedTrasparency : menuUnselectedTrasparency);
            visibility.setAlpha(menuUnselectedTrasparency);
        };

        left.setOnClickListener(listener);
        middle.setOnClickListener(listener);
        right.setOnClickListener(listener);

        visibility.setOnClickListener(view -> {
            if (buttons.getVisibility() == View.VISIBLE) {
                buttons.setVisibility(View.GONE);
                visibility.setAlpha(menuUnselectedTrasparency);
                int m = TouchInputHandler.STYLUS_INPUT_HELPER_MODE;
                visibility.setText(m == 1 ? "L" : (m == 2 ? "M" : (m == 3 ? "R" : "U")));
            } else {
                buttons.setVisibility(View.VISIBLE);
                visibility.setAlpha(menuUnselectedTrasparency);
                visibility.setText("X");

                //Calculate screen border making sure btn is fully inside the view
                float maxX = frm.getWidth() - 4 * left.getWidth();
                float maxY = frm.getHeight() - 4 * left.getHeight();

                //Make sure the Stylus menu is fully inside the screen
                overlay.setX(MathUtils.clamp(overlay.getX(), 0, maxX));
                overlay.setY(MathUtils.clamp(overlay.getY(), 0, maxY));

                int m = TouchInputHandler.STYLUS_INPUT_HELPER_MODE;
                listener.onClick(m == 1 ? left : (m == 2 ? middle : (m == 3 ? right : left)));
            }
        });
        //Simulated mouse click 1 = left , 2 = middle , 3 = right
        TouchInputHandler.STYLUS_INPUT_HELPER_MODE = 1;
        listener.onClick(left);

        visibility.setOnLongClickListener(v -> {
            v.startDragAndDrop(ClipData.newPlainText("", ""), new View.DragShadowBuilder(visibility) {
                public void onDrawShadow(Canvas canvas) {
                }
            }, null, View.DRAG_FLAG_GLOBAL);

            frm.setOnDragListener((v2, event) -> {
                //Calculate screen border making sure btn is fully inside the view
                float maxX = frm.getWidth() - visibility.getWidth();
                float maxY = frm.getHeight() - visibility.getHeight();

                switch (event.getAction()) {
                    case DragEvent.ACTION_DRAG_LOCATION:
                        //Center touch location with btn icon
                        float dX = event.getX() - visibility.getWidth() / 2.0f;
                        float dY = event.getY() - visibility.getHeight() / 2.0f;

                        //Make sure the dragged btn is inside the view with clamp
                        overlay.setX(MathUtils.clamp(dX, 0, maxX));
                        overlay.setY(MathUtils.clamp(dY, 0, maxY));
                        break;
                    case DragEvent.ACTION_DRAG_ENDED:
                        //Make sure the dragged btn is inside the view
                        overlay.setX(MathUtils.clamp(overlay.getX(), 0, maxX));
                        overlay.setY(MathUtils.clamp(overlay.getY(), 0, maxY));
                        break;
                }
                return true;
            });

            return true;
        });
    }

    void setSize(View v, int width, int height) {
        ViewGroup.LayoutParams p = v.getLayoutParams();
        p.width = (int) (width * getResources().getDisplayMetrics().density);
        p.height = (int) (height * getResources().getDisplayMetrics().density);
        v.setLayoutParams(p);
        v.setMinimumWidth((int) (width * getResources().getDisplayMetrics().density));
        v.setMinimumHeight((int) (height * getResources().getDisplayMetrics().density));
    }

    @SuppressLint("ClickableViewAccessibility")
    void initMouseAuxButtons() {
        Button left = findViewById(R.id.mouse_button_left_click);
        Button right = findViewById(R.id.mouse_button_right_click);
        Button middle = findViewById(R.id.mouse_button_middle_click);
        ImageButton pos = findViewById(R.id.mouse_buttons_position);
        LinearLayout primaryLayer = findViewById(R.id.mouse_buttons);
        LinearLayout secondaryLayer = findViewById(R.id.mouse_buttons_secondary_layer);

        SharedPreferences p = PreferenceManager.getDefaultSharedPreferences(this);
        boolean mouseHelperEnabled = p.getBoolean("showMouseHelper", false) && "1".equals(p.getString("touchMode", "1"));
        primaryLayer.setVisibility(mouseHelperEnabled ? View.VISIBLE : View.GONE);

        pos.setOnClickListener((v) -> {
            if (secondaryLayer.getOrientation() == LinearLayout.HORIZONTAL) {
                setSize(left, 48, 96);
                setSize(right, 48, 96);
                secondaryLayer.setOrientation(LinearLayout.VERTICAL);
            } else {
                setSize(left, 96, 48);
                setSize(right, 96, 48);
                secondaryLayer.setOrientation(LinearLayout.HORIZONTAL);
            }
            handler.postDelayed(() -> {
                int[] offset = new int[2];
                frm.getLocationOnScreen(offset);
                primaryLayer.setX(MathUtils.clamp(primaryLayer.getX(), offset[0], offset[0] + frm.getWidth() - primaryLayer.getWidth()));
                primaryLayer.setY(MathUtils.clamp(primaryLayer.getY(), offset[1], offset[1] + frm.getHeight() - primaryLayer.getHeight()));
            }, 10);
        });

        Map.of(left, InputStub.BUTTON_LEFT, middle, InputStub.BUTTON_MIDDLE, right, InputStub.BUTTON_RIGHT)
            .forEach((v, b) -> v.setOnTouchListener((__, e) -> {
                switch (e.getAction()) {
                    case MotionEvent.ACTION_DOWN:
                    case MotionEvent.ACTION_POINTER_DOWN:
                        getLorieView().sendMouseEvent(0, 0, b, true, true);
                        v.setPressed(true);
                        break;
                    case MotionEvent.ACTION_UP:
                    case MotionEvent.ACTION_POINTER_UP:
                        getLorieView().sendMouseEvent(0, 0, b, false, true);
                        v.setPressed(false);
                        break;
                }
                return true;
            }));

        pos.setOnTouchListener(new View.OnTouchListener() {
            final int touchSlop = (int) Math.pow(ViewConfiguration.get(MainActivity.this).getScaledTouchSlop(), 2);
            final int tapTimeout = ViewConfiguration.getTapTimeout();
            final float[] startOffset = new float[2];
            final int[] startPosition = new int[2];
            long startTime;

            @Override
            public boolean onTouch(View v, MotionEvent e) {
                switch (e.getAction()) {
                    case MotionEvent.ACTION_DOWN:
                        primaryLayer.getLocationOnScreen(startPosition);
                        startOffset[0] = e.getX();
                        startOffset[1] = e.getY();
                        startTime = SystemClock.uptimeMillis();
                        pos.setPressed(true);
                        break;
                    case MotionEvent.ACTION_MOVE: {
                        int[] offset = new int[2];
                        int[] offset2 = new int[2];
                        primaryLayer.getLocationOnScreen(offset);
                        frm.getLocationOnScreen(offset2);
                        primaryLayer.setX(MathUtils.clamp(offset[0] - startOffset[0] + e.getX(), offset2[0], offset2[0] + frm.getWidth() - primaryLayer.getWidth()));
                        primaryLayer.setY(MathUtils.clamp(offset[1] - startOffset[1] + e.getY(), offset2[1], offset2[1] + frm.getHeight() - primaryLayer.getHeight()));
                        break;
                    }
                    case MotionEvent.ACTION_UP: {
                        final int[] _pos = new int[2];
                        primaryLayer.getLocationOnScreen(_pos);
                        int deltaX = (int) (startOffset[0] - e.getX()) + (startPosition[0] - _pos[0]);
                        int deltaY = (int) (startOffset[1] - e.getY()) + (startPosition[1] - _pos[1]);
                        pos.setPressed(false);

                        if (deltaX * deltaX + deltaY * deltaY < touchSlop && SystemClock.uptimeMillis() - startTime <= tapTimeout) {
                            v.performClick();
                            return true;
                        }
                        break;
                    }
                }
                return true;
            }
        });
    }

    void onReceiveConnection() {
        try {
            if (service != null && service.asBinder().isBinderAlive()) {
                Log.v("LorieBroadcastReceiver", "Extracting logcat fd.");
                ParcelFileDescriptor logcatOutput = service.getLogcatOutput();
                if (logcatOutput != null)
                    LorieView.startLogcat(logcatOutput.detachFd());

                tryConnect();
            }
        } catch (Exception e) {
            Log.e("MainActivity", "Something went wrong while we were establishing connection", e);
        }
    }

    void tryConnect() {
        if (mClientConnected)
            return;
        try {
            Log.v("LorieBroadcastReceiver", "Extracting X connection socket.");
            ParcelFileDescriptor fd = service == null ? null : service.getXConnection();
            if (fd != null) {
                LorieView.connect(fd.detachFd());
                getLorieView().triggerCallback();
                clientConnectedStateChanged(true);
                LorieView.setClipboardSyncEnabled(PreferenceManager.getDefaultSharedPreferences(this).getBoolean("clipboardSync", false));
            } else {
                handler.postDelayed(this::tryConnect, 500);
                Log.v("LorieBroadcastReceiver", "Try Connect.");
            }
        } catch (Exception e) {
            Log.e("MainActivity", "Something went wrong while we were establishing connection", e);
            service = null;

            // We should reset the View for the case if we have sent it's surface to the client.
            getLorieView().regenerate();
        }
    }

    public void setX11FocusedChanged(boolean x11Focused) {
        FullscreenWorkaround.setX11Focused(x11Focused);
    }

    protected void onPreferencesChanged(String key) {
        boolean startFresh = false;
        if ("additionalKbdVisible".equals(key) ||
            "showAdditionalKbd".contentEquals(key)) {
            toggleExtraKeys(true, false);
        }
        if ("showMouseHelper".contentEquals(key) ||
            "forceLandscape".contentEquals(key) ||
            "fullscreen".contentEquals(key) ||
            "hideCutout".contentEquals(key)) {
            startFresh = true;
        }

        SharedPreferences p = PreferenceManager.getDefaultSharedPreferences(this);
        LorieView lorieView = getLorieView();

        switch (key) {
            case "enableGlobalFloatBallMenu":
            case "enableFloatBallMenu": {
                boolean enableGlobalFloatBallMenu = p.getBoolean("enableGlobalFloatBallMenu", false);
                mEnableFloatBallMenu = p.getBoolean("enableFloatBallMenu", false);
                if (termuxActivityListener != null) {
                    termuxActivityListener.setFloatBallMenu(mEnableFloatBallMenu, enableGlobalFloatBallMenu);
                    if (!mEnableFloatBallMenu) {
                        mRaiseSoftKeyBoard = false;
                    }
                }
                break;
            }
            case "touchMode": {
                int mode = Integer.parseInt(p.getString("touchMode", "1"));
                mInputHandler.setInputMode(mode);
                break;
            }
            case "tapToMove": {
                mInputHandler.setTapToMove(p.getBoolean("tapToMove", false));
                break;
            }
            case "preferScancodes": {
                mInputHandler.setPreferScancodes(p.getBoolean("preferScancodes", false));
                break;
            }
            case "pointerCapture": {
                mInputHandler.setPointerCaptureEnabled(p.getBoolean("pointerCapture", false));
                if (!p.getBoolean("pointerCapture", false) && lorieView.hasPointerCapture())
                    lorieView.releasePointerCapture();
                break;
            }
            case "scaleTouchpad": {
                mInputHandler.setApplyDisplayScaleFactorToTouchpad(p.getBoolean("scaleTouchpad", true));
                break;
            }
            case "touch_sensitivity": {
                mInputHandler.setLongPressedDelay(p.getInt("touch_sensitivity", 1));
                break;
            }
            case "dexMetaKeyCapture": {
                SamsungDexUtils.dexMetaKeyCapture(this, p.getBoolean("dexMetaKeyCapture", false));
                break;
            }
            case "filterOutWinkey": {
                filterOutWinKey = p.getBoolean("filterOutWinkey", false);
                break;
            }
            case "enableAccessibilityServiceAutomatically": {
                if (p.getBoolean("enableAccessibilityServiceAutomatically", false)) {
                    try {
                        Settings.Secure.putString(getContentResolver(), Settings.Secure.ENABLED_ACCESSIBILITY_SERVICES, "com.termux.x11/.utils.KeyInterceptor");
                        Settings.Secure.putString(getContentResolver(), Settings.Secure.ACCESSIBILITY_ENABLED, "1");
                    } catch (SecurityException e) {
                        new AlertDialog.Builder(this)
                            .setTitle("Permission denied")
                            .setMessage("Android requires WRITE_SECURE_SETTINGS permission to start accessibility service automatically.\n" +
                                "Please, launch this command using ADB:\n" +
                                "adb shell pm grant com.termux.x11 android.permission.WRITE_SECURE_SETTINGS")
                            .setNegativeButton("OK", null)
                            .create()
                            .show();

                        SharedPreferences.Editor edit = p.edit();
                        edit.putBoolean("enableAccessibilityServiceAutomatically", false);
                        edit.commit();
                    }
                } else if (checkSelfPermission(WRITE_SECURE_SETTINGS) == PERMISSION_GRANTED)
                    KeyInterceptor.shutdown();
                break;
            }
            case "clipboardSync": {
                LorieView.setClipboardSyncEnabled(p.getBoolean("clipboardSync", false));
                break;
            }
            case "forceLandscape": {
                int requestedOrientation = p.getBoolean("forceLandscape", false) ?
                    ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE : ActivityInfo.SCREEN_ORIENTATION_PORTRAIT;

                if (getRequestedOrientation() != requestedOrientation) {
                    setRequestedOrientation(requestedOrientation);
                    if (null != termuxActivityListener) {
                        termuxActivityListener.onChangeOrientation(requestedOrientation);
                        getLorieView().regenerate();
                    }
                }
                break;
            }
            case "showStylusClickOverride":
            case "showMouseHelper": {
                findViewById(R.id.mouse_buttons).setVisibility(p.getBoolean("showMouseHelper", false) && "1".equals(p.getString("touchMode", "1")) && mClientConnected ? View.VISIBLE : View.GONE);
                LinearLayout buttons = findViewById(R.id.mouse_helper_visibility);
                if (p.getBoolean("showStylusClickOverride", false)) {
                    buttons.setVisibility(View.VISIBLE);
                } else {
                    //Reset default input back to normal
                    TouchInputHandler.STYLUS_INPUT_HELPER_MODE = 1;
                    final float menuUnselectedTrasparency = 0.66f;
                    final float menuSelectedTrasparency = 1.0f;
                    findViewById(R.id.button_left_click).setAlpha(menuSelectedTrasparency);
                    findViewById(R.id.button_right_click).setAlpha(menuUnselectedTrasparency);
                    findViewById(R.id.button_middle_click).setAlpha(menuUnselectedTrasparency);
                    findViewById(R.id.button_visibility).setAlpha(menuUnselectedTrasparency);
                    buttons.setVisibility(View.GONE);
                }
                break;
            }
        }
        setTerminalToolbarView();
        if (startFresh) {
            onWindowFocusChanged(true);
        }
        lorieView.triggerCallback();
    }

    @Override
    public void onResume() {
        super.onResume();

        setTerminalToolbarView();
        getLorieView().requestFocus();
    }

    @Override
    public void onPause() {
        super.onPause();
    }

    public LorieView getLorieView() {
        return findViewById(R.id.lorieView);
    }

    public ViewPager getDisplayTerminalToolbarViewPager() {
        return findViewById(R.id.display_terminal_toolbar_view_pager);
    }

    private void setTerminalToolbarView() {
        final ViewPager terminalToolbarViewPager = getDisplayTerminalToolbarViewPager();

        terminalToolbarViewPager.setAdapter(new X11ToolbarViewPager.PageAdapter(this, (v, k, e) -> mInputHandler.sendKeyEvent(getLorieView(), e)));
        terminalToolbarViewPager.addOnPageChangeListener(new X11ToolbarViewPager.OnPageChangeListener(this, terminalToolbarViewPager));

        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(this);
        boolean enabled = preferences.getBoolean("showAdditionalKbd", true);
        boolean showNow = enabled && preferences.getBoolean("additionalKbdVisible", true);

        terminalToolbarViewPager.setVisibility(showNow ? View.VISIBLE : View.GONE);
        findViewById(com.termux.x11.R.id.display_terminal_toolbar_view_pager).requestFocus();

        handler.postDelayed(() -> {
            if (mExtraKeys != null) {
                ViewGroup.LayoutParams layoutParams = terminalToolbarViewPager.getLayoutParams();
                layoutParams.height = Math.round(37.5f * getResources().getDisplayMetrics().density *
                    (mExtraKeys.getExtraKeysInfo() == null ? 0 : mExtraKeys.getExtraKeysInfo().getMatrix().length));
                terminalToolbarViewPager.setLayoutParams(layoutParams);
            }
        }, 200);
    }

    public void toggleExtraKeys(boolean visible, boolean saveState) {
        runOnUiThread(() -> {
            SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(this);
            boolean enabled = preferences.getBoolean("showAdditionalKbd", true);
            ViewPager pager = getDisplayTerminalToolbarViewPager();
            ViewGroup parent = (ViewGroup) pager.getParent();
            boolean show = enabled && mClientConnected && visible;

            if (show) {
                setTerminalToolbarView();
                getDisplayTerminalToolbarViewPager().bringToFront();
            } else {
                parent.removeView(pager);
                parent.addView(pager, 0);
            }

            if (enabled && saveState) {
                SharedPreferences.Editor edit = preferences.edit();
                edit.putBoolean("additionalKbdVisible", show);
                edit.commit();
            }

            pager.setVisibility(show ? View.VISIBLE : View.INVISIBLE);

            getLorieView().requestFocus();
        });
    }

    public void toggleExtraKeys() {
        int visibility = getDisplayTerminalToolbarViewPager().getVisibility();
        toggleExtraKeys(visibility != View.VISIBLE, true);
        getLorieView().requestFocus();
    }

    public boolean handleKey(KeyEvent e) {
        if (filterOutWinKey && (e.getKeyCode() == KEYCODE_META_LEFT || e.getKeyCode() == KEYCODE_META_RIGHT || e.isMetaPressed()))
            return false;
        mLorieKeyListener.onKey(getLorieView(), e.getKeyCode(), e);
        return true;
    }

//    int orientation;

    @Override
    public void onConfigurationChanged(@NonNull Configuration newConfig) {
        super.onConfigurationChanged(newConfig);

        if (newConfig.orientation != orientation) {
            switchSoftKeyboard(true);
        }

        orientation = newConfig.orientation;
        if (termuxActivityListener != null) {
            SharedPreferences p = PreferenceManager.getDefaultSharedPreferences(this);
            boolean forceLandscape = p.getBoolean("forceLandscape", false);
            if (!forceLandscape) {
                termuxActivityListener.onChangeOrientation(newConfig.orientation);
            } else {
                termuxActivityListener.onChangeOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
            }
            handler.postDelayed(() -> {
                getLorieView().regenerate();
            }, 1000);
        }
        setTerminalToolbarView();
//        Log.d("onConfigurationChanged","orientation:"+orientation);
    }

    public int getOrientation() {
        WindowManager windowManager = (WindowManager) getSystemService(WINDOW_SERVICE);
        Display display = windowManager.getDefaultDisplay();
        int rotation = display.getRotation();

        switch (rotation) {
            case Surface.ROTATION_90:
            case Surface.ROTATION_270:
                return ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE;
            default:
                return ActivityInfo.SCREEN_ORIENTATION_PORTRAIT;
        }
    }

    @SuppressLint("RestrictedApi")
    public void switchSoftKeyboard(boolean hide) {
        InputMethodManager imm = (InputMethodManager) getSystemService(Activity.INPUT_METHOD_SERVICE);
        View view = getCurrentFocus();
        if (view == null) {
            view = getLorieView();
            view.requestFocus();
        }

        if (hide) {
            imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
        } else {
            if (null != termuxActivityListener) {
                handler.postDelayed(() -> loriePreferenceFragment.updatePreferencesLayout(), 500);
                termuxActivityListener.onX11PreferenceSwitchChange(false);
            }
            imm.showSoftInput(view, 0);
        }
    }

    public void openSoftKeyboardWithBackKeyPressed(boolean raiseSoftKeyBoard) {
        mRaiseSoftKeyBoard = raiseSoftKeyBoard;
    }

    @SuppressLint("WrongConstant")
    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);

        SharedPreferences p = PreferenceManager.getDefaultSharedPreferences(this);
        Window window = getWindow();
        View decorView = window.getDecorView();
        boolean fullscreen = p.getBoolean("fullscreen", false);
        boolean reseed = p.getBoolean("Reseed", true);

        Intent intent = getIntent();
        fullscreen = fullscreen || (null != intent && intent.getBooleanExtra(REQUEST_LAUNCH_EXTERNAL_DISPLAY, false));

        int requestedOrientation = p.getBoolean("forceLandscape", false) ?
            ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE : ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED;
        if (getRequestedOrientation() != requestedOrientation)
            setRequestedOrientation(requestedOrientation);
//        if (getOrientation() != requestedOrientation)
//            setRequestedOrientation(requestedOrientation);
        if (hasFocus) {
            if (SDK_INT >= VERSION_CODES.P) {
                if (p.getBoolean("hideCutout", false)) {
                    getWindow().getAttributes().layoutInDisplayCutoutMode = (SDK_INT >= VERSION_CODES.R) ?
                        LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS :
                        LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
                } else {
                    getWindow().getAttributes().layoutInDisplayCutoutMode = LAYOUT_IN_DISPLAY_CUTOUT_MODE_DEFAULT;
                }
            }

            window.setStatusBarColor(Color.BLACK);
            window.setNavigationBarColor(Color.BLACK);
        }

        window.setFlags(FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS | FLAG_KEEP_SCREEN_ON | FLAG_TRANSLUCENT_STATUS, 0);
        if (hasFocus) {
            if (fullscreen) {
                window.addFlags(FLAG_FULLSCREEN);
                decorView.setSystemUiVisibility(
                    View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                        | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
            } else {
                window.clearFlags(FLAG_FULLSCREEN);
                decorView.setSystemUiVisibility(0);
            }
        }

        if (p.getBoolean("keepScreenOn", true))
            window.addFlags(FLAG_KEEP_SCREEN_ON);
        else
            window.clearFlags(FLAG_KEEP_SCREEN_ON);
        window.setSoftInputMode((reseed ? SOFT_INPUT_ADJUST_RESIZE : SOFT_INPUT_ADJUST_PAN) | SOFT_INPUT_STATE_HIDDEN);
        ((FrameLayout) findViewById(R.id.id_display_window)).getChildAt(0).setFitsSystemWindows(!fullscreen);
        SamsungDexUtils.dexMetaKeyCapture(this, hasFocus && p.getBoolean("dexMetaKeyCapture", false));

        if (hasFocus) {
            getLorieView().regenerate();
            getLorieView().requestLayout();
        }
        getLorieView().requestFocus();
    }

    public static boolean hasPipPermission(@NonNull Context context) {
        AppOpsManager appOpsManager = (AppOpsManager) context.getSystemService(Context.APP_OPS_SERVICE);
        if (appOpsManager == null)
            return false;
        else if (Build.VERSION.SDK_INT >= VERSION_CODES.Q)
            return appOpsManager.unsafeCheckOpNoThrow(AppOpsManager.OPSTR_PICTURE_IN_PICTURE, android.os.Process.myUid(), context.getPackageName()) == AppOpsManager.MODE_ALLOWED;
        else
            return appOpsManager.checkOpNoThrow(AppOpsManager.OPSTR_PICTURE_IN_PICTURE, android.os.Process.myUid(), context.getPackageName()) == AppOpsManager.MODE_ALLOWED;
    }

    @Override
    public void onUserLeaveHint() {
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(this);
        if (preferences.getBoolean("PIP", false) && hasPipPermission(this)) {
            enterPictureInPictureMode();
        }
    }

    @Override
    public void onPictureInPictureModeChanged(boolean isInPictureInPictureMode, @NonNull Configuration newConfig) {
        toggleExtraKeys(!isInPictureInPictureMode, false);

        frm.setPadding(0, 0, 0, 0);
        super.onPictureInPictureModeChanged(isInPictureInPictureMode, newConfig);
    }

    /**
     * @noinspection NullableProblems
     */
    @SuppressLint("WrongConstant")
    @Override
    public WindowInsets onApplyWindowInsets(View v, WindowInsets insets) {
        handler.postDelayed(() -> getLorieView().triggerCallback(), 100);
        return insets;
    }

    /**
     * Manually toggle soft keyboard visibility
     *
     * @param context calling context
     */
    public static void toggleKeyboardVisibility(Context context) {
        InputMethodManager inputMethodManager = (InputMethodManager) context.getSystemService(Context.INPUT_METHOD_SERVICE);
        Log.d("MainActivity", "Toggling keyboard visibility");
        if (inputMethodManager != null)
            inputMethodManager.toggleSoftInput(InputMethodManager.SHOW_FORCED, 0);
    }

    @SuppressWarnings("SameParameterValue")
    void clientConnectedStateChanged(boolean connected) {
        runOnUiThread(() -> {
            SharedPreferences p = PreferenceManager.getDefaultSharedPreferences(this);
            mClientConnected = connected;
            toggleExtraKeys(connected && p.getBoolean("additionalKbdVisible", true), true);
            findViewById(R.id.mouse_buttons).setVisibility(p.getBoolean("showMouseHelper", false) && "1".equals(p.getString("touchMode", "1")) && mClientConnected ? View.VISIBLE : View.GONE);
            findViewById(R.id.stub).setVisibility(connected ? View.INVISIBLE : View.VISIBLE);
            getLorieView().setVisibility(connected ? View.VISIBLE : View.INVISIBLE);
            getLorieView().regenerate();

            // We should recover connection in the case if file descriptor for some reason was broken...
            if (!connected)
                tryConnect();

            if (connected)
                getLorieView().setPointerIcon(PointerIcon.getSystemIcon(this, PointerIcon.TYPE_NULL));
        });
    }

    private void checkXEvents() {
        if (!mClientConnected) {
            getLorieView().handleXEvents();
        }
        handler.postDelayed(this::checkXEvents, 100);
    }

    public void showProcessManagerDialog() {
        (new TaskManagerDialog(this)).show();
    }
}
