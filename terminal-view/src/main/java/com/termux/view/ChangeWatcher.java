package com.termux.view;

import android.content.Context;
import android.text.Editable;
import android.text.Selection;
import android.text.SpanWatcher;
import android.text.Spannable;
import android.text.TextWatcher;
import android.view.inputmethod.InputMethodManager;

import com.termux.view.inputmethod.TerminalInputConnection;

class ChangeWatcher implements TextWatcher, SpanWatcher {
    final TerminalView mTerminalView;

    ChangeWatcher(TerminalView terminalView) {
        this.mTerminalView = terminalView;
    }

    private void sendUpdateSelection() {
        final InputMethodManager imm = (InputMethodManager) mTerminalView.getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
        final Editable text = mTerminalView.getImeBuffer();
        if (imm != null && text != null) {
            final int selectionStart = Selection.getSelectionStart(text);
            final int selectionEnd = Selection.getSelectionEnd(text);

            final int candStart = TerminalInputConnection.getComposingSpanStart(text);
            final int candEnd = TerminalInputConnection.getComposingSpanEnd(text);
            imm.updateSelection(mTerminalView, selectionStart, selectionEnd, candStart, candEnd);
        }
    }


    private void spanChange(Object what) {
        if (what == Selection.SELECTION_START || what == Selection.SELECTION_END) {
            sendUpdateSelection();
        }
    }

    @Override
    public void onSpanAdded(Spannable text, Object what, int start, int end) {

        spanChange(what);
    }


    @Override
    public void onSpanRemoved(Spannable text, Object what, int start, int end) {
        spanChange(what);

    }

    @Override
    public void onSpanChanged(Spannable text, Object what, int ostart, int oend, int nstart, int nend) {
        spanChange(what);
    }

    @Override
    public void beforeTextChanged(CharSequence s, int start, int count, int after) {

    }

    @Override
    public void onTextChanged(CharSequence s, int start, int before, int count) {
    }

    @Override
    public void afterTextChanged(Editable s) {
        mTerminalView.invalidate();
    }
}
