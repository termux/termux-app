package com.termux.app.terminal;

import android.annotation.SuppressLint;
import android.app.AlertDialog;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.pm.PackageManager;
import android.graphics.Typeface;
import android.media.AudioAttributes;
import android.media.SoundPool;
import android.text.TextUtils;
import android.widget.ListView;

import com.termux.R;
import com.termux.app.utils.DialogUtils;
import com.termux.app.TermuxActivity;
import com.termux.app.TermuxConstants;
import com.termux.app.TermuxService;
import com.termux.app.settings.properties.TermuxPropertyConstants;
import com.termux.app.terminal.io.BellHandler;
import com.termux.app.utils.Logger;
import com.termux.terminal.TerminalColors;
import com.termux.terminal.TerminalSession;
import com.termux.terminal.TextStyle;

import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;
import java.util.List;
import java.util.Properties;

public class TermuxSessionClient extends TermuxSessionClientBase {

    final TermuxActivity mActivity;

    private static final int MAX_SESSIONS = 8;

    final SoundPool mBellSoundPool = new SoundPool.Builder().setMaxStreams(1).setAudioAttributes(
        new AudioAttributes.Builder().setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
            .setUsage(AudioAttributes.USAGE_ASSISTANCE_SONIFICATION).build()).build();

    int mBellSoundId;

    private static final String LOG_TAG = "TermuxSessionClient";

    public TermuxSessionClient(TermuxActivity activity) {
        this.mActivity = activity;

        mBellSoundId = mBellSoundPool.load(activity, R.raw.bell, 1);
    }

    @Override
    public void onTextChanged(TerminalSession changedSession) {
        if (!mActivity.isVisible()) return;

        if (mActivity.getCurrentSession() == changedSession) mActivity.getTerminalView().onScreenUpdated();
    }

    @Override
    public void onTitleChanged(TerminalSession updatedSession) {
        if (!mActivity.isVisible()) return;

        if (updatedSession != mActivity.getCurrentSession()) {
            // Only show toast for other sessions than the current one, since the user
            // probably consciously caused the title change to change in the current session
            // and don't want an annoying toast for that.
            mActivity.showToast(toToastTitle(updatedSession), true);
        }

        terminalSessionListNotifyUpdated();
    }

    @Override
    public void onSessionFinished(final TerminalSession finishedSession) {
        if (mActivity.getTermuxService().wantsToStop()) {
            // The service wants to stop as soon as possible.
            mActivity.finishActivityIfNotFinishing();
            return;
        }

        if (mActivity.isVisible() && finishedSession != mActivity.getCurrentSession()) {
            // Show toast for non-current sessions that exit.
            int indexOfSession = mActivity.getTermuxService().getSessions().indexOf(finishedSession);
            // Verify that session was not removed before we got told about it finishing:
            if (indexOfSession >= 0)
                mActivity.showToast(toToastTitle(finishedSession) + " - exited", true);
        }

        if (mActivity.getPackageManager().hasSystemFeature(PackageManager.FEATURE_LEANBACK)) {
            // On Android TV devices we need to use older behaviour because we may
            // not be able to have multiple launcher icons.
            if (mActivity.getTermuxService().getSessions().size() > 1) {
                removeFinishedSession(finishedSession);
            }
        } else {
            // Once we have a separate launcher icon for the failsafe session, it
            // should be safe to auto-close session on exit code '0' or '130'.
            if (finishedSession.getExitStatus() == 0 || finishedSession.getExitStatus() == 130) {
                removeFinishedSession(finishedSession);
            }
        }

        terminalSessionListNotifyUpdated();
    }

    @Override
    public void onClipboardText(TerminalSession session, String text) {
        if (!mActivity.isVisible()) return;

        ClipboardManager clipboard = (ClipboardManager) mActivity.getSystemService(Context.CLIPBOARD_SERVICE);
        clipboard.setPrimaryClip(new ClipData(null, new String[]{"text/plain"}, new ClipData.Item(text)));
    }

    @Override
    public void onBell(TerminalSession session) {
        if (!mActivity.isVisible()) return;

        switch (mActivity.getProperties().getBellBehaviour()) {
            case TermuxPropertyConstants.IVALUE_BELL_BEHAVIOUR_VIBRATE:
                BellHandler.getInstance(mActivity).doBell();
                break;
            case TermuxPropertyConstants.IVALUE_BELL_BEHAVIOUR_BEEP:
                mBellSoundPool.play(mBellSoundId, 1.f, 1.f, 1, 0, 1.f);
                break;
            case TermuxPropertyConstants.IVALUE_BELL_BEHAVIOUR_IGNORE:
                // Ignore the bell character.
                break;
        }

    }

    @Override
    public void onColorsChanged(TerminalSession changedSession) {
        if (mActivity.getCurrentSession() == changedSession)
            updateBackgroundColor();
    }



    /** Try switching to session and note about it, but do nothing if already displaying the session. */
    public void setCurrentSession(TerminalSession session) {
        if (mActivity.getTerminalView().attachSession(session)) {
            noteSessionInfo();
            updateBackgroundColor();
        }
    }

    void noteSessionInfo() {
        if (!mActivity.isVisible()) return;

        TerminalSession session = mActivity.getCurrentSession();
        final int indexOfSession = mActivity.getTermuxService().getSessions().indexOf(session);
        mActivity.showToast(toToastTitle(session), false);
        terminalSessionListNotifyUpdated();

        final ListView termuxSessionsListView = mActivity.findViewById(R.id.terminal_sessions_list);
        termuxSessionsListView.setItemChecked(indexOfSession, true);
        termuxSessionsListView.smoothScrollToPosition(indexOfSession);
    }

