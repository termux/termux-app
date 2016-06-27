package com.termux.app;

import android.content.Context;
import android.media.AudioManager;
import android.support.v4.widget.DrawerLayout;
import android.view.Gravity;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.inputmethod.InputMethodManager;

import com.termux.terminal.KeyHandler;
import com.termux.terminal.TerminalEmulator;
import com.termux.terminal.TerminalSession;
import com.termux.view.TerminalKeyListener;

import java.util.List;

public final class TermuxKeyListener implements TerminalKeyListener {

    final TermuxActivity mActivity;

    /** Keeping track of the special keys acting as Ctrl and Fn for the soft keyboard and other hardware keys. */
    boolean mVirtualControlKeyDown, mVirtualFnKeyDown;

    public TermuxKeyListener(TermuxActivity activity) {
        this.mActivity = activity;
    }

    @Override
    public float onScale(float scale) {
        if (scale < 0.9f || scale > 1.1f) {
            boolean increase = scale > 1.f;
            mActivity.changeFontSize(increase);
            return 1.0f;
        }
        return scale;
    }

    @Override
    public void onSingleTapUp(MotionEvent e) {
        InputMethodManager mgr = (InputMethodManager) mActivity.getSystemService(Context.INPUT_METHOD_SERVICE);
        mgr.showSoftInput(mActivity.mTerminalView, InputMethodManager.SHOW_IMPLICIT);
    }

    @Override
    public boolean shouldBackButtonBeMappedToEscape() {
        return mActivity.mSettings.mBackIsEscape;
    }

    @Override
    public void copyModeChanged(boolean copyMode) {
        // Disable drawer while copying.
        mActivity.getDrawer().setDrawerLockMode(copyMode ? DrawerLayout.LOCK_MODE_LOCKED_CLOSED : DrawerLayout.LOCK_MODE_UNLOCKED);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent e, TerminalSession currentSession) {
        if (handleVirtualKeys(keyCode, e, true)) return true;

        TermuxService service = mActivity.mTermService;

        if (keyCode == KeyEvent.KEYCODE_ENTER && !currentSession.isRunning()) {
            // Return pressed with finished session - remove it.
            currentSession.finishIfRunning();

            int index = service.removeTermSession(currentSession);
            mActivity.mListViewAdapter.notifyDataSetChanged();
            if (mActivity.mTermService.getSessions().isEmpty()) {
                // There are no sessions to show, so finish the activity.
                mActivity.finish();
            } else {
                if (index >= service.getSessions().size()) {
                    index = service.getSessions().size() - 1;
                }
                mActivity.switchToSession(service.getSessions().get(index));
            }
            return true;
        } else if (e.isCtrlPressed() && e.isShiftPressed()) {
            // Get the unmodified code point:
            int unicodeChar = e.getUnicodeChar(0);

            if (keyCode == KeyEvent.KEYCODE_DPAD_DOWN || unicodeChar == 'n'/* next */) {
                mActivity.switchToSession(true);
            } else if (keyCode == KeyEvent.KEYCODE_DPAD_UP || unicodeChar == 'p' /* previous */) {
                mActivity.switchToSession(false);
            } else if (keyCode == KeyEvent.KEYCODE_DPAD_RIGHT) {
                mActivity.getDrawer().openDrawer(Gravity.LEFT);
            } else if (keyCode == KeyEvent.KEYCODE_DPAD_LEFT) {
                mActivity.getDrawer().closeDrawers();
            } else if (unicodeChar == 'f'/* full screen */) {
                mActivity.toggleImmersive();
            } else if (unicodeChar == 'k'/* keyboard */) {
                InputMethodManager imm = (InputMethodManager) mActivity.getSystemService(Context.INPUT_METHOD_SERVICE);
                imm.toggleSoftInput(InputMethodManager.SHOW_FORCED, 0);
            } else if (unicodeChar == 'm'/* menu */) {
                mActivity.mTerminalView.showContextMenu();
            } else if (unicodeChar == 'r'/* rename */) {
                mActivity.renameSession(currentSession);
            } else if (unicodeChar == 'c'/* create */) {
                mActivity.addNewSession(false, null);
            } else if (unicodeChar == 'u' /* urls */) {
                mActivity.showUrlSelection();
            } else if (unicodeChar == 'v') {
                mActivity.doPaste();
            } else if (unicodeChar == '+' || e.getUnicodeChar(KeyEvent.META_SHIFT_ON) == '+') {
                // We also check for the shifted char here since shift may be required to produce '+',
                // see https://github.com/termux/termux-api/issues/2
                mActivity.changeFontSize(true);
            } else if (unicodeChar == '-') {
                mActivity.changeFontSize(false);
            } else if (unicodeChar >= '1' && unicodeChar <= '9') {
                int num = unicodeChar - '1';
                if (service.getSessions().size() > num)
                    mActivity.switchToSession(service.getSessions().get(num));
            }
            return true;
        }

        return false;

    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent e) {
        return handleVirtualKeys(keyCode, e, false);
    }

    @Override
    public boolean readControlKey() {
        return (mActivity.mExtraKeysView != null && mActivity.mExtraKeysView.readControlButton()) || mVirtualControlKeyDown;
    }

