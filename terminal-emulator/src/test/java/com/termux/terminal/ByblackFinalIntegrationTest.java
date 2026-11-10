package com.termux.terminal;

import org.junit.Test;
import java.io.ByteArrayOutputStream;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.concurrent.*;

import static org.junit.Assert.*;

public class ByblackFinalIntegrationTest {
    private static byte[] readExactly(ByteQueue queue, int length) {
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        byte[] buffer = new byte[Math.min(4096, length)];
        while (out.size() < length) {
            int n = queue.read(buffer, true);
            if (n <= 0) throw new AssertionError("Unexpected queue closure");
            out.write(buffer, 0, n);
        }
        return out.toByteArray();
    }

    private static void close(ByblackUnifiedInput input) throws Exception {
        input.close();
        assertTrue("Input worker did not stop", input.awaitTermination(2000));
        assertEquals(0, input.pendingBytes());
    }

    private static void assertPaste(String text, boolean bracketed, String expected) throws Exception {
        ByteQueue queue = new ByteQueue(4096);
        ByblackUnifiedInput input = new ByblackUnifiedInput(queue);
        ExecutorService consumer = Executors.newSingleThreadExecutor();
        try {
            byte[] bytes = expected.getBytes(StandardCharsets.UTF_8);
            Future<byte[]> result = consumer.submit(() -> readExactly(queue, bytes.length));
            assertEquals(ByblackUnifiedInput.Result.ACCEPTED, input.offerPaste(text, bracketed));
            assertArrayEquals(bytes, result.get(5, TimeUnit.SECONDS));
        } finally {
            close(input);
            consumer.shutdownNow();
        }
    }

    @Test(timeout = 30000)
    public void largePasteThenKeyboardAndBracket() throws Exception {
        ByteQueue queue = new ByteQueue(4096);
        ByblackUnifiedInput input = new ByblackUnifiedInput(queue);
        char[] chars = new char[1024 * 1024];
        Arrays.fill(chars, 'P');
        String payload = new String(chars);
        byte[] expected = ("\033[200~" + payload + "\033[201~K").getBytes(StandardCharsets.UTF_8);
        ExecutorService consumer = Executors.newSingleThreadExecutor();
        try {
            Future<byte[]> result = consumer.submit(() -> readExactly(queue, expected.length));
            assertEquals(ByblackUnifiedInput.Result.ACCEPTED, input.offerPaste(payload, true));
            assertEquals(ByblackUnifiedInput.Result.ACCEPTED, input.offer(new byte[]{'K'}, 0, 1));
            assertArrayEquals(expected, result.get(20, TimeUnit.SECONDS));
        } finally {
            close(input);
            consumer.shutdownNow();
        }
    }

    @Test(timeout = 10000)
    public void sanitizeAllControlsAndNewlines() throws Exception {
        StringBuilder controls = new StringBuilder("A\033");
        for (char c = 0x80; c <= 0x9f; c++) controls.append(c);
        controls.append("B\r\nC\nD\rE Türkçe 😀");
        assertPaste(controls.toString(), false, "AB\rC\rD\rE Türkçe 😀");
    }

    @Test(timeout = 10000)
    public void bracketedSanitizedPaste() throws Exception {
        assertPaste("A\033B\n", true, "\033[200~AB\r\033[201~");
    }

    @Test(timeout = 10000)
    public void sanitizedEmptyPasteStillTerminatesBracket() throws Exception {
        assertPaste("\033\u0080", true, "\033[200~\033[201~");
    }

    @Test(timeout = 10000)
    public void pasteLeavesRoomForKeyboardWithoutBlockingProducer() throws Exception {
        ByteQueue queue = new ByteQueue(1);
        ByblackUnifiedInput input = new ByblackUnifiedInput(queue);
        try {
            char[] chars = new char[(ByblackUnifiedInput.MAX_PENDING - ByblackUnifiedInput.INPUT_RESERVE) / 2 - 32];
            Arrays.fill(chars, 'P');
            assertEquals(ByblackUnifiedInput.Result.ACCEPTED, input.offerPaste(new String(chars), true));
            assertEquals(ByblackUnifiedInput.Result.FULL, input.offerPaste("another paste", false));
            assertEquals(ByblackUnifiedInput.Result.ACCEPTED, input.offer(new byte[]{'K'}, 0, 1));
            assertTrue(input.pendingBytes() <= ByblackUnifiedInput.MAX_PENDING);
        } finally {
            close(input);
        }
    }

