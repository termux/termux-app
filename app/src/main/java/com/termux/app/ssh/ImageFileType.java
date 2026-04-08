package com.termux.app.ssh;

import java.util.HashSet;
import java.util.Set;

/**
 * Utility class for determining if a file is a supported image format.
 *
 * Supported formats: jpg, jpeg, png, gif, webp, bmp, svg
 */
public final class ImageFileType {

    /** Set of supported image file extensions (lowercase, without leading dot) */
    private static final Set<String> IMAGE_EXTENSIONS = new HashSet<>();

    static {
        IMAGE_EXTENSIONS.add("jpg");
        IMAGE_EXTENSIONS.add("jpeg");
        IMAGE_EXTENSIONS.add("png");
        IMAGE_EXTENSIONS.add("gif");
        IMAGE_EXTENSIONS.add("webp");
        IMAGE_EXTENSIONS.add("bmp");
        IMAGE_EXTENSIONS.add("svg");
    }

    // Private constructor to prevent instantiation
    private ImageFileType() {
        throw new AssertionError("ImageFileType is a utility class and cannot be instantiated");
    }

    /**
     * Check if a file is a supported image format based on its extension.
     *
     * @param filename Full filename with extension
     * @return true if the file appears to be a supported image format
     */
    public static boolean isImageFile(String filename) {
        if (filename == null || filename.isEmpty()) {
            return false;
        }

        // Extract extension
        int dotIndex = filename.lastIndexOf('.');
        if (dotIndex <= 0 || dotIndex == filename.length() - 1) {
            // No extension or dot at start/end
            return false;
        }

        String extension = filename.substring(dotIndex + 1).toLowerCase().trim();
        return IMAGE_EXTENSIONS.contains(extension);
    }

    /**
     * Check if a file extension (without leading dot) is a supported image format.
     *
     * @param extension File extension (case-insensitive)
     * @return true if the extension is a supported image format
     */
    public static boolean isImageExtension(String extension) {
        if (extension == null || extension.isEmpty()) {
            return false;
        }

        String lowerExt = extension.toLowerCase().trim();
        return IMAGE_EXTENSIONS.contains(lowerExt);
    }

    /**
     * Get all supported image file extensions.
     *
     * @return Set of all supported image extensions (lowercase)
     */
    public static Set<String> getSupportedExtensions() {
        return new HashSet<>(IMAGE_EXTENSIONS);
    }
}