package com.termux.app.terminal.managers;

import android.app.Activity;
import android.content.Context;
import android.widget.ListView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.termux.R;
import com.termux.app.TermuxService;
import com.termux.app.terminal.TermuxSessionsListViewController;
import com.termux.app.terminal.TermuxTerminalSessionActivityClient;
import com.termux.shared.logger.Logger;
import com.termux.shared.shell.command.ExecutionCommand;
import com.termux.shared.termux.shell.TermuxShellManager;
import com.termux.shared.termux.shell.command.runner.terminal.TermuxSession;
import com.termux.terminal.TerminalSession;
import com.termux.view.TerminalView;

import java.util.ArrayList;
import java.util.List;

/**
 * Manages terminal sessions for TermuxActivity.
 * Follows Single Responsibility Principle by handling only session-related operations.
 */
public class TermuxSessionManager {
    
    private static final String LOG_TAG = "TermuxSessionManager";
    
    private final Activity mActivity;
    private final TermuxSessionsListViewController mTermuxSessionListViewController;
    
    private TermuxService mTermuxService;
    private TerminalView mTerminalView;
    private TermuxTerminalSessionActivityClient mTermuxTerminalSessionActivityClient;
    
    private TerminalSession mCurrentSession;
    private int mCurrentSessionIndex = -1;
    
    /**
     * Interface for session event callbacks.
     */
    public interface SessionEventListener {
        void onSessionCreated(TerminalSession session);
        void onSessionChanged(TerminalSession oldSession, TerminalSession newSession);
        void onSessionFinished(TerminalSession session);
        void onAllSessionsFinished();
    }
    
    private SessionEventListener mSessionEventListener;
    
    public TermuxSessionManager(@NonNull Activity activity) {
        this.mActivity = activity;
        this.mTermuxSessionListViewController = new TermuxSessionsListViewController(
            activity, 
            activity.findViewById(R.id.left_drawer_list)
        );
    }
    
    /**
     * Initialize the session manager with required dependencies.
     */
    public void initialize(@NonNull TermuxService service, 
                          @NonNull TerminalView terminalView,
                          @NonNull TermuxTerminalSessionActivityClient sessionClient) {
        this.mTermuxService = service;
        this.mTerminalView = terminalView;
        this.mTermuxTerminalSessionActivityClient = sessionClient;
        
        updateSessionsList();
    }
    
    /**
     * Set the session event listener.
     */
    public void setSessionEventListener(SessionEventListener listener) {
        this.mSessionEventListener = listener;
    }
    
    /**
     * Create a new terminal session.
     */
    public TerminalSession createNewSession(@Nullable String sessionName, 
                                           @Nullable String workingDirectory) {
        if (mTermuxService == null) {
            Logger.logError(LOG_TAG, "Cannot create session - service not initialized");
            return null;
        }
        
        Logger.logVerbose(LOG_TAG, "Creating new session: " + sessionName);
        
        ExecutionCommand executionCommand = new ExecutionCommand();
        executionCommand.sessionName = sessionName;
        executionCommand.workingDirectory = workingDirectory;
        executionCommand.isLoginShell = true;
        
        TerminalSession session = mTermuxService.createTermuxSession(
            executionCommand, 
            mTermuxTerminalSessionActivityClient
        );
        
        if (session != null) {
            updateSessionsList();
            setCurrentSession(session);
            
            if (mSessionEventListener != null) {
                mSessionEventListener.onSessionCreated(session);
            }
        }
        
        return session;
    }
    
    /**
     * Switch to a specific session.
     */
    public void switchToSession(TerminalSession session) {
        if (session == null || mTerminalView == null) {
            return;
        }
        
        TerminalSession oldSession = mCurrentSession;
        setCurrentSession(session);
        
        if (mSessionEventListener != null && oldSession != session) {
            mSessionEventListener.onSessionChanged(oldSession, session);
        }
    }
    
    /**
     * Switch to session by index.
     */
    public void switchToSession(int index) {
        List<TerminalSession> sessions = getAllSessions();
        
        if (index >= 0 && index < sessions.size()) {
            switchToSession(sessions.get(index));
        }
    }
    
