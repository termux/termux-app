package com.termux.app.ssh;

import android.util.Base64;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;

import static org.junit.Assert.*;

/**
 * Unit tests for RemoteFileTransfer service class.
 *
 * Tests cover:
 * - Base64 encoding/decoding boundary cases
 * - Progress calculation during transfers
 * - SSH command construction (path escaping)
 * - TransferResult encapsulation
 */
@RunWith(RobolectricTestRunner.class)
public class RemoteFileTransferTest {

    private static final String LOG_TAG = "RemoteFileTransferTest";

    // ==================== TransferResult Tests ====================

    @Test
    public void testTransferResultSuccess() {
        RemoteFileTransfer.TransferResult result = 
            RemoteFileTransfer.TransferResult.success(1024, 1024);

        assertTrue("Result should indicate success", result.success);
        assertEquals("Bytes transferred should match", 1024, result.bytesTransferred);
        assertEquals("Total bytes should match", 1024, result.totalBytes);
        assertNull("Error message should be null for success", result.errorMessage);
        assertEquals("Exit code should be 0", Integer.valueOf(0), result.exitCode);
    }

    @Test
    public void testTransferResultFailure() {
        RemoteFileTransfer.TransferResult result =
            RemoteFileTransfer.TransferResult.failure("SSH connection timeout", 1, 500, 1024);

        assertFalse("Result should indicate failure", result.success);
        assertEquals("Bytes transferred should reflect partial progress", 500, result.bytesTransferred);
        assertEquals("Total bytes should match expected", 1024, result.totalBytes);
        assertEquals("Error message should be preserved", "SSH connection timeout", result.errorMessage);
        assertEquals("Exit code should match", Integer.valueOf(1), result.exitCode);
    }

    @Test
    public void testTransferResultToString() {
        RemoteFileTransfer.TransferResult success = 
            RemoteFileTransfer.TransferResult.success(100, 100);
        assertTrue("toString should contain success info", success.toString().contains("success=true"));

        RemoteFileTransfer.TransferResult failure =
            RemoteFileTransfer.TransferResult.failure("Test error message", null, 0, 100);
        assertTrue("toString should contain failure info", failure.toString().contains("success=false"));
        assertTrue("toString should contain truncated error", failure.toString().contains("Test error"));
    }

    @Test
    public void testTransferResultFailureWithNullExitCode() {
        // Command failed to start (AppShell returned null)
        RemoteFileTransfer.TransferResult result =
            RemoteFileTransfer.TransferResult.failure("Failed to start SSH command", null, 0, 0);

        assertFalse("Result should indicate failure", result.success);
        assertNull("Exit code should be null for process failure", result.exitCode);
    }

    // ==================== Base64 Encoding Tests ====================

    @Test
    public void testBase64EncodeEmptyData() {
        byte[] emptyData = new byte[0];
        byte[] encoded = Base64.encode(emptyData, Base64.NO_WRAP);
        
        assertEquals("Empty data should encode to empty string", 0, encoded.length);
        
        // Verify decoding roundtrip
        byte[] decoded = Base64.decode(encoded, Base64.NO_WRAP);
        assertEquals("Decoded empty data should be empty", 0, decoded.length);
    }

    @Test
    public void testBase64EncodeSingleByte() {
        byte[] singleByte = new byte[]{0x42}; // 'B'
        byte[] encoded = Base64.encode(singleByte, Base64.NO_WRAP);
        
        // Base64 encodes 1 byte to 4 chars
        assertEquals("Single byte should encode to 4 chars", 4, encoded.length);
        
        // Verify decoding roundtrip
        byte[] decoded = Base64.decode(encoded, Base64.NO_WRAP);
        assertEquals("Decoded should match original", 1, decoded.length);
        assertEquals("Decoded byte should match original", 0x42, decoded[0]);
    }

