package com.termux.app;

import android.annotation.SuppressLint;
import android.app.AlertDialog;
import android.content.ActivityNotFoundException;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.net.Uri;
import android.os.Bundle;
import android.os.IBinder;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.Gravity;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.ListView;
import android.widget.RelativeLayout;
import android.widget.Toast;

import com.termux.R;
import com.termux.app.api.file.FileReceiverActivity;
import com.termux.app.terminal.TermuxActivityRootView;
import com.termux.app.terminal.TermuxTerminalSessionActivityClient;
import com.termux.app.terminal.io.TermuxTerminalExtraKeys;
import com.termux.shared.activities.ReportActivity;
import com.termux.shared.activity.ActivityUtils;
import com.termux.shared.activity.media.AppCompatActivityUtils;
import com.termux.shared.data.IntentUtils;
import com.termux.shared.android.PermissionUtils;
import com.termux.shared.data.DataUtils;
import com.termux.shared.termux.TermuxConstants;
import com.termux.shared.termux.TermuxConstants.TERMUX_APP.TERMUX_ACTIVITY;
import com.termux.app.activities.HelpActivity;
import com.termux.app.activities.SettingsActivity;
import com.termux.shared.termux.crash.TermuxCrashUtils;
import com.termux.shared.termux.settings.preferences.TermuxAppSharedPreferences;
import com.termux.app.terminal.TermuxSessionsListViewController;
import com.termux.app.terminal.io.TerminalToolbarViewPager;
import com.termux.app.terminal.TermuxTerminalViewClient;
import com.termux.shared.termux.extrakeys.ExtraKeysView;
import com.termux.shared.termux.interact.TextInputDialogUtils;
import com.termux.shared.logger.Logger;
import com.termux.shared.termux.TermuxUtils;
import com.termux.shared.termux.settings.properties.TermuxAppSharedProperties;
import com.termux.shared.termux.theme.TermuxThemeUtils;
import com.termux.shared.theme.NightMode;
import com.termux.shared.view.ViewUtils;
import com.termux.terminal.TerminalSession;
import com.termux.terminal.TerminalSessionClient;
import com.termux.view.TerminalView;
import com.termux.view.TerminalViewClient;

import com.termux.app.kyzz.appearance.KyzzAppearanceManager;
import com.termux.app.kyzz.editor.KyzzCodeEditorView;
import com.termux.app.kyzz.fonts.KyzzFontManager;
import com.termux.app.kyzz.modules.KyzzModuleItem;
import com.termux.app.kyzz.modules.KyzzModuleRegistry;
import com.termux.app.kyzz.packageadapter.KyzzGitAdapter;
import com.termux.app.kyzz.packageadapter.KyzzNpmAdapter;
import com.termux.app.kyzz.packageadapter.KyzzPipAdapter;
import com.termux.app.kyzz.packageadapter.KyzzPkgAdapter;
import com.termux.app.kyzz.ui.KyzzCommandPalette;
import com.termux.app.kyzz.workspace.KyzzWorkspaceManager;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.SeekBar;
import android.widget.TextView;
import java.io.File;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.drawerlayout.widget.DrawerLayout;
import androidx.viewpager.widget.ViewPager;

import java.util.Arrays;

/**
 * A terminal emulator activity.
 * <p/>
 * See
 * <ul>
 * <li>http://www.mongrel-phones.com.au/default/how_to_make_a_local_service_and_bind_to_it_in_android</li>
 * <li>https://code.google.com/p/android/issues/detail?id=6426</li>
 * </ul>
 * about memory leaks.
 */
public final class TermuxActivity extends AppCompatActivity implements ServiceConnection {

    /**
     * The connection to the {@link TermuxService}. Requested in {@link #onCreate(Bundle)} with a call to
     * {@link #bindService(Intent, ServiceConnection, int)}, and obtained and stored in
     * {@link #onServiceConnected(ComponentName, IBinder)}.
     */
    TermuxService mTermuxService;

    /**
     * The {@link TerminalView} shown in  {@link TermuxActivity} that displays the terminal.
     */
    TerminalView mTerminalView;

    /**
     *  The {@link TerminalViewClient} interface implementation to allow for communication between
     *  {@link TerminalView} and {@link TermuxActivity}.
     */
    TermuxTerminalViewClient mTermuxTerminalViewClient;

    /**
     *  The {@link TerminalSessionClient} interface implementation to allow for communication between
     *  {@link TerminalSession} and {@link TermuxActivity}.
     */
    TermuxTerminalSessionActivityClient mTermuxTerminalSessionActivityClient;

    /**
     * Termux app shared preferences manager.
     */
    private TermuxAppSharedPreferences mPreferences;

    /**
     * Termux app SharedProperties loaded from termux.properties
     */
    private TermuxAppSharedProperties mProperties;

    /**
     * The root view of the {@link TermuxActivity}.
     */
    TermuxActivityRootView mTermuxActivityRootView;

    /**
     * The space at the bottom of {@link @mTermuxActivityRootView} of the {@link TermuxActivity}.
     */
    View mTermuxActivityBottomSpaceView;

    /**
     * The terminal extra keys view.
     */
    ExtraKeysView mExtraKeysView;

    /**
     * The client for the {@link #mExtraKeysView}.
     */
    TermuxTerminalExtraKeys mTermuxTerminalExtraKeys;

    /**
     * The termux sessions list controller.
     */
    TermuxSessionsListViewController mTermuxSessionListViewController;

    /**
     * The {@link TermuxActivity} broadcast receiver for various things like terminal style configuration changes.
     */
    private final BroadcastReceiver mTermuxActivityBroadcastReceiver = new TermuxActivityBroadcastReceiver();

    /**
     * The last toast shown, used cancel current toast before showing new in {@link #showToast(String, boolean)}.
     */
    Toast mLastToast;

    /**
     * If between onResume() and onStop(). Note that only one session is in the foreground of the terminal view at the
     * time, so if the session causing a change is not in the foreground it should probably be treated as background.
     */
    private boolean mIsVisible;

    /**
     * If onResume() was called after onCreate().
     */
    private boolean mIsOnResumeAfterOnCreate = false;

    /**
     * If activity was restarted like due to call to {@link #recreate()} after receiving
     * {@link TERMUX_ACTIVITY#ACTION_RELOAD_STYLE}, system dark night mode was changed or activity
     * was killed by android.
     */
    private boolean mIsActivityRecreated = false;

    /**
     * The {@link TermuxActivity} is in an invalid state and must not be run.
     */
    private boolean mIsInvalidState;

    private int mNavBarHeight;

    private float mTerminalToolbarDefaultHeight;


    private static final int CONTEXT_MENU_SELECT_URL_ID = 0;
    private static final int CONTEXT_MENU_SHARE_TRANSCRIPT_ID = 1;
    private static final int CONTEXT_MENU_SHARE_SELECTED_TEXT = 10;
    private static final int CONTEXT_MENU_AUTOFILL_USERNAME = 11;
    private static final int CONTEXT_MENU_AUTOFILL_PASSWORD = 2;
    private static final int CONTEXT_MENU_RESET_TERMINAL_ID = 3;
    private static final int CONTEXT_MENU_KILL_PROCESS_ID = 4;
    private static final int CONTEXT_MENU_STYLING_ID = 5;
    private static final int CONTEXT_MENU_TOGGLE_KEEP_SCREEN_ON = 6;
    private static final int CONTEXT_MENU_HELP_ID = 7;
    private static final int CONTEXT_MENU_SETTINGS_ID = 8;
    private static final int CONTEXT_MENU_REPORT_ID = 9;

    private static final String ARG_TERMINAL_TOOLBAR_TEXT_INPUT = "terminal_toolbar_text_input";
    private static final String ARG_ACTIVITY_RECREATED = "activity_recreated";

    private static final String LOG_TAG = "TermuxActivity";

    @Override
    public void onCreate(Bundle savedInstanceState) {
        Logger.logDebug(LOG_TAG, "onCreate");
        mIsOnResumeAfterOnCreate = true;

        if (savedInstanceState != null)
            mIsActivityRecreated = savedInstanceState.getBoolean(ARG_ACTIVITY_RECREATED, false);

        // Delete ReportInfo serialized object files from cache older than 14 days
        ReportActivity.deleteReportInfoFilesOlderThanXDays(this, 14, false);

        // Load Termux app SharedProperties from disk
        mProperties = TermuxAppSharedProperties.getProperties();
        reloadProperties();

        setActivityTheme();

        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_termux);