    @Test(timeout = 10000)
    public void rejectedPasteDoesNotSendPartialData() throws Exception {
        ByteQueue queue = new ByteQueue(16);
        ByblackUnifiedInput input = new ByblackUnifiedInput(queue);
        try {
            String oversized = new String(new char[ByblackUnifiedInput.MAX_PENDING / 2]);
            assertEquals(ByblackUnifiedInput.Result.FULL, input.offerPaste(oversized, true));
            assertEquals(ByblackUnifiedInput.Result.ACCEPTED, input.offer(new byte[]{'K'}, 0, 1));
            assertArrayEquals(new byte[]{'K'}, readExactly(queue, 1));
            assertEquals(0, queue.read(new byte[16], false));
        } finally {
            close(input);
        }
    }

    @Test(timeout = 10000)
    public void byteInputIsCopiedAndPreservesOffset() throws Exception {
        ByteQueue queue = new ByteQueue(1);
        ByblackUnifiedInput input = new ByblackUnifiedInput(queue);
        try {
            byte[] source = {0, 1, 2, 3, 4};
            assertEquals(ByblackUnifiedInput.Result.ACCEPTED, input.offer(source, 1, 3));
            Arrays.fill(source, (byte) 9);
            assertArrayEquals(new byte[]{1, 2, 3}, readExactly(queue, 3));
        } finally {
            close(input);
        }
    }

    @Test(timeout = 10000)
    public void tinyJobsAreBoundedAndCloseReleasesBlockedWriter() throws Exception {
        ByteQueue queue = new ByteQueue(1);
        ByblackUnifiedInput input = new ByblackUnifiedInput(queue);
        try {
            int accepted = 0;
            while (input.offer(new byte[]{1}, 0, 1) == ByblackUnifiedInput.Result.ACCEPTED) {
                assertTrue(++accepted < ByblackUnifiedInput.MAX_PENDING / 2);
            }
            assertTrue(accepted > 0);
            assertTrue(input.pendingBytes() <= ByblackUnifiedInput.MAX_PENDING);
        } finally {
            close(input);
        }
        assertEquals(ByblackUnifiedInput.Result.CLOSED, input.offer(new byte[]{1}, 0, 1));
        assertEquals(ByblackUnifiedInput.Result.CLOSED, input.offerPaste("x", false));
        close(input);
    }

    @Test(timeout = 10000)
    public void downstreamClosureStopsWorkerAndRejectsLaterInput() throws Exception {
        ByteQueue queue = new ByteQueue(1);
        ByblackUnifiedInput input = new ByblackUnifiedInput(queue);
        try {
            assertEquals(ByblackUnifiedInput.Result.ACCEPTED, input.offer(new byte[8192], 0, 8192));
            queue.close();
            assertTrue(input.awaitTermination(2000));
            assertEquals(ByblackUnifiedInput.Result.CLOSED, input.offerPaste("x", false));
            assertEquals(0, input.pendingBytes());
        } finally {
            close(input);
        }
    }

    @Test(timeout = 10000)
    public void emptyAndInvalidInput() throws Exception {
        ByteQueue queue = new ByteQueue(1);
        ByblackUnifiedInput input = new ByblackUnifiedInput(queue);
        try {
            assertEquals(ByblackUnifiedInput.Result.ACCEPTED, input.offer(new byte[0], 0, 0));
            assertEquals(ByblackUnifiedInput.Result.ACCEPTED, input.offerPaste(null, false));
            assertEquals(ByblackUnifiedInput.Result.ACCEPTED, input.offerPaste("", false));
            assertEquals(0, input.pendingBytes());
            assertThrows(IllegalArgumentException.class, () -> input.offer(new byte[1], 1, 1));
            assertThrows(IllegalArgumentException.class, () -> input.offer(new byte[1], -1, 1));
            assertThrows(IllegalArgumentException.class, () -> input.offer(new byte[1], 0, -1));
            assertThrows(NullPointerException.class, () -> input.offer(null, 0, 1));
        } finally {
            close(input);
        }
    }
}
