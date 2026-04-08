package com.termux.app.ssh;

import org.junit.Test;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import static org.junit.Assert.*;

/**
 * Test for symlink path handling bug fix:
 * When symlink target is absolute path like /root/termux-app/.gsd,
 * and ls -laF output shows the target directly in name field,
 * the path was incorrectly concatenated.
 */
public class SymlinkPathTest {

    private static final Pattern LS_LINE_PATTERN = Pattern.compile(
        "^([bcdlsp-][rwxst-]{9})\\s+" +
        "(\\d+)\\s+" +
        "(\\S+)\\s+" +
        "(\\S+)\\s+" +
        "(\\d+)\\s+" +
        "(\\w{3}\\s+\\d{1,2})\\s+" +
        "(\\d{1,2}:\\d{2}|\\d{4})\\s+" +
        "(.+)$"
    );

    private static final Pattern SYMLINK_PATTERN = Pattern.compile("^(.+?)\\s+->\\s+(.+)$");

    /**
     * Simulate the parseLSLine logic with the fix applied.
     */
    private ParsedResult parseLSLine(String line, String basePath) {
        Matcher matcher = LS_LINE_PATTERN.matcher(line);
        if (!matcher.matches()) return null;
        
        String permissions = matcher.group(1);
        String nameField = matcher.group(8);
        char typeChar = permissions.charAt(0);
        RemoteFile.FileType type = RemoteFile.getTypeFromPermissionChar(typeChar);
        
        String name = nameField;
        String symlinkTarget = null;
        boolean symlinkTargetIsDirectory = false;
        
        // Check for symlink type indicator (@)
        if (name.endsWith("@")) {
            name = name.substring(0, name.length() - 1);
        }
        
        // Handle symlinks: parse "name -> target"
        if (type == RemoteFile.FileType.SYMLINK) {
            Matcher symlinkMatcher = SYMLINK_PATTERN.matcher(name);
            if (symlinkMatcher.matches()) {
                name = symlinkMatcher.group(1);
                symlinkTarget = symlinkMatcher.group(2);
                
                if (name.endsWith("@")) {
                    name = name.substring(0, name.length() - 1);
                }
                
                if (symlinkTarget.endsWith("/")) {
                    symlinkTarget = symlinkTarget.substring(0, symlinkTarget.length() - 1);
                    symlinkTargetIsDirectory = true;
                }
            } else {
                // FIX: Handle abnormal symlink output where name is absolute path
                if (name.startsWith("/")) {
                    symlinkTarget = name;
                    // Remove trailing / before extracting name segment
                    if (symlinkTarget.endsWith("/")) {
                        symlinkTarget = symlinkTarget.substring(0, symlinkTarget.length() - 1);
                        symlinkTargetIsDirectory = true;
                    }
                    // Extract last segment from the path (after removing trailing /)
                    String pathForExtraction = name;
                    if (pathForExtraction.endsWith("/")) {
                        pathForExtraction = pathForExtraction.substring(0, pathForExtraction.length() - 1);
                    }
                    int lastSlash = pathForExtraction.lastIndexOf('/');
                    if (lastSlash >= 0 && lastSlash < pathForExtraction.length() - 1) {
                        name = pathForExtraction.substring(lastSlash + 1);
                        if (name.endsWith("@") || name.endsWith("/")) {
                            name = name.substring(0, name.length() - 1);
                        }
                    }
                }
                if (name.endsWith("@")) {
                    name = name.substring(0, name.length() - 1);
                }
            }
        } else if (type == RemoteFile.FileType.DIRECTORY) {
            if (name.endsWith("/")) {
                name = name.substring(0, name.length() - 1);
            }
        }
        
        // Build full path
        String normalizedBase = basePath.endsWith("/") && basePath.length() > 1
            ? basePath.substring(0, basePath.length() - 1)
            : basePath;
        String fullPath = normalizedBase + "/" + name;
        
        ParsedResult result = new ParsedResult();
        result.name = name;
        result.path = fullPath;
        result.symlinkTarget = symlinkTarget;
        result.symlinkTargetIsDirectory = symlinkTargetIsDirectory;
        return result;
    }
    
    static class ParsedResult {
        String name;
        String path;
        String symlinkTarget;
        boolean symlinkTargetIsDirectory;
    }

