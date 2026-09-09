package com.termux.view;

import org.junit.Test;

import static org.junit.Assert.*;

/** Pure-Java checks for the conservative, display-only flow gate and grid heuristic. */
public class TerminalPresentationPolicyTest {

    @Test public void mixedRowArrowDirectionDoesNotChangeAtLatinBoundaries() {
        String text = "اب abc";
        for (int cursor = 0; cursor <= text.length(); cursor++)
            assertTrue(TerminalPresentationPolicy.rtlArrows(text.toCharArray(), text.length()));
    }

    @Test public void pureLatinArrowsRemainNativeAndBlankRowsDefaultRtl() {
        assertFalse(TerminalPresentationPolicy.rtlArrows("abc 123".toCharArray(), 7));
        assertTrue(TerminalPresentationPolicy.rtlArrows("   ".toCharArray(), 3));
        assertTrue(TerminalPresentationPolicy.rtlArrows("123".toCharArray(), 3));
    }

    @Test public void marksNeverStopNativeColumnMapping() {
        String text = "بَجد";
        assertArrayEquals(new int[]{2, 3}, TerminalPresentationPolicy.cellRange(text.toCharArray(), text.length(), 2));
    }

    @Test public void bothHalvesOfWideCharactersMapToFullWidthEdges() {
        String text = "A😀B";
        assertArrayEquals(new int[]{1, 3}, TerminalPresentationPolicy.cellRange(text.toCharArray(), text.length(), 1));
        assertArrayEquals(new int[]{1, 3}, TerminalPresentationPolicy.cellRange(text.toCharArray(), text.length(), 2));
        assertArrayEquals(new int[]{3, 4}, TerminalPresentationPolicy.cellRange(text.toCharArray(), text.length(), 3));
    }

    private static boolean grid(String text) {
        return TerminalPresentationPolicy.looksLikeGrid(text.toCharArray(), text.length());
    }

    @Test public void flowRequiresEnabledAndAllInteractiveModesOff() {
        for (int flags = 0; flags < 16; flags++) {
            boolean enabled = (flags & 1) != 0;
            boolean alternate = (flags & 2) != 0;
            boolean mouse = (flags & 4) != 0;
            boolean applicationCursor = (flags & 8) != 0;
            assertEquals("Mode flags " + flags, flags == 1,
                TerminalPresentationPolicy.flowAllowed(enabled, alternate, mouse, applicationCursor));
        }
    }

    @Test public void preMarshmallowAlwaysUsesNativeRendererPolicy() {
        for (int sdk : new int[]{21, 22}) {
            for (int flags = 0; flags < 16; flags++) {
                assertFalse(TerminalPresentationPolicy.flowAllowed(sdk, (flags & 1) != 0,
                    (flags & 2) != 0, (flags & 4) != 0, (flags & 8) != 0));
            }
        }
    }

    @Test public void api23AndLaterStillRequireExplicitOptInAndSafeModes() {
        for (int sdk : new int[]{23, 24, 35}) {
            for (int flags = 0; flags < 16; flags++) {
                assertEquals("SDK " + sdk + " mode flags " + flags, flags == 1,
                    TerminalPresentationPolicy.flowAllowed(sdk, (flags & 1) != 0,
                        (flags & 2) != 0, (flags & 4) != 0, (flags & 8) != 0));
            }
        }
    }

    @Test public void emptyAndSpaceOnlyRowsAreNotGrids() {
        assertFalse(grid(""));
        assertFalse(grid(" "));
        assertFalse(grid("                                        "));
    }

    @Test public void arabicAndMixedParagraphsAreNotGrids() {
        assertFalse(grid("مرحبا بالعالم"));
        assertFalse(grid("مرحبا world 123 بالعالم"));
        assertFalse(grid("English prose with ordinary spaces."));
    }

    @Test public void trailingTerminalPaddingDoesNotMakeParagraphAGrid() {
        assertFalse(grid("مرحبا بالعالم                    "));
        assertFalse(grid("hello world  "));
    }

