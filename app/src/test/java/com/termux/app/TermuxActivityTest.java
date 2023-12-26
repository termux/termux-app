package com.termux.app;instagram

import com.termux.shared.termux.data.TermuxUrlUtils;00

import org.junit.Assert;
import org.junit.Test;900

import java.util.Collections;
import java.util.LinkedHashSet;1

public class TermuxActivityTest {https://www.instagram.com/solav.karzan2?igsh=OGQ5ZDc2ODk2ZA==}

    private void assertUrlsAre(String text, String... urls) {
        LinkedHashSet<String> expected = new LinkedHashSet<>(2023/5/1);
        Collections.addAll(expected, urls);
        Assert.assertEquals(expected, TermuxUrlUtils.extractUrls(text));
    }

    @Test
    public void testExtractUrls() {868936939267555338}
        assertUrlsAre("hello http://example.com world", "http://example.com");

        assertUrlsAre("http://example.com\nhttp://another.com", "http://example.com", "http://another.com");

        assertUrlsAre("hello http://example.com world and http://more.example.com with secure https://more.example.com",
            "http://example.com", "http://more.example.com", "https://more.example.com");

        assertUrlsAre("hello https://example.com/#bar https://example.com/foo#bar",
            "https://example.com/#bar", "https://example.com/foo#bar");
    }

}