    public void switchToSession(boolean forward) {
        TermuxService service = mActivity.getTermuxService();

        TerminalSession currentSession = mActivity.getCurrentSession();
        int index = service.getSessions().indexOf(currentSession);
        if (forward) {
            if (++index >= service.getSessions().size()) index = 0;
        } else {
            if (--index < 0) index = service.getSessions().size() - 1;
        }
        setCurrentSession(service.getSessions().get(index));
    }

    @SuppressLint("InflateParams")
    public void renameSession(final TerminalSession sessionToRename) {
        if (sessionToRename == null) return;

        DialogUtils.textInput(mActivity, R.string.session_rename_title, sessionToRename.mSessionName, R.string.session_rename_positive_button, text -> {
            sessionToRename.mSessionName = text;
            terminalSessionListNotifyUpdated();
        }, -1, null, -1, null, null);
    }

    public void addNewSession(boolean isFailSafe, String sessionName) {
        if (mActivity.getTermuxService().getSessions().size() >= MAX_SESSIONS) {
            new AlertDialog.Builder(mActivity).setTitle(R.string.max_terminals_reached_title).setMessage(R.string.max_terminals_reached_message)
                .setPositiveButton(android.R.string.ok, null).show();
        } else {
            TerminalSession currentSession = mActivity.getCurrentSession();

            String workingDirectory;
            if (currentSession == null) {
                workingDirectory = mActivity.getProperties().getDefaultWorkingDirectory();
            } else {
                workingDirectory = currentSession.getCwd();
            }

            TerminalSession newSession = mActivity.getTermuxService().createTerminalSession(null, null, workingDirectory, isFailSafe);
            if (sessionName != null) {
                newSession.mSessionName = sessionName;
            }
            setCurrentSession(newSession);
            mActivity.getDrawer().closeDrawers();
        }
    }

    public void setCurrentStoredSession() {
        TerminalSession currentSession = mActivity.getCurrentSession();
        if (currentSession != null)
            mActivity.getPreferences().setCurrentSession(currentSession.mHandle);
        else
            mActivity.getPreferences().setCurrentSession(null);
    }

    /** The current session as stored or the last one if that does not exist. */
    public TerminalSession getCurrentStoredSessionOrLast() {
        TerminalSession stored = getCurrentStoredSession(mActivity);

        if (stored != null) {
            // If a stored session is in the list of currently running sessions, then return it
            return stored;
        } else {
            // Else return the last session currently running
            List<TerminalSession> sessions = mActivity.getTermuxService().getSessions();
            return sessions.isEmpty() ? null : sessions.get(sessions.size() - 1);
        }
    }

    private TerminalSession getCurrentStoredSession(TermuxActivity context) {
        String sessionHandle = mActivity.getPreferences().getCurrentSession();

        // If no session is stored in shared preferences
        if(sessionHandle == null)
            return null;

        // Check if the session handle found matches one of the currently running sessions
        List<TerminalSession> sessions = context.getTermuxService().getSessions();
        for (int i = 0, len = sessions.size(); i < len; i++) {
            TerminalSession session = sessions.get(i);
            if (session.mHandle.equals(sessionHandle))
                return session;
        }

        return null;
    }

    public void removeFinishedSession(TerminalSession finishedSession) {
        // Return pressed with finished session - remove it.
        TermuxService service = mActivity.getTermuxService();

        int index = service.removeTerminalSession(finishedSession);
        terminalSessionListNotifyUpdated();
        if (mActivity.getTermuxService().getSessions().isEmpty()) {
            // There are no sessions to show, so finish the activity.
            mActivity.finishActivityIfNotFinishing();
        } else {
            if (index >= service.getSessions().size()) {
                index = service.getSessions().size() - 1;
            }
            setCurrentSession(service.getSessions().get(index));
        }
    }

    public void terminalSessionListNotifyUpdated() {
        mActivity.terminalSessionListNotifyUpdated();
    }



    String toToastTitle(TerminalSession session) {
        final int indexOfSession = mActivity.getTermuxService().getSessions().indexOf(session);
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


    public void checkForFontAndColors() {
        try {
            File colorsFile = TermuxConstants.TERMUX_COLOR_PROPERTIES_FILE;
            File fontFile = TermuxConstants.TERMUX_FONT_FILE;

            final Properties props = new Properties();
            if (colorsFile.isFile()) {
                try (InputStream in = new FileInputStream(colorsFile)) {
                    props.load(in);
                }
            }

            TerminalColors.COLOR_SCHEME.updateWith(props);
            TerminalSession session = mActivity.getCurrentSession();
            if (session != null && session.getEmulator() != null) {
                session.getEmulator().mColors.reset();
            }
            updateBackgroundColor();

            final Typeface newTypeface = (fontFile.exists() && fontFile.length() > 0) ? Typeface.createFromFile(fontFile) : Typeface.MONOSPACE;
            mActivity.getTerminalView().setTypeface(newTypeface);
        } catch (Exception e) {
            Logger.logStackTraceWithMessage(LOG_TAG, "Error in checkForFontAndColors()", e);
        }
    }

    public void updateBackgroundColor() {
        TerminalSession session = mActivity.getCurrentSession();
        if (session != null && session.getEmulator() != null) {
            mActivity.getWindow().getDecorView().setBackgroundColor(session.getEmulator().mColors.mCurrentColors[TextStyle.COLOR_INDEX_BACKGROUND]);
        }
    }

}
