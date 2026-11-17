package com.termux.terminal;

public class ScrollPreservationTest extends TerminalTestCase {

	public void testFollowsOutputOnlyWhenAtBottom() {
		withTerminalSized(3, 3).enterString("111222333444555");
		assertEquals(2, mTerminal.getScreen().getActiveTranscriptRows());
		assertTrue(mTerminal.isFollowingOutput());
		assertEquals(0.0, mTerminal.getViewportOffset(), 1e-6);

		// New output arrives while at bottom -> stays at 0:
		enterString("666777");
		assertEquals(4, mTerminal.getScreen().getActiveTranscriptRows());
		assertEquals(0.0, mTerminal.getViewportOffset(), 1e-6);

		// User scrolls 1.4 rows up into history (sub-row pixel offset preserved):
		mTerminal.setViewportOffset(-1.4);
		assertFalse(mTerminal.isFollowingOutput());
		assertEquals(-1.4, mTerminal.getViewportOffset(), 1e-6);

		// 2 more rows of output arrive -> viewport shifts by -2.0 so visible content stays stationary:
		enterString("888999");
		assertEquals(6, mTerminal.getScreen().getActiveTranscriptRows());
		assertEquals(-3.4, mTerminal.getViewportOffset(), 1e-6);

		// Minor sub-row jitter within 0.15 row of bottom snaps back to follow output:
		mTerminal.setViewportOffset(-0.10);
		assertTrue(mTerminal.isFollowingOutput());
		assertEquals(0.0, mTerminal.getViewportOffset(), 1e-6);
		enterString("AAABBB");
		assertEquals(0.0, mTerminal.getViewportOffset(), 1e-6);
	}

	public void testResizeRowCountPreservesExactSubRowPosition() {
		withTerminalSized(3, 4).enterString("000111222333444555666777888999");
		assertEquals(6, mTerminal.getScreen().getActiveTranscriptRows());

		mTerminal.setViewportOffset(-2.3);
		assertEquals(-2.3, mTerminal.getViewportOffset(), 1e-6);

		// Height shrinks from 4 rows to 2 rows (+2 lines pushed into transcript = 8):
		resize(3, 2);
		assertEquals(8, mTerminal.getScreen().getActiveTranscriptRows());
		assertEquals(-4.3, mTerminal.getViewportOffset(), 1e-6);
		assertEquals(3.7, mTerminal.getScreen().getBaseAbsoluteRow() + mTerminal.getViewportOffset(), 1e-6);

		// Height expands back from 2 rows to 4 rows:
		resize(3, 4);
		assertEquals(6, mTerminal.getScreen().getActiveTranscriptRows());
		assertEquals(-2.3, mTerminal.getViewportOffset(), 1e-6);
	}

	public void testColumnReflowPreservesAnchoredLine() {
		withTerminalSized(6, 2).enterString("AAAAAA\r\nBBBBBB\r\nCCCCCC\r\nDDDDDD");
		assertEquals(2, mTerminal.getScreen().getActiveTranscriptRows());

		mTerminal.setViewportOffset(-1.0);
		assertEquals("BBBBBB", mTerminal.getScreen().getSelectedText(0, mTerminal.getTopRow(), 6, mTerminal.getTopRow()).trim());

		resize(3, 2);
		assertEquals(6, mTerminal.getScreen().getActiveTranscriptRows());
		assertEquals("BBB", mTerminal.getScreen().getSelectedText(0, mTerminal.getTopRow(), 3, mTerminal.getTopRow()).trim());
	}

	public void testCsi3JClearResetsViewportWithoutStrandingAtTopOnLongSessionResize() {
		withTerminalSized(6, 4);
		// Overflow default ring buffer capacity or multi-screen history, then scroll up:
		for (int i = 0; i < 300; i++) {
			enterString("L" + i + "\r\n");
		}
		mTerminal.setViewportOffset(-5.0);
		assertFalse(mTerminal.isFollowingOutput());

		// Window shrinks to narrower floating/split width and TUI wipes/replays via CSI 3 J:
		resize(3, 3);
		enterString("\033[H\033[2J\033[3J");
		for (int i = 0; i < 300; i++) {
			enterString("L" + i + "\r\n");
		}
		assertTrue(mTerminal.isFollowingOutput());
		assertEquals(0.0, mTerminal.getViewportOffset(), 1e-6);
	}

	public void testAutoScrollDisabledAtBottomSurvivesOutputAndCsi3J() {
		withTerminalSized(3, 3).enterString("111222333444555666777888999");
		assertEquals(0.0, mTerminal.getViewportOffset(), 1e-6);

		mTerminal.toggleAutoScrollDisabled();
		assertTrue(mTerminal.isAutoScrollDisabled());

		enterString("\033[H\033[2J\033[3J111222333444");
		assertEquals(1, mTerminal.getScreen().getActiveTranscriptRows());
		assertEquals(-1.0, mTerminal.getViewportOffset(), 1e-6);

		mTerminal.toggleAutoScrollDisabled();
		assertFalse(mTerminal.isAutoScrollDisabled());
		assertEquals(0.0, mTerminal.getViewportOffset(), 1e-6);
	}

}
