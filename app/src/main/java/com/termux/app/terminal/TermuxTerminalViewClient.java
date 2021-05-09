package com.termux.app.terminal;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.ActivityNotFoundException;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.inputmethodservice.InputMethodService;
import android.media.AudioManager;
import android.net.Uri;
import android.text.TextUtils;
import android.view.Gravity;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
import android.widget.ListView;
import android.widget.Toast;

import com.termux.R;
import com.termux.app.TermuxActivity;
import com.termux.shared.shell.ShellUtils;
import com.termux.shared.terminal.TermuxTerminalViewClientBase;
import com.termux.shared.termux.TermuxConstants;
import com.termux.app.activities.ReportActivity;
import com.termux.app.models.ReportInfo;
import com.termux.app.models.UserAction;
import com.termux.app.terminal.io.KeyboardShortcut;
import com.termux.app.terminal.io.extrakeys.ExtraKeysView;
import com.termux.shared.settings.properties.TermuxPropertyConstants;
import com.termux.shared.data.DataUtils;
import com.termux.shared.logger.Logger;
import com.termux.shared.markdown.MarkdownUtils;
import com.termux.shared.termux.TermuxUtils;
import com.termux.terminal.KeyHandler;
import com.termux.terminal.TerminalEmulator;
import com.termux.terminal.TerminalSession;

import java.util.Arrays;
import java.util.Collections;
import java.util.LinkedHashSet;
import java.util.List;

import androidx.drawerlayout.widget.DrawerLayout;

public class TermuxTerminalViewClient extends TermuxTerminalViewClientBase {

    final TermuxActivity mActivity;

    final TermuxTerminalSessionClient mTermuxTerminalSessionClient;

    /** Keeping track of the special keys acting as Ctrl and Fn for the soft keyboard and other hardware keys. */
    boolean mVirtualControlKeyDown, mVirtualFnKeyDown;

    public TermuxTerminalViewClient(TermuxActivity activity, TermuxTerminalSessionClient termuxTerminalSessionClient) {
        this.mActivity = activity;
        this.mTermuxTerminalSessionClient = termuxTerminalSessionClient;
    }

    @Override
    public float onScale(float scale) {
        if (scale < 0.9f || scale > 1.1f) {
            boolean increase = scale > 1.f;
            changeFontSize(increase);
            return 1.0f;
        }
        return scale;
    }



    @Override
    public void onSingleTapUp(MotionEvent e) {
        showSoftKeyboard(mActivity, mActivity.getTerminalView());
    }

    @Override
    public boolean shouldBackButtonBeMappedToEscape() {
        return mActivity.getProperties().isBackKeyTheEscapeKey();
    }

    @Override
    public boolean shouldEnforceCharBasedInput() {
        return mActivity.getProperties().isEnforcingCharBasedInput();
    }

    @Override
    public boolean shouldUseCtrlSpaceWorkaround() {
        return mActivity.getProperties().isUsingCtrlSpaceWorkaround();
    }



    @Override
    public void copyModeChanged(boolean copyMode) {
        // Disable drawer while copying.
        mActivity.getDrawer().setDrawerLockMode(copyMode ? DrawerLayout.LOCK_MODE_LOCKED_CLOSED : DrawerLayout.LOCK_MODE_UNLOCKED);
    }



