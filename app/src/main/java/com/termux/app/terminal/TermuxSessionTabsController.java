package com.termux.app.terminal;

import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Typeface;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.HorizontalScrollView;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.termux.R;
import com.termux.app.TermuxActivity;
import com.termux.shared.termux.shell.command.runner.terminal.TermuxSession;
import com.termux.terminal.TerminalSession;

import java.util.List;

public class TermuxSessionTabsController {

    private final TermuxActivity mActivity;
    private final LinearLayout mTabsContainer;
    private final HorizontalScrollView mTabsScroll;
    private int mCurrentSessionIndex = -1;

    public TermuxSessionTabsController(TermuxActivity activity) {
        this.mActivity = activity;
        this.mTabsContainer = activity.findViewById(R.id.session_tabs);
        this.mTabsScroll = activity.findViewById(R.id.session_tabs_scroll);
    }

    public void updateTabs(List<TermuxSession> sessions) {
        if (mTabsContainer == null) return;

        mTabsContainer.removeAllViews();

        TerminalSession currentSession = mActivity.getCurrentSession();
        int currentSessionIndex = -1;

        for (int i = 0; i < sessions.size(); i++) {
            TermuxSession termuxSession = sessions.get(i);
            TerminalSession terminalSession = termuxSession.getTerminalSession();

            if (terminalSession == currentSession) {
                currentSessionIndex = i;
            }

            View tabView = createTabView(termuxSession, i);
            mTabsContainer.addView(tabView);
        }

        if (currentSessionIndex != mCurrentSessionIndex) {
            mCurrentSessionIndex = currentSessionIndex;
            scrollToTab(currentSessionIndex);
        }
    }

    private View createTabView(TermuxSession termuxSession, int position) {
        LayoutInflater inflater = LayoutInflater.from(mActivity);
        View tabView = inflater.inflate(R.layout.item_session_tab, mTabsContainer, false);

        TextView titleView = tabView.findViewById(R.id.session_tab_title);
        ImageButton closeButton = tabView.findViewById(R.id.session_tab_close);

        TerminalSession terminalSession = termuxSession.getTerminalSession();
        if (terminalSession != null) {
            String name = terminalSession.mSessionName;
            String title = terminalSession.getTitle();

            String displayTitle = (name != null && !name.isEmpty()) ? name : 
                                  (title != null && !title.isEmpty()) ? title : "Terminal";
            
            // Truncate if too long
            if (displayTitle.length() > 15) {
                displayTitle = displayTitle.substring(0, 12) + "...";
            }
            
            titleView.setText(displayTitle);

            // Set text color based on session state
            boolean sessionRunning = terminalSession.isRunning();
            if (!sessionRunning && terminalSession.getExitStatus() != 0) {
                titleView.setTextColor(Color.RED);
            } else {
                titleView.setTextColor(Color.WHITE);
            }

            // Strike through for finished sessions
            if (!sessionRunning) {
                titleView.setPaintFlags(titleView.getPaintFlags() | Paint.STRIKE_THRU_TEXT_FLAG);
            } else {
                titleView.setPaintFlags(titleView.getPaintFlags() & ~Paint.STRIKE_THRU_TEXT_FLAG);
            }
        }

        // Highlight current session
        TerminalSession currentSession = mActivity.getCurrentSession();
        boolean isSelected = terminalSession == currentSession;
        tabView.setSelected(isSelected);

        // Click to switch session
        tabView.setOnClickListener(v -> {
            if (terminalSession != null) {
                mActivity.getTermuxTerminalSessionClient().setCurrentSession(terminalSession);
            }
        });

        // Long click to rename
        tabView.setOnLongClickListener(v -> {
            if (terminalSession != null) {
                mActivity.getTermuxTerminalSessionClient().renameSession(terminalSession);
            }
            return true;
        });

        // Close button
        closeButton.setOnClickListener(v -> {
            if (terminalSession != null) {
                mActivity.getTermuxTerminalSessionClient().removeFinishedSession(terminalSession);
            }
        });

        return tabView;
    }

    private void scrollToTab(int index) {
        if (mTabsContainer == null || mTabsScroll == null) return;
        if (index < 0 || index >= mTabsContainer.getChildCount()) return;

        final View tabView = mTabsContainer.getChildAt(index);
        if (tabView == null) return;

        mTabsScroll.post(() -> {
            int scrollX = tabView.getLeft() - mTabsScroll.getWidth() / 2 + tabView.getWidth() / 2;
            mTabsScroll.smoothScrollTo(scrollX, 0);
        });
    }

    public void setCurrentSession(int index) {
        if (mTabsContainer == null) return;
        if (index < 0 || index >= mTabsContainer.getChildCount()) return;

        // Update selection state
        for (int i = 0; i < mTabsContainer.getChildCount(); i++) {
            View child = mTabsContainer.getChildAt(i);
            child.setSelected(i == index);
        }

        mCurrentSessionIndex = index;
        scrollToTab(index);
    }
}
