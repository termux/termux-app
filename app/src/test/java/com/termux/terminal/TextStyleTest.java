package com.termux.terminal;

import junit.framework.TestCase;

public class TextStyleTest extends TestCase {

	private static final int[] ALL_EFFECTS = new int[]{0, TextStyle.CHARACTER_ATTRIBUTE_BOLD, TextStyle.CHARACTER_ATTRIBUTE_ITALIC,
			TextStyle.CHARACTER_ATTRIBUTE_UNDERLINE, TextStyle.CHARACTER_ATTRIBUTE_BLINK, TextStyle.CHARACTER_ATTRIBUTE_INVERSE,
			TextStyle.CHARACTER_ATTRIBUTE_INVISIBLE, TextStyle.CHARACTER_ATTRIBUTE_STRIKETHROUGH, TextStyle.CHARACTER_ATTRIBUTE_PROTECTED,
			TextStyle.CHARACTER_ATTRIBUTE_DIM};

	public void testEncodingSingle() {
		for (int fx : ALL_EFFECTS) {
			for (int fg = 0; fg < TextStyle.NUM_INDEXED_COLORS; fg++) {
				for (int bg = 0; bg < TextStyle.NUM_INDEXED_COLORS; bg++) {
					int encoded = TextStyle.encode(fg, bg, fx);
					assertEquals(fg, TextStyle.decodeForeColor(encoded));
					assertEquals(bg, TextStyle.decodeBackColor(encoded));
					assertEquals(fx, TextStyle.decodeEffect(encoded));
				}
			}
		}
	}

	public void testEncodingCombinations() {
		for (int f1 : ALL_EFFECTS) {
			for (int f2 : ALL_EFFECTS) {
				int combined = f1 | f2;
				assertEquals(combined, TextStyle.decodeEffect(TextStyle.encode(0, 0, combined)));
			}
		}
	}

	public void testEncodingStrikeThrough() {
		int encoded = TextStyle.encode(TextStyle.COLOR_INDEX_FOREGROUND, TextStyle.COLOR_INDEX_BACKGROUND,
				TextStyle.CHARACTER_ATTRIBUTE_STRIKETHROUGH);
		assertTrue((TextStyle.decodeEffect(encoded) & TextStyle.CHARACTER_ATTRIBUTE_STRIKETHROUGH) != 0);
	}

	public void testEncodingProtected() {
		int encoded = TextStyle.encode(TextStyle.COLOR_INDEX_FOREGROUND, TextStyle.COLOR_INDEX_BACKGROUND,
				TextStyle.CHARACTER_ATTRIBUTE_STRIKETHROUGH);
		assertTrue((TextStyle.decodeEffect(encoded) & TextStyle.CHARACTER_ATTRIBUTE_PROTECTED) == 0);
		encoded = TextStyle.encode(TextStyle.COLOR_INDEX_FOREGROUND, TextStyle.COLOR_INDEX_BACKGROUND,
				TextStyle.CHARACTER_ATTRIBUTE_STRIKETHROUGH | TextStyle.CHARACTER_ATTRIBUTE_PROTECTED);
		assertTrue((TextStyle.decodeEffect(encoded) & TextStyle.CHARACTER_ATTRIBUTE_PROTECTED) != 0);
	}

}
