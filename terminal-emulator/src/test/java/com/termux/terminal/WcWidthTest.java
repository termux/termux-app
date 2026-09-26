package com.termux.terminal;

import junit.framework.TestCase;

public class WcWidthTest extends TestCase {

	private static void assertWidthIs(int expectedWidth, int codePoint) {
		int wcWidth = WcWidth.width(codePoint);
		assertEquals(expectedWidth, wcWidth);
	}

	public void testPrintableAscii() {
		for (int i = 0x20; i <= 0x7E; i++) {
			assertWidthIs(1, i);
		}
	}

	public void testSomeWidthOne() {
		assertWidthIs(1, 'å');
		assertWidthIs(1, 'ä');
		assertWidthIs(1, 'ö');
		assertWidthIs(1, 0x23F2);
	}

	public void testSomeWide() {
		assertWidthIs(2, 'Ａ');
		assertWidthIs(2, 'Ｂ');
		assertWidthIs(2, 'Ｃ');
		assertWidthIs(2, '中');
		assertWidthIs(2, '文');

		assertWidthIs(2, 0x679C);
		assertWidthIs(2, 0x679D);

		assertWidthIs(2, 0x2070E);
		assertWidthIs(2, 0x20731);

		assertWidthIs(1, 0x1F781);
	}

	public void testSomeNonWide() {
		assertWidthIs(1, 0x1D11E);
		assertWidthIs(1, 0x1D11F);
	}

	public void testCombining() {
		assertWidthIs(0, 0x0302);
		assertWidthIs(0, 0x0308);
		assertWidthIs(0, 0xFE0F);
	}

	public void testWordJoiner() {
		// https://en.wikipedia.org/wiki/Word_joiner
		// The word joiner (WJ) is a code point in Unicode used to separate words when using scripts
		// that do not use explicit spacing. It is encoded since Unicode version 3.2
		// (released in 2002) as U+2060 WORD JOINER (HTML &#8288;).
		// The word joiner does not produce any space, and prohibits a line break at its position.
		assertWidthIs(0, 0x2060);
	}

	public void testSofthyphen() {
		// http://osdir.com/ml/internationalization.linux/2003-05/msg00006.html:
		// "Existing implementation practice in terminals is that the SOFT HYPHEN is
		// a spacing graphical character, and the purpose of my wcwidth() was to
		// predict the advancement of the cursor position after a string is sent to
		// a terminal. Hence, I have no choice but to keep wcwidth(SOFT HYPHEN) = 1.
		// VT100-style terminals do not hyphenate."
		assertWidthIs(1, 0x00AD);
	}

	public void testHangul() {
		assertWidthIs(1, 0x11A3);
	}

	public void testEmojis() {
		assertWidthIs(2, 0x1F428); // KOALA.
		assertWidthIs(2, 0x231a);  // WATCH.
		assertWidthIs(2, 0x1F643); // UPSIDE-DOWN FACE (Unicode 8).
	}

	public void testEmojiVariationSequences() {
		// https://github.com/termux/termux-app/issues/5251 - "❤️", "🗡️" and "🗺️" are narrow by
		// default (Emoji_Presentation=No) but render as double-width emoji when immediately
		// followed by U+FE0F VARIATION SELECTOR-16.
		assertEquals(1, WcWidth.width(0x2764)); // HEAVY BLACK HEART alone.
		assertEquals(1, WcWidth.width(0x1F5E1)); // DAGGER KNIFE alone.
		assertEquals(1, WcWidth.width(0x1F5FA)); // WORLD MAP alone.

		// BMP base character (no surrogate) followed by VS16.
		assertEquals(2, WcWidth.width(new char[]{0x2764, WcWidth.VARIATION_SELECTOR_16}, 0));
		// No following character at all: just the base character's own width.
		assertEquals(1, WcWidth.width(new char[]{0x2764}, 0));

		// Supplementary-plane base character (surrogate pair) followed by VS16.
		char[] dagger = new char[3];
		Character.toChars(0x1F5E1, dagger, 0);
		dagger[2] = WcWidth.VARIATION_SELECTOR_16;
		assertEquals(2, WcWidth.width(dagger, 0)); // 2-arg form defaults the limit to the full array.
		assertEquals(2, WcWidth.width(dagger, 0, dagger.length)); // Explicit limit including the VS16.
		assertEquals(1, WcWidth.width(dagger, 0, 2)); // A limit excluding the VS16 must not widen.

		// A code point already Emoji_Presentation=Yes (e.g. green heart, U+1F49A) followed by VS16
		// is unaffected: it is already width 2 without needing the variation-selector special case.
		char[] greenHeart = new char[3];
		Character.toChars(0x1F49A, greenHeart, 0);
		greenHeart[2] = WcWidth.VARIATION_SELECTOR_16;
		assertEquals(2, WcWidth.width(greenHeart, 0, greenHeart.length));

		// A VARIATION SELECTOR-16 not following an emoji-variation base stays zero width.
		assertEquals(0, WcWidth.width(new char[]{'a', WcWidth.VARIATION_SELECTOR_16}, 1, 2));
	}

}
