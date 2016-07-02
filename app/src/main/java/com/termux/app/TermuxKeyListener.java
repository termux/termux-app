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
    public int getLeftKey() {
        return mActivity.mSettings.mLeftKey;
    }

    @Override
    public int getRightKey() {
        return mActivity.mSettings.mRightKey;
    }

    @Override
    public int getUpKey() {
        return mActivity.mSettings.mUpKey;
    }

    @Override
    public int getDownKey() {
        return mActivity.mSettings.mDownKey;
    }

    @Override
    public int getPgUpKey() {
        return mActivity.mSettings.mPgUpKey;
    }

    @Override
    public int getPgDownKey() {
        return mActivity.mSettings.mPgDownKey;
    }

    @Override
    public int getTabKey() {
        return mActivity.mSettings.mTabKey;
    }

    @Override
    public int getInsertKey() {
        return mActivity.mSettings.mInsertKey;
    }

    @Override
    public int getHomeKey() {
        return mActivity.mSettings.mHomeKey;
    }

    @Override
    public int getUderscoreKey() {
        return mActivity.mSettings.mUderscoreKey;
    }

    @Override
    public int getPipeKey() {
        return mActivity.mSettings.mPipeKey;
    }

    @Override
    public int getEscapeKey() {
        return mActivity.mSettings.mEscapeKey;
    }

    @Override
    public int getHatKey() {
        return mActivity.mSettings.mHatKey;
    }

    @Override
    public int getJmbBackKey() {
        return mActivity.mSettings.mJmbBackKey;
    }

    @Override
    public int getJmbForwardKey() {
        return mActivity.mSettings.mJmbForwardKey;
    }

    @Override
    public int getEmacsXKey() {
        return mActivity.mSettings.mEmacsXKey;
    }

    @Override
    public int getShowVolKey() {
        return mActivity.mSettings.mShowVolKey;
    }

    @Override
    public int getWriteModeKey() {
        return mActivity.mSettings.mWriteModeKey;
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

            if(lowerCase == getUpKey()){
                resultingKeyCode = KeyEvent.KEYCODE_DPAD_UP;
            }else if(lowerCase == getLeftKey()){
                resultingKeyCode = KeyEvent.KEYCODE_DPAD_LEFT;
            }else if(lowerCase == getDownKey()){
                resultingKeyCode = KeyEvent.KEYCODE_DPAD_DOWN;
            }else if(lowerCase == getRightKey()){
                resultingKeyCode = KeyEvent.KEYCODE_DPAD_RIGHT;

            // Page up and down.
          }else if(lowerCase == getPgUpKey()){
                resultingKeyCode = KeyEvent.KEYCODE_PAGE_UP;
            }else if(lowerCase == getPgDownKey()){
                resultingKeyCode = KeyEvent.KEYCODE_PAGE_DOWN;

            // Some special keys:
          }else if(lowerCase == getTabKey()){
                resultingKeyCode = KeyEvent.KEYCODE_TAB;
            }else if(lowerCase == getInsertKey()){
                resultingKeyCode = KeyEvent.KEYCODE_INSERT;
            }else if(lowerCase == getHomeKey()){
                resultingKeyCode = KeyEvent.KEYCODE_MOVE_HOME;

            // Special characters to input.
          }else if(lowerCase == getUderscoreKey()){
                resultingCodePoint = '_';
            }else if(lowerCase == getPipeKey()){
                resultingCodePoint = '|';

            // Function keys.
          }else if(lowerCase == '1' ||
                    lowerCase == '2' ||
                    lowerCase == '3' ||
                    lowerCase == '4' ||
                    lowerCase == '5' ||
                    lowerCase == '6' ||
                    lowerCase == '7' ||
                    lowerCase == '8' ||
                    lowerCase == '9' ){
                resultingKeyCode = (codePoint - '1') + KeyEvent.KEYCODE_F1;
            }else if(lowerCase == '0'){
                resultingKeyCode = KeyEvent.KEYCODE_F10;

            // Other special keys.
          }else if(lowerCase == getEscapeKey()){
                resultingCodePoint = /*Escape*/ 27;
            }else if(lowerCase == getHatKey()){
                resultingCodePoint = /*^.*/ 28;

            }else if(lowerCase == getJmbBackKey() ||
                      lowerCase == getJmbForwardKey() ||
                      lowerCase == getEmacsXKey()){
              // alt+b, jumping backward in readline.
              // alf+f, jumping forward in readline.
              // alt+x, common in emacs.
                resultingCodePoint = lowerCase;
                altDown = true;

            // Volume control.
          }else if(lowerCase == getShowVolKey()){
                resultingCodePoint = -1;
                AudioManager audio = (AudioManager) mActivity.getSystemService(Context.AUDIO_SERVICE);
                audio.adjustSuggestedStreamVolume(AudioManager.ADJUST_SAME, AudioManager.USE_DEFAULT_STREAM_TYPE, AudioManager.FLAG_SHOW_UI);

            // Writing mode:
          }else if(lowerCase == getWriteModeKey()){
                mActivity.toggleShowExtraKeys();
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
