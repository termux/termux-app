package com.termux.terminal;

import junit.framework.TestCase;

import java.util.Arrays;
import java.util.Random;

public class TerminalRowTest extends TestCase {

	/** The properties of these code points are validated in {@link #testStaticConstants()}. */
	private static final int ONE_JAVA_CHAR_DISPLAY_WIDTH_TWO_1 = 0x679C;
	private static final int ONE_JAVA_CHAR_DISPLAY_WIDTH_TWO_2 = 0x679D;
	private static final int TWO_JAVA_CHARS_DISPLAY_WIDTH_TWO_1 = 0x2070E;
	private static final int TWO_JAVA_CHARS_DISPLAY_WIDTH_TWO_2 = 0x20731;

	/** Unicode Character 'MUSICAL SYMBOL G CLEF' (U+1D11E). Two java chars required for this. */
	static final int TWO_JAVA_CHARS_DISPLAY_WIDTH_ONE_1 = 0x1D11E;
	/** Unicode Character 'MUSICAL SYMBOL G CLEF OTTAVA ALTA' (U+1D11F). Two java chars required for this. */
	private static final int TWO_JAVA_CHARS_DISPLAY_WIDTH_ONE_2 = 0x1D11F;

	private final int COLUMNS = 80;

	/** A combining character. */
	private static final int DIARESIS_CODEPOINT = 0x0308;

	private TerminalRow row;

	@Override
	protected void setUp() throws Exception {
		super.setUp();
		row = new TerminalRow(COLUMNS, TextStyle.NORMAL);
	}

	private void assertLineStartsWith(int... codePoints) {
		char[] chars = row.mText;
		int charIndex = 0;
		for (int i = 0; i < codePoints.length; i++) {
			int lineCodePoint = chars[charIndex++];
			if (Character.isHighSurrogate((char) lineCodePoint)) {
				lineCodePoint = Character.toCodePoint((char) lineCodePoint, chars[charIndex++]);
			}
			assertEquals("Differing a code point index=" + i, codePoints[i], lineCodePoint);
		}
	}

	private void assertColumnCharIndicesStartsWith(int... indices) {
		for (int i = 0; i < indices.length; i++) {
			int expected = indices[i];
			int actual = row.findStartOfColumn(i);
			assertEquals("At index=" + i, expected, actual);
		}
	}

	public void testSimpleDiaresis() {
		row.setChar(0, DIARESIS_CODEPOINT, 0);
		assertEquals(81, row.getSpaceUsed());
		row.setChar(0, DIARESIS_CODEPOINT, 0);
		assertEquals(82, row.getSpaceUsed());
		assertLineStartsWith(' ', DIARESIS_CODEPOINT, DIARESIS_CODEPOINT, ' ');
	}

	public void testStaticConstants() {
		assertEquals(1, Character.charCount(ONE_JAVA_CHAR_DISPLAY_WIDTH_TWO_1));
		assertEquals(1, Character.charCount(ONE_JAVA_CHAR_DISPLAY_WIDTH_TWO_2));
		assertEquals(2, WcWidth.width(ONE_JAVA_CHAR_DISPLAY_WIDTH_TWO_1));
		assertEquals(2, WcWidth.width(ONE_JAVA_CHAR_DISPLAY_WIDTH_TWO_2));

		assertEquals(2, Character.charCount(TWO_JAVA_CHARS_DISPLAY_WIDTH_ONE_1));
		assertEquals(2, Character.charCount(TWO_JAVA_CHARS_DISPLAY_WIDTH_ONE_2));
		assertEquals(1, WcWidth.width(TWO_JAVA_CHARS_DISPLAY_WIDTH_ONE_1));
		assertEquals(1, WcWidth.width(TWO_JAVA_CHARS_DISPLAY_WIDTH_ONE_2));

		assertEquals(2, Character.charCount(TWO_JAVA_CHARS_DISPLAY_WIDTH_TWO_1));
		assertEquals(2, Character.charCount(TWO_JAVA_CHARS_DISPLAY_WIDTH_TWO_2));
		assertEquals(2, WcWidth.width(TWO_JAVA_CHARS_DISPLAY_WIDTH_TWO_1));
		assertEquals(2, WcWidth.width(TWO_JAVA_CHARS_DISPLAY_WIDTH_TWO_2));

		assertEquals(1, Character.charCount(DIARESIS_CODEPOINT));
		assertEquals(0, WcWidth.width(DIARESIS_CODEPOINT));
	}

