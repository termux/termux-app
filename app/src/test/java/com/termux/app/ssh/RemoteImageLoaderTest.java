package com.termux.app.ssh;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;

import static org.junit.Assert.*;

/**
 * Unit tests for RemoteImageLoader utility class.
 *
 * Tests focus on:
 * - ImageLoadResult construction and state
 * - Base64 decode correctness
 * - Integration with ImageFileType
 *
 * Note: BitmapFactory tests are limited because Robolectric shadows don't
 * properly implement inJustDecodeBounds behavior (returns bitmap instead of null).
 * Full BitmapFactory behavior is tested on real devices via instrumentation tests.
 */
@RunWith(RobolectricTestRunner.class)
public class RemoteImageLoaderTest {

    // ========== ImageLoadResult tests ==========

    @Test
    public void ImageLoadResult_success_createsValidResult() {
        Bitmap mockBitmap = Bitmap.createBitmap(100, 100, Bitmap.Config.RGB_565);
        RemoteImageLoader.ImageLoadResult result = RemoteImageLoader.ImageLoadResult.success(
            mockBitmap, 100, 100, 5000, false, null
        );

        assertTrue(result.success);
        assertNotNull(result.bitmap);
        assertEquals(100, result.width);
        assertEquals(100, result.height);
        assertFalse(result.wasDownsampled);
        assertEquals(5000, result.fileSize);
        assertNull(result.warning);
        assertNull(result.errorMessage);
        assertEquals(0, result.exitCode.intValue());
    }

    @Test
    public void ImageLoadResult_success_withDownsampling_warningIncluded() {
        Bitmap mockBitmap = Bitmap.createBitmap(2048, 2048, Bitmap.Config.RGB_565);
        String warning = "Image too large, downsampling applied";
        RemoteImageLoader.ImageLoadResult result = RemoteImageLoader.ImageLoadResult.success(
            mockBitmap, 2048, 2048, 10000, true, warning
        );

        assertTrue(result.success);
        assertNotNull(result.bitmap);
        assertTrue(result.wasDownsampled);
        assertNotNull(result.warning);
        assertEquals("Image too large, downsampling applied", result.warning);
    }

    @Test
    public void ImageLoadResult_failure_createsValidResult() {
        String errorMsg = "Connection failed";
        RemoteImageLoader.ImageLoadResult result = RemoteImageLoader.ImageLoadResult.failure(
            errorMsg, 127, 0
        );

        assertFalse(result.success);
        assertNull(result.bitmap);
        assertEquals("Connection failed", result.errorMessage);
        assertEquals(127, result.exitCode.intValue());
        assertEquals(0, result.width);
        assertEquals(0, result.height);
        assertFalse(result.wasDownsampled);
    }

    @Test
    public void ImageLoadResult_failure_nullExitCode() {
        RemoteImageLoader.ImageLoadResult result = RemoteImageLoader.ImageLoadResult.failure(
            "Socket not found", null, 0
        );

        assertFalse(result.success);
        assertNull(result.exitCode);
    }

    @Test
    public void ImageLoadResult_tooLarge_createsValidResult() {
        String warning = "Image exceeds 4096x4096 limit (8000x6000)";
        RemoteImageLoader.ImageLoadResult result = RemoteImageLoader.ImageLoadResult.tooLarge(
            8000, 6000, 50000, warning
        );

        assertFalse(result.success);
        assertNull(result.bitmap);
        assertEquals(8000, result.width);
        assertEquals(6000, result.height);
        assertEquals(50000, result.fileSize);
        assertNotNull(result.warning);
    }

    @Test
    public void ImageLoadResult_toString_containsKeyFields() {
        Bitmap mockBitmap = Bitmap.createBitmap(100, 200, Bitmap.Config.RGB_565);
        RemoteImageLoader.ImageLoadResult result = RemoteImageLoader.ImageLoadResult.success(
            mockBitmap, 100, 200, 5000, false, null
        );

        String str = result.toString();
        assertTrue(str.contains("success=true"));
        assertTrue(str.contains("width=100"));
        assertTrue(str.contains("height=200"));
        assertTrue(str.contains("fileSize=5000"));
        assertTrue(str.contains("wasDownsampled=false"));
    }

    @Test
    public void ImageLoadResult_toString_truncatesLongErrorMessage() {
        // Create a message longer than 100 chars
        StringBuilder longError = new StringBuilder();
        for (int i = 0; i < 20; i++) {
            longError.append("abcdefghij"); // 10 chars each, total 200
        }
        
        RemoteImageLoader.ImageLoadResult result = RemoteImageLoader.ImageLoadResult.failure(
            longError.toString(), 1, 100
        );

        String str = result.toString();
        assertTrue(str.contains("success=false"));
        // The truncated message should end with "..." 
        assertTrue(str.contains("..."));
        // The original 200-char message should be truncated
        assertFalse(str.contains("abcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghij"));
    }

    // ========== Base64 decode correctness tests ==========

