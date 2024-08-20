package com.termux.x11.utils;

import android.accessibilityservice.AccessibilityService;
import android.util.Log;
import android.view.KeyEvent;
import android.view.accessibility.AccessibilityEvent;

import com.termux.x11.MainActivity;
import com.termux.x11.R;

import java.util.LinkedHashSet;

public class KeyInterceptor extends AccessibilityService {
    LinkedHashSet<Integer> pressedKeys = new LinkedHashSet<>();

    private static KeyInterceptor self;

    public KeyInterceptor() {
        self = this;
    }

    public static void shutdown() {
        if (self != null) {
            self.disableSelf();
            self.pressedKeys.clear();
            self = null;
        }
    }

    @Override
    public boolean onKeyEvent(KeyEvent event) {
        boolean ret = false;
        MainActivity instance = MainActivity.getInstance();

        if (instance == null)
            return false;

        boolean intercept = instance.hasWindowFocus()
                && (instance.findViewById(R.id.display_terminal_toolbar_text_input) == null
                || !instance.findViewById(R.id.display_terminal_toolbar_text_input).isFocused());

        if (intercept || (event.getAction() == KeyEvent.ACTION_UP && pressedKeys.contains(event.getKeyCode())))
            ret = instance.handleKey(event);

        if (intercept && event.getAction() == KeyEvent.ACTION_DOWN)
            pressedKeys.add(event.getKeyCode());
        else
        // We should send key releases to activity for the case if user was pressing some keys when Activity lost focus.
        // I.e. if user switched window with Win+Tab or if he was pressing Ctrl while switching activity.
        if (event.getAction() == KeyEvent.ACTION_UP)
            pressedKeys.remove(event.getKeyCode());

        Log.d("KeyInterceptor", "" + (event.getUnicodeChar() != 0 ? (char) event.getUnicodeChar() : "") + " " + (event.getCharacters() != null ? event.getCharacters() : "") + " " + (ret ? " " : " not ") + "intercepted event " + event);

        return ret;
    }

    @Override
    public void onAccessibilityEvent(AccessibilityEvent e) {
        // Disable self if it is automatically started on device boot or when activity finishes.
        if (MainActivity.getInstance() == null || MainActivity.getInstance().isFinishing()) {
            android.util.Log.d("KeyInterceptor", "finishing");
            shutdown();
        }
    }

    @Override
    public void onInterrupt() {
    }
}
