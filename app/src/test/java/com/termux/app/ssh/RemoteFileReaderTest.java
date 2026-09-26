package com.termux.app.ssh;

import org.junit.Test;
import org.junit.Before;

import static org.junit.Assert.*;

/**
 * Unit tests for RemoteFileReader path escaping and result parsing.
 */
public class RemoteFileReaderTest {

    @Test
    public void testEscapePathSimple() {
        // Simple path without special characters
        String path = "/home/user/file.txt";
        String escaped = RemoteFileReader.escapePathForSSH(path);
        assertEquals("'/home/user/file.txt'", escaped);
    }

    @Test
    public void testEscapePathWithSpaces() {
        // Path with spaces
        String path = "/home/user/my file.txt";
        String escaped = RemoteFileReader.escapePathForSSH(path);
        assertEquals("'/home/user/my file.txt'", escaped);
    }

    @Test
    public void testEscapePathWithSingleQuote() {
        // Path containing a single quote - most critical test
        // The escape mechanism: 'path' with ' inside becomes 'path'\''path'
        String path = "/home/user/it's mine.txt";
        String escaped = RemoteFileReader.escapePathForSSH(path);
        // Expected: '/home/user/it'\''s mine.txt'
        assertEquals("'/home/user/it'\\''s mine.txt'", escaped);
    }

    @Test
    public void testEscapePathWithMultipleSingleQuotes() {
        // Path with multiple single quotes
        String path = "/home/user/'quoted'/file.txt";
        String escaped = RemoteFileReader.escapePathForSSH(path);
        // Expected: '/home/user/'\''quoted'\''/file.txt'
        assertEquals("'/home/user/'\\''quoted'\\''/file.txt'", escaped);
    }

    @Test
    public void testEscapePathWithSpecialChars() {
        // Path with various special characters
        String path = "/home/user/$var/file&name#.txt";
        String escaped = RemoteFileReader.escapePathForSSH(path);
        // All special chars are safe inside single quotes
        assertEquals("'/home/user/$var/file&name#.txt'", escaped);
    }

    @Test
    public void testEscapePathWithBackslash() {
        // Backslash inside single quotes is literal
        String path = "/home/user/backup\\file.txt";
        String escaped = RemoteFileReader.escapePathForSSH(path);
        assertEquals("'/home/user/backup\\file.txt'", escaped);
    }

    @Test
    public void testEscapePathWithDoubleQuotes() {
        // Double quotes inside single quotes are literal
        String path = "/home/user/\"quoted\".txt";
        String escaped = RemoteFileReader.escapePathForSSH(path);
        assertEquals("'/home/user/\"quoted\".txt'", escaped);
    }

    @Test
    public void testEscapePathWithNewline() {
        // Newline characters in path (rare but possible)
        String path = "/home/user/file\nname.txt";
        String escaped = RemoteFileReader.escapePathForSSH(path);
        assertEquals("'/home/user/file\nname.txt'", escaped);
    }

    @Test
    public void testEscapePathEmpty() {
        // Empty path
        String path = "";
        String escaped = RemoteFileReader.escapePathForSSH(path);
        assertEquals("''", escaped);
    }

    @Test
    public void testEscapePathRoot() {
        // Root directory
        String path = "/";
        String escaped = RemoteFileReader.escapePathForSSH(path);
        assertEquals("'/'", escaped);
    }

    @Test
    public void testReadResultSuccess() {
        RemoteFileReader.ReadResult result = 
            new RemoteFileReader.ReadResult(0, "Hello World", null);
        
        assertTrue("Should indicate success", result.isSuccess());
        assertEquals("Exit code should be 0", 0, result.getExitCode());
        assertEquals("Content should match", "Hello World", result.getContent());
        assertNull("Error message should be null on success", result.getErrorMessage());
    }

    @Test
    public void testReadResultError() {
        RemoteFileReader.ReadResult result = 
            new RemoteFileReader.ReadResult(1, null, "No such file or directory");
        
        assertFalse("Should indicate failure", result.isSuccess());
        assertEquals("Exit code should be 1", 1, result.getExitCode());
        assertNull("Content should be null on error", result.getContent());
        assertEquals("Error message should match", "No such file or directory", result.getErrorMessage());
    }

    @Test
    public void testReadResultEmptyFile() {
        // Empty file returns success with empty content
        RemoteFileReader.ReadResult result = 
            new RemoteFileReader.ReadResult(0, "", null);
        
        assertTrue("Empty file should be success", result.isSuccess());
        assertEquals("Exit code should be 0", 0, result.getExitCode());
        assertEquals("Content should be empty string", "", result.getContent());
        assertNull("Error message should be null", result.getErrorMessage());
    }

