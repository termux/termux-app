package com.termux.window;

import android.annotation.SuppressLint;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.os.IBinder;
import android.os.Vibrator;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;

import com.termux.terminal.EmulatorDebug;
import com.termux.terminal.TerminalSession;

import java.io.File;
import java.io.IOException;

public class TermuxFloatService extends Service {

	public static final String ACTION_HIDE = "com.termux.float.hide";
	public static final String ACTION_SHOW = "com.termux.float.show";

	/**
     * Note that this is a symlink on the Android M preview.
     */
    @SuppressLint("SdCardPath")
    public static final String FILES_PATH = "/data/data/com.termux/files";
    public static final String PREFIX_PATH = FILES_PATH + "/usr";
    public static final String HOME_PATH = FILES_PATH + "/home";

    /**
     * The notification id supplied to {@link #startForeground(int, Notification)}.
     * <p/>
     * Note that the javadoc for that method says it cannot be zero.
     */
    private static final int NOTIFICATION_ID = 0xdead1337;

    private static final int MIN_FONTSIZE = 16;
    private static final int DEFAULT_FONTSIZE = 24;
    private static final String FONTSIZE_KEY = "fontsize";
    private TermuxFloatView mFloatingWindow;
    private int mFontSize;
	private boolean mVisibleWindow = true;

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @SuppressLint({"InflateParams"})
    @Override
    public void onCreate() {
        super.onCreate();

        final SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);
        try {
            mFontSize = Integer.parseInt(prefs.getString(FONTSIZE_KEY, Integer.toString(DEFAULT_FONTSIZE)));
        } catch (NumberFormatException | ClassCastException e) {
            mFontSize = DEFAULT_FONTSIZE;
        }

		TermuxFloatView floatingWindow = (TermuxFloatView) ((LayoutInflater) getSystemService(Context.LAYOUT_INFLATER_SERVICE)).inflate(R.layout.activity_main, null);
		floatingWindow.initializeFloatingWindow();
		floatingWindow.mTerminalView.setTextSize(mFontSize);

        TerminalSession session = createTermSession();
		floatingWindow.mTerminalView.attachSession(session);

