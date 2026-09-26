package com.termux.app.terminal.io;

import android.view.KeyEvent;
import android.view.View;
import android.widget.EditText;
import android.widget.LinearLayout;

import com.termux.R;
import com.termux.app.TermuxActivity;
import com.termux.shared.termux.extrakeys.ExtraKeysView;
import com.termux.terminal.TerminalSession;

/**
 * Terminal toolbar setup.
 *
 * <p>Historically the toolbar was a two page {@link androidx.viewpager.widget.ViewPager}: page 0
 * held the extra keys and page 1 the text input. Switching pages required a horizontal swipe, but
 * the text input consumed horizontal touches for cursor movement, so returning to the extra keys
 * meant first scrolling the caret all the way back to the start of the text. With a long line that
 * is painful.
 *
 * <p>Both views are now stacked in a vertical {@link LinearLayout} and visible at the same time:
 * the text input row sits directly above the extra keys, next to the terminal. The text input row
 * can be hidden and shown with the {@code TEXTBAR} extra key.
 */
public class TerminalToolbarViewPager {

    /** Wire up the extra keys view and the text input row of the toolbar. */
    public static void setup(TermuxActivity activity, String savedTextInput) {
        setupExtraKeys(activity);
        setupTextInput(activity, savedTextInput);
    }

    private static void setupExtraKeys(TermuxActivity activity) {
        ExtraKeysView extraKeysView = activity.findViewById(R.id.terminal_toolbar_extra_keys);
        if (extraKeysView == null) return;

        extraKeysView.setExtraKeysViewClient(activity.getTermuxTerminalExtraKeys());
        extraKeysView.setButtonTextAllCaps(activity.getProperties().shouldExtraKeysTextBeAllCaps());
        activity.setExtraKeysView(extraKeysView);
        extraKeysView.reload(activity.getTermuxTerminalExtraKeys().getExtraKeysInfo(),
            activity.getTerminalToolbarRowHeight());

        // apply extra keys fix if enabled in prefs
        if (activity.getProperties().isUsingFullScreen() && activity.getProperties().isUsingFullScreenWorkAround()) {
            FullScreenWorkAround.apply(activity);
        }
    }

    private static void setupTextInput(TermuxActivity activity, String savedTextInput) {
        final EditText editText = activity.findViewById(R.id.terminal_toolbar_text_input);
        if (editText == null) return;

        if (savedTextInput != null)
            editText.setText(savedTextInput);

        // The field wraps onto several lines for long input, but Enter sends rather than inserting
        // a newline: that is what the terminal below expects, and it keeps the row free of a send
        // button competing for horizontal space.
        editText.setOnKeyListener((v, keyCode, event) -> {
            if (keyCode == KeyEvent.KEYCODE_ENTER && event.getAction() == KeyEvent.ACTION_DOWN) {
                sendTextInput(activity, editText);
                return true;
            }
            return false;
        });

        editText.setOnEditorActionListener((v, actionId, event) -> {
            sendTextInput(activity, editText);
            return true;
        });
    }

    private static void sendTextInput(TermuxActivity activity, EditText editText) {
        TerminalSession session = activity.getCurrentSession();
        if (session == null) return;

        if (session.isRunning()) {
            String textToSend = editText.getText().toString();
            if (textToSend.isEmpty()) textToSend = "\r";
            session.write(textToSend);
        } else {
            activity.getTermuxTerminalSessionClient().removeFinishedSession(session);
        }
        editText.setText("");
    }

    /**
     * Show or hide the text input row. Returns true if it is visible afterwards.
     */
    public static boolean toggleTextInput(TermuxActivity activity) {
        View row = activity.findViewById(R.id.terminal_toolbar_text_input_row);
        if (row == null) return false;

        boolean showNow = row.getVisibility() != View.VISIBLE;
        row.setVisibility(showNow ? View.VISIBLE : View.GONE);

        if (showNow) {
            View editText = activity.findViewById(R.id.terminal_toolbar_text_input);
            if (editText != null) editText.requestFocus();
        } else {
            if (activity.getTerminalView() != null) activity.getTerminalView().requestFocus();
        }
        return showNow;
    }

    public static boolean isTextInputVisible(TermuxActivity activity) {
        View row = activity.findViewById(R.id.terminal_toolbar_text_input_row);
        return row != null && row.getVisibility() == View.VISIBLE;
    }

}
