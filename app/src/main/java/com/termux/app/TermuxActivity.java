package com.termux.app;

import android.Manifest;
import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.ActivityNotFoundException;
import android.content.BroadcastReceiver;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Typeface;
import android.media.AudioAttributes;
import android.media.SoundPool;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.IBinder;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.TextUtils;
import android.text.style.StyleSpan;
import android.util.Log;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
import android.widget.ArrayAdapter;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import com.termux.R;
import com.termux.terminal.EmulatorDebug;
import com.termux.terminal.TerminalColors;
import com.termux.terminal.TerminalSession;
import com.termux.terminal.TerminalSession.SessionChangedCallback;
import com.termux.terminal.TextStyle;
import com.termux.view.TerminalView;

import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;
import java.util.Arrays;
import java.util.Collections;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Properties;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.drawerlayout.widget.DrawerLayout;
import androidx.viewpager.widget.PagerAdapter;
import androidx.viewpager.widget.ViewPager;

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
public final class TermuxActivity extends Activity implements ServiceConnection {

    public static final String TERMUX_FAILSAFE_SESSION_ACTION = "com.termux.app.failsafe_session";

    private static final int CONTEXTMENU_SELECT_URL_ID = 0;
    private static final int CONTEXTMENU_SHARE_TRANSCRIPT_ID = 1;
    private static final int CONTEXTMENU_PASTE_ID = 3;
    private static final int CONTEXTMENU_KILL_PROCESS_ID = 4;
    private static final int CONTEXTMENU_RESET_TERMINAL_ID = 5;
    private static final int CONTEXTMENU_STYLING_ID = 6;
    private static final int CONTEXTMENU_HELP_ID = 8;
    private static final int CONTEXTMENU_TOGGLE_KEEP_SCREEN_ON = 9;

    private static final int MAX_SESSIONS = 8;

    private static final int REQUESTCODE_PERMISSION_STORAGE = 1234;

    private static final String RELOAD_STYLE_ACTION = "com.termux.app.reload_style";

    /** The main view of the activity showing the terminal. Initialized in onCreate(). */
    @SuppressWarnings("NullableProblems")
    @NonNull
    TerminalView mTerminalView;

    ExtraKeysView mExtraKeysView;

    TermuxPreferences mSettings;

    /**
     * The connection to the {@link TermuxService}. Requested in {@link #onCreate(Bundle)} with a call to
     * {@link #bindService(Intent, ServiceConnection, int)}, and obtained and stored in
     * {@link #onServiceConnected(ComponentName, IBinder)}.
     */
    TermuxService mTermService;

    /** Initialized in {@link #onServiceConnected(ComponentName, IBinder)}. */
    ArrayAdapter<TerminalSession> mListViewAdapter;

    /** The last toast shown, used cancel current toast before showing new in {@link #showToast(String, boolean)}. */
    Toast mLastToast;

    /**
     * If between onResume() and onStop(). Note that only one session is in the foreground of the terminal view at the
     * time, so if the session causing a change is not in the foreground it should probably be treated as background.
     */
    boolean mIsVisible;

    boolean mIsUsingBlackUI;

    final SoundPool mBellSoundPool = new SoundPool.Builder().setMaxStreams(1).setAudioAttributes(
        new AudioAttributes.Builder().setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
            .setUsage(AudioAttributes.USAGE_ASSISTANCE_SONIFICATION).build()).build();
    int mBellSoundId;

