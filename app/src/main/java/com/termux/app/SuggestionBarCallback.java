package com.termux.app;

public interface SuggestionBarCallback {
    public void reloadSuggestionBar(char inputChar);
    public void reloadSuggestionBar(boolean delete, boolean enter);
}
