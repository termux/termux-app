package com.termux.app.ssh;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;

import java.util.Set;

import static org.junit.Assert.*;

/**
 * Unit tests for ImageFileType utility class.
 */
@RunWith(RobolectricTestRunner.class)
public class ImageFileTypeTest {

    // ========== isImageFile tests ==========

    @Test
    public void isImageFile_jpg_returnsTrue() {
        assertTrue(ImageFileType.isImageFile("image.jpg"));
    }

    @Test
    public void isImageFile_jpeg_returnsTrue() {
        assertTrue(ImageFileType.isImageFile("image.jpeg"));
    }

    @Test
    public void isImageFile_png_returnsTrue() {
        assertTrue(ImageFileType.isImageFile("image.png"));
    }

    @Test
    public void isImageFile_gif_returnsTrue() {
        assertTrue(ImageFileType.isImageFile("animation.gif"));
    }

    @Test
    public void isImageFile_webp_returnsTrue() {
        assertTrue(ImageFileType.isImageFile("modern.webp"));
    }

    @Test
    public void isImageFile_bmp_returnsTrue() {
        assertTrue(ImageFileType.isImageFile("bitmap.bmp"));
    }

    @Test
    public void isImageFile_svg_returnsTrue() {
        assertTrue(ImageFileType.isImageFile("vector.svg"));
    }

    @Test
    public void isImageFile_uppercaseExtension_returnsTrue() {
        assertTrue(ImageFileType.isImageFile("image.JPG"));
        assertTrue(ImageFileType.isImageFile("image.PNG"));
        assertTrue(ImageFileType.isImageFile("image.GIF"));
    }

    @Test
    public void isImageFile_mixedCaseExtension_returnsTrue() {
        assertTrue(ImageFileType.isImageFile("image.JpG"));
        assertTrue(ImageFileType.isImageFile("image.PnG"));
    }

    @Test
    public void isImageFile_unsupportedExtension_returnsFalse() {
        assertFalse(ImageFileType.isImageFile("document.pdf"));
        assertFalse(ImageFileType.isImageFile("archive.zip"));
        assertFalse(ImageFileType.isImageFile("video.mp4"));
        assertFalse(ImageFileType.isImageFile("audio.mp3"));
        assertFalse(ImageFileType.isImageFile("text.txt"));
        assertFalse(ImageFileType.isImageFile("code.java"));
    }

    @Test
    public void isImageFile_noExtension_returnsFalse() {
        assertFalse(ImageFileType.isImageFile("image"));
        assertFalse(ImageFileType.isImageFile("filename"));
    }

    @Test
    public void isImageFile_dotAtStart_returnsFalse() {
        // Hidden files like .gitignore are not images
        assertFalse(ImageFileType.isImageFile(".hidden"));
        assertFalse(ImageFileType.isImageFile(".jpg"));  // edge case: just extension as hidden file
    }

    @Test
    public void isImageFile_dotAtEnd_returnsFalse() {
        assertFalse(ImageFileType.isImageFile("image."));
        assertFalse(ImageFileType.isImageFile("filename."));
    }

    @Test
    public void isImageFile_nullInput_returnsFalse() {
        assertFalse(ImageFileType.isImageFile(null));
    }

    @Test
    public void isImageFile_emptyInput_returnsFalse() {
        assertFalse(ImageFileType.isImageFile(""));
        assertFalse(ImageFileType.isImageFile("   "));
    }

    @Test
    public void isImageFile_multipleDots_returnsCorrectResult() {
        // Should use last dot for extension
        assertTrue(ImageFileType.isImageFile("archive.tar.png"));
        assertTrue(ImageFileType.isImageFile("photo.backup.jpg"));
        assertFalse(ImageFileType.isImageFile("image.png.txt"));
    }

