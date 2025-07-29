package com.termux.app.terminal.io;

import java.util.ArrayList;

/**
 * Manages command history for the terminal toolbar text input view.
 * Provides functionality similar to bash command history with up/down navigation.
 */
public class TextInputHistory {

    private static final int DEFAULT_MAX_HISTORY_SIZE = 20;

    private final ArrayList<String> history;
    private final int maxHistorySize;
    private int currentIndex; // -1 means not navigating history, 0+ means navigating
    private String currentEdit; // Stores user's current input when navigating history

    /**
     * Creates a new TextInputHistory with default maximum size.
     */
    public TextInputHistory() {
        this(DEFAULT_MAX_HISTORY_SIZE);
    }

    /**
     * Creates a new TextInputHistory with specified maximum size.
     * @param maxHistorySize Maximum number of history entries to store
     */
    public TextInputHistory(int maxHistorySize) {
        this.maxHistorySize = Math.max(1, maxHistorySize);
        this.history = new ArrayList<>(this.maxHistorySize);
        this.currentIndex = -1;
        this.currentEdit = "";
    }

    /**
     * Adds a new entry to the history.
     * Duplicate consecutive entries are not added.
     * @param text The text to add to history (empty/null entries are ignored)
     */
    public void addEntry(String text) {
        if (text == null || text.trim().isEmpty()) {
            return;
        }

        // Don't add if it's the same as the last entry
        if (!history.isEmpty() && history.get(history.size() - 1).equals(text)) {
            return;
        }

        // Add to end of history
        history.add(text);

        // Remove oldest entries if we exceed max size
        if (history.size() > maxHistorySize) {
            history.remove(0);
        }

        // Reset navigation state
        resetNavigation();
    }

    /**
     * Navigates up in history (to older entries).
     * @param currentText The current text in the input field
     * @return The previous history entry, or null if at the beginning
     */
    public String navigateUp(String currentText) {
        if (history.isEmpty()) {
            return null;
        }

        // If not currently navigating, store current text and start from the end
        if (currentIndex == -1) {
            currentEdit = currentText != null ? currentText : "";
            currentIndex = history.size();
        }

        // Move up (to older entries)
        if (currentIndex > 0) {
            currentIndex--;
            return history.get(currentIndex);
        }

        // Already at the oldest entry
        return history.get(currentIndex);
    }

    /**
     * Navigates down in history (to newer entries).
     * @return The next history entry, current edit, or null if at the end
     */
    public String navigateDown() {
        if (currentIndex == -1 || history.isEmpty()) {
            return null;
        }

        // Move down (to newer entries)
        currentIndex++;

        // If we've gone past the newest entry, return to current edit
        if (currentIndex >= history.size()) {
            String result = currentEdit;
            resetNavigation();
            return result;
        }

        return history.get(currentIndex);
    }

    /**
     * Resets the navigation state to normal input mode.
     */
    public void resetNavigation() {
        currentIndex = -1;
        currentEdit = "";
    }

    /**
     * Checks if currently navigating through history.
     * @return true if in navigation mode, false if in normal input mode
     */
    public boolean isNavigating() {
        return currentIndex != -1;
    }

    /**
     * Gets the current history size.
     * @return Number of entries in history
     */
    public int size() {
        return history.size();
    }

    /**
     * Checks if history is empty.
     * @return true if no history entries exist
     */
    public boolean isEmpty() {
        return history.isEmpty();
    }

    /**
     * Clears all history entries.
     */
    public void clear() {
        history.clear();
        resetNavigation();
    }

    /**
     * Gets a copy of the current history for persistence or debugging.
     * @return A new ArrayList containing all history entries
     */
    public ArrayList<String> getHistoryCopy() {
        return new ArrayList<>(history);
    }

    /**
     * Restores history from a list (for persistence).
     * @param historyEntries List of history entries to restore
     */
    public void restoreHistory(ArrayList<String> historyEntries) {
        if (historyEntries == null) {
            return;
        }

        clear();

        // Add entries while respecting max size
        int startIndex = Math.max(0, historyEntries.size() - maxHistorySize);
        for (int i = startIndex; i < historyEntries.size(); i++) {
            String entry = historyEntries.get(i);
            if (entry != null && !entry.trim().isEmpty()) {
                history.add(entry);
            }
        }
    }
}