        // Load termux shared preferences
        // This will also fail if TermuxConstants.TERMUX_PACKAGE_NAME does not equal applicationId
        mPreferences = TermuxAppSharedPreferences.build(this, true);
        if (mPreferences == null) {
            // An AlertDialog should have shown to kill the app, so we don't continue running activity code
            mIsInvalidState = true;
            return;
        }

        setMargins();

        mTermuxActivityRootView = findViewById(R.id.activity_termux_root_view);
        mTermuxActivityRootView.setActivity(this);
        mTermuxActivityBottomSpaceView = findViewById(R.id.activity_termux_bottom_space_view);
        mTermuxActivityRootView.setOnApplyWindowInsetsListener(new TermuxActivityRootView.WindowInsetsListener());

        View content = findViewById(android.R.id.content);
        content.setOnApplyWindowInsetsListener((v, insets) -> {
            mNavBarHeight = insets.getSystemWindowInsetBottom();
            return insets;
        });

        if (mProperties.isUsingFullScreen()) {
            getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
        }

        setTermuxTerminalViewAndClients();

        setTerminalToolbarView(savedInstanceState);

        setSettingsButtonView();

        setNewSessionButtonView();

        setToggleKeyboardView();

        registerForContextMenu(mTerminalView);

        initKyzzFeatures();

        FileReceiverActivity.updateFileReceiverActivityComponentsState(this);

        try {
            // Start the {@link TermuxService} and make it run regardless of who is bound to it
            Intent serviceIntent = new Intent(this, TermuxService.class);
            startService(serviceIntent);

            // Attempt to bind to the service, this will call the {@link #onServiceConnected(ComponentName, IBinder)}
            // callback if it succeeds.
            if (!bindService(serviceIntent, this, 0))
                throw new RuntimeException("bindService() failed");
        } catch (Exception e) {
            Logger.logStackTraceWithMessage(LOG_TAG,"TermuxActivity failed to start TermuxService", e);
            Logger.showToast(this,
                getString(e.getMessage() != null && e.getMessage().contains("app is in background") ?
                    R.string.error_termux_service_start_failed_bg : R.string.error_termux_service_start_failed_general),
                true);
            mIsInvalidState = true;
            return;
        }

