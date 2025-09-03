package com.termux.shared.net.url;

import androidx.annotation.Nullable;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.Collections;
import java.util.LinkedHashSet;
import java.util.regex.Pattern;

/**
 * Utility class for URL-related operations.
 */
public class UrlUtils {

    /** The parts of a {@link URI}. */
    public enum UrlPart {
        AUTHORITY,
        FILE,
        HOST,
        REF,
        FRAGMENT,
        PATH,
        PORT,
        PROTOCOL,
        QUERY,
        USER_INFO
    }

    private static final String LOG_TAG = "UrlUtils";

    /**
     * Joins a base URL and a destination into a single URL string.
     *
     * @param base The base URL.
     * @param destination The destination URL.
     * @param logError Whether to log an error if the URLs cannot be joined.
     * @return The joined URL as a string, or {@code null} if an error occurs.
     */
    @Nullable
    public static String joinUrl(final String base, String destination, boolean logError) {
        if (base == null || base.isEmpty()) return null;
        try {
            URI baseUri = new URI(base);
            URI resolvedUri = baseUri.resolve(destination);
            return resolvedUri.toString();
        } catch (URISyntaxException e) {
            if (logError)
                Logger.logError(LOG_TAG, "Failed to join URL base \"" + base + "\" and destination \"" + destination + "\": " + e.getMessage());
            return null;
        }
    }

    /**
     * Converts a URL string to a {@link URI} object.
     *
     * @param urlString The URL string.
     * @return The corresponding {@link URI}, or {@code null} if the URL string is invalid.
     */
    @Nullable
    public static URI getUri(String urlString) {
        if (urlString == null || urlString.isEmpty()) return null;
        try {
            return new URI(urlString);
        } catch (URISyntaxException e) {
            return null;
        }
    }

    /**
     * Extracts a specific part from a URL string.
     *
     * @param urlString The URL string.
     * @param urlPart The part to extract.
     * @return The specified part of the URL, or {@code null} if the URL is invalid or the part does not exist.
     */
    @Nullable
    public static String getUrlPart(String urlString, UrlPart urlPart) {
        URI uri = getUri(urlString);
        if (uri == null) return null;
        switch (urlPart) {
            case AUTHORITY:
                return uri.getAuthority();
            case FILE:
                return uri.getPath() + (uri.getQuery() != null ? "?" + uri.getQuery() : "");
            case HOST:
                return uri.getHost();
            case REF:
            case FRAGMENT:
                return uri.getFragment();
            case PATH:
                return uri.getPath();
            case PORT:
                int port = uri.getPort();
                return port == -1 ? null : String.valueOf(port);
            case PROTOCOL:
                return uri.getScheme();
            case QUERY:
                return uri.getQuery();
            case USER_INFO:
                return uri.getUserInfo();
            default:
                return null;
        }
    }

    /**
     * Removes the protocol and "www." prefix from a URL string.
     *
     * @param urlString The URL string.
     * @return The URL string without the protocol and "www." prefix, or {@code null} if the input is null.
     */
    public static String removeProtocol(String urlString) {
        if (urlString == null) return null;
        return urlString.replaceFirst("(?i)^(https?://)?(www\\.)?", "");
    }

    /**
     * Compares two URL strings for equality, ignoring protocol, "www." prefix, and trailing slashes.
     *
     * @param url1 The first URL string.
     * @param url2 The second URL string.
     * @return {@code true} if the URLs are equal after normalization; {@code false} otherwise.
     */
    public static boolean areUrlsEqual(String url1, String url2) {
        if (url1 == null && url2 == null) return true;
        if (url1 == null || url2 == null) return false;

        String normalizedUrl1 = normalizeUrl(url1);
        String normalizedUrl2 = normalizeUrl(url2);
        return normalizedUrl1.equals(normalizedUrl2);
    }

    private static String normalizeUrl(String url) {
        if (url == null) return "";
        String noProtocol = removeProtocol(url).replaceAll("/+$", "").toLowerCase();
        // Strip query and fragment for equality comparison
        int queryIdx = noProtocol.indexOf('?');
        if (queryIdx != -1) noProtocol = noProtocol.substring(0, queryIdx);
        int fragmentIdx = noProtocol.indexOf('#');
        if (fragmentIdx != -1) noProtocol = noProtocol.substring(0, fragmentIdx);
        return noProtocol;
    }

    /**
     * Extracts URLs from a given text.
     *
     * @param text The text to search.
     * @return A set of URLs found in the text.
     */
    public static LinkedHashSet<String> extractUrls(String text) {
        if (text == null || text.isEmpty()) return new LinkedHashSet<>();

        // Improved regex for matching URLs more robustly
        String regex = "\\bhttps?://[\\w\\-._~:/?#[\\]@!$&'()*+,;=%]+";
        Pattern pattern = Pattern.compile(regex, Pattern.CASE_INSENSITIVE);
        var matcher = pattern.matcher(text);

        LinkedHashSet<String> urls = new LinkedHashSet<>();
        while (matcher.find()) {
            urls.add(matcher.group());
        }
        return urls;
    }
}