    /**
     * Test normal symlink with absolute path target - should work correctly.
     */
    @Test
    public void testNormalSymlinkWithAbsolutePathTarget() {
        String line = "lrwxrwxrwx  1 user group   10 Jan 15 10:30 .gsd@ -> /root/termux-app/.gsd/";
        String basePath = "/root/termux-app";
        
        ParsedResult result = parseLSLine(line, basePath);
        
        assertNotNull("Should parse successfully", result);
        assertEquals("name should be .gsd", ".gsd", result.name);
        assertEquals("symlinkTarget should be absolute path", 
            "/root/termux-app/.gsd", result.symlinkTarget);
        assertEquals("path should be correctly concatenated", 
            "/root/termux-app/.gsd", result.path);
        assertTrue("target should be directory", result.symlinkTargetIsDirectory);
    }

    /**
     * Test the bug case: abnormal symlink where name field shows absolute path directly.
     * This is the key fix - previously this would cause path duplication.
     */
    @Test
    public void testAbnormalSymlinkNameIsAbsolutePath() {
        // Hypothetical abnormal ls output where target appears as name
        String line = "lrwxrwxrwx  1 user group   32 Mar 29 15:41 /root/termux-app/.gsd";
        String basePath = "/root/termux-app/.gsd";
        
        ParsedResult result = parseLSLine(line, basePath);
        
        assertNotNull("Should parse successfully", result);
        assertEquals("name should be extracted as .gsd", ".gsd", result.name);
        assertEquals("symlinkTarget should be the absolute path", 
            "/root/termux-app/.gsd", result.symlinkTarget);
        // KEY FIX: path should be /root/termux-app/.gsd/.gsd, NOT /root/termux-app/.gsd//root/termux-app/.gsd
        assertEquals("path should be correctly concatenated with extracted name", 
            "/root/termux-app/.gsd/.gsd", result.path);
    }

    /**
     * Test abnormal symlink with trailing slash (directory indicator).
     */
    @Test
    public void testAbnormalSymlinkWithTrailingSlash() {
        String line = "lrwxrwxrwx  1 user group   32 Mar 29 15:41 /root/termux-app/.gsd/";
        String basePath = "/some/other/path";
        
        ParsedResult result = parseLSLine(line, basePath);
        
        assertNotNull("Should parse successfully", result);
        // The name field is "/root/termux-app/.gsd/" which starts with /
        // After removing trailing / from the whole field, then extracting last segment:
        // - symlinkTarget becomes "/root/termux-app/.gsd" (trailing / removed)
        // - name should be extracted from "/root/termux-app/.gsd/" -> ".gsd"
        assertEquals("name should be extracted as .gsd", ".gsd", result.name);
        assertEquals("symlinkTarget should not have trailing slash", 
            "/root/termux-app/.gsd", result.symlinkTarget);
        assertTrue("target should be marked as directory", result.symlinkTargetIsDirectory);
        assertEquals("path should be correctly concatenated", 
            "/some/other/path/.gsd", result.path);
    }

    /**
     * Test normal symlink without trailing slash.
     */
    @Test
    public void testNormalSymlinkWithoutTrailingSlash() {
        String line = "lrwxrwxrwx  1 user group   10 Jan 15 10:30 link@ -> /some/target";
        String basePath = "/base";
        
        ParsedResult result = parseLSLine(line, basePath);
        
        assertNotNull("Should parse successfully", result);
        assertEquals("name should be link", "link", result.name);
        assertEquals("symlinkTarget should be /some/target", "/some/target", result.symlinkTarget);
        assertEquals("path should be /base/link", "/base/link", result.path);
        assertFalse("target should not be directory", result.symlinkTargetIsDirectory);
    }

    /**
     * Test regular directory with trailing slash.
     */
    @Test
    public void testRegularDirectory() {
        String line = "drwxr-xr-x  2 user group 4096 Jan 15 10:30 mydir/";
        String basePath = "/base";
        
        ParsedResult result = parseLSLine(line, basePath);
        
        assertNotNull("Should parse successfully", result);
        assertEquals("name should be mydir", "mydir", result.name);
        assertEquals("path should be /base/mydir", "/base/mydir", result.path);
        assertNull("symlinkTarget should be null", result.symlinkTarget);
    }

    /**
     * Test regular file.
     */
    @Test
    public void testRegularFile() {
        String line = "-rw-r--r--  1 user group  123 Jan 15 10:30 file.txt";
        String basePath = "/base";
        
        ParsedResult result = parseLSLine(line, basePath);
        
        assertNotNull("Should parse successfully", result);
        assertEquals("name should be file.txt", "file.txt", result.name);
        assertEquals("path should be /base/file.txt", "/base/file.txt", result.path);
        assertNull("symlinkTarget should be null", result.symlinkTarget);
    }
}