    @Test
    public void testReadResultNullContent() {
        // Null content (error case)
        RemoteFileReader.ReadResult result = 
            new RemoteFileReader.ReadResult(2, null, "Permission denied");
        
        assertFalse("Should indicate failure", result.isSuccess());
        assertNull("Content should be null", result.getContent());
        assertEquals("Error should match", "Permission denied", result.getErrorMessage());
    }

    @Test
    public void testReadResultToString() {
        RemoteFileReader.ReadResult successResult = 
            new RemoteFileReader.ReadResult(0, "content", null);
        assertTrue("toString should contain 'success'", 
            successResult.toString().contains("success"));
        assertTrue("toString should contain contentLength", 
            successResult.toString().contains("contentLength=7"));
        
        RemoteFileReader.ReadResult errorResult = 
            new RemoteFileReader.ReadResult(1, null, "error message");
        assertTrue("toString should contain 'error'", 
            errorResult.toString().contains("error"));
        assertTrue("toString should contain exitCode", 
            errorResult.toString().contains("exitCode=1"));
    }

    @Test
    public void testTruncateForLogShort() {
        String shortStr = "short string";
        String truncated = RemoteFileReader.truncateForLog(shortStr);
        assertEquals("Short strings should not be truncated", shortStr, truncated);
    }

    @Test
    public void testTruncateForLogLong() {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < 300; i++) {
            sb.append('x');
        }
        String longStr = sb.toString();
        String truncated = RemoteFileReader.truncateForLog(longStr);
        
        assertTrue("Truncated should be shorter", truncated.length() < longStr.length());
        assertTrue("Should end with ...", truncated.endsWith("..."));
        assertEquals("Should be exactly 203 chars (200 + 3)", 203, truncated.length());
    }

    @Test
    public void testTruncateForLogNull() {
        String truncated = RemoteFileReader.truncateForLog(null);
        assertEquals("(null)", truncated);
    }

    @Test
    public void testTruncateForLogExact200() {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < 200; i++) {
            sb.append('x');
        }
        String exact200 = sb.toString();
        String truncated = RemoteFileReader.truncateForLog(exact200);
        
        // Exactly 200 chars should NOT be truncated
        assertEquals("Exactly 200 chars should not be truncated", exact200, truncated);
        assertFalse("Should not end with ...", truncated.endsWith("..."));
    }

    @Test
    public void testWriteResultSuccess() {
        RemoteFileWriter.WriteResult result = 
            new RemoteFileWriter.WriteResult(0, null);
        
        assertTrue("Should indicate success", result.isSuccess());
        assertEquals("Exit code should be 0", 0, result.getExitCode());
        assertNull("Error message should be null on success", result.getErrorMessage());
    }

    @Test
    public void testWriteResultError() {
        RemoteFileWriter.WriteResult result = 
            new RemoteFileWriter.WriteResult(1, "Write failed: permission denied");
        
        assertFalse("Should indicate failure", result.isSuccess());
        assertEquals("Exit code should be 1", 1, result.getExitCode());
        assertEquals("Error message should match", 
            "Write failed: permission denied", result.getErrorMessage());
    }

    @Test
    public void testWriteResultToString() {
        RemoteFileWriter.WriteResult successResult = 
            new RemoteFileWriter.WriteResult(0, null);
        assertTrue("toString should contain 'success'", 
            successResult.toString().contains("success"));
        
        RemoteFileWriter.WriteResult errorResult = 
            new RemoteFileWriter.WriteResult(2, "some error");
        assertTrue("toString should contain 'error'", 
            errorResult.toString().contains("error"));
        assertTrue("toString should contain exitCode", 
            errorResult.toString().contains("exitCode=2"));
    }

    @Test
    public void testEscapePathWriterSimple() {
        // Writer uses same escape function - verify it exists and works
        String path = "/home/user/output.txt";
        String escaped = RemoteFileWriter.escapePathForSSH(path);
        assertEquals("'/home/user/output.txt'", escaped);
    }

    @Test
    public void testEscapePathWriterWithSingleQuote() {
        String path = "/home/user/output's file.txt";
        String escaped = RemoteFileWriter.escapePathForSSH(path);
        assertEquals("'/home/user/output'\\''s file.txt'", escaped);
    }
}