	public void testOneColumn() {
		assertEquals(0, row.findStartOfColumn(0));
		row.setChar(0, 'a', 0);
		assertEquals(0, row.findStartOfColumn(0));
	}

	public void testAscii() {
		assertEquals(0, row.findStartOfColumn(0));
		row.setChar(0, 'a', 0);
		assertLineStartsWith('a', ' ', ' ');
		assertEquals(1, row.findStartOfColumn(1));
		assertEquals(80, row.getSpaceUsed());
		row.setChar(0, 'b', 0);
		assertEquals(1, row.findStartOfColumn(1));
		assertEquals(2, row.findStartOfColumn(2));
		assertEquals(80, row.getSpaceUsed());
		assertColumnCharIndicesStartsWith(0, 1, 2, 3);

		char[] someChars = new char[]{'a', 'c', 'e', '4', '5', '6', '7', '8'};

		char[] rawLine = new char[80];
		Arrays.fill(rawLine, ' ');
		Random random = new Random();
		for (int i = 0; i < 1000; i++) {
			int lineIndex = random.nextInt(rawLine.length);
			int charIndex = random.nextInt(someChars.length);
			rawLine[lineIndex] = someChars[charIndex];
			row.setChar(lineIndex, someChars[charIndex], 0);
		}
		char[] lineChars = row.mText;
		for (int i = 0; i < rawLine.length; i++) {
			assertEquals(rawLine[i], lineChars[i]);
		}
	}

	public void testUnicode() {
		assertEquals(0, row.findStartOfColumn(0));
		assertEquals(80, row.getSpaceUsed());

		row.setChar(0, TWO_JAVA_CHARS_DISPLAY_WIDTH_ONE_1, 0);
		assertEquals(81, row.getSpaceUsed());
		assertEquals(0, row.findStartOfColumn(0));
		assertEquals(2, row.findStartOfColumn(1));
		assertLineStartsWith(TWO_JAVA_CHARS_DISPLAY_WIDTH_ONE_1, ' ', ' ');
		assertColumnCharIndicesStartsWith(0, 2, 3, 4);

		row.setChar(0, 'a', 0);
		assertEquals(80, row.getSpaceUsed());
		assertEquals(0, row.findStartOfColumn(0));
		assertEquals(1, row.findStartOfColumn(1));
		assertLineStartsWith('a', ' ', ' ');
		assertColumnCharIndicesStartsWith(0, 1, 2, 3);

		row.setChar(0, TWO_JAVA_CHARS_DISPLAY_WIDTH_ONE_1, 0);
		row.setChar(1, 'a', 0);
		assertLineStartsWith(TWO_JAVA_CHARS_DISPLAY_WIDTH_ONE_1, 'a', ' ');

		row.setChar(0, TWO_JAVA_CHARS_DISPLAY_WIDTH_ONE_1, 0);
		row.setChar(1, TWO_JAVA_CHARS_DISPLAY_WIDTH_ONE_2, 0);
		assertLineStartsWith(TWO_JAVA_CHARS_DISPLAY_WIDTH_ONE_1, TWO_JAVA_CHARS_DISPLAY_WIDTH_ONE_2, ' ');
		assertColumnCharIndicesStartsWith(0, 2, 4, 5);
		assertEquals(82, row.getSpaceUsed());
	}

