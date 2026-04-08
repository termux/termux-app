package com.termux.app.ssh;

import org.junit.Test;
import org.junit.Before;

import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.List;

import static org.junit.Assert.*;

/**
 * Unit tests for RemoteFileLister ls -la output parsing.
 */
public class RemoteFileListerTest {

    /** Regex pattern matching the one in RemoteFileLister */
    private static final Pattern LS_LINE_PATTERN = Pattern.compile(
        "^([bcdlsp-][rwxst-]{9})\\s+" +      // group 1: permissions (10 chars)
        "(\\d+)\\s+" +                        // group 2: links
        "(\\S+)\\s+" +                        // group 3: owner
        "(\\S+)\\s+" +                        // group 4: group
        "(\\d+)\\s+" +                        // group 5: size
        "(\\w{3}\\s+\\d{1,2})\\s+" +          // group 6: date part (Jan 15)
        "(\\d{1,2}:\\d{2}|\\d{4})\\s+" +      // group 7: time or year (10:30 or 2024)
        "(.+)$"                               // group 8: name (may contain symlink target)
    );

    private static final Pattern SYMLINK_PATTERN = Pattern.compile("^(.+?)\\s+->\\s+(.+)$");

    @Test
    public void testParseDirectoryLine() {
        String line = "drwxr-xr-x  2 user group 4096 Jan 15 10:30 mydir";
        Matcher matcher = LS_LINE_PATTERN.matcher(line);
        
        assertTrue("Should match directory line", matcher.matches());
        assertEquals("permissions", "drwxr-xr-x", matcher.group(1));
        assertEquals("links", "2", matcher.group(2));
        assertEquals("owner", "user", matcher.group(3));
        assertEquals("group", "group", matcher.group(4));
        assertEquals("size", "4096", matcher.group(5));
        assertEquals("date", "Jan 15", matcher.group(6));
        assertEquals("time", "10:30", matcher.group(7));
        assertEquals("name", "mydir", matcher.group(8));
    }

    @Test
    public void testParseFileLine() {
        String line = "-rw-r--r--  1 user group  123 Jan 15 10:30 myfile.txt";
        Matcher matcher = LS_LINE_PATTERN.matcher(line);
        
        assertTrue("Should match file line", matcher.matches());
        assertEquals("permissions", "-rw-r--r--", matcher.group(1));
        assertEquals("links", "1", matcher.group(2));
        assertEquals("owner", "user", matcher.group(3));
        assertEquals("group", "group", matcher.group(4));
        assertEquals("size", "123", matcher.group(5));
        assertEquals("name", "myfile.txt", matcher.group(8));
    }

    @Test
    public void testParseSymlinkLine() {
        String line = "lrwxrwxrwx  1 user group   10 Jan 15 10:30 linkname -> target";
        Matcher matcher = LS_LINE_PATTERN.matcher(line);
        
        assertTrue("Should match symlink line", matcher.matches());
        assertEquals("permissions", "lrwxrwxrwx", matcher.group(1));
        assertEquals("name field", "linkname -> target", matcher.group(8));
        
        // Parse symlink target
        Matcher symlinkMatcher = SYMLINK_PATTERN.matcher(matcher.group(8));
        assertTrue("Should parse symlink", symlinkMatcher.matches());
        assertEquals("link name", "linkname", symlinkMatcher.group(1));
        assertEquals("target", "target", symlinkMatcher.group(2));
    }

    @Test
    public void testParseOldFileLine() {
        // Older files show year instead of time
        String line = "-rw-r--r--  1 user group  123 Jan 15  2024 oldfile.txt";
        Matcher matcher = LS_LINE_PATTERN.matcher(line);
        
        assertTrue("Should match old file line with year", matcher.matches());
        assertEquals("time/year", "2024", matcher.group(7));
        assertEquals("name", "oldfile.txt", matcher.group(8));
    }

    @Test
    public void testParseSpecialPermissions() {
        // Test setuid/setgid/sticky bit files
        String line = "-rwsr-sr-x  1 root root 12345 Jan 1 12:00 special";
        Matcher matcher = LS_LINE_PATTERN.matcher(line);
        
        assertTrue("Should match special permissions", matcher.matches());
        assertEquals("permissions", "-rwsr-sr-x", matcher.group(1));
    }

    @Test
    public void testParseBlockDevice() {
        String line = "brw-r--r--  1 root root  123 Jan 1 12:00 blockdev";
        Matcher matcher = LS_LINE_PATTERN.matcher(line);
        
        assertTrue("Should match block device", matcher.matches());
        assertEquals("type char", 'b', matcher.group(1).charAt(0));
    }

    @Test
    public void testParseCharacterDevice() {
        String line = "crw-r--r--  1 root root  123 Jan 1 12:00 chardev";
        Matcher matcher = LS_LINE_PATTERN.matcher(line);
        
        assertTrue("Should match character device", matcher.matches());
        assertEquals("type char", 'c', matcher.group(1).charAt(0));
    }

    @Test
    public void testParseSocket() {
        String line = "srwxrwxrwx  1 user group    0 Jan 1 12:00 socket";
        Matcher matcher = LS_LINE_PATTERN.matcher(line);
        
        assertTrue("Should match socket", matcher.matches());
        assertEquals("type char", 's', matcher.group(1).charAt(0));
    }

