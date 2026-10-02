
package com.termux.view.inputmethod;

import android.content.Context;
import android.os.Bundle;
import android.text.Editable;
import android.text.Selection;
import android.util.Log;
import android.view.KeyEvent;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.CompletionInfo;
import android.view.inputmethod.ExtractedText;
import android.view.inputmethod.ExtractedTextRequest;
import android.view.inputmethod.InputContentInfo;
import android.view.inputmethod.InputMethodManager;

import androidx.annotation.NonNull;

import com.termux.terminal.TerminalEmulator;
import com.termux.view.TerminalView;


/**
 *
 */
public class TerminalInputConnection extends BaseInputConnection {
    private static final boolean DEBUG = false;
    private static final String TAG = "TerminalInputConnection";
    private final Editable editable;
    private final TerminalView mTargetView;

    public TerminalInputConnection(TerminalView targetView) {
        super(targetView, true);
        mTargetView = targetView;

        editable = targetView.getImeBuffer();
        editable.clear();
        Selection.setSelection(editable, 0);
    }

    @Override
    public Editable getEditable() {
        return editable;
    }

    @Override
    public boolean commitText(CharSequence text, int newCursorPosition) {
        if (DEBUG) Log.d(TAG, "commitText: " + text.length() + "   " + text.getClass() + "  ");
        //Some input methods convert the enter key to '\n', split it
        boolean sendEnter = false;
        final int last;
        if (text != null && (last = text.length() - 1) >= 0 && text.charAt(last) == '\n') {
            text = text.subSequence(0, last);
            sendEnter = true;
        }

        super.commitText(text, newCursorPosition);


        final Editable editable = getEditable();

        sendTextToTerminal(editable);

        editable.clear();

        if (sendEnter) {
            sendEnterKey();

            try {
                // Simulate insert '\n', then clear()
                InputMethodManager imm = (InputMethodManager) mTargetView.getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
                if (imm != null) {
                    imm.updateSelection(mTargetView, 1, 1, -1, -1);
                    imm.updateSelection(mTargetView, 0, 0, -1, -1);
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
        }

        return true;
    }

    private void sendEnterKey() {
        sendKeyEvent(new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_ENTER));
        sendKeyEvent(new KeyEvent(KeyEvent.ACTION_UP, KeyEvent.KEYCODE_ENTER));
    }

    @Override
    public ExtractedText getExtractedText(ExtractedTextRequest request, int flags) {
        if (DEBUG) Log.d(TAG, "getExtractedText: " + flags);
        final ExtractedText extract = new ExtractedText();
        final Editable editable = getEditable();
        extract.startOffset = 0;
        int a = Selection.getSelectionStart(editable);
        int b = Selection.getSelectionEnd(editable);
        if (a > b) {
            int tmp = a;
            a = b;
            b = tmp;
        }
        extract.selectionStart = a;
        extract.selectionEnd = b;
        extract.text = editable;
        return extract;
    }

    @Override
    public boolean beginBatchEdit() {
        if (DEBUG) Log.d(TAG, "beginBatchEdit: ");
        return super.beginBatchEdit();
    }

    @Override
    public boolean endBatchEdit() {
        if (DEBUG) Log.d(TAG, "endBatchEdit: ");
        return super.endBatchEdit();
    }

    @Override
    public boolean clearMetaKeyStates(int states) {
        if (DEBUG) Log.d(TAG, "clearMetaKeyStates: ");
        return super.clearMetaKeyStates(states);
    }

    @Override
    public CharSequence getTextAfterCursor(int length, int flags) {
        if (DEBUG) Log.d(TAG, "getTextAfterCursor: " + length + "  " + flags);
        return super.getTextAfterCursor(length, flags);
    }

    @Override
    public CharSequence getTextBeforeCursor(int length, int flags) {
        if (DEBUG) Log.d(TAG, "getTextBeforeCursor: " + length + "  " + flags);
        return super.getTextBeforeCursor(length, flags);
    }


    @Override
    public boolean setComposingText(CharSequence text, int newCursorPosition) {
        if (DEBUG) Log.d(TAG, "setComposingText: " + text + "  " + newCursorPosition);
        return super.setComposingText(text, newCursorPosition);
    }

    @Override
    public boolean setComposingRegion(int start, int end) {
        if (DEBUG) Log.d(TAG, "setComposingRegion: " + start + "   " + end);
        return super.setComposingRegion(start, end);
    }

    @Override
    public boolean setSelection(int start, int end) {
        if (DEBUG) Log.d(TAG, "setSelection: ");
        return super.setSelection(start, end);
    }

    @Override
    public boolean deleteSurroundingText(int leftLength, int rightLength) {
        if (DEBUG) {
            Log.d(TAG, "deleteSurroundingText: " + leftLength + "  " + rightLength);
        }
        return super.deleteSurroundingText(leftLength, rightLength);
    }

    @Override
    public boolean sendKeyEvent(KeyEvent event) {
        if (DEBUG) Log.d(TAG, "sendKeyEvent: " + event.getDisplayLabel());
        return super.sendKeyEvent(event);
    }

    @Override
    public boolean finishComposingText() {
        if (DEBUG) {
            Log.d(TAG, "finishComposingText: " + getEditable() + "  " + getComposingSpanStart(getEditable()) + "  " + getComposingSpanEnd(getEditable()));
        }
        super.finishComposingText();

        final Editable editable = getEditable();
        sendTextToTerminal(editable);

        editable.clear();


        return true;
    }

    @Override
    public int getCursorCapsMode(int reqModes) {
        if (DEBUG) Log.d(TAG, "getCursorCapsMode: " + reqModes);
        return super.getCursorCapsMode(reqModes);
    }

    @Override
    public boolean commitContent(@NonNull InputContentInfo inputContentInfo, int flags, Bundle opts) {
        if (DEBUG) Log.d(TAG, "commitContent: ");
        return super.commitContent(inputContentInfo, flags, opts);
    }

    @Override
    public boolean commitCompletion(CompletionInfo text) {
        if (DEBUG) Log.d(TAG, "commitCompletion: " + text.getText());
        return super.commitCompletion(text);
    }

    private void sendTextToTerminal(CharSequence text) {
        mTargetView.stopTextSelectionMode();
        final int textLengthInChars = text.length();
        for (int i = 0; i < textLengthInChars; i++) {
            char firstChar = text.charAt(i);
            int codePoint;
            if (Character.isHighSurrogate(firstChar)) {
                if (++i < textLengthInChars) {
                    codePoint = Character.toCodePoint(firstChar, text.charAt(i));
                } else {
                    // At end of string, with no low surrogate following the high:
                    codePoint = TerminalEmulator.UNICODE_REPLACEMENT_CHAR;
                }
            } else {
                codePoint = firstChar;
            }

            // Check onKeyDown() for details.
            if (mTargetView.mClient.readShiftKey())
                codePoint = Character.toUpperCase(codePoint);

            boolean ctrlHeld = false;
            if (codePoint <= 31 && codePoint != 27) {
                if (codePoint == '\n') {
                    // The AOSP keyboard and descendants seems to send \n as text when the enter key is pressed,
                    // instead of a key event like most other keyboard apps. A terminal expects \r for the enter
                    // key (although when icrnl is enabled this doesn't make a difference - run 'stty -icrnl' to
                    // check the behaviour).
                    codePoint = '\r';
                }

                // E.g. penti keyboard for ctrl input.
                ctrlHeld = true;
                switch (codePoint) {
                    case 31:
                        codePoint = '_';
                        break;
                    case 30:
                        codePoint = '^';
                        break;
                    case 29:
                        codePoint = ']';
                        break;
                    case 28:
                        codePoint = '\\';
                        break;
                    default:
                        codePoint += 96;
                        break;
                }
            }

            mTargetView.inputCodePoint(TerminalView.KEY_EVENT_SOURCE_SOFT_KEYBOARD, codePoint, ctrlHeld, false);
        }
    }
}
