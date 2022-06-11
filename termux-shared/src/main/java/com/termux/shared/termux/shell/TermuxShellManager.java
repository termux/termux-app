package com.termux.shared.termux.shell;

import android.content.Context;
import android.content.Intent;
import android.widget.ArrayAdapter;

import androidx.annotation.NonNull;

import com.termux.shared.shell.command.ExecutionCommand;
import com.termux.shared.shell.command.runner.app.AppShell;
import com.termux.shared.termux.settings.preferences.TermuxAppSharedPreferences;
import com.termux.shared.termux.shell.command.runner.terminal.TermuxSession;

import java.util.ArrayList;
import java.util.List;

public class TermuxShellManager {

    private static TermuxShellManager shellManager;

    private static int SHELL_ID = 0;

    protected final Context mContext;

    /**
     * The foreground TermuxSessions which this service manages.
     * Note that this list is observed by an activity, like TermuxActivity.mTermuxSessionListViewController,
     * so any changes must be made on the UI thread and followed by a call to
     * {@link ArrayAdapter#notifyDataSetChanged()}.
     */
    public final List<TermuxSession> mTermuxSessions = new ArrayList<>();

    /**
     * The background TermuxTasks which this service manages.
     */
    public final List<AppShell> mTermuxTasks = new ArrayList<>();

    /**
     * The pending plugin ExecutionCommands that have yet to be processed by this service.
     */
    public final List<ExecutionCommand> mPendingPluginExecutionCommands = new ArrayList<>();



    public TermuxShellManager(@NonNull Context context) {
        mContext = context.getApplicationContext();
    }

    /**
     * Initialize the {@link #shellManager}.
     *
     * @param context The {@link Context} for operations.
     * @return Returns the {@link TermuxShellManager}.
     */
    public static TermuxShellManager init(@NonNull Context context) {
        if (shellManager == null)
            shellManager = new TermuxShellManager(context);

        return shellManager;
    }

    /**
     * Get the {@link #shellManager}.
     *
     * @return Returns the {@link TermuxShellManager}.
     */
    public static TermuxShellManager getShellManager() {
        return shellManager;
    }


    public static synchronized int getNextShellId() {
        return SHELL_ID++;
    }

}