    @Test
    public void testParsePipe() {
        String line = "prw-r--r--  1 user group    0 Jan 1 12:00 pipe";
        Matcher matcher = LS_LINE_PATTERN.matcher(line);
        
        assertTrue("Should match pipe", matcher.matches());
        assertEquals("type char", 'p', matcher.group(1).charAt(0));
    }

    @Test
    public void testParseFileNameWithSpaces() {
        String line = "-rw-r--r--  1 user group  123 Jan 15 10:30 file with spaces.txt";
        Matcher matcher = LS_LINE_PATTERN.matcher(line);
        
        assertTrue("Should match file with spaces in name", matcher.matches());
        assertEquals("name", "file with spaces.txt", matcher.group(8));
    }

    @Test
    public void testTotalLineNotMatched() {
        String line = "total 123";
        Matcher matcher = LS_LINE_PATTERN.matcher(line);
        
        assertFalse("Should not match 'total' line", matcher.matches());
    }

    @Test
    public void testEmptyLineNotMatched() {
        String line = "";
        Matcher matcher = LS_LINE_PATTERN.matcher(line);
        
        assertFalse("Should not match empty line", matcher.matches());
    }

    @Test
    public void testRemoteFileTypeFromPermissionChar() {
        assertEquals("directory", RemoteFile.FileType.DIRECTORY, 
            RemoteFile.getTypeFromPermissionChar('d'));
        assertEquals("file", RemoteFile.FileType.FILE, 
            RemoteFile.getTypeFromPermissionChar('-'));
        assertEquals("symlink", RemoteFile.FileType.SYMLINK, 
            RemoteFile.getTypeFromPermissionChar('l'));
        assertEquals("other", RemoteFile.FileType.OTHER, 
            RemoteFile.getTypeFromPermissionChar('b'));
    }

    @Test
    public void testRemoteFileSizeFormatting() {
        RemoteFile smallFile = new RemoteFile("test", "/test", RemoteFile.FileType.FILE, 
            512, "Jan 1 10:00", "rw-r--r--", "user", "group", null);
        assertEquals("512 B", smallFile.getSizeFormatted());

        RemoteFile kbFile = new RemoteFile("test", "/test", RemoteFile.FileType.FILE, 
            1536, "Jan 1 10:00", "rw-r--r--", "user", "group", null);
        assertEquals("1.5 KB", kbFile.getSizeFormatted());

        RemoteFile mbFile = new RemoteFile("test", "/test", RemoteFile.FileType.FILE, 
            2 * 1024 * 1024 + 300 * 1024, "Jan 1 10:00", "rw-r--r--", "user", "group", null);
        assertEquals("2.3 MB", mbFile.getSizeFormatted());
    }

    @Test
    public void testRemoteFileIsDirectoryMethod() {
        RemoteFile dir = new RemoteFile("dir", "/dir", RemoteFile.FileType.DIRECTORY, 
            0, "Jan 1 10:00", "rwxr-xr-x", "user", "group", null);
        assertTrue(dir.isDirectory());
        assertFalse(dir.isFile());
        assertFalse(dir.isSymlink());

        RemoteFile file = new RemoteFile("file", "/file", RemoteFile.FileType.FILE, 
            100, "Jan 1 10:00", "rw-r--r--", "user", "group", null);
        assertFalse(file.isDirectory());
        assertTrue(file.isFile());
        assertFalse(file.isSymlink());

        RemoteFile symlink = new RemoteFile("link", "/link", RemoteFile.FileType.SYMLINK, 
            10, "Jan 1 10:00", "rwxrwxrwx", "user", "group", "/target");
        assertFalse(symlink.isDirectory());
        assertFalse(symlink.isFile());
        assertTrue(symlink.isSymlink());
        assertEquals("/target", symlink.getSymlinkTarget());
    }

    @Test
    public void testRemoteFileIsDirectoryOrSymlinkToDirectory() {
        // Regular directory
        RemoteFile dir = new RemoteFile("dir", "/dir", RemoteFile.FileType.DIRECTORY, 
            0, "Jan 1 10:00", "rwxr-xr-x", "user", "group", null);
        assertTrue("Regular directory should return true", dir.isDirectoryOrSymlinkToDirectory());

        // Symlink pointing to directory
        RemoteFile symlinkToDir = new RemoteFile("linkdir", "/linkdir", RemoteFile.FileType.SYMLINK, 
            10, "Jan 1 10:00", "rwxrwxrwx", "user", "group", "/actual_dir", true);
        assertTrue("Symlink to directory should return true", symlinkToDir.isDirectoryOrSymlinkToDirectory());
        assertTrue("symlinkTargetIsDirectory should be true", symlinkToDir.isSymlinkTargetDirectory());

        // Symlink pointing to file
        RemoteFile symlinkToFile = new RemoteFile("linkfile", "/linkfile", RemoteFile.FileType.SYMLINK, 
            10, "Jan 1 10:00", "rwxrwxrwx", "user", "group", "/actual_file.txt", false);
        assertFalse("Symlink to file should return false", symlinkToFile.isDirectoryOrSymlinkToDirectory());
        assertFalse("symlinkTargetIsDirectory should be false", symlinkToFile.isSymlinkTargetDirectory());

        // Regular file
        RemoteFile file = new RemoteFile("file", "/file", RemoteFile.FileType.FILE, 
            100, "Jan 1 10:00", "rw-r--r--", "user", "group", null);
        assertFalse("Regular file should return false", file.isDirectoryOrSymlinkToDirectory());
    }
}