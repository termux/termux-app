package com.termux.app.ssh;

import org.junit.Test;
import org.junit.Before;

import static org.junit.Assert.*;

/**
 * Unit tests for RemoteFileOperator command building and path escaping logic.
 */
public class RemoteFileOperatorTest {

    private SSHConnectionInfo testConnection;

    @Before
    public void setUp() {
        testConnection = new SSHConnectionInfo("testuser", "testhost", 22,
            "/tmp/ssh-control-testuser@testhost:22");
    }

    // ==================== Path Escaping Tests ====================

    @Test
    public void testEscapeSimplePath() {
        String path = "/home/user/file.txt";
        String escaped = RemoteFileOperator.escapePath(path);
        assertEquals("'/home/user/file.txt'", escaped);
    }

    @Test
    public void testEscapePathWithSpaces() {
        String path = "/home/user/my file.txt";
        String escaped = RemoteFileOperator.escapePath(path);
        assertEquals("'/home/user/my file.txt'", escaped);
    }

    @Test
    public void testEscapePathWithSingleQuote() {
        String path = "/home/user/it's mine.txt";
        String escaped = RemoteFileOperator.escapePath(path);
        // Single quote is escaped as '\''
        assertEquals("'/home/user/it'\\''s mine.txt'", escaped);
    }

    @Test
    public void testEscapePathWithMultipleSingleQuotes() {
        String path = "/home/user/'quoted'/file.txt";
        String escaped = RemoteFileOperator.escapePath(path);
        assertEquals("'/home/user/'\\''quoted'\\''/file.txt'", escaped);
    }

    @Test
    public void testEscapePathWithSpecialChars() {
        String path = "/home/user/$var&*!@#.txt";
        String escaped = RemoteFileOperator.escapePath(path);
        assertEquals("'/home/user/$var&*!@#.txt'", escaped);
    }

    @Test
    public void testEscapeEmptyPath() {
        String path = "";
        String escaped = RemoteFileOperator.escapePath(path);
        assertEquals("''", escaped);
    }

    @Test
    public void testEscapeRootPath() {
        String path = "/";
        String escaped = RemoteFileOperator.escapePath(path);
        assertEquals("'/'", escaped);
    }

    @Test
    public void testEscapePathWithBackslash() {
        String path = "/home/user\\file.txt";
        String escaped = RemoteFileOperator.escapePath(path);
        assertEquals("'/home/user\\file.txt'", escaped);
    }

    @Test
    public void testEscapePathWithNewline() {
        String path = "/home/user/file\nname.txt";
        String escaped = RemoteFileOperator.escapePath(path);
        assertEquals("'/home/user/file\nname.txt'", escaped);
    }

    // ==================== OperationResult Tests ====================

    @Test
    public void testOperationResultSuccess() {
        RemoteFileOperator.OperationResult result = new RemoteFileOperator.OperationResult(
            true, 0, "output", "", null);

        assertTrue(result.success);
        assertEquals(Integer.valueOf(0), result.exitCode);
        assertEquals("output", result.stdout);
        assertEquals("", result.stderr);
        assertNull(result.errorMessage);
    }

    @Test
    public void testOperationResultFailure() {
        RemoteFileOperator.OperationResult result = new RemoteFileOperator.OperationResult(
            false, 1, "", "No such file", "File not found");

        assertFalse(result.success);
        assertEquals(Integer.valueOf(1), result.exitCode);
        assertEquals("", result.stdout);
        assertEquals("No such file", result.stderr);
        assertEquals("File not found", result.errorMessage);
    }

    @Test
    public void testOperationResultToString() {
        RemoteFileOperator.OperationResult result = new RemoteFileOperator.OperationResult(
            true, 0, "output", "", null);

        String str = result.toString();
        assertTrue(str.contains("success=true"));
        assertTrue(str.contains("exitCode=0"));
    }

    @Test
    public void testOperationResultToStringWithLongStderr() {
        // Create a result with stderr longer than 100 chars to test truncation
        String longError = "This is a very long error message that exceeds one hundred characters and should be truncated in the toString output to prevent huge logs";
        RemoteFileOperator.OperationResult result = new RemoteFileOperator.OperationResult(
            false, 1, "", longError, "Error");

        String str = result.toString();
        // toString truncates stderr to 100 chars
        assertTrue(str.contains("success=false"));
        assertTrue(str.contains("exitCode=1"));
        // Verify stderr is truncated (contains "...") and doesn't contain full message
        assertTrue(str.contains("stderr='"));
        assertTrue(str.contains("..."));
        // Full message is 140+ chars, truncated version should be shorter
        assertFalse(str.contains("prevent huge logs")); // This part should be truncated away
    }

    // ==================== SSHConnectionInfo Tests ====================

    @Test
    public void testConnectionInfoToString() {
        assertEquals("testuser@testhost:22", testConnection.toString());
    }

    @Test
    public void testConnectionInfoGetters() {
        assertEquals("testuser", testConnection.getUser());
        assertEquals("testhost", testConnection.getHost());
        assertEquals(22, testConnection.getPort());
        assertEquals("/tmp/ssh-control-testuser@testhost:22", testConnection.getSocketPath());
    }