        // Send the {@link TermuxConstants#BROADCAST_TERMUX_OPENED} broadcast to notify apps that Termux
        // app has been opened.
        TermuxUtils.sendTermuxOpenedBroadcast(this);
    }

    @Override
    public void onStart() {
        super.onStart();

        Logger.logDebug(LOG_TAG, "onStart");

        if (mIsInvalidState) return;

        mIsVisible = true;

        if (mTermuxTerminalSessionActivityClient != null)
            mTermuxTerminalSessionActivityClient.onStart();

        if (mTermuxTerminalViewClient != null)
            mTermuxTerminalViewClient.onStart();

        if (mPreferences.isTerminalMarginAdjustmentEnabled())
            addTermuxActivityRootViewGlobalLayoutListener();

        registerTermuxActivityBroadcastReceiver();
    }

    @Override
    public void onResume() {
        super.onResume();

        Logger.logVerbose(LOG_TAG, "onResume");

        if (mIsInvalidState) return;

        if (mTermuxTerminalSessionActivityClient != null)
            mTermuxTerminalSessionActivityClient.onResume();

        if (mTermuxTerminalViewClient != null)
            mTermuxTerminalViewClient.onResume();

        // Check if a crash happened on last run of the app or if a plugin crashed and show a
        // notification with the crash details if it did
        TermuxCrashUtils.notifyAppCrashFromCrashLogFile(this, LOG_TAG);

        mIsOnResumeAfterOnCreate = false;
    }

    @Override
    protected void onStop() {
        super.onStop();

        Logger.logDebug(LOG_TAG, "onStop");

        if (mIsInvalidState) return;

        mIsVisible = false;

        if (mTermuxTerminalSessionActivityClient != null)
            mTermuxTerminalSessionActivityClient.onStop();

        if (mTermuxTerminalViewClient != null)
            mTermuxTerminalViewClient.onStop();

        removeTermuxActivityRootViewGlobalLayoutListener();

        unregisterTermuxActivityBroadcastReceiver();
        getDrawer().closeDrawers();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();

        Logger.logDebug(LOG_TAG, "onDestroy");

        if (mIsInvalidState) return;

        if (mTermuxService != null) {
            // Do not leave service and session clients with references to activity.
            mTermuxService.unsetTermuxTerminalSessionClient();
            mTermuxService = null;
        }

        try {
            unbindService(this);
        } catch (Exception e) {
            // ignore.
        }
    }

    @Override
    public void onSaveInstanceState(@NonNull Bundle savedInstanceState) {
        Logger.logVerbose(LOG_TAG, "onSaveInstanceState");

        super.onSaveInstanceState(savedInstanceState);
        saveTerminalToolbarTextInput(savedInstanceState);
        savedInstanceState.putBoolean(ARG_ACTIVITY_RECREATED, true);
    }





    /**
     * Part of the {@link ServiceConnection} interface. The service is bound with
     * {@link #bindService(Intent, ServiceConnection, int)} in {@link #onCreate(Bundle)} which will cause a call to this
     * callback method.
     */
    @Override
    public void onServiceConnected(ComponentName componentName, IBinder service) {
        Logger.logDebug(LOG_TAG, "onServiceConnected");

        mTermuxService = ((TermuxService.LocalBinder) service).service;

        setTermuxSessionsListView();

        final Intent intent = getIntent();
        setIntent(null);

        if (mTermuxService.isTermuxSessionsEmpty()) {
            if (mIsVisible) {
                TermuxInstaller.setupBootstrapIfNeeded(TermuxActivity.this, () -> {
                    if (mTermuxService == null) return; // Activity might have been destroyed.
                    try {
                        boolean launchFailsafe = false;
                        if (intent != null && intent.getExtras() != null) {
                            launchFailsafe = intent.getExtras().getBoolean(TERMUX_ACTIVITY.EXTRA_FAILSAFE_SESSION, false);
                        }
                        mTermuxTerminalSessionActivityClient.addNewSession(launchFailsafe, null);
                    } catch (WindowManager.BadTokenException e) {
                        // Activity finished - ignore.
                    }
                });
            } else {
                // The service connected while not in foreground - just bail out.
                finishActivityIfNotFinishing();
            }
        } else {
            // If termux was started from launcher "New session" shortcut and activity is recreated,
            // then the original intent will be re-delivered, resulting in a new session being re-added
            // each time.
            if (!mIsActivityRecreated && intent != null && Intent.ACTION_RUN.equals(intent.getAction())) {
                // Android 7.1 app shortcut from res/xml/shortcuts.xml.
                boolean isFailSafe = intent.getBooleanExtra(TERMUX_ACTIVITY.EXTRA_FAILSAFE_SESSION, false);
                mTermuxTerminalSessionActivityClient.addNewSession(isFailSafe, null);
            } else {
                mTermuxTerminalSessionActivityClient.setCurrentSession(mTermuxTerminalSessionActivityClient.getCurrentStoredSessionOrLast());
            }
        }

        // Update the {@link TerminalSession} and {@link TerminalEmulator} clients.
        mTermuxService.setTermuxTerminalSessionClient(mTermuxTerminalSessionActivityClient);
    }

    @Override
    public void onServiceDisconnected(ComponentName name) {
        Logger.logDebug(LOG_TAG, "onServiceDisconnected");

        // Respect being stopped from the {@link TermuxService} notification action.
        finishActivityIfNotFinishing();
    }






    private void reloadProperties() {
        mProperties.loadTermuxPropertiesFromDisk();

        if (mTermuxTerminalViewClient != null)
            mTermuxTerminalViewClient.onReloadProperties();
    }



    private void setActivityTheme() {
        // Update NightMode.APP_NIGHT_MODE
        TermuxThemeUtils.setAppNightMode(mProperties.getNightMode());

        // Set activity night mode. If NightMode.SYSTEM is set, then android will automatically
        // trigger recreation of activity when uiMode/dark mode configuration is changed so that
        // day or night theme takes affect.
        AppCompatActivityUtils.setNightMode(this, NightMode.getAppNightMode().getName(), true);
    }

    private void setMargins() {
        RelativeLayout relativeLayout = findViewById(R.id.activity_termux_root_relative_layout);
        int marginHorizontal = mProperties.getTerminalMarginHorizontal();
        int marginVertical = mProperties.getTerminalMarginVertical();
        ViewUtils.setLayoutMarginsInDp(relativeLayout, marginHorizontal, marginVertical, marginHorizontal, marginVertical);
    }



    public void addTermuxActivityRootViewGlobalLayoutListener() {
        getTermuxActivityRootView().getViewTreeObserver().addOnGlobalLayoutListener(getTermuxActivityRootView());
    }

    public void removeTermuxActivityRootViewGlobalLayoutListener() {
        if (getTermuxActivityRootView() != null)
            getTermuxActivityRootView().getViewTreeObserver().removeOnGlobalLayoutListener(getTermuxActivityRootView());
    }



    private void setTermuxTerminalViewAndClients() {
        // Set termux terminal view and session clients
        mTermuxTerminalSessionActivityClient = new TermuxTerminalSessionActivityClient(this);
        mTermuxTerminalViewClient = new TermuxTerminalViewClient(this, mTermuxTerminalSessionActivityClient);

        // Set termux terminal view
        mTerminalView = findViewById(R.id.terminal_view);
        mTerminalView.setTerminalViewClient(mTermuxTerminalViewClient);

        if (mTermuxTerminalViewClient != null)
            mTermuxTerminalViewClient.onCreate();

        if (mTermuxTerminalSessionActivityClient != null)
            mTermuxTerminalSessionActivityClient.onCreate();
    }

    private void setTermuxSessionsListView() {
        ListView termuxSessionsListView = findViewById(R.id.terminal_sessions_list);
        mTermuxSessionListViewController = new TermuxSessionsListViewController(this, mTermuxService.getTermuxSessions());
        termuxSessionsListView.setAdapter(mTermuxSessionListViewController);
        termuxSessionsListView.setOnItemClickListener(mTermuxSessionListViewController);
        termuxSessionsListView.setOnItemLongClickListener(mTermuxSessionListViewController);
    }



    private void setTerminalToolbarView(Bundle savedInstanceState) {
        mTermuxTerminalExtraKeys = new TermuxTerminalExtraKeys(this, mTerminalView,
            mTermuxTerminalViewClient, mTermuxTerminalSessionActivityClient);

        final ViewPager terminalToolbarViewPager = getTerminalToolbarViewPager();
        if (mPreferences.shouldShowTerminalToolbar()) terminalToolbarViewPager.setVisibility(View.VISIBLE);

        ViewGroup.LayoutParams layoutParams = terminalToolbarViewPager.getLayoutParams();
        mTerminalToolbarDefaultHeight = layoutParams.height;

        setTerminalToolbarHeight();

        String savedTextInput = null;
        if (savedInstanceState != null)
            savedTextInput = savedInstanceState.getString(ARG_TERMINAL_TOOLBAR_TEXT_INPUT);

        terminalToolbarViewPager.setAdapter(new TerminalToolbarViewPager.PageAdapter(this, savedTextInput));
        terminalToolbarViewPager.addOnPageChangeListener(new TerminalToolbarViewPager.OnPageChangeListener(this, terminalToolbarViewPager));
    }

    private void setTerminalToolbarHeight() {
        final ViewPager terminalToolbarViewPager = getTerminalToolbarViewPager();
        if (terminalToolbarViewPager == null) return;

        ViewGroup.LayoutParams layoutParams = terminalToolbarViewPager.getLayoutParams();
        layoutParams.height = Math.round(mTerminalToolbarDefaultHeight *
            (mTermuxTerminalExtraKeys.getExtraKeysInfo() == null ? 0 : mTermuxTerminalExtraKeys.getExtraKeysInfo().getMatrix().length) *
            mProperties.getTerminalToolbarHeightScaleFactor());
        terminalToolbarViewPager.setLayoutParams(layoutParams);
    }

    public void toggleTerminalToolbar() {
        final ViewPager terminalToolbarViewPager = getTerminalToolbarViewPager();
        if (terminalToolbarViewPager == null) return;

        final boolean showNow = mPreferences.toogleShowTerminalToolbar();
        Logger.showToast(this, (showNow ? getString(R.string.msg_enabling_terminal_toolbar) : getString(R.string.msg_disabling_terminal_toolbar)), true);
        terminalToolbarViewPager.setVisibility(showNow ? View.VISIBLE : View.GONE);
        if (showNow && isTerminalToolbarTextInputViewSelected()) {
            // Focus the text input view if just revealed.
            findViewById(R.id.terminal_toolbar_text_input).requestFocus();
        }
    }

    private void saveTerminalToolbarTextInput(Bundle savedInstanceState) {
        if (savedInstanceState == null) return;

        final EditText textInputView = findViewById(R.id.terminal_toolbar_text_input);
        if (textInputView != null) {
            String textInput = textInputView.getText().toString();
            if (!textInput.isEmpty()) savedInstanceState.putString(ARG_TERMINAL_TOOLBAR_TEXT_INPUT, textInput);
        }
    }



    private void setSettingsButtonView() {
        ImageButton settingsButton = findViewById(R.id.settings_button);
        settingsButton.setOnClickListener(v -> {
            ActivityUtils.startActivity(this, new Intent(this, SettingsActivity.class));
        });
    }

    private void setNewSessionButtonView() {
        View newSessionButton = findViewById(R.id.new_session_button);
        newSessionButton.setOnClickListener(v -> mTermuxTerminalSessionActivityClient.addNewSession(false, null));
        newSessionButton.setOnLongClickListener(v -> {
            TextInputDialogUtils.textInput(TermuxActivity.this, R.string.title_create_named_session, null,
                R.string.action_create_named_session_confirm, text -> mTermuxTerminalSessionActivityClient.addNewSession(false, text),
                R.string.action_new_session_failsafe, text -> mTermuxTerminalSessionActivityClient.addNewSession(true, text),
                -1, null, null);
            return true;
        });
    }

    private void setToggleKeyboardView() {
        findViewById(R.id.toggle_keyboard_button).setOnClickListener(v -> {
            mTermuxTerminalViewClient.onToggleSoftKeyboardRequest();
            getDrawer().closeDrawers();
        });

        findViewById(R.id.toggle_keyboard_button).setOnLongClickListener(v -> {
            toggleTerminalToolbar();
            return true;
        });
    }





    @SuppressLint("RtlHardcoded")
    @Override
    public void onBackPressed() {
        if (getDrawer().isDrawerOpen(Gravity.LEFT)) {
            getDrawer().closeDrawers();
        } else {
            finishActivityIfNotFinishing();
        }
    }

    public void finishActivityIfNotFinishing() {
        // prevent duplicate calls to finish() if called from multiple places
        if (!TermuxActivity.this.isFinishing()) {
            finish();
        }
    }

    /** Show a toast and dismiss the last one if still visible. */
    public void showToast(String text, boolean longDuration) {
        if (text == null || text.isEmpty()) return;
        if (mLastToast != null) mLastToast.cancel();
        mLastToast = Toast.makeText(TermuxActivity.this, text, longDuration ? Toast.LENGTH_LONG : Toast.LENGTH_SHORT);
        mLastToast.setGravity(Gravity.TOP, 0, 0);
        mLastToast.show();
    }



    @Override
    public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo) {
        TerminalSession currentSession = getCurrentSession();
        if (currentSession == null) return;

        boolean autoFillEnabled = mTerminalView.isAutoFillEnabled();

        menu.add(Menu.NONE, CONTEXT_MENU_SELECT_URL_ID, Menu.NONE, R.string.action_select_url);
        menu.add(Menu.NONE, CONTEXT_MENU_SHARE_TRANSCRIPT_ID, Menu.NONE, R.string.action_share_transcript);
        if (!DataUtils.isNullOrEmpty(mTerminalView.getStoredSelectedText()))
            menu.add(Menu.NONE, CONTEXT_MENU_SHARE_SELECTED_TEXT, Menu.NONE, R.string.action_share_selected_text);
        if (autoFillEnabled)
            menu.add(Menu.NONE, CONTEXT_MENU_AUTOFILL_USERNAME, Menu.NONE, R.string.action_autofill_username);
        if (autoFillEnabled)
            menu.add(Menu.NONE, CONTEXT_MENU_AUTOFILL_PASSWORD, Menu.NONE, R.string.action_autofill_password);
        menu.add(Menu.NONE, CONTEXT_MENU_RESET_TERMINAL_ID, Menu.NONE, R.string.action_reset_terminal);
        menu.add(Menu.NONE, CONTEXT_MENU_KILL_PROCESS_ID, Menu.NONE, getResources().getString(R.string.action_kill_process, getCurrentSession().getPid())).setEnabled(currentSession.isRunning());
        menu.add(Menu.NONE, CONTEXT_MENU_STYLING_ID, Menu.NONE, R.string.action_style_terminal);
        menu.add(Menu.NONE, CONTEXT_MENU_TOGGLE_KEEP_SCREEN_ON, Menu.NONE, R.string.action_toggle_keep_screen_on).setCheckable(true).setChecked(mPreferences.shouldKeepScreenOn());
        menu.add(Menu.NONE, CONTEXT_MENU_HELP_ID, Menu.NONE, R.string.action_open_help);
        menu.add(Menu.NONE, CONTEXT_MENU_SETTINGS_ID, Menu.NONE, R.string.action_open_settings);
        menu.add(Menu.NONE, CONTEXT_MENU_REPORT_ID, Menu.NONE, R.string.action_report_issue);
    }

    /** Hook system menu to show context menu instead. */
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        mTerminalView.showContextMenu();
        return false;
    }

    @Override
    public boolean onContextItemSelected(MenuItem item) {
        TerminalSession session = getCurrentSession();

        switch (item.getItemId()) {
            case CONTEXT_MENU_SELECT_URL_ID:
                mTermuxTerminalViewClient.showUrlSelection();
                return true;
            case CONTEXT_MENU_SHARE_TRANSCRIPT_ID:
                mTermuxTerminalViewClient.shareSessionTranscript();
                return true;
            case CONTEXT_MENU_SHARE_SELECTED_TEXT:
                mTermuxTerminalViewClient.shareSelectedText();
                return true;
            case CONTEXT_MENU_AUTOFILL_USERNAME:
                mTerminalView.requestAutoFillUsername();
                return true;
            case CONTEXT_MENU_AUTOFILL_PASSWORD:
                mTerminalView.requestAutoFillPassword();
                return true;
            case CONTEXT_MENU_RESET_TERMINAL_ID:
                onResetTerminalSession(session);
                return true;
            case CONTEXT_MENU_KILL_PROCESS_ID:
                showKillSessionDialog(session);
                return true;
            case CONTEXT_MENU_STYLING_ID:
                showStylingDialog();
                return true;
            case CONTEXT_MENU_TOGGLE_KEEP_SCREEN_ON:
                toggleKeepScreenOn();
                return true;
            case CONTEXT_MENU_HELP_ID:
                ActivityUtils.startActivity(this, new Intent(this, HelpActivity.class));
                return true;
            case CONTEXT_MENU_SETTINGS_ID:
                ActivityUtils.startActivity(this, new Intent(this, SettingsActivity.class));
                return true;
            case CONTEXT_MENU_REPORT_ID:
                mTermuxTerminalViewClient.reportIssueFromTranscript();
                return true;
            default:
                return super.onContextItemSelected(item);
        }
    }

    @Override
    public void onContextMenuClosed(Menu menu) {
        super.onContextMenuClosed(menu);
        // onContextMenuClosed() is triggered twice if back button is pressed to dismiss instead of tap for some reason
        mTerminalView.onContextMenuClosed(menu);
    }

    private void showKillSessionDialog(TerminalSession session) {
        if (session == null) return;

        final AlertDialog.Builder b = new AlertDialog.Builder(this);
        b.setIcon(android.R.drawable.ic_dialog_alert);
        b.setMessage(R.string.title_confirm_kill_process);
        b.setPositiveButton(android.R.string.yes, (dialog, id) -> {
            dialog.dismiss();
            session.finishIfRunning();
        });
        b.setNegativeButton(android.R.string.no, null);
        b.show();
    }

    private void onResetTerminalSession(TerminalSession session) {
        if (session != null) {
            session.reset();
            showToast(getResources().getString(R.string.msg_terminal_reset), true);

            if (mTermuxTerminalSessionActivityClient != null)
                mTermuxTerminalSessionActivityClient.onResetTerminalSession();
        }
    }

    private void showStylingDialog() {
        Intent stylingIntent = new Intent();
        stylingIntent.setClassName(TermuxConstants.TERMUX_STYLING_PACKAGE_NAME, TermuxConstants.TERMUX_STYLING_APP.TERMUX_STYLING_ACTIVITY_NAME);
        try {
            startActivity(stylingIntent);
        } catch (ActivityNotFoundException | IllegalArgumentException e) {
            // The startActivity() call is not documented to throw IllegalArgumentException.
            // However, crash reporting shows that it sometimes does, so catch it here.
            new AlertDialog.Builder(this).setMessage(getString(R.string.error_styling_not_installed))
                .setPositiveButton(R.string.action_styling_install,
                    (dialog, which) -> ActivityUtils.startActivity(this, new Intent(Intent.ACTION_VIEW, Uri.parse(TermuxConstants.TERMUX_STYLING_FDROID_PACKAGE_URL))))
                .setNegativeButton(android.R.string.cancel, null).show();
        }
    }
    private void toggleKeepScreenOn() {
        if (mTerminalView.getKeepScreenOn()) {
            mTerminalView.setKeepScreenOn(false);
            mPreferences.setKeepScreenOn(false);
        } else {
            mTerminalView.setKeepScreenOn(true);
            mPreferences.setKeepScreenOn(true);
        }
    }



    /**
     * For processes to access primary external storage (/sdcard, /storage/emulated/0, ~/storage/shared),
     * termux needs to be granted legacy WRITE_EXTERNAL_STORAGE or MANAGE_EXTERNAL_STORAGE permissions
     * if targeting targetSdkVersion 30 (android 11) and running on sdk 30 (android 11) and higher.
     */
    public void requestStoragePermission(boolean isPermissionCallback) {
        new Thread() {
            @Override
            public void run() {
                // Do not ask for permission again
                int requestCode = isPermissionCallback ? -1 : PermissionUtils.REQUEST_GRANT_STORAGE_PERMISSION;

                // If permission is granted, then also setup storage symlinks.
                if(PermissionUtils.checkAndRequestLegacyOrManageExternalStoragePermission(
                    TermuxActivity.this, requestCode, !isPermissionCallback)) {
                    if (isPermissionCallback)
                        Logger.logInfoAndShowToast(TermuxActivity.this, LOG_TAG,
                            getString(com.termux.shared.R.string.msg_storage_permission_granted_on_request));

                    TermuxInstaller.setupStorageSymlinks(TermuxActivity.this);
                } else {
                    if (isPermissionCallback)
                        Logger.logInfoAndShowToast(TermuxActivity.this, LOG_TAG,
                            getString(com.termux.shared.R.string.msg_storage_permission_not_granted_on_request));
                }
            }
        }.start();
    }

    public static final int REQUEST_PICK_WALLPAPER = 2001;
    public static final int REQUEST_PICK_FONT = 2002;

    @Override
    protected void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        Logger.logVerbose(LOG_TAG, "onActivityResult: requestCode: " + requestCode + ", resultCode: "  + resultCode + ", data: "  + IntentUtils.getIntentString(data));
        if (requestCode == PermissionUtils.REQUEST_GRANT_STORAGE_PERMISSION) {
            requestStoragePermission(true);
        } else if (requestCode == REQUEST_PICK_WALLPAPER && resultCode == RESULT_OK && data != null && data.getData() != null) {
            android.net.Uri selectedImageUri = data.getData();
            try {
                final int takeFlags = Intent.FLAG_GRANT_READ_URI_PERMISSION;
                getContentResolver().takePersistableUriPermission(selectedImageUri, takeFlags);
            } catch (Exception ignored) {}
            if (mKyzzAppearanceManager != null) {
                mKyzzAppearanceManager.setWallpaperUri(selectedImageUri.toString());
                applyKyzzAppearance();
                Toast.makeText(this, "Wallpaper applied!", Toast.LENGTH_SHORT).show();
            }
        } else if (requestCode == REQUEST_PICK_FONT && resultCode == RESULT_OK && data != null && data.getData() != null) {
            android.net.Uri fontUri = data.getData();
            try {
                // Copy font file to app files dir so Typeface can load from file path
                File fontDir = new File(getFilesDir(), "fonts");
                fontDir.mkdirs();
                File targetFontFile = new File(fontDir, "custom_font.ttf");
                java.io.InputStream in = getContentResolver().openInputStream(fontUri);
                java.io.FileOutputStream out = new java.io.FileOutputStream(targetFontFile);
                byte[] buffer = new byte[8192];
                int read;
                while ((read = in.read(buffer)) != -1) {
                    out.write(buffer, 0, read);
                }
                in.close();
                out.close();

                if (mKyzzFontManager != null) {
                    mKyzzFontManager.setCustomFontPath(targetFontFile.getAbsolutePath());
                    android.graphics.Typeface tf = mKyzzFontManager.loadTypeface();
                    if (mTerminalView != null) {
                        mTerminalView.setTypeface(tf);
                    }
                    Toast.makeText(this, "Custom font loaded successfully!", Toast.LENGTH_SHORT).show();
                }
            } catch (Exception e) {
                Toast.makeText(this, "Failed to load font: " + e.getMessage(), Toast.LENGTH_LONG).show();
            }
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        Logger.logVerbose(LOG_TAG, "onRequestPermissionsResult: requestCode: " + requestCode + ", permissions: "  + Arrays.toString(permissions) + ", grantResults: "  + Arrays.toString(grantResults));
        if (requestCode == PermissionUtils.REQUEST_GRANT_STORAGE_PERMISSION) {
            requestStoragePermission(true);
        }
    }



    public int getNavBarHeight() {
        return mNavBarHeight;
    }

    public TermuxActivityRootView getTermuxActivityRootView() {
        return mTermuxActivityRootView;
    }

    public View getTermuxActivityBottomSpaceView() {
        return mTermuxActivityBottomSpaceView;
    }

    public ExtraKeysView getExtraKeysView() {
        return mExtraKeysView;
    }

    public TermuxTerminalExtraKeys getTermuxTerminalExtraKeys() {
        return mTermuxTerminalExtraKeys;
    }

    public void setExtraKeysView(ExtraKeysView extraKeysView) {
        mExtraKeysView = extraKeysView;
    }

    public DrawerLayout getDrawer() {
        return (DrawerLayout) findViewById(R.id.drawer_layout);
    }


    public ViewPager getTerminalToolbarViewPager() {
        return (ViewPager) findViewById(R.id.terminal_toolbar_view_pager);
    }

    public float getTerminalToolbarDefaultHeight() {
        return mTerminalToolbarDefaultHeight;
    }

    public boolean isTerminalViewSelected() {
        return getTerminalToolbarViewPager().getCurrentItem() == 0;
    }

    public boolean isTerminalToolbarTextInputViewSelected() {
        return getTerminalToolbarViewPager().getCurrentItem() == 1;
    }


    public void termuxSessionListNotifyUpdated() {
        mTermuxSessionListViewController.notifyDataSetChanged();
    }

    public boolean isVisible() {
        return mIsVisible;
    }

    public boolean isOnResumeAfterOnCreate() {
        return mIsOnResumeAfterOnCreate;
    }

    public boolean isActivityRecreated() {
        return mIsActivityRecreated;
    }



    public TermuxService getTermuxService() {
        return mTermuxService;
    }

    public TerminalView getTerminalView() {
        return mTerminalView;
    }

    public TermuxTerminalViewClient getTermuxTerminalViewClient() {
        return mTermuxTerminalViewClient;
    }

    public TermuxTerminalSessionActivityClient getTermuxTerminalSessionClient() {
        return mTermuxTerminalSessionActivityClient;
    }

    @Nullable
    public TerminalSession getCurrentSession() {
        if (mTerminalView != null)
            return mTerminalView.getCurrentSession();
        else
            return null;
    }

    public TermuxAppSharedPreferences getPreferences() {
        return mPreferences;
    }

    public TermuxAppSharedProperties getProperties() {
        return mProperties;
    }




    public static void updateTermuxActivityStyling(Context context, boolean recreateActivity) {
        // Make sure that terminal styling is always applied.
        Intent stylingIntent = new Intent(TERMUX_ACTIVITY.ACTION_RELOAD_STYLE);
        stylingIntent.putExtra(TERMUX_ACTIVITY.EXTRA_RECREATE_ACTIVITY, recreateActivity);
        context.sendBroadcast(stylingIntent);
    }

    private void registerTermuxActivityBroadcastReceiver() {
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(TERMUX_ACTIVITY.ACTION_NOTIFY_APP_CRASH);
        intentFilter.addAction(TERMUX_ACTIVITY.ACTION_RELOAD_STYLE);
        intentFilter.addAction(TERMUX_ACTIVITY.ACTION_REQUEST_PERMISSIONS);

        registerReceiver(mTermuxActivityBroadcastReceiver, intentFilter);
    }

    private void unregisterTermuxActivityBroadcastReceiver() {
        unregisterReceiver(mTermuxActivityBroadcastReceiver);
    }

    private void fixTermuxActivityBroadcastReceiverIntent(Intent intent) {
        if (intent == null) return;

        String extraReloadStyle = intent.getStringExtra(TERMUX_ACTIVITY.EXTRA_RELOAD_STYLE);
        if ("storage".equals(extraReloadStyle)) {
            intent.removeExtra(TERMUX_ACTIVITY.EXTRA_RELOAD_STYLE);
            intent.setAction(TERMUX_ACTIVITY.ACTION_REQUEST_PERMISSIONS);
        }
    }

    class TermuxActivityBroadcastReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (intent == null) return;

            if (mIsVisible) {
                fixTermuxActivityBroadcastReceiverIntent(intent);

                switch (intent.getAction()) {
                    case TERMUX_ACTIVITY.ACTION_NOTIFY_APP_CRASH:
                        Logger.logDebug(LOG_TAG, "Received intent to notify app crash");
                        TermuxCrashUtils.notifyAppCrashFromCrashLogFile(context, LOG_TAG);
                        return;
                    case TERMUX_ACTIVITY.ACTION_RELOAD_STYLE:
                        Logger.logDebug(LOG_TAG, "Received intent to reload styling");
                        reloadActivityStyling(intent.getBooleanExtra(TERMUX_ACTIVITY.EXTRA_RECREATE_ACTIVITY, true));
                        return;
                    case TERMUX_ACTIVITY.ACTION_REQUEST_PERMISSIONS:
                        Logger.logDebug(LOG_TAG, "Received intent to request storage permissions");
                        requestStoragePermission(false);
                        return;
                    default:
                }
            }
        }
    }

    private void reloadActivityStyling(boolean recreateActivity) {
        if (mProperties != null) {
            reloadProperties();

            if (mExtraKeysView != null) {
                mExtraKeysView.setButtonTextAllCaps(mProperties.shouldExtraKeysTextBeAllCaps());
                mExtraKeysView.reload(mTermuxTerminalExtraKeys.getExtraKeysInfo(), mTerminalToolbarDefaultHeight);
            }

            // Update NightMode.APP_NIGHT_MODE
            TermuxThemeUtils.setAppNightMode(mProperties.getNightMode());
        }

        setMargins();
        setTerminalToolbarHeight();

        FileReceiverActivity.updateFileReceiverActivityComponentsState(this);

        if (mTermuxTerminalSessionActivityClient != null)
            mTermuxTerminalSessionActivityClient.onReloadActivityStyling();

        if (mTermuxTerminalViewClient != null)
            mTermuxTerminalViewClient.onReloadActivityStyling();

        // To change the activity and drawer theme, activity needs to be recreated.
        // It will destroy the activity, including all stored variables and views, and onCreate()
        // will be called again. Extra keys input text, terminal sessions and transcripts will be preserved.
        if (recreateActivity) {
            Logger.logDebug(LOG_TAG, "Recreating activity");
            TermuxActivity.this.recreate();
        }
    }



    public static void startTermuxActivity(@NonNull final Context context) {
        ActivityUtils.startActivity(context, newInstance(context));
    }

    public static Intent newInstance(@NonNull final Context context) {
        Intent intent = new Intent(context, TermuxActivity.class);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        return intent;
    }

    /* =========================================================================
     * KYZZ TERM EXTENDED FEATURES (Code Mode, Explorer, Modules, Appearance)
     * ========================================================================= */
    private KyzzAppearanceManager mKyzzAppearanceManager;
    private KyzzFontManager mKyzzFontManager;
    private KyzzWorkspaceManager mKyzzWorkspaceManager;
    private View mKyzzCodePanel;
    private View mKyzzExplorerPanel;
    private View mKyzzModulesPanel;
    private View mKyzzAppearancePanel;
    private KyzzCodeEditorView mKyzzCodeEditor;
    private TextView mKyzzEditorFileTitle;
    private TextView mKyzzExplorerPathText;
    private ListView mKyzzExplorerList;
    private ListView mKyzzModulesList;
    private File mKyzzCurrentExplorerDir;
    private ImageView mKyzzWallpaperView;
    private View mKyzzOverlayView;

    private void initKyzzFeatures() {
        mKyzzAppearanceManager = new KyzzAppearanceManager(this);
        mKyzzFontManager = new KyzzFontManager(this);
        mKyzzWorkspaceManager = new KyzzWorkspaceManager(this);

        mKyzzCodePanel = findViewById(R.id.kyzz_code_panel);
        mKyzzExplorerPanel = findViewById(R.id.kyzz_explorer_panel);
        mKyzzModulesPanel = findViewById(R.id.kyzz_modules_panel);
        mKyzzAppearancePanel = findViewById(R.id.kyzz_appearance_panel);
        mKyzzWallpaperView = findViewById(R.id.kyzz_wallpaper_view);
        mKyzzOverlayView = findViewById(R.id.kyzz_overlay_view);

        // Top bar navigation tabs
        Button tabTerminal = findViewById(R.id.kyzz_tab_terminal);
        Button tabCode = findViewById(R.id.kyzz_tab_code);
        Button tabExplorer = findViewById(R.id.kyzz_tab_explorer);
        Button tabModules = findViewById(R.id.kyzz_tab_modules);
        Button tabAppearance = findViewById(R.id.kyzz_tab_appearance);
        Button btnPalette = findViewById(R.id.kyzz_btn_palette);
        ImageButton btnDrawer = findViewById(R.id.kyzz_btn_drawer);

        if (tabTerminal != null) tabTerminal.setOnClickListener(v -> switchKyzzMode("terminal"));
        if (tabCode != null) tabCode.setOnClickListener(v -> switchKyzzMode("code"));
        if (tabExplorer != null) tabExplorer.setOnClickListener(v -> switchKyzzMode("explorer"));
        if (tabModules != null) tabModules.setOnClickListener(v -> switchKyzzMode("modules"));
        if (tabAppearance != null) tabAppearance.setOnClickListener(v -> switchKyzzMode("appearance"));

        if (btnDrawer != null) {
            btnDrawer.setOnClickListener(v -> {
                DrawerLayout drawer = getDrawer();
                if (drawer != null) {
                    if (drawer.isDrawerOpen(Gravity.START)) {
                        drawer.closeDrawer(Gravity.START);
                    } else {
                        drawer.openDrawer(Gravity.START);
                    }
                }
            });
        }

        if (btnPalette != null) {
            btnPalette.setOnClickListener(v -> openKyzzCommandPalette());
        }

        setupKyzzCodePanel();
        setupKyzzExplorerPanel();
        setupKyzzModulesPanel();
        setupKyzzAppearancePanel();
        applyKyzzAppearance();
    }

    private void switchKyzzMode(String mode) {
        if (mTerminalView != null) mTerminalView.setVisibility("terminal".equals(mode) ? View.VISIBLE : View.GONE);
        if (mKyzzCodePanel != null) mKyzzCodePanel.setVisibility("code".equals(mode) ? View.VISIBLE : View.GONE);
        if (mKyzzExplorerPanel != null) {
            mKyzzExplorerPanel.setVisibility("explorer".equals(mode) ? View.VISIBLE : View.GONE);
            if ("explorer".equals(mode)) refreshKyzzExplorerList();
        }
        if (mKyzzModulesPanel != null) {
            mKyzzModulesPanel.setVisibility("modules".equals(mode) ? View.VISIBLE : View.GONE);
            if ("modules".equals(mode)) refreshKyzzModulesList(null);
        }
        if (mKyzzAppearancePanel != null) mKyzzAppearancePanel.setVisibility("appearance".equals(mode) ? View.VISIBLE : View.GONE);

        // Update top bar tab visual active highlight
        int activeColor = android.graphics.Color.parseColor("#1f1f33");
        int inactiveColor = android.graphics.Color.parseColor("#12121e");
        Button tabTerminal = findViewById(R.id.kyzz_tab_terminal);
        Button tabCode = findViewById(R.id.kyzz_tab_code);
        Button tabExplorer = findViewById(R.id.kyzz_tab_explorer);
        Button tabModules = findViewById(R.id.kyzz_tab_modules);
        Button tabAppearance = findViewById(R.id.kyzz_tab_appearance);
        if (tabTerminal != null) tabTerminal.setBackgroundColor("terminal".equals(mode) ? activeColor : inactiveColor);
        if (tabCode != null) tabCode.setBackgroundColor("code".equals(mode) ? activeColor : inactiveColor);
        if (tabExplorer != null) tabExplorer.setBackgroundColor("explorer".equals(mode) ? activeColor : inactiveColor);
        if (tabModules != null) tabModules.setBackgroundColor("modules".equals(mode) ? activeColor : inactiveColor);
        if (tabAppearance != null) tabAppearance.setBackgroundColor("appearance".equals(mode) ? activeColor : inactiveColor);
    }

    private void setupKyzzCodePanel() {
        if (mKyzzCodePanel == null) return;
        mKyzzCodeEditor = findViewById(R.id.kyzz_code_editor);
        mKyzzEditorFileTitle = findViewById(R.id.kyzz_editor_file_title);
        Button btnSave = findViewById(R.id.kyzz_editor_btn_save);
        Button btnFind = findViewById(R.id.kyzz_editor_btn_find);
        Button btnRun = findViewById(R.id.kyzz_editor_btn_run);

        if (btnSave != null) {
            btnSave.setOnClickListener(v -> {
                if (mKyzzCodeEditor != null) {
                    try {
                        mKyzzCodeEditor.saveFile();
                        Toast.makeText(this, "File saved successfully", Toast.LENGTH_SHORT).show();
                        if (mKyzzEditorFileTitle != null && mKyzzCodeEditor.getCurrentFile() != null) {
                            mKyzzEditorFileTitle.setText(mKyzzCodeEditor.getCurrentFile().getName());
                        }
                    } catch (Exception e) {
                        Toast.makeText(this, "Save error: " + e.getMessage(), Toast.LENGTH_LONG).show();
                    }
                }
            });
        }

        if (btnFind != null) {
            btnFind.setOnClickListener(v -> {
                final EditText input = new EditText(this);
                input.setHint("Enter line number");
                input.setInputType(android.text.InputType.TYPE_CLASS_NUMBER);
                new AlertDialog.Builder(this)
                    .setTitle("Go to Line")
                    .setView(input)
                    .setPositiveButton("Go", (d, w) -> {
                        try {
                            int line = Integer.parseInt(input.getText().toString().trim());
                            if (mKyzzCodeEditor != null) mKyzzCodeEditor.goToLine(line);
                        } catch (Exception ignored) {}
                    })
                    .setNegativeButton("Cancel", null)
                    .show();
            });
        }

        if (btnRun != null) {
            btnRun.setOnClickListener(v -> {
                if (mKyzzCodeEditor != null && mKyzzCodeEditor.getCurrentFile() != null) {
                    File f = mKyzzCodeEditor.getCurrentFile();
                    String cmd = "";
                    String name = f.getName().toLowerCase();
                    if (name.endsWith(".py")) cmd = "python \"" + f.getAbsolutePath() + "\"\n";
                    else if (name.endsWith(".js")) cmd = "node \"" + f.getAbsolutePath() + "\"\n";
                    else if (name.endsWith(".sh")) cmd = "bash \"" + f.getAbsolutePath() + "\"\n";
                    else cmd = "\"" + f.getAbsolutePath() + "\"\n";

                    switchKyzzMode("terminal");
                    TerminalSession session = getCurrentSession();
                    if (session != null && !cmd.isEmpty()) {
                        session.write(cmd);
                    }
                } else {
                    Toast.makeText(this, "No active saved script to run", Toast.LENGTH_SHORT).show();
                }
            });
        }

        final TextView statusLang = findViewById(R.id.kyzz_editor_status_lang);
        final TextView statusPos = findViewById(R.id.kyzz_editor_status_pos);
        if (mKyzzCodeEditor != null) {
            mKyzzCodeEditor.setOnCursorChangeListener((line, col, language) -> {
                if (statusLang != null) statusLang.setText(language != null ? language.toUpperCase() : "PLAIN TEXT");
                if (statusPos != null) statusPos.setText("Ln " + line + ", Col " + col);
            });
        }
    }

    private void setupKyzzExplorerPanel() {
        if (mKyzzExplorerPanel == null) return;
        mKyzzExplorerPathText = findViewById(R.id.kyzz_explorer_path_text);
        mKyzzExplorerList = findViewById(R.id.kyzz_explorer_list);
        Button btnUp = findViewById(R.id.kyzz_explorer_btn_up);
        Button btnNewFile = findViewById(R.id.kyzz_explorer_btn_new_file);
        Button btnNewDir = findViewById(R.id.kyzz_explorer_btn_new_dir);

        mKyzzCurrentExplorerDir = new File(mKyzzWorkspaceManager.getCurrentWorkspacePath());

        if (btnUp != null) {
            btnUp.setOnClickListener(v -> {
                if (mKyzzCurrentExplorerDir != null && mKyzzCurrentExplorerDir.getParentFile() != null) {
                    mKyzzCurrentExplorerDir = mKyzzCurrentExplorerDir.getParentFile();
                    refreshKyzzExplorerList();
                }
            });
        }

        if (btnNewFile != null) {
            btnNewFile.setOnClickListener(v -> {
                final EditText input = new EditText(this);
                input.setHint("file name (e.g. main.py)");
                new AlertDialog.Builder(this)
                    .setTitle("Create File")
                    .setView(input)
                    .setPositiveButton("Create", (d, w) -> {
                        String name = input.getText().toString().trim();
                        if (!name.isEmpty() && mKyzzCurrentExplorerDir != null) {
                            try {
                                File nf = new File(mKyzzCurrentExplorerDir, name);
                                nf.createNewFile();
                                refreshKyzzExplorerList();
                            } catch (Exception e) {
                                Toast.makeText(this, "Failed: " + e.getMessage(), Toast.LENGTH_SHORT).show();
                            }
                        }
                    })
                    .setNegativeButton("Cancel", null)
                    .show();
            });
        }

        if (btnNewDir != null) {
            btnNewDir.setOnClickListener(v -> {
                final EditText input = new EditText(this);
                input.setHint("directory name");
                new AlertDialog.Builder(this)
                    .setTitle("Create Directory")
                    .setView(input)
                    .setPositiveButton("Create", (d, w) -> {
                        String name = input.getText().toString().trim();
                        if (!name.isEmpty() && mKyzzCurrentExplorerDir != null) {
                            File nd = new File(mKyzzCurrentExplorerDir, name);
                            nd.mkdirs();
                            refreshKyzzExplorerList();
                        }
                    })
                    .setNegativeButton("Cancel", null)
                    .show();
            });
        }

        if (mKyzzExplorerList != null) {
            mKyzzExplorerList.setOnItemClickListener((parent, view, position, id) -> {
                String itemStr = (String) parent.getItemAtPosition(position);
                boolean isDir = itemStr.startsWith("[DIR] ");
                String name = isDir ? itemStr.substring(6) : itemStr.substring(7);
                File selected = new File(mKyzzCurrentExplorerDir, name);
                if (selected.isDirectory()) {
                    mKyzzCurrentExplorerDir = selected;
                    refreshKyzzExplorerList();
                } else if (selected.isFile()) {
                    try {
                        if (mKyzzCodeEditor != null) {
                            mKyzzCodeEditor.openFile(selected);
                            if (mKyzzEditorFileTitle != null) {
                                mKyzzEditorFileTitle.setText(selected.getName());
                            }
                            switchKyzzMode("code");
                        }
                    } catch (Exception e) {
                        Toast.makeText(this, "Cannot open file: " + e.getMessage(), Toast.LENGTH_SHORT).show();
                    }
                }
            });
        }
    }

    private void refreshKyzzExplorerList() {
        if (mKyzzCurrentExplorerDir == null || !mKyzzCurrentExplorerDir.exists()) {
            mKyzzCurrentExplorerDir = new File(mKyzzWorkspaceManager.getDefaultWorkspacePath());
        }
        if (mKyzzExplorerPathText != null) {
            mKyzzExplorerPathText.setText(mKyzzCurrentExplorerDir.getAbsolutePath());
        }
        File[] files = mKyzzCurrentExplorerDir.listFiles();
        java.util.List<String> items = new java.util.ArrayList<>();
        if (files != null) {
            // Sort: directories first, then files
            java.util.Arrays.sort(files, (f1, f2) -> {
                if (f1.isDirectory() && !f2.isDirectory()) return -1;
                if (!f1.isDirectory() && f2.isDirectory()) return 1;
                return f1.getName().compareToIgnoreCase(f2.getName());
            });
            for (File f : files) {
                if (f.isDirectory()) {
                    items.add("[DIR] " + f.getName());
                } else {
                    items.add("[FILE] " + f.getName());
                }
            }
        }
        if (mKyzzExplorerList != null) {
            ArrayAdapter<String> adapter = new ArrayAdapter<>(this, android.R.layout.simple_list_item_1, items);
            mKyzzExplorerList.setAdapter(adapter);
        }
    }

    private void setupKyzzModulesPanel() {
        if (mKyzzModulesPanel == null) return;
        final EditText searchInput = findViewById(R.id.kyzz_module_search_input);
        Button btnRefresh = findViewById(R.id.kyzz_module_btn_refresh);
        mKyzzModulesList = findViewById(R.id.kyzz_modules_list);

        if (searchInput != null) {
            searchInput.addTextChangedListener(new android.text.TextWatcher() {
                @Override
                public void beforeTextChanged(CharSequence s, int start, int count, int after) {}
                @Override
                public void onTextChanged(CharSequence s, int start, int before, int count) {
                    refreshKyzzModulesList(s.toString());
                }
                @Override
                public void afterTextChanged(android.text.Editable s) {}
            });
        }

        if (btnRefresh != null) {
            btnRefresh.setOnClickListener(v -> {
                String query = searchInput != null ? searchInput.getText().toString() : null;
                refreshKyzzModulesList(query);
            });
        }

        if (mKyzzModulesList != null) {
            mKyzzModulesList.setOnItemClickListener((parent, view, position, id) -> {
                Object itemObj = parent.getItemAtPosition(position);
                if (itemObj instanceof KyzzModuleItem) {
                    KyzzModuleItem module = (KyzzModuleItem) itemObj;
                    new AlertDialog.Builder(this)
                        .setTitle(module.getName() + " (" + module.getCategory() + ")")
                        .setMessage(module.getDescription() + "\n\nCommand: " + module.getInstallCommand())
                        .setPositiveButton("Install in Terminal", (d, w) -> {
                            switchKyzzMode("terminal");
                            TerminalSession session = getCurrentSession();
                            if (session != null) {
                                session.write(module.getInstallCommand() + "\n");
                            }
                        })
                        .setNegativeButton("Cancel", null)
                        .show();
                }
            });
        }
    }

    private void refreshKyzzModulesList(String query) {
        String prefix = TermuxConstants.TERMUX_PREFIX_DIR_PATH;
        java.util.List<KyzzModuleItem> all = KyzzModuleRegistry.getDefaultModules(prefix);
        final java.util.List<KyzzModuleItem> filtered = new java.util.ArrayList<>();
        for (KyzzModuleItem item : all) {
            if (query == null || query.trim().isEmpty() ||
                item.getName().toLowerCase().contains(query.toLowerCase()) ||
                item.getCategory().toLowerCase().contains(query.toLowerCase()) ||
                item.getDescription().toLowerCase().contains(query.toLowerCase())) {
                filtered.add(item);
            }
        }
        if (mKyzzModulesList != null) {
            ArrayAdapter<KyzzModuleItem> adapter = new ArrayAdapter<KyzzModuleItem>(this, android.R.layout.simple_list_item_2, android.R.id.text1, filtered) {
                @NonNull
                @Override
                public View getView(int position, @Nullable View convertView, @NonNull ViewGroup parent) {
                    View view = super.getView(position, convertView, parent);
                    TextView text1 = view.findViewById(android.R.id.text1);
                    TextView text2 = view.findViewById(android.R.id.text2);
                    KyzzModuleItem it = filtered.get(position);
                    text1.setText(it.getName() + " [" + it.getCategory() + "]" + (it.isInstalled() ? " (Installed)" : ""));
                    text1.setTextColor(it.isInstalled() ? android.graphics.Color.parseColor("#c3e88d") : android.graphics.Color.parseColor("#ffffff"));
                    text2.setText(it.getDescription());
                    text2.setTextColor(android.graphics.Color.parseColor("#a0a8b7"));
                    return view;
                }
            };
            mKyzzModulesList.setAdapter(adapter);
        }
    }

    private void setupKyzzAppearancePanel() {
        if (mKyzzAppearancePanel == null) return;
        Button btnKyzz = findViewById(R.id.kyzz_theme_preset_kyzz);
        Button btnVsCode = findViewById(R.id.kyzz_theme_preset_vscode);
        Button btnCyberpunk = findViewById(R.id.kyzz_theme_preset_cyberpunk);
        Button btnPickWall = findViewById(R.id.kyzz_btn_pick_wallpaper);
        Button btnResetWall = findViewById(R.id.kyzz_btn_reset_wallpaper);
        Button btnPickFont = findViewById(R.id.kyzz_btn_pick_font);
        SeekBar seekOpacity = findViewById(R.id.kyzz_seek_opacity);
        final TextView labelOpacity = findViewById(R.id.kyzz_label_opacity);
        SeekBar seekFontSize = findViewById(R.id.kyzz_seek_font_size);
        final TextView labelFontSize = findViewById(R.id.kyzz_label_font_size);

        if (btnPickWall != null) {
            btnPickWall.setOnClickListener(v -> {
                try {
                    Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                    intent.addCategory(Intent.CATEGORY_OPENABLE);
                    intent.setType("image/*");
                    startActivityForResult(intent, REQUEST_PICK_WALLPAPER);
                } catch (Exception e) {
                    // Fallback to GET_CONTENT
                    try {
                        Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
                        intent.setType("image/*");
                        startActivityForResult(intent, REQUEST_PICK_WALLPAPER);
                    } catch (Exception ex) {
                        Toast.makeText(this, "No image picker found: " + ex.getMessage(), Toast.LENGTH_SHORT).show();
                    }
                }
            });
        }

        if (btnPickFont != null) {
            btnPickFont.setOnClickListener(v -> {
                try {
                    Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                    intent.addCategory(Intent.CATEGORY_OPENABLE);
                    intent.setType("*/*");
                    startActivityForResult(intent, REQUEST_PICK_FONT);
                } catch (Exception e) {
                    try {
                        Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
                        intent.setType("*/*");
                        startActivityForResult(intent, REQUEST_PICK_FONT);
                    } catch (Exception ex) {
                        Toast.makeText(this, "No file picker found: " + ex.getMessage(), Toast.LENGTH_SHORT).show();
                    }
                }
            });
        }

        if (btnKyzz != null) btnKyzz.setOnClickListener(v -> {
            mKyzzAppearanceManager.setThemeId("kyzz_dark");
            mKyzzAppearanceManager.setOverlayColor(android.graphics.Color.parseColor("#181824"));
            applyKyzzAppearance();
            Toast.makeText(this, "Applied Kyzz Dark theme", Toast.LENGTH_SHORT).show();
        });

        if (btnVsCode != null) btnVsCode.setOnClickListener(v -> {
            mKyzzAppearanceManager.setThemeId("vscode_dark");
            mKyzzAppearanceManager.setOverlayColor(android.graphics.Color.parseColor("#1e1e1e"));
            applyKyzzAppearance();
            Toast.makeText(this, "Applied VS Code Dark theme", Toast.LENGTH_SHORT).show();
        });

        if (btnCyberpunk != null) btnCyberpunk.setOnClickListener(v -> {
            mKyzzAppearanceManager.setThemeId("cyberpunk");
            mKyzzAppearanceManager.setOverlayColor(android.graphics.Color.parseColor("#1a0026"));
            applyKyzzAppearance();
            Toast.makeText(this, "Applied Cyber Neon theme", Toast.LENGTH_SHORT).show();
        });

        if (btnResetWall != null) btnResetWall.setOnClickListener(v -> {
            mKyzzAppearanceManager.setWallpaperUri(null);
            applyKyzzAppearance();
            Toast.makeText(this, "Wallpaper reset to pure black", Toast.LENGTH_SHORT).show();
        });

        if (seekOpacity != null) {
            seekOpacity.setProgress(mKyzzAppearanceManager.getWallpaperOpacity());
            seekOpacity.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
                @Override
                public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                    if (labelOpacity != null) labelOpacity.setText("Overlay Opacity: " + progress + "%");
                    if (fromUser) {
                        mKyzzAppearanceManager.setWallpaperOpacity(progress);
                        applyKyzzAppearance();
                    }
                }
                @Override
                public void onStartTrackingTouch(SeekBar seekBar) {}
                @Override
                public void onStopTrackingTouch(SeekBar seekBar) {}
            });
        }

        if (seekFontSize != null) {
            int currentSize = mKyzzFontManager.getFontSize();
            seekFontSize.setProgress(currentSize);
            seekFontSize.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
                @Override
                public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                    int size = Math.max(8, progress);
                    if (labelFontSize != null) labelFontSize.setText("Terminal Font Size: " + size + "sp");
                    if (fromUser) {
                        mKyzzFontManager.setFontSize(size);
                        if (mTerminalView != null) mTerminalView.setTextSize(size);
                    }
                }
                @Override
                public void onStartTrackingTouch(SeekBar seekBar) {}
                @Override
                public void onStopTrackingTouch(SeekBar seekBar) {}
            });
        }
    }

    private void applyKyzzAppearance() {
        if (mKyzzAppearanceManager == null) return;
        String wallUri = mKyzzAppearanceManager.getWallpaperUri();
        int opacity = mKyzzAppearanceManager.getWallpaperOpacity();
        int overlayColor = mKyzzAppearanceManager.getOverlayColor();

        if (wallUri != null && mKyzzWallpaperView != null && mKyzzOverlayView != null) {
            try {
                mKyzzWallpaperView.setImageURI(android.net.Uri.parse(wallUri));
                mKyzzWallpaperView.setVisibility(View.VISIBLE);
                mKyzzOverlayView.setVisibility(View.VISIBLE);
                // Calculate alpha from opacity percent 0..100
                float alpha = (float) opacity / 100f;
                mKyzzOverlayView.setBackgroundColor(overlayColor);
                mKyzzOverlayView.setAlpha(alpha);
            } catch (Exception ignored) {
                mKyzzWallpaperView.setVisibility(View.GONE);
                mKyzzOverlayView.setVisibility(View.GONE);
            }
        } else {
            if (mKyzzWallpaperView != null) mKyzzWallpaperView.setVisibility(View.GONE);
            if (mKyzzOverlayView != null) mKyzzOverlayView.setVisibility(View.GONE);
        }
    }

    private void openKyzzCommandPalette() {
        KyzzCommandPalette.show(this, commandId -> {
            switch (commandId) {
                case "term_new":
                    if (mTermuxTerminalSessionActivityClient != null) {
                        mTermuxTerminalSessionActivityClient.addNewSession(false, null);
                    }
                    switchKyzzMode("terminal");
                    break;
                case "term_clear":
                    TerminalSession s = getCurrentSession();
                    if (s != null) s.reset();
                    switchKyzzMode("terminal");
                    break;
                case "term_rename":
                    TerminalSession current = getCurrentSession();
                    if (mTermuxTerminalSessionActivityClient != null && current != null) {
                        mTermuxTerminalSessionActivityClient.renameSession(current);
                    }
                    break;
                case "file_new":
                    switchKyzzMode("explorer");
                    break;
                case "file_open":
                    switchKyzzMode("explorer");
                    break;
                case "file_save":
                    if (mKyzzCodeEditor != null) {
                        try {
                            mKyzzCodeEditor.saveFile();
                            Toast.makeText(this, "Saved", Toast.LENGTH_SHORT).show();
                        } catch (Exception e) {
                            Toast.makeText(this, "Error: " + e.getMessage(), Toast.LENGTH_SHORT).show();
                        }
                    }
                    break;
                case "workspace_open":
                    switchKyzzMode("explorer");
                    break;
                case "module_center":
                    switchKyzzMode("modules");
                    break;
                case "pkg_update":
                    switchKyzzMode("terminal");
                    TerminalSession ts = getCurrentSession();
                    if (ts != null) ts.write("pkg update -y\n");
                    break;
                case "appearance_theme":
                case "appearance_wallpaper":
                case "appearance_font":
                    switchKyzzMode("appearance");
                    break;
                case "app_settings":
                    ActivityUtils.startActivity(this, new Intent(this, SettingsActivity.class));
                    break;
            }
        });
    }

}

