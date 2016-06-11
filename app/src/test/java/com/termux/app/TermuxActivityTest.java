package com.termux.app;

import junit.framework.TestCase;

import java.util.Collections;
import java.util.LinkedHashSet;

public class TermuxActivityTest extends TestCase {

	private void assertUrlsAre(String text, String... urls) {
		LinkedHashSet<String> expected = new LinkedHashSet<>();
		Collections.addAll(expected, urls);
		assertEquals(expected, TermuxActivity.extractUrls(text));
	}

	public void testExtractUrls() {
		assertUrlsAre("hello http://example.com world", "http://example.com");

		assertUrlsAre("http://example.com\nhttp://another.com", "http://example.com", "http://another.com");

		assertUrlsAre("hello http://example.com world and http://more.example.com with secure https://more.example.com",
				"http://example.com", "http://more.example.com", "https://more.example.com");
	}

}