	public void testDoubleWidth() {
		row.setChar(0, 'a', 0);
		row.setChar(1, ONE_JAVA_CHAR_DISPLAY_WIDTH_TWO_2, 0);
		assertLineStartsWith('a', ONE_JAVA_CHAR_DISPLAY_WIDTH_TWO_2, ' ');
		assertColumnCharIndicesStartsWith(0, 1, 1, 2);
		row.setChar(0, ONE_JAVA_CHAR_DISPLAY_WIDTH_TWO_1, 0);
		assertLineStartsWith(ONE_JAVA_CHAR_DISPLAY_WIDTH_TWO_1, ' ', ' ');
		assertColumnCharIndicesStartsWith(0, 0, 1, 2);

		row.setChar(0, ' ', 0);
		assertLineStartsWith(' ', ' ', ' ', ' ');
		assertColumnCharIndicesStartsWith(0, 1, 2, 3, 4);
		row.setChar(0, ONE_JAVA_CHAR_DISPLAY_WIDTH_TWO_1, 0);
		row.setChar(2, ONE_JAVA_CHAR_DISPLAY_WIDTH_TWO_2, 0);
		assertLineStartsWith(ONE_JAVA_CHAR_DISPLAY_WIDTH_TWO_1, ONE_JAVA_CHAR_DISPLAY_WIDTH_TWO_2);
		assertColumnCharIndicesStartsWith(0, 0, 1, 1, 2);
		row.setChar(0, 'a', 0);
		assertLineStartsWith('a', ' ', ONE_JAVA_CHAR_DISPLAY_WIDTH_TWO_2, ' ');
	}

	/** Just as {@link #testDoubleWidth()} but requires a surrogate pair. */
	public void testDoubleWidthSurrogage() {
		row.setChar(0, 'a', 0);
		assertColumnCharIndicesStartsWith(0, 1, 2, 3, 4);

		row.setChar(1, TWO_JAVA_CHARS_DISPLAY_WIDTH_TWO_2, 0);
		assertColumnCharIndicesStartsWith(0, 1, 1, 3, 4);
		assertLineStartsWith('a', TWO_JAVA_CHARS_DISPLAY_WIDTH_TWO_2, ' ');
		row.setChar(0, TWO_JAVA_CHARS_DISPLAY_WIDTH_TWO_1, 0);
		assertColumnCharIndicesStartsWith(0, 0, 2, 3, 4);
		assertLineStartsWith(TWO_JAVA_CHARS_DISPLAY_WIDTH_TWO_1, ' ', ' ', ' ');

		row.setChar(0, ' ', 0);
		assertLineStartsWith(' ', ' ', ' ', ' ');
		row.setChar(0, TWO_JAVA_CHARS_DISPLAY_WIDTH_TWO_1, 0);
		row.setChar(1, TWO_JAVA_CHARS_DISPLAY_WIDTH_TWO_2, 0);
		assertLineStartsWith(' ', TWO_JAVA_CHARS_DISPLAY_WIDTH_TWO_2, ' ');
		row.setChar(0, 'a', 0);
		assertLineStartsWith('a', TWO_JAVA_CHARS_DISPLAY_WIDTH_TWO_2, ' ');
	}

	public void testReplacementChar() {
		row.setChar(0, TerminalEmulator.UNICODE_REPLACEMENT_CHAR, 0);
		row.setChar(1, 'Y', 0);
		assertLineStartsWith(TerminalEmulator.UNICODE_REPLACEMENT_CHAR, 'Y', ' ', ' ');
	}

