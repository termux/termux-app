package com.termux.display.utils;

import static com.termux.display.extrakeys.TermuxX11ExtraKeysConstants.PRIMARY_KEY_CODES_FOR_STRINGS;
import static com.termux.display.MainActivity.toggleKeyboardVisibility;
import static java.nio.charset.StandardCharsets.UTF_8;

import android.annotation.SuppressLint;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.text.TextUtils;
import android.util.Log;
import android.view.KeyCharacterMap;
import android.view.KeyEvent;
import android.view.View;
import android.widget.Button;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.viewpager.widget.ViewPager;

import com.termux.display.extrakeys.TermuxX11ExtraKeysConstants;
import com.termux.display.extrakeys.TermuxX11ExtraKeysInfo;
import com.termux.display.extrakeys.*;
import com.termux.display.LoriePreferences;
import com.termux.display.MainActivity;
import com.termux.display.R;

import org.json.JSONException;

public class TermuxX11ExtraKeys implements TermuxExtraKeysView.IExtraKeysView {
    @SuppressWarnings("FieldCanBeLocal")
    private final String LOG_TAG = "TermuxX11ExtraKeys";
    private final View.OnKeyListener mEventListener;
    private final MainActivity mActivity;
    private final TermuxExtraKeysView mTermuxExtraKeysView;
    private TermuxX11ExtraKeysInfo mTermuxX11ExtraKeysInfo;

    private boolean ctrlDown;
    private boolean altDown;
    private boolean shiftDown;
    private boolean metaDown;

    /** Defines the key for extra keys */
    public static final String DEFAULT_IVALUE_EXTRA_KEYS = "[['ESC','/',{key: '-', popup: '|'},'HOME','UP','END','PGUP'], ['TAB','CTRL','ALT','LEFT','DOWN','RIGHT','PGDN']]"; // Double row

    public TermuxX11ExtraKeys(@NonNull View.OnKeyListener eventlistener, MainActivity activity, TermuxExtraKeysView extrakeysview) {
        mEventListener = eventlistener;
        mActivity = activity;
        mTermuxExtraKeysView = extrakeysview;
    }

    private final KeyCharacterMap mVirtualKeyboardKeyCharacterMap = KeyCharacterMap.load(KeyCharacterMap.VIRTUAL_KEYBOARD);
    static final String ACTION_START_PREFERENCES_ACTIVITY = "com.termux.x11.start_preferences_activity";

    @Override
    public void onExtraKeyButtonClick(View view, TermuxX11ExtraKeyButton buttonInfo, Button button) {
        Log.e("keys", "key " + buttonInfo.display);
        if (buttonInfo.macro) {
            String[] keys = buttonInfo.key.split(" ");
            boolean ctrlDown = false;
            boolean altDown = false;
            boolean shiftDown = false;
            boolean metaDown = false;
            boolean fnDown = false;
            for (String key : keys) {
                if (TermuxX11SpecialButton.CTRL.getKey().equals(key)) {
                    ctrlDown = true;
                } else if (TermuxX11SpecialButton.ALT.getKey().equals(key)) {
                    altDown = true;
                } else if (TermuxX11SpecialButton.SHIFT.getKey().equals(key)) {
                    shiftDown = true;
                } else if (TermuxX11SpecialButton.META.getKey().equals(key)) {
                    metaDown = true;
                } else if (TermuxX11SpecialButton.FN.getKey().equals(key)) {
                    fnDown = true;
                } else {
                    ctrlDown = false;
                    altDown = false;
                    shiftDown = false;
                    metaDown = false;
                    fnDown = false;
                }
                onLorieExtraKeyButtonClick(view, key, ctrlDown, altDown, shiftDown, metaDown, fnDown);
            }
        } else {
            onLorieExtraKeyButtonClick(view, buttonInfo.key, false, false, false, false, false);
        }
    }

    protected void onTerminalExtraKeyButtonClick(@SuppressWarnings("unused") View view, String key, boolean ctrlDown, boolean altDown, boolean shiftDown, boolean metaDown, @SuppressWarnings("unused") boolean fnDown) {
        if (this.ctrlDown != ctrlDown) {
            this.ctrlDown = ctrlDown;
            mActivity.getLorieView().sendKeyEvent(0, KeyEvent.KEYCODE_CTRL_LEFT, ctrlDown);
        }

        if (this.altDown != altDown) {
            this.altDown = altDown;
            mActivity.getLorieView().sendKeyEvent(0, KeyEvent.KEYCODE_ALT_LEFT, altDown);
        }

        if (this.shiftDown != shiftDown) {
            this.shiftDown = shiftDown;
            mActivity.getLorieView().sendKeyEvent(0, KeyEvent.KEYCODE_SHIFT_LEFT, shiftDown);
        }

        if (this.metaDown != metaDown) {
            this.metaDown = metaDown;
            mActivity.getLorieView().sendKeyEvent(0, KeyEvent.KEYCODE_META_LEFT, metaDown);
        }

        if (PRIMARY_KEY_CODES_FOR_STRINGS.containsKey(key)) {
            Integer keyCode = PRIMARY_KEY_CODES_FOR_STRINGS.get(key);
            if (keyCode == null) return;

            mActivity.getLorieView().sendKeyEvent(0, keyCode, true);
            mActivity.getLorieView().sendKeyEvent(0, keyCode, false);
        } else {
            // not a control char
            mActivity.getLorieView().sendTextEvent(key.getBytes(UTF_8));
        }
    }