    @Test public void alignedColumnsKeepGrid() {
        assertTrue(grid("name  size  date"));
        assertTrue(grid("ملف    الحجم"));
        assertTrue(grid("a b  c"));
    }

    @Test public void twoOrMoreLeadingSpacesKeepIndentation() {
        assertTrue(grid("  indented paragraph"));
        assertTrue(grid("    مرحبا"));
        assertTrue(grid(" one leading space")); // A one-cell primary-buffer indent is an anchor too.
    }

    @Test public void tabsKeepGridEvenAtRowEnd() {
        assertTrue(grid("\tindented"));
        assertTrue(grid("name\tvalue"));
        assertTrue(grid("paragraph\t"));
        assertTrue(grid("\t"));
    }

    @Test public void unicodeTableBordersAndBlockGraphicsKeepGrid() {
        assertTrue(grid("│ a │ b │"));
        assertTrue(grid("┌───┬───┐"));
        assertTrue(grid("progress █"));
        assertTrue(grid("\u2500"));
        assertTrue(grid("\u259f"));
    }

    @Test public void asciiBordersAndSeparatorsKeepGrid() {
        assertTrue(grid("+---+---+"));
        assertTrue(grid("---"));
        assertTrue(grid("==="));
        assertTrue(grid("section --- next"));
        assertTrue(grid("header ==="));
    }

    @Test public void shortHyphensAndEqualsAreNotSeparators() {
        assertFalse(grid("-"));
        assertFalse(grid("--"));
        assertFalse(grid("="));
        assertFalse(grid("=="));
        assertFalse(grid("a-b c=d --help"));
    }

    @Test public void pathLettersSlashesAndDriveColonDoNotTriggerGrid() {
        assertFalse(grid("/data/data/com.termux/files/home"));
        assertFalse(grid("C:\\Users\\Alice\\notes.txt"));
        assertFalse(grid("~/work/project-name/file.txt"));
        assertFalse(grid("المسار /home/user/ملف.txt"));
        assertFalse(grid("abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ"));
    }

    @Test public void lengthBoundsInspectionToUsedText() {
        char[] text = "hello  ignored --- │".toCharArray();
        assertFalse(TerminalPresentationPolicy.looksLikeGrid(text, 5));
        assertTrue(TerminalPresentationPolicy.looksLikeGrid(text, text.length));
        assertFalse(TerminalPresentationPolicy.looksLikeGrid(text, 0));
    }

    @Test public void renderingIsAvailableInAllTuiModesButFlowNavigationIsNot() {
        for (int sdk : new int[]{21, 22, 23, 28, 35}) {
            for (int flags = 0; flags < 16; flags++) {
                boolean enabled = (flags & 1) != 0;
                assertEquals(sdk >= 23 && enabled, TerminalPresentationPolicy.renderingAllowed(sdk, enabled));
                assertEquals(sdk >= 23 && flags == 1, TerminalPresentationPolicy.flowAllowed(sdk, enabled,
                    (flags & 2) != 0, (flags & 4) != 0, (flags & 8) != 0));
            }
        }
    }

    @Test public void bulletsAndBorderAnchorsPreventWholeRowPacking() {
        for (String text : new String[]{"• مرحبا", "- عربي", "12. عربي", "^G مساعدة", "a|ب"})
            assertTrue(text, grid(text));
        assertFalse(grid("/home/user/ملف.txt"));
        assertTrue(TerminalPresentationPolicy.hasStrongRtl("عربي 123".toCharArray(), 0, 8));
        assertFalse(TerminalPresentationPolicy.hasStrongRtl("123 /tmp".toCharArray(), 0, 8));
    }

    @Test public void doesNotMutateLogicalInput() {
        char[] text = "  مرحبا  world ---   ".toCharArray();
        char[] original = text.clone();
        assertTrue(TerminalPresentationPolicy.looksLikeGrid(text, text.length));
        assertArrayEquals(original, text);
    }
}