	public void testSurrogateCharsWithNormalDisplayWidth() {
		// These requires a UTF-16 surrogate pair, and has a display width of one.
		int first = 0x1D306;
		int second = 0x1D307;
		// Assert the above statement:
		assertEquals(2, Character.toChars(first).length);
		assertEquals(2, Character.toChars(second).length);

		row.setChar(0, second, 0);
		assertEquals(second, Character.toCodePoint(row.mText[0], row.mText[1]));
		assertEquals(' ', row.mText[2]);
		assertEquals(2, row.findStartOfColumn(1));

		row.setChar(0, first, 0);
		assertEquals(first, Character.toCodePoint(row.mText[0], row.mText[1]));
		assertEquals(' ', row.mText[2]);
		assertEquals(2, row.findStartOfColumn(1));

		row.setChar(1, second, 0);
		row.setChar(2, 'a', 0);
		assertEquals(first, Character.toCodePoint(row.mText[0], row.mText[1]));
		assertEquals(second, Character.toCodePoint(row.mText[2], row.mText[3]));
		assertEquals('a', row.mText[4]);
		assertEquals(' ', row.mText[5]);
		assertEquals(0, row.findStartOfColumn(0));
		assertEquals(2, row.findStartOfColumn(1));
		assertEquals(4, row.findStartOfColumn(2));
		assertEquals(5, row.findStartOfColumn(3));
		assertEquals(6, row.findStartOfColumn(4));

		row.setChar(0, ' ', 0);
		assertEquals(' ', row.mText[0]);
		assertEquals(second, Character.toCodePoint(row.mText[1], row.mText[2]));
		assertEquals('a', row.mText[3]);
		assertEquals(' ', row.mText[4]);
		assertEquals(0, row.findStartOfColumn(0));
		assertEquals(1, row.findStartOfColumn(1));
		assertEquals(3, row.findStartOfColumn(2));
		assertEquals(4, row.findStartOfColumn(3));
		assertEquals(5, row.findStartOfColumn(4));

		for (int i = 0; i < 80; i++) {
			row.setChar(i, i % 2 == 0 ? first : second, 0);
		}
		for (int i = 0; i < 80; i++) {
			int idx = row.findStartOfColumn(i);
			assertEquals(i % 2 == 0 ? first : second, Character.toCodePoint(row.mText[idx], row.mText[idx + 1]));
		}
		for (int i = 0; i < 80; i++) {
			row.setChar(i, i % 2 == 0 ? 'a' : 'b', 0);
		}
		for (int i = 0; i < 80; i++) {
			int idx = row.findStartOfColumn(i);
			assertEquals(i, idx);
			assertEquals(i % 2 == 0 ? 'a' : 'b', row.mText[i]);
		}
	}

	public void testOverwritingDoubleDisplayWidthWithNormalDisplayWidth() {
		// Initial "OO "
		row.setChar(0, ONE_JAVA_CHAR_DISPLAY_WIDTH_TWO_1, 0);
		assertEquals(ONE_JAVA_CHAR_DISPLAY_WIDTH_TWO_1, row.mText[0]);
		assertEquals(' ', row.mText[1]);
		assertEquals(0, row.findStartOfColumn(0));
		assertEquals(0, row.findStartOfColumn(1));
		assertEquals(1, row.findStartOfColumn(2));

		// Setting first column to a clears second: "a  "
		row.setChar(0, 'a', 0);
		assertEquals('a', row.mText[0]);
		assertEquals(' ', row.mText[1]);
		assertEquals(0, row.findStartOfColumn(0));
		assertEquals(1, row.findStartOfColumn(1));
		assertEquals(2, row.findStartOfColumn(2));

		// Back to initial "OO "
		row.setChar(0, ONE_JAVA_CHAR_DISPLAY_WIDTH_TWO_1, 0);
		assertEquals(ONE_JAVA_CHAR_DISPLAY_WIDTH_TWO_1, row.mText[0]);
		assertEquals(' ', row.mText[1]);
		assertEquals(0, row.findStartOfColumn(0));
		assertEquals(0, row.findStartOfColumn(1));
		assertEquals(1, row.findStartOfColumn(2));

		// Setting first column to a clears first: " a "
		row.setChar(1, 'a', 0);
		assertEquals(' ', row.mText[0]);
		assertEquals('a', row.mText[1]);
		assertEquals(' ', row.mText[2]);
		assertEquals(0, row.findStartOfColumn(0));
		assertEquals(1, row.findStartOfColumn(1));
		assertEquals(2, row.findStartOfColumn(2));
	}

	public void testOverwritingDoubleDisplayWidthWithSelf() {
		row.setChar(0, ONE_JAVA_CHAR_DISPLAY_WIDTH_TWO_1, 0);
		row.setChar(0, ONE_JAVA_CHAR_DISPLAY_WIDTH_TWO_1, 0);
		assertEquals(ONE_JAVA_CHAR_DISPLAY_WIDTH_TWO_1, row.mText[0]);
		assertEquals(' ', row.mText[1]);
		assertEquals(0, row.findStartOfColumn(0));
		assertEquals(0, row.findStartOfColumn(1));
		assertEquals(1, row.findStartOfColumn(2));
	}

