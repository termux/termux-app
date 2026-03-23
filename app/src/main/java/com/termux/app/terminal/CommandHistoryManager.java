package com.termux.app.terminal;

import android.os.AsyncTask;
import android.util.Log;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class CommandHistoryManager {
    private static final String LOG_TAG = "CommandHistoryManager";
    private static final String BASH_HISTORY_FILE = "/data/data/com.termux/files/home/.bash_history";
    private static final String ZSH_HISTORY_FILE = "/data/data/com.termux/files/home/.zsh_history";

    private final List<String> mHistory = new ArrayList<>();
    private boolean mIsLoading = false;

    public interface HistoryLoadListener {
        void onHistoryLoaded();
    }

    public void loadHistory(final HistoryLoadListener listener) {
        if (mIsLoading) return;
        mIsLoading = true;

        new AsyncTask<Void, Void, List<String>>() {
            @Override
            protected List<String> doInBackground(Void... voids) {
                Set<String> uniqueCommands = new HashSet<>();
                List<String> commands = new ArrayList<>();

                readHistoryFile(BASH_HISTORY_FILE, uniqueCommands, commands);
                readHistoryFile(ZSH_HISTORY_FILE, uniqueCommands, commands);

                // Reverse so recent commands come first
                Collections.reverse(commands);
                return commands;
            }

            @Override
            protected void onPostExecute(List<String> commands) {
                mHistory.clear();
                mHistory.addAll(commands);
                mIsLoading = false;
                if (listener != null) {
                    listener.onHistoryLoaded();
                }
            }
        }.execute();
    }

    private void readHistoryFile(String filePath, Set<String> uniqueCommands, List<String> commands) {
        File file = new File(filePath);
        if (!file.exists() || !file.canRead()) return;

        try (BufferedReader reader = new BufferedReader(new FileReader(file))) {
            String line;
            while ((line = reader.readLine()) != null) {
                line = line.trim();
                // Basic cleanup for zsh history format ": 1234567890:0;command"
                if (line.startsWith(":") && line.contains(";")) {
                    int semicolonIndex = line.indexOf(';');
                    if (semicolonIndex != -1 && semicolonIndex < line.length() - 1) {
                        line = line.substring(semicolonIndex + 1).trim();
                    }
                }

                if (!line.isEmpty() && !uniqueCommands.contains(line)) {
                    uniqueCommands.add(line);
                    commands.add(line);
                } else if (!line.isEmpty() && uniqueCommands.contains(line)) {
                    // Move to end (more recent)
                    commands.remove(line);
                    commands.add(line);
                }
            }
        } catch (IOException e) {
            Log.e(LOG_TAG, "Error reading history file: " + filePath, e);
        }
    }

    public List<String> getSuggestions(String prefix) {
        if (prefix == null || prefix.trim().isEmpty()) {
            return Collections.emptyList();
        }

        List<String> suggestions = new ArrayList<>();
        String lowerPrefix = prefix.toLowerCase();

        for (String command : mHistory) {
            if (command.toLowerCase().startsWith(lowerPrefix)) {
                suggestions.add(command);
                if (suggestions.size() >= 10) { // Limit suggestions
                    break;
                }
            }
        }
        return suggestions;
    }
}