    @Test
    public void base64Decode_simpleData_decodesCorrectly() {
        // Test data: "Hello World" in base64
        String base64Data = "SGVsbG8gV29ybGQ=";
        byte[] decoded = android.util.Base64.decode(base64Data, android.util.Base64.NO_WRAP);

        String decodedString = new String(decoded);
        assertEquals("Hello World", decodedString);
    }

    @Test
    public void base64Decode_imageData_roundtripCorrect() {
        // Create test data - bytes need explicit cast for values > 127
        byte[] testData = new byte[] {
            (byte)0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, (byte)0x1A, (byte)0x0A,
            0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
            0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
            0x08, 0x06, 0x00, 0x00, 0x00,
            0x1F, 0x15, (byte)0xC4, (byte)0x89
        };

        // Encode then decode
        String encoded = android.util.Base64.encodeToString(testData, android.util.Base64.NO_WRAP);
        byte[] decoded = android.util.Base64.decode(encoded, android.util.Base64.NO_WRAP);

        // Verify roundtrip
        assertEquals(testData.length, decoded.length);
        for (int i = 0; i < testData.length; i++) {
            assertEquals("Mismatch at index " + i, testData[i], decoded[i]);
        }
    }

    @Test
    public void base64Decode_emptyData_returnsEmptyArray() {
        String base64Data = "";
        byte[] decoded = android.util.Base64.decode(base64Data, android.util.Base64.NO_WRAP);
        assertEquals(0, decoded.length);
    }

    @Test
    public void base64Decode_invalidData_mayReturnGarbage() {
        // Android's Base64.decode is lenient - it may not throw for invalid data
        // but instead return garbage or empty data
        String invalidBase64 = "!!!\0\0\0!!!";
        byte[] decoded = android.util.Base64.decode(invalidBase64, android.util.Base64.NO_WRAP);
        // The result may be empty or garbage - just verify it doesn't crash
        assertNotNull(decoded);
    }

    // ========== BitmapFactory tests (limited due to Robolectric behavior) ==========

    @Test
    public void BitmapFactory_decodeByteArray_createsBitmap() {
        // Robolectric shadow returns a bitmap for valid PNG data
        Bitmap testBitmap = Bitmap.createBitmap(100, 50, Bitmap.Config.RGB_565);
        byte[] imageData = compressBitmapToBytes(testBitmap, Bitmap.CompressFormat.PNG);

        Bitmap decodedBitmap = BitmapFactory.decodeByteArray(imageData, 0, imageData.length);

        // On real device this would work; Robolectric shadow returns a bitmap
        assertNotNull(decodedBitmap);
    }

    @Test
    public void BitmapFactory_inSampleSize_reducesBitmapSize() {
        // Create 200x200 test bitmap
        Bitmap testBitmap = Bitmap.createBitmap(200, 200, Bitmap.Config.RGB_565);
        byte[] imageData = compressBitmapToBytes(testBitmap, Bitmap.CompressFormat.PNG);

        // Decode with inSampleSize=2
        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inSampleSize = 2;
        Bitmap decodedBitmap = BitmapFactory.decodeByteArray(imageData, 0, imageData.length, options);

        assertNotNull(decodedBitmap);
        // inSampleSize=2 should reduce each dimension by factor of 2
        assertTrue(decodedBitmap.getWidth() <= 100);
        assertTrue(decodedBitmap.getHeight() <= 100);
    }

    // ========== ImageFileType integration tests ==========

    @Test
    public void ImageFileType_isImageFile_worksForRemotePath() {
        // Verify ImageFileType works with remote paths (which may include slashes)
        assertTrue(ImageFileType.isImageFile("/home/user/photo.jpg"));
        assertTrue(ImageFileType.isImageFile("/var/www/images/banner.png"));
        assertTrue(ImageFileType.isImageFile("~/Downloads/picture.gif"));
        assertFalse(ImageFileType.isImageFile("/etc/config.conf"));
        assertFalse(ImageFileType.isImageFile("/var/log/app.log"));
    }

    @Test
    public void ImageFileType_isImageFile_allSupportedFormats() {
        // Verify all formats supported by RemoteImageLoader
        assertTrue(ImageFileType.isImageFile("image.jpg"));
        assertTrue(ImageFileType.isImageFile("image.jpeg"));
        assertTrue(ImageFileType.isImageFile("image.png"));
        assertTrue(ImageFileType.isImageFile("image.gif"));
        assertTrue(ImageFileType.isImageFile("image.webp"));
        assertTrue(ImageFileType.isImageFile("image.bmp"));
        assertTrue(ImageFileType.isImageFile("image.svg"));
    }

    // ========== Private helper methods ==========

    /**
     * Compress a Bitmap to byte array for testing.
     */
    private byte[] compressBitmapToBytes(Bitmap bitmap, Bitmap.CompressFormat format) {
        java.io.ByteArrayOutputStream baos = new java.io.ByteArrayOutputStream();
        bitmap.compress(format, 100, baos);
        return baos.toByteArray();
    }
}