	public void testNormalCharsWithDoubleDisplayWidth() {
		// These fit in one java char, and has a display width of two.
		assertTrue(ONE_JAVA_CHAR_DISPLAY_WIDTH_TWO_1 != ONE_JAVA_CHAR_DISPLAY_WIDTH_TWO_2);
		assertEquals(1, Character.charCount(ONE_JAVA_CHAR_DISPLAY_WIDTH_TWO_1));
		assertEquals(1, Character.charCount(ONE_JAVA_CHAR_DISPLAY_WIDTH_TWO_2));
		assertEquals(2, WcWidth.width(ONE_JAVA_CHAR_DISPLAY_WIDTH_TWO_1));
		assertEquals(2, WcWidth.width(ONE_JAVA_CHAR_DISPLAY_WIDTH_TWO_2));

		row.setChar(0, ONE_JAVA_CHAR_DISPLAY_WIDTH_TWO_1, 0);
		assertEquals(ONE_JAVA_CHAR_DISPLAY_WIDTH_TWO_1, row.mText[0]);
		assertEquals(0, row.findStartOfColumn(1));
		assertEquals(' ', row.mText[1]);

		row.setChar(0, 'a', 0);
		assertEquals('a', row.mText[0]);
		assertEquals(' ', row.mText[1]);
		assertEquals(1, row.findStartOfColumn(1));

		row.setChar(0, ONE_JAVA_CHAR_DISPLAY_WIDTH_TWO_1, 0);
		assertEquals(ONE_JAVA_CHAR_DISPLAY_WIDTH_TWO_1, row.mText[0]);
		// The first character fills both first columns.
		assertEquals(0, row.findStartOfColumn(1));
		row.setChar(2, 'a', 0);
		assertEquals(ONE_JAVA_CHAR_DISPLAY_WIDTH_TWO_1, row.mText[0]);
		assertEquals('a', row.mText[1]);
		assertEquals(1, row.findStartOfColumn(2));

		row.setChar(0, 'c', 0);
		assertEquals('c', row.mText[0]);
		assertEquals(' ', row.mText[1]);
		assertEquals('a', row.mText[2]);
		assertEquals(' ', row.mText[3]);
		assertEquals(0, row.findStartOfColumn(0));
		assertEquals(1, row.findStartOfColumn(1));
		assertEquals(2, row.findStartOfColumn(2));
	}

	public void testNormalCharsWithDoubleDisplayWidthOverlapping() {
		// These fit in one java char, and has a display width of two.
		row.setChar(0, ONE_JAVA_CHAR_DISPLAY_WIDTH_TWO_1, 0);
		row.setChar(2, ONE_JAVA_CHAR_DISPLAY_WIDTH_TWO_2, 0);
		row.setChar(4, 'a', 0);
		// O = ONE_JAVA_CHAR_DISPLAY_WIDTH_TWO
		// A = ANOTHER_JAVA_CHAR_DISPLAY_WIDTH_TWO
		// "OOAAa    "
		assertEquals(0, row.findStartOfColumn(0));
		assertEquals(0, row.findStartOfColumn(1));
		assertEquals(1, row.findStartOfColumn(2));
		assertEquals(1, row.findStartOfColumn(3));
		assertEquals(2, row.findStartOfColumn(4));
		assertEquals(ONE_JAVA_CHAR_DISPLAY_WIDTH_TWO_1, row.mText[0]);
		assertEquals(ONE_JAVA_CHAR_DISPLAY_WIDTH_TWO_2, row.mText[1]);
		assertEquals('a', row.mText[2]);
		assertEquals(' ', row.mText[3]);

		row.setChar(1, ONE_JAVA_CHAR_DISPLAY_WIDTH_TWO_2, 0);
		// " AA a    "
		assertEquals(' ', row.mText[0]);
		assertEquals(ONE_JAVA_CHAR_DISPLAY_WIDTH_TWO_2, row.mText[1]);
		assertEquals(' ', row.mText[2]);
		assertEquals('a', row.mText[3]);
		assertEquals(' ', row.mText[4]);
		assertEquals(0, row.findStartOfColumn(0));
		assertEquals(1, row.findStartOfColumn(1));
		assertEquals(1, row.findStartOfColumn(2));
		assertEquals(2, row.findStartOfColumn(3));
		assertEquals(3, row.findStartOfColumn(4));
	}

