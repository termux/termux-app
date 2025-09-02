package com.termux.app;

import org.junit.Assert;
import org.junit.Test;
import org.nibor.autolink.LinkExtractor;
import org.nibor.autolink.LinkSpan;
import org.nibor.autolink.LinkType;

import java.util.ArrayList;
import java.util.Collections;
import java.util.EnumSet;
import java.util.LinkedHashSet;
import java.util.List;

public class TermuxActivityTest {

    private List<String> extractUrlsWithAutolink(String text) {
        LinkExtractor extractor = LinkExtractor.builder()
            .linkTypes(EnumSet.of(LinkType.URL))
            .build();

        List<String> urls = new ArrayList<>();
        for (LinkSpan span : extractor.extractLinks(text)) {
            urls.add(text.substring(span.getBeginIndex(), span.getEndIndex()));
        }
        return urls;
    }

    private void assertUrlsAre(String text, String... expectedUrls) {
        LinkedHashSet<String> expected = new LinkedHashSet<>();
        Collections.addAll(expected, expectedUrls);

        LinkedHashSet<String> actual = new LinkedHashSet<>(extractUrlsWithAutolink(text));
        Assert.assertEquals(expected, actual);
    }

    @Test
    public void testExtractUrls() {
        assertUrlsAre("hello http://example.com world", "http://example.com");
        
        assertUrlsAre("http://example.com\nhttp://another.com", "http://example.com", "http://another.com");
        
        assertUrlsAre("hello http://example.com world and http://more.example.com with secure https://more.example.com",
            "http://example.com", "http://more.example.com", "https://more.example.com");
        
        assertUrlsAre("hello https://example.com/#bar https://example.com/foo#bar",
            "https://example.com/#bar", "https://example.com/foo#bar");
        
        assertUrlsAre("Check trailing dot: http://example.com.", "http://example.com");
        
        assertUrlsAre("Nested (http://example.com), end", "http://example.com");
        
        assertUrlsAre("Emoji domain: https://üñîçøðé.com/ä", "https://üñîçøðé.com/ä");
    }
}