    @Test
    public void testConnectionInfoEquality() {
        SSHConnectionInfo same = new SSHConnectionInfo("testuser", "testhost", 22,
            "/different/socket/path");
        SSHConnectionInfo differentUser = new SSHConnectionInfo("otheruser", "testhost", 22,
            "/tmp/ssh-control-otheruser@testhost:22");
        SSHConnectionInfo differentHost = new SSHConnectionInfo("testuser", "otherhost", 22,
            "/tmp/ssh-control-testuser@otherhost:22");

        // Equality is based on user, host, port (not socketPath)
        assertEquals(testConnection, same);
        assertNotEquals(testConnection, differentUser);
        assertNotEquals(testConnection, differentHost);
    }

    @Test
    public void testConnectionInfoParseFromFilename() {
        SSHConnectionInfo parsed = SSHConnectionInfo.parseFromFilename(
            "user@host:22", "/tmp/ssh-control-user@host:22");

        assertNotNull(parsed);
        assertEquals("user", parsed.getUser());
        assertEquals("host", parsed.getHost());
        assertEquals(22, parsed.getPort());
        assertEquals("/tmp/ssh-control-user@host:22", parsed.getSocketPath());
    }

    @Test
    public void testConnectionInfoParseInvalidFilename() {
        // Missing @
        assertNull(SSHConnectionInfo.parseFromFilename("nohost:22", "/path"));
        // Missing :
        assertNull(SSHConnectionInfo.parseFromFilename("user@host", "/path"));
        // Invalid port
        assertNull(SSHConnectionInfo.parseFromFilename("user@host:abc", "/path"));
        // Empty user
        assertNull(SSHConnectionInfo.parseFromFilename("@host:22", "/path"));
    }

    // ==================== Edge Cases ====================

    @Test
    public void testEscapePathPreservesTrailingSlash() {
        String path = "/home/user/dir/";
        String escaped = RemoteFileOperator.escapePath(path);
        assertEquals("'/home/user/dir/'", escaped);
    }

    @Test
    public void testEscapePathWithUnicode() {
        String path = "/home/user/文件.txt";
        String escaped = RemoteFileOperator.escapePath(path);
        assertEquals("'/home/user/文件.txt'", escaped);
    }

    @Test
    public void testEscapePathWithDollarSign() {
        // $ is a special char in shell but safe inside single quotes
        String path = "/home/user/$HOME/file.txt";
        String escaped = RemoteFileOperator.escapePath(path);
        assertEquals("'/home/user/$HOME/file.txt'", escaped);
    }

    @Test
    public void testEscapePathWithBackticks() {
        // Backticks are command substitution but safe inside single quotes
        String path = "/home/user/`cmd`.txt";
        String escaped = RemoteFileOperator.escapePath(path);
        assertEquals("'/home/user/`cmd`.txt'", escaped);
    }

    @Test
    public void testEscapePathWithSemicolon() {
        // Semicolon could be command separator but safe inside single quotes
        String path = "/home/user/file;rm.txt";
        String escaped = RemoteFileOperator.escapePath(path);
        assertEquals("'/home/user/file;rm.txt'", escaped);
    }

    @Test
    public void testEscapePathWithPipe() {
        String path = "/home/user/a|b.txt";
        String escaped = RemoteFileOperator.escapePath(path);
        assertEquals("'/home/user/a|b.txt'", escaped);
    }

    @Test
    public void testEscapePathWithDoubleQuote() {
        // Double quotes are safe inside single quotes
        String path = "/home/user/\"quoted\".txt";
        String escaped = RemoteFileOperator.escapePath(path);
        assertEquals("'/home/user/\"quoted\".txt'", escaped);
    }

    // ==================== Integration-Style Tests ====================

    @Test
    public void testConnectionInfoWithCustomPort() {
        SSHConnectionInfo customPort = new SSHConnectionInfo("admin", "server.example.com", 2222,
            "/tmp/ssh-control-admin@server.example.com:2222");

        assertEquals(2222, customPort.getPort());
        assertEquals("admin@server.example.com:2222", customPort.toString());
    }

    @Test
    public void testConnectionInfoWithIPHost() {
        SSHConnectionInfo ipHost = new SSHConnectionInfo("root", "192.168.1.100", 22,
            "/tmp/ssh-control-root@192.168.1.100:22");

        assertEquals("192.168.1.100", ipHost.getHost());
        assertEquals("root@192.168.1.100:22", ipHost.toString());
    }

    @Test
    public void testConnectionInfoHashCode() {
        SSHConnectionInfo same = new SSHConnectionInfo("testuser", "testhost", 22,
            "/different/socket");
        SSHConnectionInfo different = new SSHConnectionInfo("other", "testhost", 22,
            "/tmp/ssh-control-other@testhost:22");

        // Equal objects should have same hashCode
        assertEquals(testConnection.hashCode(), same.hashCode());
        // Different objects may have different hashCode (not required but expected)
        assertNotEquals(testConnection.hashCode(), different.hashCode());
    }
}