	// https://github.com/jackpal/Android-Terminal-Emulator/issues/145
	public void testCrashATE145() {
		// 0xC2541 is unassigned, use display width 1 for UNICODE_REPLACEMENT_CHAR.
		// assertEquals(1, WcWidth.width(0xC2541));
		assertEquals(2, Character.charCount(0xC2541));

		assertEquals(2, WcWidth.width(0x73EE));
		assertEquals(1, Character.charCount(0x73EE));

		assertEquals(0, WcWidth.width(0x009F));
		assertEquals(1, Character.charCount(0x009F));

		int[] points = new int[]{0xC2541, 'a', '8', 0x73EE, 0x009F, 0x881F, 0x8324, 0xD4C9, 0xFFFD, 'B', 0x009B, 0x61C9, 'Z'};
		// int[] expected = new int[] { TerminalEmulator.UNICODE_REPLACEMENT_CHAR, 'a', '8', 0x73EE, 0x009F, 0x881F, 0x8324, 0xD4C9, 0xFFFD,
		// 'B', 0x009B, 0x61C9, 'Z' };
		int currentColumn = 0;
		for (int point : points) {
			row.setChar(currentColumn, point, 0);
			currentColumn += WcWidth.width(point);
		}
		// assertLineStartsWith(points);
		// assertEquals(Character.highSurrogate(0xC2541), line.mText[0]);
		// assertEquals(Character.lowSurrogate(0xC2541), line.mText[1]);
		// assertEquals('a', line.mText[2]);
		// assertEquals('8', line.mText[3]);
		// assertEquals(Character.highSurrogate(0x73EE), line.mText[4]);
		// assertEquals(Character.lowSurrogate(0x73EE), line.mText[5]);
		//
		// char[] chars = line.mText;
		// int charIndex = 0;
		// for (int i = 0; i < points.length; i++) {
		// char c = chars[charIndex];
		// charIndex++;
		// int thisPoint = (int) c;
		// if (Character.isHighSurrogate(c)) {
		// thisPoint = Character.toCodePoint(c, chars[charIndex]);
		// charIndex++;
		// }
		// assertEquals("At index=" + i + ", charIndex=" + charIndex + ", char=" + (char) thisPoint, points[i], thisPoint);
		// }
	}

	public void testNormalization() {
		// int lowerCaseN = 0x006E;
		// int combiningTilde = 0x0303;
		// int combined = 0x00F1;
		row.setChar(0, 0x006E, 0);
		assertEquals(80, row.getSpaceUsed());
		row.setChar(0, 0x0303, 0);
		assertEquals(81, row.getSpaceUsed());
		// assertEquals("\u00F1  ", new String(term.getScreen().getLine(0)));
		assertLineStartsWith(0x006E, 0x0303, ' ');
	}

	public void testInsertWideAtLastColumn() {
		row.setChar(COLUMNS - 2, 'Z', 0);
		row.setChar(COLUMNS - 1, 'a', 0);
		assertEquals('Z', row.mText[row.findStartOfColumn(COLUMNS - 2)]);
		assertEquals('a', row.mText[row.findStartOfColumn(COLUMNS - 1)]);
		row.setChar(COLUMNS - 1, 'ö', 0);
		assertEquals('Z', row.mText[row.findStartOfColumn(COLUMNS - 2)]);
		assertEquals('ö', row.mText[row.findStartOfColumn(COLUMNS - 1)]);
		// line.setChar(COLUMNS - 1, ONE_JAVA_CHAR_DISPLAY_WIDTH_TWO_1);
		// assertEquals('Z', line.mText[line.findStartOfColumn(COLUMNS - 2)]);
		// assertEquals(' ', line.mText[line.findStartOfColumn(COLUMNS - 1)]);
	}

}