    @Override
    public boolean performExtraKeyButtonHapticFeedback(View view, TermuxX11ExtraKeyButton buttonInfo, Button button) {
        MainActivity.handler.postDelayed(() -> {
            boolean pressed;
            switch (buttonInfo.key) {
                case "CTRL":
                    pressed = Boolean.TRUE.equals(mTermuxExtraKeysView.readSpecialButton(TermuxX11SpecialButton.CTRL, false));
                    mActivity.getLorieView().sendKeyEvent(0, KeyEvent.KEYCODE_CTRL_LEFT, pressed);
                    break;
                case "ALT":
                    pressed = Boolean.TRUE.equals(mTermuxExtraKeysView.readSpecialButton(TermuxX11SpecialButton.ALT, false));
                    mActivity.getLorieView().sendKeyEvent(0, KeyEvent.KEYCODE_ALT_LEFT, pressed);
                    break;
                case "SHIFT":
                    pressed = Boolean.TRUE.equals(mTermuxExtraKeysView.readSpecialButton(TermuxX11SpecialButton.SHIFT, false));
                    mActivity.getLorieView().sendKeyEvent(0, KeyEvent.KEYCODE_SHIFT_LEFT, pressed);
                    break;
                case "META":
                    pressed = Boolean.TRUE.equals(mTermuxExtraKeysView.readSpecialButton(TermuxX11SpecialButton.META, false));
                    mActivity.getLorieView().sendKeyEvent(0, KeyEvent.KEYCODE_META_LEFT, pressed);
                    break;
            }
        }, 100);

        return false;
    }

    public void paste(CharSequence input) {
        KeyEvent[] events = mVirtualKeyboardKeyCharacterMap.getEvents(input.toString().toCharArray());
        if (events != null) {
            for (KeyEvent event : events) {
                int keyCode = event.getKeyCode();
                mEventListener.onKey(mActivity.getLorieView(), keyCode, event);
            }
        }
    }

    private ViewPager getToolbarViewPager() {
        return mActivity.findViewById(R.id.display_terminal_toolbar_view_pager);
    }

    @SuppressLint("RtlHardcoded")
    public void onLorieExtraKeyButtonClick(View view, String key, boolean ctrlDown, boolean altDown, boolean shiftDown, boolean metaDown, boolean fnDown) {
        if ("KEYBOARD".equals(key)) {
            if (getToolbarViewPager()!=null) {
                getToolbarViewPager().requestFocus();
                toggleKeyboardVisibility(mActivity);
            }
        } else if ("DRAWER".equals(key)) {
            Intent preferencesIntent = new Intent(mActivity, LoriePreferences.class);
            preferencesIntent.setAction(ACTION_START_PREFERENCES_ACTIVITY);
            mActivity.startActivity(preferencesIntent);
        } else if ("PASTE".equals(key)) {
            ClipboardManager clipboard = (ClipboardManager) mActivity.getSystemService(Context.CLIPBOARD_SERVICE);
            ClipData clipData = clipboard.getPrimaryClip();
            if (clipData != null) {
                CharSequence pasted = clipData.getItemAt(0).coerceToText(mActivity);
                if (!TextUtils.isEmpty(pasted)) paste(pasted);
            }
        } else {
            onTerminalExtraKeyButtonClick(view, key, ctrlDown, altDown, shiftDown, metaDown, fnDown);
        }
    }

    /**
     * Set the terminal extra keys and style.
     */
    @SuppressWarnings("deprecation")
    private void setExtraKeys() {
        mTermuxX11ExtraKeysInfo = null;
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(mActivity);

        try {
            // The mMap stores the extra key and style string values while loading properties
            // Check {@link #getExtraKeysInternalPropertyValueFromValue(String)} and
            // {@link #getExtraKeysStyleInternalPropertyValueFromValue(String)}
            String extrakeys = preferences.getString("extra_keys_config", TermuxX11ExtraKeys.DEFAULT_IVALUE_EXTRA_KEYS);
            mTermuxX11ExtraKeysInfo = new TermuxX11ExtraKeysInfo(extrakeys, "extra-keys-style", TermuxX11ExtraKeysConstants.CONTROL_CHARS_ALIASES);
        } catch (JSONException e) {
            Toast.makeText(mActivity, "Could not load and set the \"extra-keys\" property from the properties file: " + e, Toast.LENGTH_LONG).show();
            Log.e(LOG_TAG, "Could not load and set the \"extra-keys\" property from the properties file: ", e);

            try {
                mTermuxX11ExtraKeysInfo = new TermuxX11ExtraKeysInfo(TermuxX11ExtraKeys.DEFAULT_IVALUE_EXTRA_KEYS, "default", TermuxX11ExtraKeysConstants.CONTROL_CHARS_ALIASES);
            } catch (JSONException e2) {
                Toast.makeText(mActivity, "Can't create default extra keys", Toast.LENGTH_LONG).show();
                Log.e(LOG_TAG, "Could create default extra keys: ", e);
                mTermuxX11ExtraKeysInfo = null;
            }
        }
    }

    public TermuxX11ExtraKeysInfo getExtraKeysInfo() {
        if (mTermuxX11ExtraKeysInfo == null)
            setExtraKeys();
        return mTermuxX11ExtraKeysInfo;
    }
}