    @Test
    public void isImageFile_pathWithSlashes_returnsCorrectResult() {
        // Should still work with path separators
        assertTrue(ImageFileType.isImageFile("/path/to/image.jpg"));
        assertTrue(ImageFileType.isImageFile("folder/subfolder/photo.png"));
        assertTrue(ImageFileType.isImageFile("~/Pictures/avatar.gif"));
    }

    // ========== isImageExtension tests ==========

    @Test
    public void isImageExtension_jpg_returnsTrue() {
        assertTrue(ImageFileType.isImageExtension("jpg"));
    }

    @Test
    public void isImageExtension_jpeg_returnsTrue() {
        assertTrue(ImageFileType.isImageExtension("jpeg"));
    }

    @Test
    public void isImageExtension_png_returnsTrue() {
        assertTrue(ImageFileType.isImageExtension("png"));
    }

    @Test
    public void isImageExtension_gif_returnsTrue() {
        assertTrue(ImageFileType.isImageExtension("gif"));
    }

    @Test
    public void isImageExtension_webp_returnsTrue() {
        assertTrue(ImageFileType.isImageExtension("webp"));
    }

    @Test
    public void isImageExtension_bmp_returnsTrue() {
        assertTrue(ImageFileType.isImageExtension("bmp"));
    }

    @Test
    public void isImageExtension_svg_returnsTrue() {
        assertTrue(ImageFileType.isImageExtension("svg"));
    }

    @Test
    public void isImageExtension_uppercase_returnsTrue() {
        assertTrue(ImageFileType.isImageExtension("JPG"));
        assertTrue(ImageFileType.isImageExtension("PNG"));
        assertTrue(ImageFileType.isImageExtension("GIF"));
    }

    @Test
    public void isImageExtension_mixedCase_returnsTrue() {
        assertTrue(ImageFileType.isImageExtension("JpG"));
        assertTrue(ImageFileType.isImageExtension("PnG"));
    }

    @Test
    public void isImageExtension_withLeadingDot_returnsFalse() {
        // Extension should be without leading dot
        assertFalse(ImageFileType.isImageExtension(".jpg"));
        assertFalse(ImageFileType.isImageExtension(".png"));
    }

    @Test
    public void isImageExtension_unsupported_returnsFalse() {
        assertFalse(ImageFileType.isImageExtension("pdf"));
        assertFalse(ImageFileType.isImageExtension("txt"));
        assertFalse(ImageFileType.isImageExtension("mp4"));
    }

    @Test
    public void isImageExtension_nullInput_returnsFalse() {
        assertFalse(ImageFileType.isImageExtension(null));
    }

    @Test
    public void isImageExtension_emptyInput_returnsFalse() {
        assertFalse(ImageFileType.isImageExtension(""));
        assertFalse(ImageFileType.isImageExtension("   "));
    }

    // ========== getSupportedExtensions tests ==========

    @Test
    public void getSupportedExtensions_returnsAllFormats() {
        Set<String> extensions = ImageFileType.getSupportedExtensions();
        assertEquals(7, extensions.size());
        assertTrue(extensions.contains("jpg"));
        assertTrue(extensions.contains("jpeg"));
        assertTrue(extensions.contains("png"));
        assertTrue(extensions.contains("gif"));
        assertTrue(extensions.contains("webp"));
        assertTrue(extensions.contains("bmp"));
        assertTrue(extensions.contains("svg"));
    }

    @Test
    public void getSupportedExtensions_returnsNewSetInstance() {
        // Should return a copy, not the internal set
        Set<String> set1 = ImageFileType.getSupportedExtensions();
        Set<String> set2 = ImageFileType.getSupportedExtensions();
        assertNotSame(set1, set2);
        assertEquals(set1, set2);
    }

    @Test
    public void getSupportedExtensions_isImmutable() {
        Set<String> extensions = ImageFileType.getSupportedExtensions();
        // Modifying returned set should not affect internal state
        extensions.add("custom");
        Set<String> freshExtensions = ImageFileType.getSupportedExtensions();
        assertFalse(freshExtensions.contains("custom"));
        assertEquals(7, freshExtensions.size());
    }
}