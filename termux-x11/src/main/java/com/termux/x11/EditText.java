package com.termux.x11;

import android.content.Context;
import android.util.AttributeSet;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;

public class EditText extends androidx.appcompat.widget.AppCompatEditText {
    public EditText(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        return new InputConnectionWrapper(super.onCreateInputConnection(outAttrs));
    }
}
