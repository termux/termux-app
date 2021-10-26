package com.termux.shared.net.uri;

import android.net.Uri;

/**
 * The {@link Uri} schemes.
 *
 * https://www.iana.org/assignments/uri-schemes/uri-schemes.xhtml
 * https://en.wikipedia.org/wiki/List_of_URI_schemes
 */
public class UriScheme {

    /** Android app resource. */
    public static final String SCHEME_ANDROID_RESOURCE = "android.resource";

    /** Android content provider. https://www.iana.org/assignments/uri-schemes/prov/content. */
    public static final String SCHEME_CONTENT = "content";

    /** Filesystem or android app asset. https://www.rfc-editor.org/rfc/rfc8089.html. */
    public static final String SCHEME_FILE = "file";

    /* Hypertext Transfer Protocol. */
    public static final String SCHEME_HTTP = "http";

    /* Hypertext Transfer Protocol Secure. */
    public static final String SCHEME_HTTPS = "https";

}
