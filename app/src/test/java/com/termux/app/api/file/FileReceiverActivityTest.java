package com.termux.app.api.file;

import com.termux.app.api.file.FileReceiverActivity;

import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;

@RunWith(RobolectricTestRunner.class)
public class FileReceiverActivityTest {

    @Test
    public void testHttpUrl() {
        Assert.assertTrue(FileReceiverActivity.isSharedTextAnUrl("http://example.com"));
    }

    @Test
    public void testHttpsUrlWithPath() {
        Assert.assertTrue(FileReceiverActivity.isSharedTextAnUrl("https://example.com/path/parameter=foo"));
    }

    @Test
    public void testMagnetUrl() {
        String magnet = "magnet:?xt=urn:btih:d540fc48eb12f2833163eed6421d449dd8f1ce1f&dn=Ubuntu+desktop+19.04+%2864bit%29";
        Assert.assertTrue(FileReceiverActivity.isSharedTextAnUrl(magnet));
    }

    @Test
    public void testFtpUrl() {
        Assert.assertTrue(FileReceiverActivity.isSharedTextAnUrl("ftp://example.com/resource"));
    }

    @Test
    public void testUnicodeUrl() {
        Assert.assertTrue(FileReceiverActivity.isSharedTextAnUrl("https://مثال.إختبار"));
    }

    @Test
    public void testWwwPrefix() {
        Assert.assertTrue(FileReceiverActivity.isSharedTextAnUrl("www.example.com"));
    }

    @Test
    public void testUrlWithEscapedCharacters() {
        Assert.assertTrue(FileReceiverActivity.isSharedTextAnUrl("https://example.com/search?q=some%20value"));
    }

    @Test
    public void testTextWithUrlButNotLink() {
        Assert.assertFalse(FileReceiverActivity.isSharedTextAnUrl("a test with example.com"));
    }

    @Test
    public void testEmptyString() {
        Assert.assertFalse(FileReceiverActivity.isSharedTextAnUrl(""));
    }

    @Test
    public void testNullInput() {
        try {
            Assert.assertFalse(FileReceiverActivity.isSharedTextAnUrl(null));
        } catch (Exception e) {
            Assert.fail("Should not throw exception on null input");
        }
    }

    @Test
    public void testSpacesBeforeUrl() {
        Assert.assertFalse(FileReceiverActivity.isSharedTextAnUrl("   https://example.com"));
    }

    @Test
    public void testRandomText() {
        Assert.assertFalse(FileReceiverActivity.isSharedTextAnUrl("This is just some random sentence."));
    }

    @Test
    public void testLongUrl() {
        String longUrl = "https://example.com/" + "a".repeat(1000);
        Assert.assertTrue(FileReceiverActivity.isSharedTextAnUrl(longUrl));
    }
    }