    private final BroadcastReceiver mBroadcastReceiever = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (mIsVisible) {
                String whatToReload = intent.getStringExtra(RELOAD_STYLE_ACTION);
                if ("storage".equals(whatToReload)) {
                    if (ensureStoragePermissionGranted())
                        TermuxInstaller.setupStorageSymlinks(TermuxActivity.this);
                    return;
                }
                checkForFontAndColors();
                mSettings.reloadFromProperties(TermuxActivity.this);

                if (mExtraKeysView != null) {
                    mExtraKeysView.reload(mSettings.mExtraKeys, ExtraKeysView.defaultCharDisplay);
                }
            }
        }
    };

    void checkForFontAndColors() {
        try {
            @SuppressLint("SdCardPath") File fontFile = new File("/data/data/com.termux/files/home/.termux/font.ttf");
            @SuppressLint("SdCardPath") File colorsFile = new File("/data/data/com.termux/files/home/.termux/colors.properties");

            final Properties props = new Properties();
            if (colorsFile.isFile()) {
                try (InputStream in = new FileInputStream(colorsFile)) {
                    props.load(in);
                }
            }

            TerminalColors.COLOR_SCHEME.updateWith(props);
            TerminalSession session = getCurrentTermSession();
            if (session != null && session.getEmulator() != null) {
                session.getEmulator().mColors.reset();
            }
            updateBackgroundColor();

            final Typeface newTypeface = (fontFile.exists() && fontFile.length() > 0) ? Typeface.createFromFile(fontFile) : Typeface.MONOSPACE;
            mTerminalView.setTypeface(newTypeface);
        } catch (Exception e) {
            Log.e(EmulatorDebug.LOG_TAG, "Error in checkForFontAndColors()", e);
        }
    }

    void updateBackgroundColor() {
        TerminalSession session = getCurrentTermSession();
        if (session != null && session.getEmulator() != null) {
            getWindow().getDecorView().setBackgroundColor(session.getEmulator().mColors.mCurrentColors[TextStyle.COLOR_INDEX_BACKGROUND]);
        }
    }

    /** For processes to access shared internal storage (/sdcard) we need this permission. */
    public boolean ensureStoragePermissionGranted() {
        if (checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED) {
            return true;
        } else {
            requestPermissions(new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, REQUESTCODE_PERMISSION_STORAGE);
            return false;
        }
    }

    @Override
    public void onCreate(Bundle bundle) {
        mSettings = new TermuxPreferences(this);
        mIsUsingBlackUI = mSettings.isUsingBlackUI();
        if (mIsUsingBlackUI) {
            this.setTheme(R.style.Theme_Termux_Black);
        } else {
            this.setTheme(R.style.Theme_Termux);
        }

        super.onCreate(bundle);

        setContentView(R.layout.drawer_layout);

        if (mIsUsingBlackUI) {
            findViewById(R.id.left_drawer).setBackgroundColor(
                getResources().getColor(android.R.color.background_dark)
            );
        }

        mTerminalView = findViewById(R.id.terminal_view);
        mTerminalView.setOnKeyListener(new TermuxViewClient(this));

        mTerminalView.setTextSize(mSettings.getFontSize());
        mTerminalView.setKeepScreenOn(mSettings.isScreenAlwaysOn());
        mTerminalView.requestFocus();

        final ViewPager viewPager = findViewById(R.id.viewpager);
        if (mSettings.mShowExtraKeys) viewPager.setVisibility(View.VISIBLE);


        ViewGroup.LayoutParams layoutParams = viewPager.getLayoutParams();
        layoutParams.height = layoutParams.height * mSettings.mExtraKeys.length;
        viewPager.setLayoutParams(layoutParams);

        viewPager.setAdapter(new PagerAdapter() {
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
                LayoutInflater inflater = LayoutInflater.from(TermuxActivity.this);
                View layout;
                if (position == 0) {
                    layout = mExtraKeysView = (ExtraKeysView) inflater.inflate(R.layout.extra_keys_main, collection, false);
                    mExtraKeysView.reload(mSettings.mExtraKeys, ExtraKeysView.defaultCharDisplay);
                } else {
                    layout = inflater.inflate(R.layout.extra_keys_right, collection, false);
                    final EditText editText = layout.findViewById(R.id.text_input);
                    editText.setOnEditorActionListener((v, actionId, event) -> {
                        TerminalSession session = getCurrentTermSession();
                        if (session != null) {
                            if (session.isRunning()) {
                                String textToSend = editText.getText().toString();
                                if (textToSend.length() == 0) textToSend = "\r";
                                session.write(textToSend);
                            } else {
                                removeFinishedSession(session);
                            }
                            editText.setText("");
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
        });

        viewPager.addOnPageChangeListener(new ViewPager.SimpleOnPageChangeListener() {
            @Override
            public void onPageSelected(int position) {
                if (position == 0) {
                    mTerminalView.requestFocus();
                } else {
                    final EditText editText = viewPager.findViewById(R.id.text_input);
                    if (editText != null) editText.requestFocus();
                }
            }
        });

        View newSessionButton = findViewById(R.id.new_session_button);
        newSessionButton.setOnClickListener(v -> addNewSession(false, null));
        newSessionButton.setOnLongClickListener(v -> {
            DialogUtils.textInput(TermuxActivity.this, R.string.session_new_named_title, null, R.string.session_new_named_positive_button,
                text -> addNewSession(false, text), R.string.new_session_failsafe, text -> addNewSession(true, text)
                , -1, null, null);
            return true;
        });

        findViewById(R.id.toggle_keyboard_button).setOnClickListener(v -> {
            InputMethodManager imm = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
            imm.toggleSoftInput(InputMethodManager.SHOW_IMPLICIT, 0);
            getDrawer().closeDrawers();
        });

        findViewById(R.id.toggle_keyboard_button).setOnLongClickListener(v -> {
            toggleShowExtraKeys();
            return true;
        });

        registerForContextMenu(mTerminalView);

        Intent serviceIntent = new Intent(this, TermuxService.class);
        // Start the service and make it run regardless of who is bound to it:
        startService(serviceIntent);
        if (!bindService(serviceIntent, this, 0))
            throw new RuntimeException("bindService() failed");

        checkForFontAndColors();

        mBellSoundId = mBellSoundPool.load(this, R.raw.bell, 1);
    }

    void toggleShowExtraKeys() {
        final ViewPager viewPager = findViewById(R.id.viewpager);
        final boolean showNow = mSettings.toggleShowExtraKeys(TermuxActivity.this);
        viewPager.setVisibility(showNow ? View.VISIBLE : View.GONE);
        if (showNow && viewPager.getCurrentItem() == 1) {
            // Focus the text input view if just revealed.
            findViewById(R.id.text_input).requestFocus();
        }
    }

    /**
     * Part of the {@link ServiceConnection} interface. The service is bound with
     * {@link #bindService(Intent, ServiceConnection, int)} in {@link #onCreate(Bundle)} which will cause a call to this
     * callback method.
     */
    @Override
    public void onServiceConnected(ComponentName componentName, IBinder service) {
        mTermService = ((TermuxService.LocalBinder) service).service;

        mTermService.mSessionChangeCallback = new SessionChangedCallback() {
            @Override
            public void onTextChanged(TerminalSession changedSession) {
                if (!mIsVisible) return;
                if (getCurrentTermSession() == changedSession) mTerminalView.onScreenUpdated();
            }

            @Override
            public void onTitleChanged(TerminalSession updatedSession) {
                if (!mIsVisible) return;
                if (updatedSession != getCurrentTermSession()) {
                    // Only show toast for other sessions than the current one, since the user
                    // probably consciously caused the title change to change in the current session
                    // and don't want an annoying toast for that.
                    showToast(toToastTitle(updatedSession), false);
                }
                mListViewAdapter.notifyDataSetChanged();
            }

            @Override
            public void onSessionFinished(final TerminalSession finishedSession) {
                if (mTermService.mWantsToStop) {
                    // The service wants to stop as soon as possible.
                    finish();
                    return;
                }
                if (mIsVisible && finishedSession != getCurrentTermSession()) {
                    // Show toast for non-current sessions that exit.
                    int indexOfSession = mTermService.getSessions().indexOf(finishedSession);
                    // Verify that session was not removed before we got told about it finishing:
                    if (indexOfSession >= 0)
                        showToast(toToastTitle(finishedSession) + " - exited", true);
                }

                if (getPackageManager().hasSystemFeature(PackageManager.FEATURE_LEANBACK)) {
                    // On Android TV devices we need to use older behaviour because we may
                    // not be able to have multiple launcher icons.
                    if (mTermService.getSessions().size() > 1) {
                        removeFinishedSession(finishedSession);
                    }
                } else {
                    // Once we have a separate launcher icon for the failsafe session, it
                    // should be safe to auto-close session on exit code '0' or '130'.
                    if (finishedSession.getExitStatus() == 0 || finishedSession.getExitStatus() == 130) {
                        removeFinishedSession(finishedSession);
                    }
                }

                mListViewAdapter.notifyDataSetChanged();
            }

            @Override
            public void onClipboardText(TerminalSession session, String text) {
                if (!mIsVisible) return;
                ClipboardManager clipboard = (ClipboardManager) getSystemService(Context.CLIPBOARD_SERVICE);
                clipboard.setPrimaryClip(new ClipData(null, new String[]{"text/plain"}, new ClipData.Item(text)));
            }

            @Override
            public void onBell(TerminalSession session) {
                if (!mIsVisible) return;

                switch (mSettings.mBellBehaviour) {
                    case TermuxPreferences.BELL_BEEP:
                        mBellSoundPool.play(mBellSoundId, 1.f, 1.f, 1, 0, 1.f);
                        break;
                    case TermuxPreferences.BELL_VIBRATE:
                        BellUtil.getInstance(TermuxActivity.this).doBell();
                        break;
                    case TermuxPreferences.BELL_IGNORE:
                        // Ignore the bell character.
                        break;
                }

            }

            @Override
            public void onColorsChanged(TerminalSession changedSession) {
                if (getCurrentTermSession() == changedSession) updateBackgroundColor();
            }
        };

        ListView listView = findViewById(R.id.left_drawer_list);
        mListViewAdapter = new ArrayAdapter<TerminalSession>(getApplicationContext(), R.layout.line_in_drawer, mTermService.getSessions()) {
            final StyleSpan boldSpan = new StyleSpan(Typeface.BOLD);
            final StyleSpan italicSpan = new StyleSpan(Typeface.ITALIC);

            @NonNull
            @Override
            public View getView(int position, View convertView, @NonNull ViewGroup parent) {
                View row = convertView;
                if (row == null) {
                    LayoutInflater inflater = getLayoutInflater();
                    row = inflater.inflate(R.layout.line_in_drawer, parent, false);
                }

                TerminalSession sessionAtRow = getItem(position);
                boolean sessionRunning = sessionAtRow.isRunning();

                TextView firstLineView = row.findViewById(R.id.row_line);
                if (mIsUsingBlackUI) {
                    firstLineView.setBackground(
                        getResources().getDrawable(R.drawable.selected_session_background_black)
                    );
                }
                String name = sessionAtRow.mSessionName;
                String sessionTitle = sessionAtRow.getTitle();

                String numberPart = "[" + (position + 1) + "] ";
                String sessionNamePart = (TextUtils.isEmpty(name) ? "" : name);
                String sessionTitlePart = (TextUtils.isEmpty(sessionTitle) ? "" : ((sessionNamePart.isEmpty() ? "" : "\n") + sessionTitle));

                String text = numberPart + sessionNamePart + sessionTitlePart;
                SpannableString styledText = new SpannableString(text);
                styledText.setSpan(boldSpan, 0, numberPart.length() + sessionNamePart.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
                styledText.setSpan(italicSpan, numberPart.length() + sessionNamePart.length(), text.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);

                firstLineView.setText(styledText);

                if (sessionRunning) {
                    firstLineView.setPaintFlags(firstLineView.getPaintFlags() & ~Paint.STRIKE_THRU_TEXT_FLAG);
                } else {
                    firstLineView.setPaintFlags(firstLineView.getPaintFlags() | Paint.STRIKE_THRU_TEXT_FLAG);
                }
                int defaultColor = mIsUsingBlackUI ? Color.WHITE : Color.BLACK;
                int color = sessionRunning || sessionAtRow.getExitStatus() == 0 ? defaultColor : Color.RED;
                firstLineView.setTextColor(color);
                return row;
            }
        };
        listView.setAdapter(mListViewAdapter);
        listView.setOnItemClickListener((parent, view, position, id) -> {
            TerminalSession clickedSession = mListViewAdapter.getItem(position);
            switchToSession(clickedSession);
            getDrawer().closeDrawers();
        });
        listView.setOnItemLongClickListener((parent, view, position, id) -> {
            final TerminalSession selectedSession = mListViewAdapter.getItem(position);
            renameSession(selectedSession);
            return true;
        });

        if (mTermService.getSessions().isEmpty()) {
            if (mIsVisible) {
                TermuxInstaller.setupIfNeeded(TermuxActivity.this, () -> {
                    if (mTermService == null) return; // Activity might have been destroyed.
                    try {
                        Bundle bundle = getIntent().getExtras();
                        boolean launchFailsafe = false;
                        if (bundle != null) {
                            launchFailsafe = bundle.getBoolean(TERMUX_FAILSAFE_SESSION_ACTION, false);
                        }
                        addNewSession(launchFailsafe, null);
                    } catch (WindowManager.BadTokenException e) {
                        // Activity finished - ignore.
                    }
                });
            } else {
                // The service connected while not in foreground - just bail out.
                finish();
            }
        } else {
            Intent i = getIntent();
            if (i != null && Intent.ACTION_RUN.equals(i.getAction())) {
                // Android 7.1 app shortcut from res/xml/shortcuts.xml.
                boolean failSafe = i.getBooleanExtra(TERMUX_FAILSAFE_SESSION_ACTION, false);
                addNewSession(failSafe, null);
            } else {
                switchToSession(getStoredCurrentSessionOrLast());
            }
        }
    }

    public void switchToSession(boolean forward) {
        TerminalSession currentSession = getCurrentTermSession();
        int index = mTermService.getSessions().indexOf(currentSession);
        if (forward) {
            if (++index >= mTermService.getSessions().size()) index = 0;
        } else {
            if (--index < 0) index = mTermService.getSessions().size() - 1;
        }
        switchToSession(mTermService.getSessions().get(index));
    }

    @SuppressLint("InflateParams")
    void renameSession(final TerminalSession sessionToRename) {
        DialogUtils.textInput(this, R.string.session_rename_title, sessionToRename.mSessionName, R.string.session_rename_positive_button, text -> {
            sessionToRename.mSessionName = text;
            mListViewAdapter.notifyDataSetChanged();
        }, -1, null, -1, null, null);
    }

    @Override
    public void onServiceDisconnected(ComponentName name) {
        // Respect being stopped from the TermuxService notification action.
        finish();
    }

    @Nullable
    TerminalSession getCurrentTermSession() {
        return mTerminalView.getCurrentSession();
    }

    @Override
    public void onStart() {
        super.onStart();
        mIsVisible = true;

        if (mTermService != null) {
            // The service has connected, but data may have changed since we were last in the foreground.
            switchToSession(getStoredCurrentSessionOrLast());
            mListViewAdapter.notifyDataSetChanged();
        }

        registerReceiver(mBroadcastReceiever, new IntentFilter(RELOAD_STYLE_ACTION));

        // The current terminal session may have changed while being away, force
        // a refresh of the displayed terminal:
        mTerminalView.onScreenUpdated();
    }

    @Override
    protected void onStop() {
        super.onStop();
        mIsVisible = false;
        TerminalSession currentSession = getCurrentTermSession();
        if (currentSession != null) TermuxPreferences.storeCurrentSession(this, currentSession);
        unregisterReceiver(mBroadcastReceiever);
        getDrawer().closeDrawers();
    }

    @Override
    public void onBackPressed() {
        if (getDrawer().isDrawerOpen(Gravity.LEFT)) {
            getDrawer().closeDrawers();
        } else {
            finish();
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (mTermService != null) {
            // Do not leave service with references to activity.
            mTermService.mSessionChangeCallback = null;
            mTermService = null;
        }
        unbindService(this);
    }

    DrawerLayout getDrawer() {
        return (DrawerLayout) findViewById(R.id.drawer_layout);
    }

    void addNewSession(boolean failSafe, String sessionName) {
        if (mTermService.getSessions().size() >= MAX_SESSIONS) {
            new AlertDialog.Builder(this).setTitle(R.string.max_terminals_reached_title).setMessage(R.string.max_terminals_reached_message)
                .setPositiveButton(android.R.string.ok, null).show();
        } else {
            TerminalSession currentSession = getCurrentTermSession();
            String workingDirectory = (currentSession == null) ? null : currentSession.getCwd();
            TerminalSession newSession = mTermService.createTermSession(null, null, workingDirectory, failSafe);
            if (sessionName != null) {
                newSession.mSessionName = sessionName;
            }
            switchToSession(newSession);
            getDrawer().closeDrawers();
        }
    }

    /** Try switching to session and note about it, but do nothing if already displaying the session. */
    void switchToSession(TerminalSession session) {
        if (mTerminalView.attachSession(session)) {
            noteSessionInfo();
            updateBackgroundColor();
        }
    }

    String toToastTitle(TerminalSession session) {
        final int indexOfSession = mTermService.getSessions().indexOf(session);
        StringBuilder toastTitle = new StringBuilder("[" + (indexOfSession + 1) + "]");
        if (!TextUtils.isEmpty(session.mSessionName)) {
            toastTitle.append(" ").append(session.mSessionName);
        }
        String title = session.getTitle();
        if (!TextUtils.isEmpty(title)) {
            // Space to "[${NR}] or newline after session name:
            toastTitle.append(session.mSessionName == null ? " " : "\n");
            toastTitle.append(title);
        }
        return toastTitle.toString();
    }

    void noteSessionInfo() {
        if (!mIsVisible) return;
        TerminalSession session = getCurrentTermSession();
        final int indexOfSession = mTermService.getSessions().indexOf(session);
        showToast(toToastTitle(session), false);
        mListViewAdapter.notifyDataSetChanged();
        final ListView lv = findViewById(R.id.left_drawer_list);
        lv.setItemChecked(indexOfSession, true);
        lv.smoothScrollToPosition(indexOfSession);
    }

    @Override
    public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo) {
        TerminalSession currentSession = getCurrentTermSession();
        if (currentSession == null) return;

        menu.add(Menu.NONE, CONTEXTMENU_SELECT_URL_ID, Menu.NONE, R.string.select_url);
        menu.add(Menu.NONE, CONTEXTMENU_SHARE_TRANSCRIPT_ID, Menu.NONE, R.string.select_all_and_share);
        menu.add(Menu.NONE, CONTEXTMENU_RESET_TERMINAL_ID, Menu.NONE, R.string.reset_terminal);
        menu.add(Menu.NONE, CONTEXTMENU_KILL_PROCESS_ID, Menu.NONE, getResources().getString(R.string.kill_process, getCurrentTermSession().getPid())).setEnabled(currentSession.isRunning());
        menu.add(Menu.NONE, CONTEXTMENU_STYLING_ID, Menu.NONE, R.string.style_terminal);
        menu.add(Menu.NONE, CONTEXTMENU_TOGGLE_KEEP_SCREEN_ON, Menu.NONE, R.string.toggle_keep_screen_on).setCheckable(true).setChecked(mSettings.isScreenAlwaysOn());
        menu.add(Menu.NONE, CONTEXTMENU_HELP_ID, Menu.NONE, R.string.help);
    }

    /** Hook system menu to show context menu instead. */
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        mTerminalView.showContextMenu();
        return false;
    }

    static LinkedHashSet<CharSequence> extractUrls(String text) {

        StringBuilder regex_sb = new StringBuilder();

        regex_sb.append("(");                       // Begin first matching group.
        regex_sb.append("(?:");                     // Begin scheme group.
        regex_sb.append("dav|");                    // The DAV proto.
        regex_sb.append("dict|");                   // The DICT proto.
        regex_sb.append("dns|");                    // The DNS proto.
        regex_sb.append("file|");                   // File path.
        regex_sb.append("finger|");                 // The Finger proto.
        regex_sb.append("ftp(?:s?)|");              // The FTP proto.
        regex_sb.append("git|");                    // The Git proto.
        regex_sb.append("gopher|");                 // The Gopher proto.
        regex_sb.append("http(?:s?)|");             // The HTTP proto.
        regex_sb.append("imap(?:s?)|");             // The IMAP proto.
        regex_sb.append("irc(?:[6s]?)|");           // The IRC proto.
        regex_sb.append("ip[fn]s|");                // The IPFS proto.
        regex_sb.append("ldap(?:s?)|");             // The LDAP proto.
        regex_sb.append("pop3(?:s?)|");             // The POP3 proto.
        regex_sb.append("redis(?:s?)|");            // The Redis proto.
        regex_sb.append("rsync|");                  // The Rsync proto.
        regex_sb.append("rtsp(?:[su]?)|");          // The RTSP proto.
        regex_sb.append("sftp|");                   // The SFTP proto.
        regex_sb.append("smb(?:s?)|");              // The SAMBA proto.
        regex_sb.append("smtp(?:s?)|");             // The SMTP proto.
        regex_sb.append("svn(?:(?:\\+ssh)?)|");     // The Subversion proto.
        regex_sb.append("tcp|");                    // The TCP proto.
        regex_sb.append("telnet|");                 // The Telnet proto.
        regex_sb.append("tftp|");                   // The TFTP proto.
        regex_sb.append("udp|");                    // The UDP proto.
        regex_sb.append("vnc|");                    // The VNC proto.
        regex_sb.append("ws(?:s?)");                // The Websocket proto.
        regex_sb.append(")://");                    // End scheme group.
        regex_sb.append(")");                       // End first matching group.


        // Begin second matching group.
        regex_sb.append("(");

        // User name and/or password in format 'user:pass@'.
        regex_sb.append("(?:\\S+(?::\\S*)?@)?");

        // Begin host group.
        regex_sb.append("(?:");

        // IP address (from http://www.regular-expressions.info/examples.html).
        regex_sb.append("(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)|");

        // Host name or domain.
        regex_sb.append("(?:(?:[a-z\\u00a1-\\uffff0-9]-*)*[a-z\\u00a1-\\uffff0-9]+)(?:(?:\\.(?:[a-z\\u00a1-\\uffff0-9]-*)*[a-z\\u00a1-\\uffff0-9]+)*(?:\\.(?:[a-z\\u00a1-\\uffff]{2,})))?|");

        // Just path. Used in case of 'file://' scheme.
        regex_sb.append("/(?:(?:[a-z\\u00a1-\\uffff0-9]-*)*[a-z\\u00a1-\\uffff0-9]+)");

        // End host group.
        regex_sb.append(")");

        // Port number.
        regex_sb.append("(?::\\d{1,5})?");

        // Resource path with optional query string.
        regex_sb.append("(?:/[a-zA-Z0-9:@%\\-._~!$&()*+,;=?/]*)?");

        // Fragment.
        regex_sb.append("(?:#[a-zA-Z0-9:@%\\-._~!$&()*+,;=?/]*)?");

        // End second matching group.
        regex_sb.append(")");

        final Pattern urlPattern = Pattern.compile(
            regex_sb.toString(),
            Pattern.CASE_INSENSITIVE | Pattern.MULTILINE | Pattern.DOTALL);

        LinkedHashSet<CharSequence> urlSet = new LinkedHashSet<>();
        Matcher matcher = urlPattern.matcher(text);

        while (matcher.find()) {
            int matchStart = matcher.start(1);
            int matchEnd = matcher.end();
            String url = text.substring(matchStart, matchEnd);
            urlSet.add(url);
        }

        return urlSet;
    }

    void showUrlSelection() {
        String text = getCurrentTermSession().getEmulator().getScreen().getTranscriptText();
        LinkedHashSet<CharSequence> urlSet = extractUrls(text);
        if (urlSet.isEmpty()) {
            new AlertDialog.Builder(this).setMessage(R.string.select_url_no_found).show();
            return;
        }

        final CharSequence[] urls = urlSet.toArray(new CharSequence[0]);
        Collections.reverse(Arrays.asList(urls)); // Latest first.

        // Click to copy url to clipboard:
        final AlertDialog dialog = new AlertDialog.Builder(TermuxActivity.this).setItems(urls, (di, which) -> {
            String url = (String) urls[which];
            ClipboardManager clipboard = (ClipboardManager) getSystemService(Context.CLIPBOARD_SERVICE);
            clipboard.setPrimaryClip(new ClipData(null, new String[]{"text/plain"}, new ClipData.Item(url)));
            Toast.makeText(TermuxActivity.this, R.string.select_url_copied_to_clipboard, Toast.LENGTH_LONG).show();
        }).setTitle(R.string.select_url_dialog_title).create();

        // Long press to open URL:
        dialog.setOnShowListener(di -> {
            ListView lv = dialog.getListView(); // this is a ListView with your "buds" in it
            lv.setOnItemLongClickListener((parent, view, position, id) -> {
                dialog.dismiss();
                String url = (String) urls[position];
                Intent i = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
                try {
                    startActivity(i, null);
                } catch (ActivityNotFoundException e) {
                    // If no applications match, Android displays a system message.
                    startActivity(Intent.createChooser(i, null));
                }
                return true;
            });
        });

        dialog.show();
    }

    @Override
    public boolean onContextItemSelected(MenuItem item) {
        TerminalSession session = getCurrentTermSession();

        switch (item.getItemId()) {
            case CONTEXTMENU_SELECT_URL_ID:
                showUrlSelection();
                return true;
            case CONTEXTMENU_SHARE_TRANSCRIPT_ID:
                if (session != null) {
                    Intent intent = new Intent(Intent.ACTION_SEND);
                    intent.setType("text/plain");
                    String transcriptText = session.getEmulator().getScreen().getTranscriptTextWithoutJoinedLines().trim();
                    // See https://github.com/termux/termux-app/issues/1166.
                    final int MAX_LENGTH = 100_000;
                    if (transcriptText.length() > MAX_LENGTH) {
                        int cutOffIndex = transcriptText.length() - MAX_LENGTH;
                        int nextNewlineIndex = transcriptText.indexOf('\n', cutOffIndex);
                        if (nextNewlineIndex != -1 && nextNewlineIndex != transcriptText.length() - 1) {
                            cutOffIndex = nextNewlineIndex + 1;
                        }
                        transcriptText = transcriptText.substring(cutOffIndex).trim();
                    }
                    intent.putExtra(Intent.EXTRA_TEXT, transcriptText);
                    intent.putExtra(Intent.EXTRA_SUBJECT, getString(R.string.share_transcript_title));
                    startActivity(Intent.createChooser(intent, getString(R.string.share_transcript_chooser_title)));
                }
                return true;
            case CONTEXTMENU_PASTE_ID:
                doPaste();
                return true;
            case CONTEXTMENU_KILL_PROCESS_ID:
                final AlertDialog.Builder b = new AlertDialog.Builder(this);
                b.setIcon(android.R.drawable.ic_dialog_alert);
                b.setMessage(R.string.confirm_kill_process);
                b.setPositiveButton(android.R.string.yes, (dialog, id) -> {
                    dialog.dismiss();
                    getCurrentTermSession().finishIfRunning();
                });
                b.setNegativeButton(android.R.string.no, null);
                b.show();
                return true;
            case CONTEXTMENU_RESET_TERMINAL_ID: {
                if (session != null) {
                    session.reset();
                    showToast(getResources().getString(R.string.reset_toast_notification), true);
                }
                return true;
            }
            case CONTEXTMENU_STYLING_ID: {
                Intent stylingIntent = new Intent();
                stylingIntent.setClassName("com.termux.styling", "com.termux.styling.TermuxStyleActivity");
                try {
                    startActivity(stylingIntent);
                } catch (ActivityNotFoundException | IllegalArgumentException e) {
                    // The startActivity() call is not documented to throw IllegalArgumentException.
                    // However, crash reporting shows that it sometimes does, so catch it here.
                    new AlertDialog.Builder(this).setMessage(R.string.styling_not_installed)
                        .setPositiveButton(R.string.styling_install, (dialog, which) -> startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse("http://play.google.com/store/apps/details?id=com.termux.styling")))).setNegativeButton(android.R.string.cancel, null).show();
                }
                return true;
            }
            case CONTEXTMENU_HELP_ID:
                startActivity(new Intent(this, TermuxHelpActivity.class));
                return true;
            case CONTEXTMENU_TOGGLE_KEEP_SCREEN_ON: {
                if(mTerminalView.getKeepScreenOn()) {
                    mTerminalView.setKeepScreenOn(false);
                    mSettings.setScreenAlwaysOn(this, false);
                } else {
                    mTerminalView.setKeepScreenOn(true);
                    mSettings.setScreenAlwaysOn(this, true);
                }
                return true;
            }
            default:
                return super.onContextItemSelected(item);
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String permissions[], @NonNull int[] grantResults) {
        if (requestCode == REQUESTCODE_PERMISSION_STORAGE && grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
            TermuxInstaller.setupStorageSymlinks(this);
        }
    }

    void changeFontSize(boolean increase) {
        mSettings.changeFontSize(this, increase);
        mTerminalView.setTextSize(mSettings.getFontSize());
    }

    void doPaste() {
        ClipboardManager clipboard = (ClipboardManager) getSystemService(Context.CLIPBOARD_SERVICE);
        ClipData clipData = clipboard.getPrimaryClip();
        if (clipData == null) return;
        CharSequence paste = clipData.getItemAt(0).coerceToText(this);
        if (!TextUtils.isEmpty(paste))
            getCurrentTermSession().getEmulator().paste(paste.toString());
    }

    /** The current session as stored or the last one if that does not exist. */
    public TerminalSession getStoredCurrentSessionOrLast() {
        TerminalSession stored = TermuxPreferences.getCurrentSession(this);
        if (stored != null) return stored;
        List<TerminalSession> sessions = mTermService.getSessions();
        return sessions.isEmpty() ? null : sessions.get(sessions.size() - 1);
    }

    /** Show a toast and dismiss the last one if still visible. */
    void showToast(String text, boolean longDuration) {
        if (mLastToast != null) mLastToast.cancel();
        mLastToast = Toast.makeText(TermuxActivity.this, text, longDuration ? Toast.LENGTH_LONG : Toast.LENGTH_SHORT);
        mLastToast.setGravity(Gravity.TOP, 0, 0);
        mLastToast.show();
    }

    public void removeFinishedSession(TerminalSession finishedSession) {
        // Return pressed with finished session - remove it.
        TermuxService service = mTermService;

        int index = service.removeTermSession(finishedSession);
        mListViewAdapter.notifyDataSetChanged();
        if (mTermService.getSessions().isEmpty()) {
            // There are no sessions to show, so finish the activity.
            finish();
        } else {
            if (index >= service.getSessions().size()) {
                index = service.getSessions().size() - 1;
            }
            switchToSession(service.getSessions().get(index));
        }
    }

}
