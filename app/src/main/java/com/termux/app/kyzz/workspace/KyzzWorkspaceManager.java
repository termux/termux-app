package com.termux.app.kyzz.workspace;

import android.content.Context;
import android.content.SharedPreferences;

import org.json.JSONArray;
import org.json.JSONException;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

public class KyzzWorkspaceManager {
    private static final String PREF_NAME = "kyzz_workspace_prefs";
    private static final String KEY_CURRENT_WORKSPACE = "current_workspace_path";
    private static final String KEY_RECENT_WORKSPACES = "recent_workspaces_json";

    private final SharedPreferences mPrefs;
    private final String mDefaultPath;

    public KyzzWorkspaceManager(Context context) {
        mPrefs = context.getSharedPreferences(PREF_NAME, Context.MODE_PRIVATE);
        // Default to Termux home directory /data/data/com.termux/files/home
        File filesDir = context.getFilesDir();
        File homeDir = new File(filesDir, "home");
        if (!homeDir.exists()) {
            homeDir = filesDir;
        }
        mDefaultPath = homeDir.getAbsolutePath();
    }

    public String getCurrentWorkspacePath() {
        String path = mPrefs.getString(KEY_CURRENT_WORKSPACE, mDefaultPath);
        File dir = new File(path);
        if (!dir.exists() || !dir.isDirectory()) {
            return mDefaultPath;
        }
        return path;
    }

    public void setCurrentWorkspacePath(String path) {
        if (path == null) return;
        mPrefs.edit().putString(KEY_CURRENT_WORKSPACE, path).apply();
        addRecentWorkspace(path);
    }

    public List<String> getRecentWorkspaces() {
        List<String> list = new ArrayList<>();
        String json = mPrefs.getString(KEY_RECENT_WORKSPACES, "[]");
        try {
            JSONArray arr = new JSONArray(json);
            for (int i = 0; i < arr.length(); i++) {
                String p = arr.getString(i);
                if (new File(p).exists()) {
                    list.add(p);
                }
            }
        } catch (JSONException ignored) {}
        if (list.isEmpty()) {
            list.add(mDefaultPath);
        }
        return list;
    }

    public void addRecentWorkspace(String path) {
        List<String> current = getRecentWorkspaces();
        current.remove(path);
        current.add(0, path);
        if (current.size() > 10) {
            current = current.subList(0, 10);
        }
        JSONArray arr = new JSONArray();
        for (String p : current) {
            arr.put(p);
        }
        mPrefs.edit().putString(KEY_RECENT_WORKSPACES, arr.toString()).apply();
    }

    public String getDefaultWorkspacePath() {
        return mDefaultPath;
    }
}
