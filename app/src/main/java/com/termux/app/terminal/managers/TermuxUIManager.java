package com.termux.app.terminal.managers;

import android.app.Activity;
import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.RelativeLayout;

import androidx.annotation.NonNull;
import androidx.drawerlayout.widget.DrawerLayout;
import androidx.viewpager.widget.ViewPager;

import com.termux.R;
import com.termux.app.terminal.TermuxActivityRootView;
import com.termux.app.terminal.io.TerminalToolbarViewPager;
import com.termux.app.terminal.io.TermuxTerminalExtraKeys;
import com.termux.shared.logger.Logger;
import com.termux.shared.termux.extrakeys.ExtraKeysView;
import com.termux.shared.termux.settings.preferences.TermuxAppSharedPreferences;
import com.termux.shared.termux.settings.properties.TermuxAppSharedProperties;
import com.termux.shared.view.ViewUtils;
import com.termux.terminal.TerminalSession;
import com.termux.view.TerminalView;

/**
 * Manages UI components for TermuxActivity.
 * Follows Single Responsibility Principle by handling only UI-related operations.
 */
public class TermuxUIManager {
    
    private static final String LOG_TAG = "TermuxUIManager";
    
    private final Activity mActivity;
    private final TermuxAppSharedPreferences mPreferences;
    private final TermuxAppSharedProperties mProperties;
    
    private TermuxActivityRootView mTermuxActivityRootView;
    private TerminalView mTerminalView;
    private TerminalToolbarViewPager mTerminalToolbarViewPager;
    private ExtraKeysView mExtraKeysView;
    private DrawerLayout mDrawer;
    private ViewPager mTerminalViewPager;
    
    public TermuxUIManager(@NonNull Activity activity, 
                          @NonNull TermuxAppSharedPreferences preferences,
                          @NonNull TermuxAppSharedProperties properties) {
        this.mActivity = activity;
        this.mPreferences = preferences;
        this.mProperties = properties;
    }
    
    /**
     * Initialize all UI components.
     */
    public void initializeUI() {
        mActivity.setContentView(R.layout.activity_termux);
        
        mTermuxActivityRootView = mActivity.findViewById(R.id.activity_termux_root_view);
        mTermuxActivityRootView.setActivity(mActivity);
        
        View content = mActivity.findViewById(android.R.id.content);
        content.setOnApplyWindowInsetsListener((v, insets) -> {
            mTermuxActivityRootView.setPadding(insets.getSystemWindowInsetLeft(),
                    insets.getSystemWindowInsetTop(),
                    insets.getSystemWindowInsetRight(), 0);
            return insets;
        });
        
        initializeTerminalView();
        initializeExtraKeys();
        initializeDrawer();
    }
    
    /**
     * Initialize the terminal view component.
     */
    private void initializeTerminalView() {
        mTerminalView = mActivity.findViewById(R.id.terminal_view);
        mTerminalView.setKeepScreenOn(mProperties.isKeepScreenOn());
        
        mTerminalViewPager = mActivity.findViewById(R.id.terminal_toolbar_view_pager);
        mTerminalToolbarViewPager = new TerminalToolbarViewPager(mActivity, mProperties);
        
        mTerminalViewPager.setAdapter(mTerminalToolbarViewPager);
        mTerminalViewPager.addOnPageChangeListener(new ViewPager.SimpleOnPageChangeListener() {
            @Override
            public void onPageSelected(int position) {
                if (position == 0) {
                    mActivity.getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_VISIBLE);
                } else {
                    mActivity.getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_HIDDEN);
                }
            }
        });
    }
    
    /**
     * Initialize the extra keys view.
     */
    private void initializeExtraKeys() {
        mExtraKeysView = mActivity.findViewById(R.id.extra_keys);
        
        if (mProperties.areExtraKeysEnabled()) {
            showExtraKeys();
        } else {
            hideExtraKeys();
        }
    }
    
    /**
     * Initialize the navigation drawer.
     */
    private void initializeDrawer() {
        mDrawer = mActivity.findViewById(R.id.drawer_layout);
        
        if (mDrawer != null) {
            mDrawer.addDrawerListener(new DrawerLayout.SimpleDrawerListener() {
                @Override
                public void onDrawerOpened(View drawerView) {
                    // Handle drawer opened
                }
                
                @Override
                public void onDrawerClosed(View drawerView) {
                    // Handle drawer closed
                }
            });
        }
    }
    
    /**
     * Show extra keys view.
     */
    public void showExtraKeys() {
        if (mExtraKeysView != null) {
            mExtraKeysView.setVisibility(View.VISIBLE);
            
            // Adjust terminal view height
            adjustTerminalViewHeight(true);
        }
    }
    
    /**
     * Hide extra keys view.
     */
    public void hideExtraKeys() {
        if (mExtraKeysView != null) {
            mExtraKeysView.setVisibility(View.GONE);
            
            // Adjust terminal view height
            adjustTerminalViewHeight(false);
        }
    }
    
    /**
     * Toggle extra keys visibility.
     */
    public void toggleExtraKeys() {
        if (mExtraKeysView != null && mExtraKeysView.getVisibility() == View.VISIBLE) {
            hideExtraKeys();
        } else {
            showExtraKeys();
        }
    }
    
    /**
     * Adjust terminal view height based on extra keys visibility.
     */
    private void adjustTerminalViewHeight(boolean extraKeysVisible) {
        if (mTerminalView == null) return;
        
        RelativeLayout.LayoutParams params = (RelativeLayout.LayoutParams) mTerminalView.getLayoutParams();
        
        if (extraKeysVisible) {
            params.addRule(RelativeLayout.ABOVE, R.id.extra_keys);
        } else {
            params.addRule(RelativeLayout.ABOVE, 0);
        }
        
        mTerminalView.setLayoutParams(params);
    }
    
    /**
     * Open the navigation drawer.
     */
    public void openDrawer() {
        if (mDrawer != null) {
            mDrawer.openDrawer(mActivity.findViewById(R.id.left_drawer));
        }
    }
    
    /**
     * Close the navigation drawer.
     */
    public void closeDrawer() {
        if (mDrawer != null) {
            mDrawer.closeDrawers();
        }
    }
    
    /**
     * Check if drawer is open.
     */
    public boolean isDrawerOpen() {
        return mDrawer != null && mDrawer.isDrawerOpen(mActivity.findViewById(R.id.left_drawer));
    }
    
    /**
     * Get the terminal view.
     */
    public TerminalView getTerminalView() {
        return mTerminalView;
    }
    
    /**
     * Get the extra keys view.
     */
    public ExtraKeysView getExtraKeysView() {
        return mExtraKeysView;
    }
    
    /**
     * Get the terminal toolbar view pager.
     */
    public TerminalToolbarViewPager getTerminalToolbarViewPager() {
        return mTerminalToolbarViewPager;
    }
    
    /**
     * Get the drawer layout.
     */
    public DrawerLayout getDrawer() {
        return mDrawer;
    }
    
    /**
     * Update UI based on preferences changes.
     */
    public void updateUIFromPreferences() {
        if (mTerminalView != null) {
            mTerminalView.setKeepScreenOn(mProperties.isKeepScreenOn());
        }
        
        if (mProperties.areExtraKeysEnabled()) {
            showExtraKeys();
        } else {
            hideExtraKeys();
        }
    }
    
    /**
     * Clean up UI resources.
     */
    public void cleanup() {
        if (mTerminalToolbarViewPager != null) {
            mTerminalToolbarViewPager.cleanup();
        }
    }
}