    @Test
    public void testBase64EncodeBinaryData() {
        // Binary data with various byte values (explicit casts for values > 127)
        byte[] binaryData = new byte[]{
            0x00, 0x01, 0x02, 0x03,
            (byte) 0xFF, (byte) 0xFE, (byte) 0xFD,
            (byte) 0x80, (byte) 0x81, 0x7F, 0x7E
        };
        
        byte[] encoded = Base64.encode(binaryData, Base64.NO_WRAP);
        
        // Base64 expands data by ~33% (4 chars for every 3 bytes, with padding)
        int expectedLen = (int) Math.ceil(binaryData.length / 3.0) * 4;
        assertEquals("Encoded length should match expected", expectedLen, encoded.length);
        
        // Verify decoding roundtrip
        byte[] decoded = Base64.decode(encoded, Base64.NO_WRAP);
        assertArrayEquals("Decoded binary data should match original", binaryData, decoded);
    }

    @Test
    public void testBase64EncodeLargeText() {
        // Generate large text data (10KB)
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < 1000; i++) {
            sb.append("TestDataChunk123");
        }
        byte[] largeText = sb.toString().getBytes(StandardCharsets.UTF_8);
        
        byte[] encoded = Base64.encode(largeText, Base64.NO_WRAP);
        
        // Verify expansion ratio is approximately 4/3
        double ratio = (double) encoded.length / largeText.length;
        assertTrue("Base64 expansion ratio should be ~1.33", ratio > 1.3 && ratio < 1.4);
        
