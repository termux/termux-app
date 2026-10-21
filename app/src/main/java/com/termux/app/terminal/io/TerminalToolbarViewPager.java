package com.termux.app.terminal.io;

import android.view.GestureDetector;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;

import androidx.annotation.NonNull;
import androidx.viewpager.widget.PagerAdapter;
import androidx.viewpager.widget.ViewPager;

import com.termux.R;
import com.termux.app.TermuxActivity;
import com.termux.shared.termux.extrakeys.ExtraKeysView;
import com.termux.terminal.TerminalSession;

public class TerminalToolbarViewPager {

    public static class PageAdapter extends PagerAdapter {

        final TermuxActivity mActivity;
        String mSavedTextInput;
        private final TextInputHistory mTextInputHistory;

        public PageAdapter(TermuxActivity activity, String savedTextInput) {
            this.mActivity = activity;
            this.mSavedTextInput = savedTextInput;
            this.mTextInputHistory = new TextInputHistory();
        }

        @Override
        public int getCount() {
            return 2;
        }

        @Override
        public boolean isViewFromObject(@NonNull View view, @NonNull Object object) {
            return view == object;
        }

        @NonNull
        @Override
        public Object instantiateItem(@NonNull ViewGroup collection, int position) {
            LayoutInflater inflater = LayoutInflater.from(mActivity);
            View layout;
            if (position == 0) {
                layout = inflater.inflate(R.layout.view_terminal_toolbar_extra_keys, collection, false);
                ExtraKeysView extraKeysView = (ExtraKeysView) layout;
                extraKeysView.setExtraKeysViewClient(mActivity.getTermuxTerminalExtraKeys());
                extraKeysView.setButtonTextAllCaps(mActivity.getProperties().shouldExtraKeysTextBeAllCaps());
                mActivity.setExtraKeysView(extraKeysView);
                extraKeysView.reload(mActivity.getTermuxTerminalExtraKeys().getExtraKeysInfo(),
                    mActivity.getTerminalToolbarDefaultHeight());

                // apply extra keys fix if enabled in prefs
                if (mActivity.getProperties().isUsingFullScreen() && mActivity.getProperties().isUsingFullScreenWorkAround()) {
                    FullScreenWorkAround.apply(mActivity);
                }

            } else {
                layout = inflater.inflate(R.layout.view_terminal_toolbar_text_input, collection, false);
                final EditText editText = layout.findViewById(R.id.terminal_toolbar_text_input);

                if (mSavedTextInput != null) {
                    editText.setText(mSavedTextInput);
                    mSavedTextInput = null;
                }

                // Set up gesture detection for up/down swipes
                setupTextInputGestureDetection(editText);

                editText.setOnEditorActionListener((v, actionId, event) -> {
                    TerminalSession session = mActivity.getCurrentSession();
                    if (session != null) {
                        if (session.isRunning()) {
                            String textToSend = editText.getText().toString();
                            if (textToSend.length() == 0) textToSend = "\r";

                            // Add to history before sending (ignore empty entries and carriage returns)
                            if (!textToSend.equals("\r") && !textToSend.trim().isEmpty()) {
                                mTextInputHistory.addEntry(textToSend);
                            }

                            session.write(textToSend);
                        } else {
                            mActivity.getTermuxTerminalSessionClient().removeFinishedSession(session);
                        }
                        editText.setText("");
                        mTextInputHistory.resetNavigation(); // Reset navigation after submission
                    }
                    return true;
                });
            }
            collection.addView(layout);
            return layout;
        }

        @Override
        public void destroyItem(@NonNull ViewGroup collection, int position, @NonNull Object view) {
            collection.removeView((View) view);
        }

        /**
         * Sets up gesture detection for the text input EditText to handle up/down swipes
         * for history navigation while preserving horizontal swipes for ViewPager.
         */
        private void setupTextInputGestureDetection(EditText editText) {
            GestureDetector gestureDetector = new GestureDetector(mActivity,
                new GestureDetector.SimpleOnGestureListener() {

                    private static final int SWIPE_THRESHOLD = 100;
                    private static final int SWIPE_VELOCITY_THRESHOLD = 100;

                    @Override
                    public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {
                        if (e1 == null || e2 == null) return false;

                        float diffY = e2.getY() - e1.getY();
                        float diffX = e2.getX() - e1.getX();

                        // Only handle vertical swipes (up/down)
                        if (Math.abs(diffY) > Math.abs(diffX) &&
                            Math.abs(diffY) > SWIPE_THRESHOLD &&
                            Math.abs(velocityY) > SWIPE_VELOCITY_THRESHOLD) {

                            if (diffY > 0) {
                                // Swipe down - navigate to newer entries
                                navigateHistoryDown(editText);
                            } else {
                                // Swipe up - navigate to older entries
                                navigateHistoryUp(editText);
                            }
                            return true;
                        }
                        return false;
                    }
                });

            editText.setOnTouchListener((v, event) -> {
                // Let the GestureDetector handle the event first
                boolean handled = gestureDetector.onTouchEvent(event);

                // If it wasn't a vertical swipe gesture, let the normal touch handling proceed
                // This preserves the EditText's normal text selection and cursor positioning
                if (!handled) {
                    v.performClick();
                    return false; // Let the EditText handle the touch normally
                }
                return true; // We handled the gesture
            });
        }

        /**
         * Navigates up in history (to older entries) and updates the EditText.
         */
        private void navigateHistoryUp(EditText editText) {
            String currentText = editText.getText().toString();
            String historyEntry = mTextInputHistory.navigateUp(currentText);

            if (historyEntry != null) {
                editText.setText(historyEntry);
                editText.setSelection(historyEntry.length()); // Move cursor to end
            }
        }

        /**
         * Navigates down in history (to newer entries) and updates the EditText.
         */
        private void navigateHistoryDown(EditText editText) {
            String historyEntry = mTextInputHistory.navigateDown();

            if (historyEntry != null) {
                editText.setText(historyEntry);
                editText.setSelection(historyEntry.length()); // Move cursor to end
            }
        }

    }



    public static class OnPageChangeListener extends ViewPager.SimpleOnPageChangeListener {

        final TermuxActivity mActivity;
        final ViewPager mTerminalToolbarViewPager;
        private PageAdapter mPageAdapter;

        public OnPageChangeListener(TermuxActivity activity, ViewPager viewPager) {
            this.mActivity = activity;
            this.mTerminalToolbarViewPager = viewPager;
        }

        /**
         * Sets the PageAdapter reference so we can access the text input history.
         */
        public void setPageAdapter(PageAdapter pageAdapter) {
            this.mPageAdapter = pageAdapter;
        }

        @Override
        public void onPageSelected(int position) {
            if (position == 0) {
                // Switching away from text input - reset navigation
                if (mPageAdapter != null) {
                    mPageAdapter.mTextInputHistory.resetNavigation();
                }
                mActivity.getTerminalView().requestFocus();
            } else {
                final EditText editText = mTerminalToolbarViewPager.findViewById(R.id.terminal_toolbar_text_input);
                if (editText != null) editText.requestFocus();
            }
        }

    }

}