    @SuppressLint("RtlHardcoded")
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent e, TerminalSession currentSession) {
        if (handleVirtualKeys(keyCode, e, true)) return true;

        if (keyCode == KeyEvent.KEYCODE_ENTER && !currentSession.isRunning()) {
            mTermuxTerminalSessionClient.removeFinishedSession(currentSession);
            return true;
        } else if (e.isCtrlPressed() && e.isAltPressed()) {
            // Get the unmodified code point:
            int unicodeChar = e.getUnicodeChar(0);

            if (keyCode == KeyEvent.KEYCODE_DPAD_DOWN || unicodeChar == 'n'/* next */) {
                mTermuxTerminalSessionClient.switchToSession(true);
            } else if (keyCode == KeyEvent.KEYCODE_DPAD_UP || unicodeChar == 'p' /* previous */) {
                mTermuxTerminalSessionClient.switchToSession(false);
            } else if (keyCode == KeyEvent.KEYCODE_DPAD_RIGHT) {
                mActivity.getDrawer().openDrawer(Gravity.LEFT);
            } else if (keyCode == KeyEvent.KEYCODE_DPAD_LEFT) {
                mActivity.getDrawer().closeDrawers();
            } else if (unicodeChar == 'k'/* keyboard */) {
                onToggleSoftKeyboardRequest();
            } else if (unicodeChar == 'm'/* menu */) {
                mActivity.getTerminalView().showContextMenu();
            } else if (unicodeChar == 'r'/* rename */) {
                mTermuxTerminalSessionClient.renameSession(currentSession);
            } else if (unicodeChar == 'c'/* create */) {
                mTermuxTerminalSessionClient.addNewSession(false, null);
            } else if (unicodeChar == 'u' /* urls */) {
                showUrlSelection();
            } else if (unicodeChar == 'v') {
                doPaste();
            } else if (unicodeChar == '+' || e.getUnicodeChar(KeyEvent.META_SHIFT_ON) == '+') {
                // We also check for the shifted char here since shift may be required to produce '+',
                // see https://github.com/termux/termux-api/issues/2
                changeFontSize(true);
            } else if (unicodeChar == '-') {
                changeFontSize(false);
            } else if (unicodeChar >= '1' && unicodeChar <= '9') {
                int index = unicodeChar - '1';
                mTermuxTerminalSessionClient.switchToSession(index);
            }
            return true;
        }

        return false;

    }



    @Override
    public boolean onKeyUp(int keyCode, KeyEvent e) {
        return handleVirtualKeys(keyCode, e, false);
    }

    /** Handle dedicated volume buttons as virtual keys if applicable. */
    private boolean handleVirtualKeys(int keyCode, KeyEvent event, boolean down) {
        InputDevice inputDevice = event.getDevice();
        if (mActivity.getProperties().areVirtualVolumeKeysDisabled()) {
            return false;
        } else if (inputDevice != null && inputDevice.getKeyboardType() == InputDevice.KEYBOARD_TYPE_ALPHABETIC) {
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



    @Override
    public boolean readControlKey() {
        return (mActivity.getExtraKeysView() != null && mActivity.getExtraKeysView().readSpecialButton(ExtraKeysView.SpecialButton.CTRL)) || mVirtualControlKeyDown;
    }

    @Override
    public boolean readAltKey() {
        return (mActivity.getExtraKeysView() != null && mActivity.getExtraKeysView().readSpecialButton(ExtraKeysView.SpecialButton.ALT));
    }

    @Override
    public boolean onLongPress(MotionEvent event) {
        return false;
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
                    resultingCodePoint = '~';
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
                case 'k':
                    mActivity.toggleTerminalToolbar();
                    mVirtualFnKeyDown=false; // force disable fn key down to restore keyboard input into terminal view, fixes termux/termux-app#1420
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
            if (codePoint == 106 /* Ctrl+j or \n */ && !session.isRunning()) {
                mTermuxTerminalSessionClient.removeFinishedSession(session);
                return true;
            }

            List<KeyboardShortcut> shortcuts = mActivity.getProperties().getSessionShortcuts();
            if (shortcuts != null && !shortcuts.isEmpty()) {
                int codePointLowerCase = Character.toLowerCase(codePoint);
                for (int i = shortcuts.size() - 1; i >= 0; i--) {
                    KeyboardShortcut shortcut = shortcuts.get(i);
                    if (codePointLowerCase == shortcut.codePoint) {
                        switch (shortcut.shortcutAction) {
                            case TermuxPropertyConstants.ACTION_SHORTCUT_CREATE_SESSION:
                                mTermuxTerminalSessionClient.addNewSession(false, null);
                                return true;
                            case TermuxPropertyConstants.ACTION_SHORTCUT_NEXT_SESSION:
                                mTermuxTerminalSessionClient.switchToSession(true);
                                return true;
                            case TermuxPropertyConstants.ACTION_SHORTCUT_PREVIOUS_SESSION:
                                mTermuxTerminalSessionClient.switchToSession(false);
                                return true;
                            case TermuxPropertyConstants.ACTION_SHORTCUT_RENAME_SESSION:
                                mTermuxTerminalSessionClient.renameSession(mActivity.getCurrentSession());
                                return true;
                        }
                    }
                }
            }
        }

        return false;
    }



    public void changeFontSize(boolean increase) {
        mActivity.getPreferences().changeFontSize(increase);
        mActivity.getTerminalView().setTextSize(mActivity.getPreferences().getFontSize());
    }


    /**
     * Toggle the soft keyboard. The {@link InputMethodManager#SHOW_FORCED} is passed as
     * {@code showFlags} so that keyboard is forcefully shown if it needs to be enabled.
     *
     * This is also important for soft keyboard to be shown when a hardware keyboard is attached, and
     * user has disabled the {@code Show on-screen keyboard while hardware keyboard is attached} toggle
     * in Android "Language and Input" settings but the current soft keyboard app overrides the
     * default implementation of {@link InputMethodService#onEvaluateInputViewShown()} and returns
     * {@code true}.
     */
    public static void toggleSoftKeyboard(Context context) {
        InputMethodManager inputMethodManager = (InputMethodManager) context.getSystemService(Context.INPUT_METHOD_SERVICE);
        inputMethodManager.toggleSoftInput(InputMethodManager.SHOW_FORCED, 0);
    }

    /**
     * Show the soft keyboard. The {@code 0} value is passed as {@code flags} so that keyboard is
     * forcefully shown.
     *
     * This is also important for soft keyboard to be shown on app startup when a hardware keyboard
     * is attached, and user has disabled the {@code Show on-screen keyboard while hardware keyboard
     * is attached} toggle in Android "Language and Input" settings but the current soft keyboard app
     * overrides the default implementation of {@link InputMethodService#onEvaluateInputViewShown()}
     * and returns {@code true}.
     * https://cs.android.com/android/platform/superproject/+/android-11.0.0_r3:frameworks/base/core/java/android/inputmethodservice/InputMethodService.java;l=1751
     *
     * Also check {@link InputMethodService#onShowInputRequested(int, boolean)} which must return
     * {@code true}, which can be done by failing its {@code ((flags&InputMethod.SHOW_EXPLICIT) == 0)}
     * check by passing {@code 0} as {@code flags}.
     * https://cs.android.com/android/platform/superproject/+/android-11.0.0_r3:frameworks/base/core/java/android/inputmethodservice/InputMethodService.java;l=2022
     */
    public static void showSoftKeyboard(Context context, View view) {
        InputMethodManager inputMethodManager = (InputMethodManager) context.getSystemService(Context.INPUT_METHOD_SERVICE);
        inputMethodManager.showSoftInput(view, 0);
    }

    public static void hideSoftKeyboard(Context context, View view) {
        InputMethodManager inputMethodManager = (InputMethodManager) context.getSystemService(Context.INPUT_METHOD_SERVICE);
        inputMethodManager.hideSoftInputFromWindow(view.getWindowToken(), 0);
    }

    public static void disableSoftKeyboard(Activity activity, View view) {
        hideSoftKeyboard(activity, view);
        setDisableSoftKeyboardFlags(activity);
    }

    public static void setDisableSoftKeyboardFlags(Activity activity) {
        activity.getWindow().setFlags(WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM, WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM);
    }

    public static void clearDisableSoftKeyboardFlags(Activity activity) {
        activity.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM);
    }

    public void setResizeTerminalViewForSoftKeyboardFlags() {
        // TODO: The flag is deprecated for API 30 and WindowInset API should be used
        // https://developer.android.com/reference/android/view/WindowManager.LayoutParams#SOFT_INPUT_ADJUST_RESIZE
        // https://medium.com/androiddevelopers/animating-your-keyboard-fb776a8fb66d
        // https://stackoverflow.com/a/65194077/14686958
        mActivity.getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);
    }

    /**
     * Called when user requests the soft keyboard to be toggled via "KEYBOARD" toggle button in
     * drawer or extra keys, or with ctrl+alt+k hardware keyboard shortcut.
     */
    public void onToggleSoftKeyboardRequest() {
        // If soft keyboard is disabled by user for Termux
        if (!mActivity.getPreferences().getSoftKeyboardEnabled()) {
            disableSoftKeyboard(mActivity, mActivity.getTerminalView());
        } else {
            clearDisableSoftKeyboardFlags(mActivity);
            toggleSoftKeyboard(mActivity);
        }
    }

    public void setSoftKeyboardState(boolean isStartup) {
        // If soft keyboard is disabled by user for Termux
        if (!mActivity.getPreferences().getSoftKeyboardEnabled()) {
            disableSoftKeyboard(mActivity, mActivity.getTerminalView());
        } else {
            // Set flag to automatically push up TerminalView when keyboard is opened instead of showing over it
            setResizeTerminalViewForSoftKeyboardFlags();

            // Clear any previous flags to disable soft keyboard in case setting updated
            clearDisableSoftKeyboardFlags(mActivity);

            // If soft keyboard is to be hidden on startup
            if (isStartup && mActivity.getProperties().shouldSoftKeyboardBeHiddenOnStartup()) {
                hideSoftKeyboard(mActivity, mActivity.getTerminalView());
            } else {
                // Force show soft keyboard
                showSoftKeyboard(mActivity, mActivity.getTerminalView());

                mActivity.getTerminalView().setOnFocusChangeListener(new View.OnFocusChangeListener() {
                    @Override
                    public void onFocusChange(View view, boolean hasFocus) {
                        if (hasFocus) {
                            showSoftKeyboard(mActivity, mActivity.getTerminalView());
                        } else {
                            hideSoftKeyboard(mActivity, mActivity.getTerminalView());
                        }
                    }
                });

                // Request focus for TerminalView
                mActivity.getTerminalView().requestFocus();
            }
        }
    }



    public void shareSessionTranscript() {
        TerminalSession session = mActivity.getCurrentSession();
        if (session == null) return;

        String transcriptText = ShellUtils.getTerminalSessionTranscriptText(session, false, true);
        if (transcriptText == null) return;

        try {
            // See https://github.com/termux/termux-app/issues/1166.
            Intent intent = new Intent(Intent.ACTION_SEND);
            intent.setType("text/plain");
            transcriptText = DataUtils.getTruncatedCommandOutput(transcriptText, DataUtils.TRANSACTION_SIZE_LIMIT_IN_BYTES, false, true, false).trim();
            intent.putExtra(Intent.EXTRA_TEXT, transcriptText);
            intent.putExtra(Intent.EXTRA_SUBJECT, mActivity.getString(R.string.title_share_transcript));
            mActivity.startActivity(Intent.createChooser(intent, mActivity.getString(R.string.title_share_transcript_with)));
        } catch (Exception e) {
            Logger.logStackTraceWithMessage("Failed to get share session transcript of length " + transcriptText.length(), e);
        }
    }

    public void showUrlSelection() {
        TerminalSession session = mActivity.getCurrentSession();
        if (session == null) return;

        String text = ShellUtils.getTerminalSessionTranscriptText(session, true, true);

        LinkedHashSet<CharSequence> urlSet = DataUtils.extractUrls(text);
        if (urlSet.isEmpty()) {
            new AlertDialog.Builder(mActivity).setMessage(R.string.title_select_url_none_found).show();
            return;
        }

        final CharSequence[] urls = urlSet.toArray(new CharSequence[0]);
        Collections.reverse(Arrays.asList(urls)); // Latest first.

        // Click to copy url to clipboard:
        final AlertDialog dialog = new AlertDialog.Builder(mActivity).setItems(urls, (di, which) -> {
            String url = (String) urls[which];
            ClipboardManager clipboard = (ClipboardManager) mActivity.getSystemService(Context.CLIPBOARD_SERVICE);
            clipboard.setPrimaryClip(new ClipData(null, new String[]{"text/plain"}, new ClipData.Item(url)));
            Toast.makeText(mActivity, R.string.msg_select_url_copied_to_clipboard, Toast.LENGTH_LONG).show();
        }).setTitle(R.string.title_select_url_dialog).create();

        // Long press to open URL:
        dialog.setOnShowListener(di -> {
            ListView lv = dialog.getListView(); // this is a ListView with your "buds" in it
            lv.setOnItemLongClickListener((parent, view, position, id) -> {
                dialog.dismiss();
                String url = (String) urls[position];
                Intent i = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
                try {
                    mActivity.startActivity(i, null);
                } catch (ActivityNotFoundException e) {
                    // If no applications match, Android displays a system message.
                    mActivity.startActivity(Intent.createChooser(i, null));
                }
                return true;
            });
        });

        dialog.show();
    }

    public void reportIssueFromTranscript() {
        TerminalSession session = mActivity.getCurrentSession();
        if (session == null) return;

        String transcriptText = ShellUtils.getTerminalSessionTranscriptText(session, false, true);
        if (transcriptText == null) return;

        transcriptText = DataUtils.getTruncatedCommandOutput(transcriptText, DataUtils.TRANSACTION_SIZE_LIMIT_IN_BYTES, false, true, false).trim();

        StringBuilder reportString = new StringBuilder();

        String title = TermuxConstants.TERMUX_APP_NAME + " Report Issue";

        reportString.append("## Transcript\n");
        reportString.append("\n").append(MarkdownUtils.getMarkdownCodeForString(transcriptText, true));

        reportString.append("\n\n").append(TermuxUtils.getAppInfoMarkdownString(mActivity, true));
        reportString.append("\n\n").append(TermuxUtils.getDeviceInfoMarkdownString(mActivity));

        String termuxAptInfo = TermuxUtils.geAPTInfoMarkdownString(mActivity);
        if (termuxAptInfo != null)
            reportString.append("\n\n").append(termuxAptInfo);

        ReportActivity.startReportActivity(mActivity, new ReportInfo(UserAction.REPORT_ISSUE_FROM_TRANSCRIPT, TermuxConstants.TERMUX_APP.TERMUX_ACTIVITY_NAME, title, null, reportString.toString(), "\n\n" + TermuxUtils.getReportIssueMarkdownString(mActivity), false));
    }

    public void doPaste() {
        TerminalSession session = mActivity.getCurrentSession();
        if (session == null) return;
        if (!session.isRunning()) return;

        ClipboardManager clipboard = (ClipboardManager) mActivity.getSystemService(Context.CLIPBOARD_SERVICE);
        ClipData clipData = clipboard.getPrimaryClip();
        if (clipData == null) return;
        CharSequence paste = clipData.getItemAt(0).coerceToText(mActivity);
        if (!TextUtils.isEmpty(paste))
            session.getEmulator().paste(paste.toString());
    }

}