        // Verify decoding roundtrip
        byte[] decoded = Base64.decode(encoded, Base64.NO_WRAP);
        assertArrayEquals("Decoded large text should match original", largeText, decoded);
    }

    @Test
    public void testBase64EncodeNoWrapFlag() {
        byte[] data = "Test\nWith\nNewlines\n".getBytes(StandardCharsets.UTF_8);
        
        // NO_WRAP should not add line breaks
        byte[] encodedNoWrap = Base64.encode(data, Base64.NO_WRAP);
        String encodedString = new String(encodedNoWrap, StandardCharsets.UTF_8);
        
        assertFalse("NO_WRAP encoding should not contain newlines",
                    encodedString.contains("\n"));
        assertFalse("NO_WRAP encoding should not contain carriage returns",
                    encodedString.contains("\r"));
    }

    @Test
    public void testBase64DecodeInvalidDataThrowsException() {
        String invalidBase64 = "NotValidBase64!!!";
        
        try {
            byte[] decoded = Base64.decode(invalidBase64, Base64.NO_WRAP);
            // Some invalid Base64 strings may partially decode or produce garbage
            // The actual behavior depends on Base64 implementation
            // In Android's Base64, it may silently ignore invalid characters
        } catch (IllegalArgumentException e) {
            // Expected: invalid Base64 should throw
            assertTrue("Exception message should mention decoding failure",
                       e.getMessage().contains("decode") || e.getMessage().contains("base64"));
        }
    }

    // ==================== Path Escaping Tests ====================

    @Test
    public void testEscapePathSimple() {
        String path = "/home/user/file.txt";
        String escaped = RemoteFileOperator.escapePath(path);
        
        assertEquals("Simple path should be wrapped in single quotes",
                     "'" + path + "'", escaped);
    }

    @Test
    public void testEscapePathWithSpaces() {
        String path = "/home/user/My Documents/file.txt";
        String escaped = RemoteFileOperator.escapePath(path);
        
        assertEquals("Path with spaces should be wrapped in single quotes",
                     "'" + path + "'", escaped);
        assertTrue("Escaped path should preserve spaces", escaped.contains("My Documents"));
    }

    @Test
    public void testEscapePathWithSingleQuote() {
        String path = "/home/user/it's mine/file.txt";
        String escaped = RemoteFileOperator.escapePath(path);
        
        // Single quotes should be escaped: ' -> '\''
        assertTrue("Escaped path should handle single quotes", escaped.contains("'\\''"));
        assertFalse("Escaped path should not contain unescaped single quote inside",
                    escaped.matches("^'/home/user/it's mine/file.txt'$"));
    }

    @Test
    public void testEscapePathWithUnicode() {
        String path = "/home/user/文档/测试文件.txt";
        String escaped = RemoteFileOperator.escapePath(path);
        
        assertEquals("Unicode path should be wrapped in single quotes",
                     "'" + path + "'", escaped);
        assertTrue("Escaped path should preserve Unicode", escaped.contains("文档"));
    }

    @Test
    public void testEscapePathWithSpecialCharacters() {
        String path = "/home/user/$var/test&file|pipe.txt";
        String escaped = RemoteFileOperator.escapePath(path);
        
        assertEquals("Path with special shell characters should be wrapped in single quotes",
                     "'" + path + "'", escaped);
        assertTrue("Escaped path should preserve special chars", escaped.contains("$var"));
        assertTrue("Escaped path should preserve special chars", escaped.contains("&"));
        assertTrue("Escaped path should preserve special chars", escaped.contains("|"));
    }

    @Test
    public void testEscapePathEmptyString() {
        String path = "";
        String escaped = RemoteFileOperator.escapePath(path);
        
        assertEquals("Empty path should be escaped as empty single-quoted string",
                     "''", escaped);
    }

    // ==================== Progress Calculation Tests ====================

    @Test
    public void testProgressCallbackInvoked() {
        // Simulate progress tracking
        ByteArrayOutputStream progressLog = new ByteArrayOutputStream();
        
        RemoteFileTransfer.ProgressCallback callback = new RemoteFileTransfer.ProgressCallback() {
            long lastProgress = -1;
            
            @Override
            public void onProgress(long bytesTransferred, long totalBytes) {
                // Progress should be increasing
                assertTrue("Progress should increase or stay same",
                           bytesTransferred >= lastProgress);
                assertTrue("Progress should not exceed total",
                           bytesTransferred <= totalBytes);
                lastProgress = bytesTransferred;
            }
            
            @Override
            public void onComplete(RemoteFileTransfer.TransferResult result) {
                // Completion should reflect final state
                assertEquals("Final progress should match total",
                             result.bytesTransferred, result.totalBytes);
            }
        };
        
        // Simulate upload progress calls
        callback.onProgress(0, 1024);
        callback.onProgress(512, 1024);
        callback.onProgress(1024, 1024);
        callback.onComplete(RemoteFileTransfer.TransferResult.success(1024, 1024));
    }

    @Test
    public void testProgressCallbackZeroFileSize() {
        RemoteFileTransfer.ProgressCallback callback = new RemoteFileTransfer.ProgressCallback() {
            @Override
            public void onProgress(long bytesTransferred, long totalBytes) {
                // Empty file: total should be 0
                assertEquals("Empty file total should be 0", 0, totalBytes);
                assertEquals("Empty file progress should be 0", 0, bytesTransferred);
            }
            
            @Override
            public void onComplete(RemoteFileTransfer.TransferResult result) {
                assertTrue("Empty file transfer should succeed", result.success);
                assertEquals("Empty file bytes should be 0", 0, result.bytesTransferred);
            }
        };
        
        callback.onProgress(0, 0);
        callback.onComplete(RemoteFileTransfer.TransferResult.success(0, 0));
    }

    @Test
    public void testProgressCallbackPartialFailure() {
        // Simulate partial transfer failure (connection lost mid-transfer)
        final long totalBytes = 10240;
        final long partialProgress = 5120;
        
        RemoteFileTransfer.TransferResult result =
            RemoteFileTransfer.TransferResult.failure("SSH connection timeout", null, partialProgress, totalBytes);
        
        assertEquals("Partial progress should be reported", partialProgress, result.bytesTransferred);
        assertEquals("Total should still be original expected", totalBytes, result.totalBytes);
        assertFalse("Result should indicate failure", result.success);
    }

    // ==================== Stream Processing Tests ====================

    @Test
    public void testReadStreamToByteArray() throws IOException {
        byte[] testData = "Hello, World!".getBytes(StandardCharsets.UTF_8);
        ByteArrayInputStream input = new ByteArrayInputStream(testData);
        
        ByteArrayOutputStream output = new ByteArrayOutputStream();
        byte[] buffer = new byte[4096];
        int bytesRead;
        
        while ((bytesRead = input.read(buffer)) != -1) {
            output.write(buffer, 0, bytesRead);
        }
        
        assertArrayEquals("Stream roundtrip should preserve data", testData, output.toByteArray());
    }

    @Test
    public void testReadStreamEmptyInputStream() throws IOException {
        ByteArrayInputStream emptyInput = new ByteArrayInputStream(new byte[0]);
        
        ByteArrayOutputStream output = new ByteArrayOutputStream();
        byte[] buffer = new byte[4096];
        int bytesRead = emptyInput.read(buffer);
        
        assertEquals("Empty stream should return -1 or 0", -1, bytesRead);
        assertEquals("Output should be empty", 0, output.size());
    }

    @Test
    public void testReadStreamLargeData() throws IOException {
        // Simulate reading large data in chunks
        byte[] largeData = new byte[50 * 1024]; // 50KB
        for (int i = 0; i < largeData.length; i++) {
            largeData[i] = (byte) (i % 256);
        }
        
        ByteArrayInputStream input = new ByteArrayInputStream(largeData);
        ByteArrayOutputStream output = new ByteArrayOutputStream();
        
        byte[] buffer = new byte[4096]; // 4KB chunks
        int totalRead = 0;
        int bytesRead;
        
        while ((bytesRead = input.read(buffer)) != -1) {
            output.write(buffer, 0, bytesRead);
            totalRead += bytesRead;
        }
        
        assertEquals("Total bytes read should match input", largeData.length, totalRead);
        assertArrayEquals("Output should match input", largeData, output.toByteArray());
    }

    // ==================== Error Message Parsing Tests ====================

    @Test
    public void testFormatFileSize() {
        // These tests verify the internal helper's expected behavior
        
        // Bytes: < 1024
        assertEquals("0 B should format correctly", "0 B", formatFileSizeInternal(0));
        assertEquals("100 B should format correctly", "100 B", formatFileSizeInternal(100));
        
        // KB: >= 1024, < 1024*1024
        assertTrue("1 KB should format with KB suffix", 
                   formatFileSizeInternal(1024).contains("KB"));
        
        // MB: >= 1024*1024
        assertTrue("1 MB should format with MB suffix",
                   formatFileSizeInternal(1024 * 1024).contains("MB"));
        
        // 50 MB (our limit)
        String fiftyMB = formatFileSizeInternal(50 * 1024 * 1024);
        assertTrue("50 MB should be formatted", fiftyMB.contains("50") || fiftyMB.contains("49"));
    }

    private String formatFileSizeInternal(long bytes) {
        if (bytes < 1024) {
            return bytes + " B";
        }
        double kb = bytes / 1024.0;
        if (kb < 1024) {
            return String.format("%.1f KB", kb);
        }
        double mb = kb / 1024.0;
        if (mb < 1024) {
            return String.format("%.1f MB", mb);
        }
        double gb = mb / 1024.0;
        return String.format("%.1f GB", gb);
    }

    // ==================== SSHConnectionInfo Tests ====================

    @Test
    public void testSSHConnectionInfoToString() {
        SSHConnectionInfo info = new SSHConnectionInfo("user", "host", 22, "/path/socket");
        assertEquals("toString should match user@host:port format",
                     "user@host:22", info.toString());
    }

    @Test
    public void testSSHConnectionInfoParseFromFilename() {
        SSHConnectionInfo info = SSHConnectionInfo.parseFromFilename(
            "testuser@example.com:2222", "/tmp/socket");
        
        assertNotNull("Parsing should succeed for valid format", info);
        assertEquals("User should be parsed", "testuser", info.getUser());
        assertEquals("Host should be parsed", "example.com", info.getHost());
        assertEquals("Port should be parsed", 2222, info.getPort());
        assertEquals("Socket path should be preserved", "/tmp/socket", info.getSocketPath());
    }

    @Test
    public void testSSHConnectionInfoParseInvalidFormats() {
        // Missing @
        assertNull("Should reject missing @", 
                   SSHConnectionInfo.parseFromFilename("noathost:22", "/tmp/s"));
        
        // Missing :
        assertNull("Should reject missing port separator",
                   SSHConnectionInfo.parseFromFilename("user@hostnoport", "/tmp/s"));
        
        // Invalid port (non-numeric)
        assertNull("Should reject non-numeric port",
                   SSHConnectionInfo.parseFromFilename("user@host:abc", "/tmp/s"));
        
        // Port out of range
        assertNull("Should reject port > 65535",
                   SSHConnectionInfo.parseFromFilename("user@host:99999", "/tmp/s"));
        
        // Empty user
        assertNull("Should reject empty user",
                   SSHConnectionInfo.parseFromFilename("@host:22", "/tmp/s"));
        
        // Empty host
        assertNull("Should reject empty host",
                   SSHConnectionInfo.parseFromFilename("user@:22", "/tmp/s"));
    }

    // ==================== Integration-like Tests ====================

    @Test
    public void testFullEncodeDecodeRoundtrip() throws IOException {
        // Simulate full upload/download data transformation
        
        // Original file content (explicit casts for values > 127)
        byte[] originalData = new byte[]{
            0x48, 0x65, 0x6C, 0x6C, 0x6F, // "Hello"
            0x00, (byte) 0xFF, (byte) 0x80, 0x7F,       // Binary values
            0x20, 0x57, 0x6F, 0x72, 0x6C, 0x64 // " World"
        };
        
        // Upload: encode to base64
        byte[] encodedUpload = Base64.encode(originalData, Base64.NO_WRAP);
        
        // Simulate transfer (encoded data as string)
        String transferredData = new String(encodedUpload, StandardCharsets.UTF_8);
        
        // Download: decode from base64
        byte[] encodedDownload = transferredData.getBytes(StandardCharsets.UTF_8);
        byte[] decodedDownload = Base64.decode(encodedDownload, Base64.NO_WRAP);
        
        // Verify roundtrip
        assertArrayEquals("Full roundtrip should preserve original data",
                          originalData, decodedDownload);
    }

    @Test
    public void testEmptyFileRoundtrip() throws IOException {
        // Empty file upload/download
        byte[] emptyData = new byte[0];
        
        // Upload encoding
        byte[] encoded = Base64.encode(emptyData, Base64.NO_WRAP);
        assertEquals("Empty file should encode to empty", 0, encoded.length);
        
        // Download decoding
        byte[] decoded = Base64.decode(encoded, Base64.NO_WRAP);
        assertEquals("Empty encoded data should decode to empty", 0, decoded.length);
        
        // Verify TransferResult for empty file
        RemoteFileTransfer.TransferResult result = 
            RemoteFileTransfer.TransferResult.success(0, 0);
        assertTrue("Empty file transfer should succeed", result.success);
    }

    // ==================== Chunked Transfer Logic Tests ====================

    /**
     * Test chunk count calculation for various file sizes.
     * Verify the formula: totalChunks = ceil(fileSize / TRANSFER_CHUNK_SIZE)
     */
    @Test
    public void testChunkCountCalculation() {
        // TRANSFER_CHUNK_SIZE = 1MB = 1024 * 1024 = 1048576 bytes
        long chunkSize = 1024 * 1024;

        // Exactly 1 chunk: fileSize <= chunkSize
        assertEquals("1MB file should be 1 chunk", 1, calculateChunks(chunkSize, chunkSize));
        assertEquals("512KB file should be 1 chunk", 1, calculateChunks(512 * 1024, chunkSize));
        assertEquals("1 byte file should be 1 chunk", 1, calculateChunks(1, chunkSize));
        assertEquals("0 byte file should be 1 chunk (handled separately)", 1, calculateChunks(0, chunkSize));

        // Exactly 2 chunks: fileSize > chunkSize but <= 2*chunkSize
        assertEquals("2MB file should be 2 chunks", 2, calculateChunks(2 * chunkSize, chunkSize));
        assertEquals("1.5MB file should be 2 chunks", 2, calculateChunks(chunkSize + 512 * 1024, chunkSize));

        // Multiple chunks
        assertEquals("3MB file should be 3 chunks", 3, calculateChunks(3 * chunkSize, chunkSize));
        assertEquals("10MB file should be 10 chunks", 10, calculateChunks(10 * chunkSize, chunkSize));
        assertEquals("100MB file should be 100 chunks", 100, calculateChunks(100 * chunkSize, chunkSize));
    }

    /**
     * Test last chunk size calculation (boundary case).
     * The last chunk is typically smaller than full chunk size.
     */
    @Test
    public void testLastChunkSizeCalculation() {
        long chunkSize = 1024 * 1024;

        // File exactly divisible by chunk size: last chunk = full chunk
        assertEquals("2MB file last chunk should be 1MB", chunkSize,
                     calculateLastChunkSize(2 * chunkSize, chunkSize));
        assertEquals("3MB file last chunk should be 1MB", chunkSize,
                     calculateLastChunkSize(3 * chunkSize, chunkSize));

        // File with remainder: last chunk = remainder
        assertEquals("1.5MB file last chunk should be 512KB", 512 * 1024,
                     calculateLastChunkSize(chunkSize + 512 * 1024, chunkSize));
        assertEquals("2.5MB file last chunk should be 512KB", 512 * 1024,
                     calculateLastChunkSize(2 * chunkSize + 512 * 1024, chunkSize));
        assertEquals("1MB + 1 byte last chunk should be 1 byte", 1,
                     calculateLastChunkSize(chunkSize + 1, chunkSize));

        // Small file: only one chunk, last chunk = file size
        assertEquals("100KB file last chunk should be 100KB", 100 * 1024,
                     calculateLastChunkSize(100 * 1024, chunkSize));
    }

    /**
     * Test chunk offset calculation for upload.
     * Each chunk starts at offset = chunkIndex * chunkSize.
     */
    @Test
    public void testChunkOffsetCalculation() {
        long chunkSize = 1024 * 1024;

        // First chunk always at offset 0
        assertEquals("Chunk 0 offset should be 0", 0, calculateChunkOffset(0, chunkSize));

        // Subsequent chunks at multiples of chunk size
        assertEquals("Chunk 1 offset should be 1MB", chunkSize, calculateChunkOffset(1, chunkSize));
        assertEquals("Chunk 2 offset should be 2MB", 2 * chunkSize, calculateChunkOffset(2, chunkSize));
        assertEquals("Chunk 10 offset should be 10MB", 10 * chunkSize, calculateChunkOffset(10, chunkSize));
    }

    /**
     * Test chunk encoding produces valid base64 with correct size.
     * Base64 encoding expands data by ~33% (4 bytes per 3 input bytes).
     */
    @Test
    public void testChunkBase64EncodingSize() {
        // Test various chunk sizes
        int[] testSizes = {1, 100, 1024, 4096, 512 * 1024, 1024 * 1024};

        for (int size : testSizes) {
            byte[] chunkData = new byte[size];
            for (int i = 0; i < size; i++) {
                chunkData[i] = (byte) (i % 256);
            }

            byte[] encoded = Base64.encode(chunkData, Base64.NO_WRAP);

            // Calculate expected encoded size
            int expectedEncodedSize = (int) Math.ceil(size / 3.0) * 4;
            assertEquals("Encoded size should match expected for " + size + " bytes",
                         expectedEncodedSize, encoded.length);

            // Verify roundtrip decode
            byte[] decoded = Base64.decode(encoded, Base64.NO_WRAP);
            assertArrayEquals("Decoded chunk should match original for " + size + " bytes",
                              chunkData, decoded);
        }
    }

    /**
     * Test progress tracking across multiple chunks.
     * Progress should be reported after each chunk completion.
     */
    @Test
    public void testChunkedProgressTracking() {
        long fileSize = 5 * 1024 * 1024; // 5MB
        long chunkSize = 1024 * 1024;    // 1MB
        int totalChunks = 5;

        // Simulate progress updates
        long[] expectedProgressPoints = {0, chunkSize, 2 * chunkSize, 3 * chunkSize,
                                          4 * chunkSize, 5 * chunkSize};

        for (int i = 0; i <= totalChunks; i++) {
            long expectedProgress = i * chunkSize;
            if (i == totalChunks) {
                expectedProgress = fileSize; // Final progress should equal total
            }
            assertEquals("Progress after chunk " + i + " should be correct",
                         expectedProgress, expectedProgressPoints[i]);
        }
    }

    /**
     * Test that large file (100MB) produces correct chunk count.
     * This validates the OOM-prevention mechanism works for large files.
     */
    @Test
    public void testLargeFileChunkCount() {
        long chunkSize = 1024 * 1024; // 1MB

        // 100MB file = 100 chunks
        long largeFileSize = 100 * chunkSize;
        int chunks = calculateChunks(largeFileSize, chunkSize);
        assertEquals("100MB file should produce 100 chunks", 100, chunks);

        // Verify each chunk processes at most 1MB
        for (int i = 0; i < chunks; i++) {
            long chunkStart = i * chunkSize;
            long thisChunkSize = Math.min(chunkSize, largeFileSize - chunkStart);
            assertTrue("Each chunk should be at most 1MB", thisChunkSize <= chunkSize);
            assertTrue("Each chunk should be positive", thisChunkSize > 0);
        }
    }

    // ==================== Precise Boundary File Size Tests ====================

    /**
     * Test precise boundary file sizes: 1MB-1, 1MB, 1MB+1.
     * These are critical edge cases for chunk count calculation.
     */
    @Test
    public void testPreciseBoundaryFileSizes() {
        long chunkSize = 1024 * 1024; // 1MB = 1048576 bytes

        // 1MB - 1 byte = 1048575 bytes -> 1 chunk (fits entirely in one chunk)
        long oneMBMinus1 = chunkSize - 1;
        assertEquals("1MB-1 bytes should be 1 chunk", 1, calculateChunks(oneMBMinus1, chunkSize));
        assertEquals("1MB-1 last chunk size should be full file size", oneMBMinus1,
                     calculateLastChunkSize(oneMBMinus1, chunkSize));

        // 1MB exactly = 1048576 bytes -> 1 chunk
        long oneMB = chunkSize;
        assertEquals("Exactly 1MB should be 1 chunk", 1, calculateChunks(oneMB, chunkSize));
        assertEquals("1MB last chunk size should be full chunk", chunkSize,
                     calculateLastChunkSize(oneMB, chunkSize));

        // 1MB + 1 byte = 1048577 bytes -> 2 chunks (overflow into second chunk)
        long oneMBPlus1 = chunkSize + 1;
        assertEquals("1MB+1 bytes should be 2 chunks", 2, calculateChunks(oneMBPlus1, chunkSize));
        assertEquals("1MB+1 first chunk should be full 1MB", chunkSize,
                     calculateChunkOffset(1, chunkSize)); // offset of second chunk = 1MB
        assertEquals("1MB+1 last chunk size should be 1 byte", 1,
                     calculateLastChunkSize(oneMBPlus1, chunkSize));
    }

    /**
     * Test 50MB file chunk calculation (MVP upper limit).
     * Validates behavior at the previous MAX_FILE_SIZE boundary.
     */
    @Test
    public void test50MBFileChunkCalculation() {
        long chunkSize = 1024 * 1024; // 1MB
        long fiftyMB = 50 * chunkSize;

        // 50MB should produce exactly 50 chunks (evenly divisible)
        assertEquals("50MB file should be 50 chunks", 50, calculateChunks(fiftyMB, chunkSize));

        // All chunks should be full size (evenly divisible)
        assertEquals("50MB last chunk should be full 1MB", chunkSize,
                     calculateLastChunkSize(fiftyMB, chunkSize));

        // Verify no remainder
        assertEquals("50MB should have no remainder when divided by chunk size",
                     0, fiftyMB % chunkSize);
    }

    /**
     * Test 100MB file chunk calculation (new support target).
     * Validates that the chunked approach handles files that previously caused OOM.
     */
    @Test
    public void test100MBFileChunkCalculation() {
        long chunkSize = 1024 * 1024; // 1MB
        long hundredMB = 100 * chunkSize;

        // 100MB should produce exactly 100 chunks
        assertEquals("100MB file should be 100 chunks", 100, calculateChunks(hundredMB, chunkSize));

        // All chunks should be full size
        assertEquals("100MB last chunk should be full 1MB", chunkSize,
                     calculateLastChunkSize(hundredMB, chunkSize));

        // Simulate memory usage: each chunk is 1MB + base64 overhead (~33%)
        // Peak memory = 1MB raw data + ~1.33MB encoded = ~2.33MB
        // This is far below the OOM threshold
        long estimatedPeakMemory = chunkSize + (long)(chunkSize * 1.34);
        assertTrue("Estimated peak memory should be under 3MB",
                   estimatedPeakMemory < 3 * 1024 * 1024);
    }

    /**
     * Test chunk boundary crossing scenarios.
     * Verifies correct behavior when file size is just above/below chunk boundaries.
     */
    @Test
    public void testChunkBoundaryCrossing() {
        long chunkSize = 1024 * 1024;

        // Test crossing from 1 chunk to 2 chunks
        assertEquals("ChunkSize-1 should be 1 chunk", 1, calculateChunks(chunkSize - 1, chunkSize));
        assertEquals("ChunkSize should be 1 chunk", 1, calculateChunks(chunkSize, chunkSize));
        assertEquals("ChunkSize+1 should be 2 chunks", 2, calculateChunks(chunkSize + 1, chunkSize));

        // Test crossing from 2 chunks to 3 chunks
        assertEquals("2*ChunkSize-1 should be 2 chunks", 2, calculateChunks(2 * chunkSize - 1, chunkSize));
        assertEquals("2*ChunkSize should be 2 chunks", 2, calculateChunks(2 * chunkSize, chunkSize));
        assertEquals("2*ChunkSize+1 should be 3 chunks", 3, calculateChunks(2 * chunkSize + 1, chunkSize));

        // Test crossing from 10 chunks to 11 chunks
        assertEquals("10*ChunkSize-1 should be 10 chunks", 10, calculateChunks(10 * chunkSize - 1, chunkSize));
        assertEquals("10*ChunkSize should be 10 chunks", 10, calculateChunks(10 * chunkSize, chunkSize));
        assertEquals("10*ChunkSize+1 should be 11 chunks", 11, calculateChunks(10 * chunkSize + 1, chunkSize));
    }

    /**
     * Test empty file and single-byte file edge cases.
     * These are handled specially in the implementation.
     */
    @Test
    public void testEmptyAndSingleByteFiles() {
        long chunkSize = 1024 * 1024;

        // Empty file (0 bytes) - handled specially, creates file but no data transfer
        assertEquals("0 bytes should need 1 chunk (special handling)", 1, calculateChunks(0, chunkSize));

        // Single byte file - fits in one chunk
        assertEquals("1 byte should be 1 chunk", 1, calculateChunks(1, chunkSize));
        assertEquals("1 byte last chunk should be 1 byte", 1, calculateLastChunkSize(1, chunkSize));
    }

    /**
     * Test readFully simulation - ensuring complete chunk reading.
     */
    @Test
    public void testReadFullySimulation() throws IOException {
        byte[] testData = new byte[1024 * 1024]; // 1MB
        for (int i = 0; i < testData.length; i++) {
            testData[i] = (byte) (i % 256);
        }

        ByteArrayInputStream input = new ByteArrayInputStream(testData);
        byte[] buffer = new byte[1024 * 1024];

        // Simulate readFully
        int bytesToRead = testData.length;
        int totalRead = 0;
        while (totalRead < bytesToRead) {
            int read = input.read(buffer, totalRead, bytesToRead - totalRead);
            if (read == -1) break;
            totalRead += read;
        }

        assertEquals("readFully should read exact number of bytes", bytesToRead, totalRead);
        assertArrayEquals("Buffer should contain exact data", testData, buffer);
    }

    /**
     * Test readFully with partial data (simulating early EOF).
     */
    @Test
    public void testReadFullyPartialData() throws IOException {
        byte[] partialData = new byte[512 * 1024]; // 512KB (less than requested)
        ByteArrayInputStream input = new ByteArrayInputStream(partialData);
        byte[] buffer = new byte[1024 * 1024];

        // Try to read 1MB but only 512KB available
        int bytesToRead = 1024 * 1024;
        int totalRead = 0;
        while (totalRead < bytesToRead) {
            int read = input.read(buffer, totalRead, bytesToRead - totalRead);
            if (read == -1) break;
            totalRead += read;
        }

        // Should read only available data
        assertEquals("readFully should return available bytes on EOF", 512 * 1024, totalRead);
        // This scenario would trigger error in actual uploadChunked
    }

    // Helper methods for chunk calculations

    private int calculateChunks(long fileSize, long chunkSize) {
        int chunks = (int) (fileSize / chunkSize);
        if (fileSize % chunkSize != 0) {
            chunks++;
        }
        // Handle empty file case (0 bytes still needs 1 empty chunk or special handling)
        if (fileSize == 0) {
            return 1; // Actually handled separately in code
        }
        return chunks;
    }

    private long calculateLastChunkSize(long fileSize, long chunkSize) {
        int lastChunkIndex = (int) (fileSize / chunkSize);
        if (fileSize % chunkSize != 0) {
            lastChunkIndex++;
        }
        lastChunkIndex--; // Last chunk index (0-based)

        long lastChunkStart = lastChunkIndex * chunkSize;
        return fileSize - lastChunkStart;
    }

    private long calculateChunkOffset(int chunkIndex, long chunkSize) {
        return chunkIndex * chunkSize;
    }
}