		try {
			floatingWindow.launchFloatingWindow();
		} catch (Exception e) {
			// Settings.canDrawOverlays() does not work (always returns false, perhaps due to sharedUserId?).
			// So instead we catch the exception and prompt here.
			startActivity(new Intent(this, TermuxFloatPermissionActivity.class).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK));
			stopSelf();
			return;
		}

		mFloatingWindow = floatingWindow;

		Toast toast = Toast.makeText(this, R.string.initial_instruction_toast, Toast.LENGTH_LONG);
		toast.setGravity(Gravity.CENTER, 0, 0);
		TextView v = (TextView) toast.getView().findViewById(android.R.id.message);
		if (v != null) v.setGravity(Gravity.CENTER);
		toast.show();

		startForeground(NOTIFICATION_ID, buildNotification());
    }

	private Notification buildNotification() {
		final Resources res = getResources();
		final String contentTitle = res.getString(R.string.notification_title);
		final String contentText = res.getString(mVisibleWindow ? R.string.notification_message_visible : R.string.notification_message_hidden);

		final String intentAction = mVisibleWindow ? ACTION_HIDE : ACTION_SHOW;
		Intent actionIntent = new Intent(this, TermuxFloatService.class).setAction(intentAction);

		Notification.Builder builder = new Notification.Builder(this).setContentTitle(contentTitle).setContentText(contentText)
				.setPriority(Notification.PRIORITY_MIN).setSmallIcon(R.mipmap.ic_service_notification)
				.setColor(0xFF000000)
				.setContentIntent(PendingIntent.getService(this, 0, actionIntent, 0))
				.setOngoing(true).setShowWhen(false);

		//final int messageId = mVisibleWindow ? R.string.toggle_hide : R.string.toggle_show;
		//builder.addAction(android.R.drawable.ic_menu_preferences, res.getString(messageId), PendingIntent.getService(this, 0, actionIntent, 0));
		return builder.build();
	}


	@SuppressLint("Wakelock")
	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
		String action = intent.getAction();
		if (ACTION_HIDE.equals(action)) {
			mVisibleWindow = false;
			mFloatingWindow.setVisibility(View.GONE);
			((NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE)).notify(NOTIFICATION_ID, buildNotification());
		} else if (ACTION_SHOW.equals(action)) {
			mFloatingWindow.setVisibility(View.VISIBLE);
			mVisibleWindow = true;
			((NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE)).notify(NOTIFICATION_ID, buildNotification());
		}
		return Service.START_NOT_STICKY;
	}

	@Override
    public void onDestroy() {
        super.onDestroy();
        if (mFloatingWindow != null) mFloatingWindow.closeFloatingWindow();
    }

    public void changeFontSize(boolean increase) {
        mFontSize += (increase ? 1 : -1) * 2;
        mFontSize = Math.max(MIN_FONTSIZE, mFontSize);

        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);
        prefs.edit().putString(FONTSIZE_KEY, Integer.toString(mFontSize)).apply();

        mFloatingWindow.mTerminalView.setTextSize(mFontSize);
    }

    // XXX: Keep in sync with TermuxService.java.
    @SuppressLint("SdCardPath")
    TerminalSession createTermSession() {
        new File(HOME_PATH).mkdirs();

        String termEnv = "TERM=xterm-256color";
        String homeEnv = "HOME=" + HOME_PATH;
        String prefixEnv = "PREFIX=" + PREFIX_PATH;
        String[] env;
        String ps1Env = "PS1=$ ";
        String ldEnv = "LD_LIBRARY_PATH=" + PREFIX_PATH + "/lib";
        String langEnv = "LANG=en_US.UTF-8";
        String pathEnv = "PATH=" + PREFIX_PATH + "/bin:" + PREFIX_PATH + "/bin/applets:" + System.getenv("PATH");
        env = new String[]{termEnv, homeEnv, prefixEnv, ps1Env, ldEnv, langEnv, pathEnv};

        String executablePath = null;
        String[] args;
        String shellName = null;
        File shell = new File(HOME_PATH, ".termux/shell");
        if (shell.exists()) {
            try {
                File canonicalFile = shell.getCanonicalFile();
                if (canonicalFile.isFile() && canonicalFile.canExecute()) {
                    executablePath = canonicalFile.getAbsolutePath();
                    String[] parts = executablePath.split("/");
                    shellName = "-" + parts[parts.length - 1];
                } else {
                    Log.w(EmulatorDebug.LOG_TAG, "$HOME/.termux/shell points to non-executable shell: " + canonicalFile.getAbsolutePath());
                }
            } catch (IOException e) {
                Log.e(EmulatorDebug.LOG_TAG, "Error checking $HOME/.termux/shell", e);
            }
        }

        if (executablePath == null) {
            // Try bash, zsh and ash in that order:
            for (String shellBinary : new String[]{"bash", "zsh", "ash"}) {
                File shellFile = new File(PREFIX_PATH + "/bin/" + shellBinary);
                if (shellFile.canExecute()) {
                    executablePath = shellFile.getAbsolutePath();
                    shellName = "-" + shellBinary;
                    break;
                }
            }
        }

        if (executablePath == null) {
            // Fall back to system shell as last resort:
            executablePath = "/system/bin/sh";
            shellName = "-sh";
        }

        args = new String[]{shellName};

        return new TerminalSession(executablePath, HOME_PATH, args, env, new TerminalSession.SessionChangedCallback() {
            @Override
            public void onTitleChanged(TerminalSession changedSession) {
                // Ignore for now.
            }

            @Override
            public void onTextChanged(TerminalSession changedSession) {
                mFloatingWindow.mTerminalView.onScreenUpdated();
            }

            @Override
            public void onSessionFinished(TerminalSession finishedSession) {
                stopSelf();
            }

            @Override
            public void onClipboardText(TerminalSession pastingSession, String text) {
                mFloatingWindow.showToast("Clipboard set:\n\"" + text + "\"", true);
                ClipboardManager clipboard = (ClipboardManager) getSystemService(Context.CLIPBOARD_SERVICE);
                clipboard.setPrimaryClip(new ClipData(null, new String[]{"text/plain"}, new ClipData.Item(text)));
            }

            @Override
            public void onBell(TerminalSession riningSession) {
                ((Vibrator) getSystemService(Context.VIBRATOR_SERVICE)).vibrate(50);
            }

            @Override
            public void onColorsChanged(TerminalSession session) {

            }
        });
    }

}
