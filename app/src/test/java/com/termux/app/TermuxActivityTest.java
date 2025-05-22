package com.termux.app;

import com.termux.shared.termux.data.TermuxUrlUtils;

import org.junit.Assert;
import org.junit.Test;

import java.util.Collections;
import java.util.LinkedHashSet;

public class TermuxActivityTest {

    private void assertUrlsAre(String text, String... urls) {
        LinkedHashSet<String> expected = new LinkedHashSet<>();
        Collections.addAll(expected, urls);
        Assert.assertEquals(expected, TermuxUrlUtils.extractUrls(text));
    }

    @Test
    public void testExtractUrls() {
        // Basic single URL
        assertUrlsAre("hello http://example.com world", "http://example.com");

        // Multiple URLs separated by newlines
        assertUrlsAre("http://example.com\nhttp://another.com",
            "http://example.com", "http://another.com");

        // Multiple mixed protocols
        assertUrlsAre("hello http://example.com and https://secure.com",
            "http://example.com", "https://secure.com");

        // URLs with fragments
        assertUrlsAre("https://example.com/#bar https://example.com/foo#bar",
            "https://example.com/#bar", "https://example.com/foo#bar");

        // FTP and mailto links
        assertUrlsAre("Check ftp://server.com/file.txt and mailto:user@example.com",
            "ftp://server.com/file.txt", "mailto:user@example.com");

        // Encoded characters in URL
        assertUrlsAre("Search: https://example.com/search?q=test%20query",
            "https://example.com/search?q=test%20query");

        // URL starting with www (if supported)
        assertUrlsAre("Visit www.example.com for info", "www.example.com");

        // URL inside parentheses
        assertUrlsAre("Click (http://example.com)", "http://example.com");

        // URL inside quotes
        assertUrlsAre("\"https://secure.example.com\"", "https://secure.example.com");

        // Plain text without links
        assertUrlsAre("This is a plain text with no links at all");

        // Duplicate URLs
        assertUrlsAre("http://example.com and again http://example.com", "http://example.com");

        // Internationalized domain name
        assertUrlsAre("Check this: https://مثال.إختبار", "https://مثال.إختبار");
    }

}
