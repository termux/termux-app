package com.termux.app.workspace;

import android.content.Context;
import android.content.SharedPreferences;
import android.text.TextUtils;

import com.google.gson.Gson;
import com.google.gson.reflect.TypeToken;
import com.termux.shared.logger.Logger;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.lang.reflect.Type;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

/**
 * Manages workspace/project directories for the Termux Workstation.
 * Handles persistence of recent/favorite projects and git detection.
 */
public class WorkspaceManager {

    private static final String LOG_TAG = "WorkspaceManager";
    private static final String PREFS_NAME = "termux_workspaces";
    private static final String KEY_WORKSPACES = "workspaces_json";

    private final Context mContext;
    private final SharedPreferences mPrefs;
    private final Gson mGson;
    private final Type mWorkspaceListType;

    private List<Workspace> mWorkspaces;

    public WorkspaceManager(Context context) {
        mContext = context.getApplicationContext();
        mPrefs = mContext.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE);
        mGson = new Gson();
        mWorkspaceListType = new TypeToken<List<Workspace>>() {}.getType();
        loadWorkspaces();
    }

    private void loadWorkspaces() {
        String json = mPrefs.getString(KEY_WORKSPACES, null);
        if (json != null) {
            try {
                mWorkspaces = mGson.fromJson(json, mWorkspaceListType);
                if (mWorkspaces == null) {
                    mWorkspaces = new ArrayList<>();
                }
            } catch (Exception e) {
                Logger.logStackTraceWithMessage(LOG_TAG, "Failed to parse workspaces", e);
                mWorkspaces = new ArrayList<>();
            }
        } else {
            mWorkspaces = new ArrayList<>();
        }
        // Sort: favorites first, then by last accessed time (most recent first)
        sortWorkspaces();
    }

    private void saveWorkspaces() {
        String json = mGson.toJson(mWorkspaces);
        mPrefs.edit().putString(KEY_WORKSPACES, json).apply();
    }

    private void sortWorkspaces() {
        Collections.sort(mWorkspaces, new Comparator<Workspace>() {
            @Override
            public int compare(Workspace w1, Workspace w2) {
                // Favorites first
                if (w1.isFavorite != w2.isFavorite) {
                    return w1.isFavorite ? -1 : 1;
                }
                // Then by last accessed (most recent first)
                return Long.compare(w2.lastAccessed, w1.lastAccessed);
            }
        });
    }

    public List<Workspace> getWorkspaces() {
        return new ArrayList<>(mWorkspaces);
    }

    public Workspace getWorkspace(String path) {
        for (Workspace ws : mWorkspaces) {
            if (ws.path.equals(path)) {
                return ws;
            }
        }
        return null;
    }

    public Workspace addWorkspace(String name, String path) {
        // Check if already exists
        Workspace existing = getWorkspace(path);
        if (existing != null) {
            existing.name = name;
            existing.lastAccessed = System.currentTimeMillis();
            existing.updateGitStatus();
            saveWorkspaces();
            sortWorkspaces();
            return existing;
        }

        Workspace ws = new Workspace(name, path);
        ws.updateGitStatus();
        mWorkspaces.add(ws);
        saveWorkspaces();
        sortWorkspaces();
        return ws;
    }

    public void removeWorkspace(String path) {
        mWorkspaces.removeIf(ws -> ws.path.equals(path));
        saveWorkspaces();
    }

    public void toggleFavorite(String path) {
        Workspace ws = getWorkspace(path);
        if (ws != null) {
            ws.isFavorite = !ws.isFavorite;
            saveWorkspaces();
            sortWorkspaces();
        }
    }

    public void updateLastAccessed(String path) {
        Workspace ws = getWorkspace(path);
        if (ws != null) {
            ws.lastAccessed = System.currentTimeMillis();
            saveWorkspaces();
            sortWorkspaces();
        }
    }

    public void updateGitStatus(String path) {
        Workspace ws = getWorkspace(path);
        if (ws != null) {
            ws.updateGitStatus();
            saveWorkspaces();
        }
    }

    /**
     * Workspace model class.
     */
    public static class Workspace {
        public String name;
        public String path;
        public boolean isFavorite;
        public long lastAccessed;
        public boolean isGitRepo;
        public String gitBranch;
        public String gitRemoteUrl;

        public Workspace() {
            // For Gson
        }

        public Workspace(String name, String path) {
            this.name = name;
            this.path = path;
            this.isFavorite = false;
            this.lastAccessed = System.currentTimeMillis();
            this.isGitRepo = false;
            this.gitBranch = null;
            this.gitRemoteUrl = null;
        }

        public void updateGitStatus() {
            File gitDir = new File(path, ".git");
            if (gitDir.exists()) {
                isGitRepo = true;
                // Try to read git branch
                File headFile = new File(gitDir, "HEAD");
                if (headFile.exists()) {
                    try {
                        String content = readFileString(headFile).trim();
                        if (content != null) {
                            if (content.startsWith("ref: refs/heads/")) {
                                gitBranch = content.substring("ref: refs/heads/".length());
                            }
                        }
                    } catch (Exception e) {
                        // Ignore
                    }
                }
                // Try to read remote URL
                File configFile = new File(gitDir, "config");
                if (configFile.exists()) {
                    try {
                        String content = readFileString(configFile).trim();
                        if (content != null) {
                            int urlIndex = content.indexOf("url = ");
                            if (urlIndex >= 0) {
                                int start = urlIndex + 6;
                                int end = content.indexOf('\n', start);
                                if (end < 0) end = content.length();
                                gitRemoteUrl = content.substring(start, end).trim();
                            }
                        }
                    } catch (Exception e) {
                        // Ignore
                    }
                }
            } else {
                isGitRepo = false;
                gitBranch = null;
                gitRemoteUrl = null;
            }
        }

        public String getDisplayName() {
            return TextUtils.isEmpty(name) ? new File(path).getName() : name;
        }
    }

    private static String readFileString(File file) {
        try (BufferedReader reader = new BufferedReader(new FileReader(file))) {
            return reader.readLine();
        } catch (IOException e) {
            return null;
        }
    }
}