    /**
     * Switch to the next session.
     */
    public void switchToNextSession() {
        List<TerminalSession> sessions = getAllSessions();
        
        if (sessions.isEmpty()) {
            return;
        }
        
        int nextIndex = (mCurrentSessionIndex + 1) % sessions.size();
        switchToSession(nextIndex);
    }
    
    /**
     * Switch to the previous session.
     */
    public void switchToPreviousSession() {
        List<TerminalSession> sessions = getAllSessions();
        
        if (sessions.isEmpty()) {
            return;
        }
        
        int prevIndex = mCurrentSessionIndex - 1;
        if (prevIndex < 0) {
            prevIndex = sessions.size() - 1;
        }
        
        switchToSession(prevIndex);
    }
    
    /**
     * Remove a specific session.
     */
    public void removeSession(TerminalSession session) {
        if (session == null || mTermuxService == null) {
            return;
        }
        
        Logger.logVerbose(LOG_TAG, "Removing session: " + session.mSessionName);
        
        int index = getAllSessions().indexOf(session);
        mTermuxService.removeTermuxSession(session);
        updateSessionsList();
        
        // Switch to another session if the current one was removed
        if (session == mCurrentSession) {
            List<TerminalSession> sessions = getAllSessions();
            
            if (!sessions.isEmpty()) {
                // Try to switch to the session at the same index, or the last one
                int newIndex = Math.min(index, sessions.size() - 1);
                switchToSession(newIndex);
            } else {
                mCurrentSession = null;
                mCurrentSessionIndex = -1;
                
                if (mSessionEventListener != null) {
                    mSessionEventListener.onAllSessionsFinished();
                }
            }
        }
        
        if (mSessionEventListener != null) {
            mSessionEventListener.onSessionFinished(session);
        }
    }
    
    /**
     * Remove the current session.
     */
    public void removeCurrentSession() {
        removeSession(mCurrentSession);
    }
    
    /**
     * Rename a session.
     */
    public void renameSession(TerminalSession session, String newName) {
        if (session == null) {
            return;
        }
        
        Logger.logVerbose(LOG_TAG, "Renaming session to: " + newName);
        
        session.mSessionName = newName;
        updateSessionsList();
    }
    
    /**
     * Get all sessions.
     */
    @NonNull
    public List<TerminalSession> getAllSessions() {
        if (mTermuxService == null) {
            return new ArrayList<>();
        }
        
        return mTermuxService.getSessions();
    }
    
    /**
     * Get the current session.
     */
    @Nullable
    public TerminalSession getCurrentSession() {
        return mCurrentSession;
    }
    
    /**
     * Get current session index.
     */
    public int getCurrentSessionIndex() {
        return mCurrentSessionIndex;
    }
    
    /**
     * Check if there are any sessions.
     */
    public boolean hasSessions() {
        return !getAllSessions().isEmpty();
    }
    
    /**
     * Get session count.
     */
    public int getSessionCount() {
        return getAllSessions().size();
    }
    
    /**
     * Update the sessions list view.
     */
    public void updateSessionsList() {
        if (mTermuxSessionListViewController != null) {
            mTermuxSessionListViewController.updateList(getAllSessions());
        }
    }
    
    /**
     * Set the current session.
     */
    private void setCurrentSession(TerminalSession session) {
        if (session == null) {
            return;
        }
        
        mCurrentSession = session;
        mCurrentSessionIndex = getAllSessions().indexOf(session);
        
        if (mTerminalView != null) {
            mTerminalView.attachSession(session);
        }
        
        // Update the sessions list to reflect the current session
        updateSessionsList();
    }
    
    /**
     * Handle session finished event.
     */
    public void onSessionFinished(TerminalSession session) {
        if (mTermuxService != null && mTermuxService.wantsToStop()) {
            // Service wants to stop, remove the session
            removeSession(session);
        } else {
            // Keep the session around for review
            Logger.logVerbose(LOG_TAG, "Session finished but keeping for review: " + session.mSessionName);
        }
    }
    
    /**
     * Check if we should show the sessions drawer.
     */
    public boolean shouldShowSessionsDrawer() {
        return getSessionCount() > 1;
    }
    
    /**
     * Get the sessions list view controller.
     */
    public TermuxSessionsListViewController getSessionsListViewController() {
        return mTermuxSessionListViewController;
    }
}