    @Override
    public boolean readAltKey() {
        return (mActivity.mExtraKeysView != null && mActivity.mExtraKeysView.readAltButton());
    }

    @Override
    public boolean onCodePoint(final int codePoint, boolean ctrlDown, TerminalSession session) {
        if (mVirtualFnKeyDown) {
            int resultingKeyCode = -1;
            int resultingCodePoint = -1;
            boolean altDown = false;
            int lowerCase = Character.toLowerCase(codePoint);
            switch (lowerCase) {
                // Arrow keys.
                case 'w':
                    resultingKeyCode = KeyEvent.KEYCODE_DPAD_UP;
                    break;
                case 'a':
                    resultingKeyCode = KeyEvent.KEYCODE_DPAD_LEFT;
                    break;
                case 's':
                    resultingKeyCode = KeyEvent.KEYCODE_DPAD_DOWN;
                    break;
                case 'd':
                    resultingKeyCode = KeyEvent.KEYCODE_DPAD_RIGHT;
                    break;

                // Page up and down.
                case 'p':
                    resultingKeyCode = KeyEvent.KEYCODE_PAGE_UP;
                    break;
                case 'n':
                    resultingKeyCode = KeyEvent.KEYCODE_PAGE_DOWN;
                    break;

                // Some special keys:
                case 't':
                    resultingKeyCode = KeyEvent.KEYCODE_TAB;
                    break;
                case 'i':
                    resultingKeyCode = KeyEvent.KEYCODE_INSERT;
                    break;
                case 'h':
                    resultingKeyCode = KeyEvent.KEYCODE_MOVE_HOME;
                    break;

                // Special characters to input.
                case 'u':
                    resultingCodePoint = '_';
                    break;
                case 'l':
                    resultingCodePoint = '|';
                    break;

                // Function keys.
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    resultingKeyCode = (codePoint - '1') + KeyEvent.KEYCODE_F1;
                    break;
                case '0':
                    resultingKeyCode = KeyEvent.KEYCODE_F10;
                    break;

                // Other special keys.
                case 'e':
                    resultingCodePoint = /*Escape*/ 27;
                    break;
                case '.':
                    resultingCodePoint = /*^.*/ 28;
                    break;

                case 'b': // alt+b, jumping backward in readline.
                case 'f': // alf+f, jumping forward in readline.
                case 'x': // alt+x, common in emacs.
                    resultingCodePoint = lowerCase;
                    altDown = true;
                    break;

                // Volume control.
                case 'v':
                    resultingCodePoint = -1;
                    AudioManager audio = (AudioManager) mActivity.getSystemService(Context.AUDIO_SERVICE);
                    audio.adjustSuggestedStreamVolume(AudioManager.ADJUST_SAME, AudioManager.USE_DEFAULT_STREAM_TYPE, AudioManager.FLAG_SHOW_UI);
                    break;

                // Writing mode:
                case 'q':
                    mActivity.toggleShowExtraKeys();
                    break;
            }

            if (resultingKeyCode != -1) {
                TerminalEmulator term = session.getEmulator();
                session.write(KeyHandler.getCode(resultingKeyCode, 0, term.isCursorKeysApplicationMode(), term.isKeypadApplicationMode()));
            } else if (resultingCodePoint != -1) {
                session.writeCodePoint(altDown, resultingCodePoint);
            }
            return true;
        } else if (ctrlDown) {
            List<TermuxPreferences.KeyboardShortcut> shortcuts = mActivity.mSettings.shortcuts;
            if (!shortcuts.isEmpty()) {
                for (int i = shortcuts.size() - 1; i >= 0; i--) {
                    TermuxPreferences.KeyboardShortcut shortcut = shortcuts.get(i);
                    if (codePoint == shortcut.codePoint) {
                        switch (shortcut.shortcutAction) {
                            case TermuxPreferences.SHORTCUT_ACTION_CREATE_SESSION:
                                mActivity.addNewSession(false, null);
                                return true;
                            case TermuxPreferences.SHORTCUT_ACTION_PREVIOUS_SESSION:
                                mActivity.switchToSession(false);
                                return true;
                            case TermuxPreferences.SHORTCUT_ACTION_NEXT_SESSION:
                                mActivity.switchToSession(true);
                                return true;
                            case TermuxPreferences.SHORTCUT_ACTION_RENAME_SESSION:
                                mActivity.renameSession(mActivity.getCurrentTermSession());
                                return true;
                        }
                    }
                }
            }
        }

        return false;
    }

    /** Handle dedicated volume buttons as virtual keys if applicable. */
    private boolean handleVirtualKeys(int keyCode, KeyEvent event, boolean down) {
        InputDevice inputDevice = event.getDevice();
        if (inputDevice != null && inputDevice.getKeyboardType() == InputDevice.KEYBOARD_TYPE_ALPHABETIC) {
            // Do not steal dedicated buttons from a full external keyboard.
            return false;
        } else if (keyCode == KeyEvent.KEYCODE_VOLUME_DOWN) {
            mVirtualControlKeyDown = down;
            return true;
        } else if (keyCode == KeyEvent.KEYCODE_VOLUME_UP) {
            mVirtualFnKeyDown = down;
            return true;
        }
        return false;
    }


}
