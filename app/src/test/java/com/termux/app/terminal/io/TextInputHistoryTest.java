package com.termux.app.terminal.io;

import org.junit.Before;
import org.junit.Test;
import static org.junit.Assert.*;
import java.util.ArrayList;

public class TextInputHistoryTest {

    private TextInputHistory history;

    @Before
    public void setUp() {
        history = new TextInputHistory(5); // Use small size for testing
    }

    @Test
    public void testAddEntry() {
        assertTrue(history.isEmpty());
        assertEquals(0, history.size());

        history.addEntry("ls");
        assertEquals(1, history.size());
        assertFalse(history.isEmpty());

        history.addEntry("cd /tmp");
        assertEquals(2, history.size());
    }

    @Test
    public void testAddEntryIgnoresEmptyAndNull() {
        history.addEntry("");
        history.addEntry("   ");
        history.addEntry(null);

        assertEquals(0, history.size());
        assertTrue(history.isEmpty());
    }

    @Test
    public void testAddEntryIgnoresDuplicates() {
        history.addEntry("ls");
        history.addEntry("ls"); // Duplicate should be ignored
        history.addEntry("cd");
        history.addEntry("cd"); // Duplicate should be ignored

        assertEquals(2, history.size());
    }

    @Test
    public void testMaxHistorySize() {
        // Add more entries than max size
        for (int i = 0; i < 10; i++) {
            history.addEntry("command" + i);
        }

        assertEquals(5, history.size()); // Should be capped at max size

        // Verify oldest entries were removed
        ArrayList<String> historyCopy = history.getHistoryCopy();
        assertEquals("command5", historyCopy.get(0)); // Oldest should be command5
        assertEquals("command9", historyCopy.get(4)); // Newest should be command9
    }

    @Test
    public void testNavigateUpFromEmpty() {
        assertFalse(history.isNavigating());
        assertNull(history.navigateUp("current text"));
        assertFalse(history.isNavigating());
    }

    @Test
    public void testNavigateUpBasic() {
        history.addEntry("first");
        history.addEntry("second");
        history.addEntry("third");

        // Navigate up should return most recent first
        assertEquals("third", history.navigateUp("current"));
        assertTrue(history.isNavigating());

        assertEquals("second", history.navigateUp("current"));
        assertEquals("first", history.navigateUp("current"));

        // Should stay at oldest entry
        assertEquals("first", history.navigateUp("current"));
    }

    @Test
    public void testNavigateDown() {
        history.addEntry("first");
        history.addEntry("second");
        history.addEntry("third");

        // Navigate up to start navigation
        assertEquals("third", history.navigateUp("current"));
        assertEquals("second", history.navigateUp("current"));

        // Navigate down
        assertEquals("third", history.navigateDown());

        // Navigate down past newest should return current edit and reset
        assertEquals("current", history.navigateDown());
        assertFalse(history.isNavigating());
    }

    @Test
    public void testNavigateDownWithoutNavigatingUp() {
        history.addEntry("test");

        assertNull(history.navigateDown());
        assertFalse(history.isNavigating());
    }

    @Test
    public void testResetNavigation() {
        history.addEntry("test");

        assertEquals("test", history.navigateUp("current"));
        assertTrue(history.isNavigating());

        history.resetNavigation();
        assertFalse(history.isNavigating());
    }

    @Test
    public void testNavigationPreservesCurrentEdit() {
        history.addEntry("old command");

        String currentEdit = "new command being typed";
        assertEquals("old command", history.navigateUp(currentEdit));

        // Navigate back down should return the current edit
        assertEquals(currentEdit, history.navigateDown());
        assertFalse(history.isNavigating());
    }

    @Test
    public void testClear() {
        history.addEntry("test1");
        history.addEntry("test2");
        assertEquals(2, history.size());

        history.clear();
        assertEquals(0, history.size());
        assertTrue(history.isEmpty());
        assertFalse(history.isNavigating());
    }

    @Test
    public void testGetHistoryCopy() {
        history.addEntry("test1");
        history.addEntry("test2");

        ArrayList<String> copy = history.getHistoryCopy();
        assertEquals(2, copy.size());
        assertEquals("test1", copy.get(0));
        assertEquals("test2", copy.get(1));

        // Verify it's a copy (modifications don't affect original)
        copy.add("test3");
        assertEquals(2, history.size());
    }

    @Test
    public void testRestoreHistory() {
        ArrayList<String> entries = new ArrayList<>();
        entries.add("restored1");
        entries.add("restored2");
        entries.add("restored3");

        history.restoreHistory(entries);

        assertEquals(3, history.size());
        assertEquals("restored1", history.navigateUp(""));
        assertEquals("restored2", history.navigateUp(""));
        assertEquals("restored3", history.navigateUp(""));
    }

    @Test
    public void testRestoreHistoryExceedsMaxSize() {
        ArrayList<String> entries = new ArrayList<>();
        for (int i = 0; i < 10; i++) {
            entries.add("entry" + i);
        }

        history.restoreHistory(entries);

        // Should only keep the most recent entries
        assertEquals(5, history.size());
        ArrayList<String> copy = history.getHistoryCopy();
        assertEquals("entry5", copy.get(0)); // Oldest kept
        assertEquals("entry9", copy.get(4)); // Newest kept
    }

    @Test
    public void testRestoreHistoryWithNullAndEmpty() {
        ArrayList<String> entries = new ArrayList<>();
        entries.add("valid1");
        entries.add("");
        entries.add(null);
        entries.add("  ");
        entries.add("valid2");

        history.restoreHistory(entries);

        assertEquals(2, history.size()); // Only valid entries should be kept
    }

    @Test
    public void testRestoreHistoryWithNull() {
        history.addEntry("existing");
        history.restoreHistory(null);

        assertEquals(0, history.size()); // Should clear existing history
    }

    @Test
    public void testAddEntryResetsNavigation() {
        history.addEntry("test1");
        assertEquals("test1", history.navigateUp("current"));
        assertTrue(history.isNavigating());

        history.addEntry("test2");
        assertFalse(history.isNavigating()); // Should reset navigation
    }
}