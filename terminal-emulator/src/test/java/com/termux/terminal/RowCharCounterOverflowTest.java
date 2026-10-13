package com.termux.terminal;

/**
 * Regression test for https://github.com/termux/termux-app/issues/5340.
 *
 * <p>A terminal row wider than 1023 columns can need more than
 * {@link Short#MAX_VALUE} java chars when every column holds a base
 * supplementary code point plus the max allowed combining chars.
 * {@link TerminalRow} used to count that in a {@code short}, which wrapped
 * negative and crashed the next {@code setChar} in {@code System.arraycopy}.
 * Master already stores the counter as an {@code int} (commit 5532cc4);
 * this test locks that in.
 */
public class RowCharCounterOverflowTest extends TerminalTestCase {

    public void testRowCharCounterOverflow() {
        StringBuilder cell = new StringBuilder();
        cell.appendCodePoint(0x10000); // width 1, two java chars
        for (int i = 0; i < 15; i++) cell.appendCodePoint(0x1D167); // width 0, two java chars each
        StringBuilder row = new StringBuilder();
        for (int column = 0; column < 1100; column++) row.append(cell);
        // 1100 columns * 32 java chars = 35200 chars, over Short.MAX_VALUE.
        // Used to throw ArrayIndexOutOfBoundsException from System.arraycopy.
        withTerminalSized(1100, 3).enterString(row.toString());
    }
}
