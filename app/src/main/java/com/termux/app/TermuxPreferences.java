package com.termux.app;

import android.content.Context;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.support.annotation.IntDef;
import android.util.Log;
import android.util.TypedValue;
import android.widget.Toast;

import com.termux.terminal.TerminalSession;

import java.io.File;
import java.io.FileInputStream;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.Properties;

final class TermuxPreferences {

	@IntDef({BELL_VIBRATE, BELL_BEEP, BELL_IGNORE})
	@Retention(RetentionPolicy.SOURCE)
	public @interface AsciiBellBehaviour {}

	static final int BELL_VIBRATE = 1;
	static final int BELL_BEEP = 2;
	static final int BELL_IGNORE = 3;

	private final int MIN_FONTSIZE;
	private static final int MAX_FONTSIZE = 256;

	private static final String FULLSCREEN_KEY = "fullscreen";
	private static final String FONTSIZE_KEY = "fontsize";
	private static final String CURRENT_SESSION_KEY = "current_session";
	private static final String SHOW_WELCOME_DIALOG_KEY = "intro_dialog";

	private boolean mFullScreen;
	private int mFontSize;

	@AsciiBellBehaviour
	int mBellBehaviour = BELL_VIBRATE;

	boolean mBackIsEscape = true;

	TermuxPreferences(Context context) {
		reloadFromProperties(context);
		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(context);

		float dipInPixels = TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 1, context.getResources().getDisplayMetrics());

		// This is a bit arbitrary and sub-optimal. We want to give a sensible default for minimum font size
		// to prevent invisible text due to zoom be mistake:
		MIN_FONTSIZE = (int) (4f * dipInPixels);

		mFullScreen = prefs.getBoolean(FULLSCREEN_KEY, false);

		// http://www.google.com/design/spec/style/typography.html#typography-line-height
		int defaultFontSize = Math.round(12 * dipInPixels);
		// Make it divisible by 2 since that is the minimal adjustment step:
		if (defaultFontSize % 2 == 1) defaultFontSize--;

		try {
			mFontSize = Integer.parseInt(prefs.getString(FONTSIZE_KEY, Integer.toString(defaultFontSize)));
		} catch (NumberFormatException | ClassCastException e) {
			mFontSize = defaultFontSize;
		}
		mFontSize = Math.max(MIN_FONTSIZE, Math.min(mFontSize, MAX_FONTSIZE));
	}

	boolean isFullScreen() {
		return mFullScreen;
	}

	void setFullScreen(Context context, boolean newValue) {
		mFullScreen = newValue;
		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(context);
		prefs.edit().putBoolean(FULLSCREEN_KEY, newValue).apply();
	}

	int getFontSize() {
		return mFontSize;
	}

	void changeFontSize(Context context, boolean increase) {
		mFontSize += (increase ? 1 : -1) * 2;
		mFontSize = Math.max(MIN_FONTSIZE, Math.min(mFontSize, MAX_FONTSIZE));

		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(context);
		prefs.edit().putString(FONTSIZE_KEY, Integer.toString(mFontSize)).apply();
	}

	static void storeCurrentSession(Context context, TerminalSession session) {
		PreferenceManager.getDefaultSharedPreferences(context).edit().putString(TermuxPreferences.CURRENT_SESSION_KEY, session.mHandle).commit();
	}

	static TerminalSession getCurrentSession(TermuxActivity context) {
		String sessionHandle = PreferenceManager.getDefaultSharedPreferences(context).getString(TermuxPreferences.CURRENT_SESSION_KEY, "");
		for (int i = 0, len = context.mTermService.getSessions().size(); i < len; i++) {
			TerminalSession session = context.mTermService.getSessions().get(i);
			if (session.mHandle.equals(sessionHandle)) return session;
		}
		return null;
	}

	public static boolean isShowWelcomeDialog(Context context) {
		return PreferenceManager.getDefaultSharedPreferences(context).getBoolean(SHOW_WELCOME_DIALOG_KEY, true);
	}

	public static void disableWelcomeDialog(Context context) {
		PreferenceManager.getDefaultSharedPreferences(context).edit().putBoolean(SHOW_WELCOME_DIALOG_KEY, false).apply();
	}

	public void reloadFromProperties(Context context) {
		try {
			File propsFile = new File(TermuxService.HOME_PATH + "/.config/termux/termux.properties");
			Properties props = new Properties();
			if (propsFile.isFile() && propsFile.canRead()) {
				try (FileInputStream in = new FileInputStream(propsFile)) {
					props.load(in);
				}
			}

			switch (props.getProperty("bell-character", "vibrate")) {
				case "beep":
					mBellBehaviour = BELL_BEEP;
					break;
				case "ignore":
					mBellBehaviour = BELL_IGNORE;
					break;
				default: // "vibrate".
					mBellBehaviour = BELL_VIBRATE;
					break;
			}

			mBackIsEscape = "escape".equals(props.getProperty("back-key", "back"));
		} catch (Exception e) {
			Toast.makeText(context, "Error loading properties: " + e.getMessage(), Toast.LENGTH_LONG).show();
			Log.e("termux", "Error loading props", e);
		}
	}

}
