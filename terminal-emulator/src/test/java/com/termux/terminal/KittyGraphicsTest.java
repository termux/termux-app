package com.termux.terminal;

import java.util.Arrays;
import java.util.Base64;
import java.util.HashSet;
import java.util.Set;

/**
 * Tests for the kitty graphics protocol `APC _G` commands.
 *
 * - https://sw.kovidgoyal.net/kitty/graphics-protocol/
 *
 * Note that the android `Bitmap` and `BitmapFactory` classes are not available in unit tests since
 * `unitTests.returnDefaultValues` is enabled for the `terminal-emulator` module, which makes
 * `Bitmap.createBitmap()` return `null`, so no image can ever actually be placed on the screen here.
 * Commands that display an image therefore always respond with an `EBADPNG` error in these tests,
 * while the same commands respond with `OK` on a device. Everything up to creating the bitmap does
 * not depend on the android platform and is covered, and the code that operates on placements after
 * they have been created is covered by creating the placements directly instead, check
 * {@link #testDeleteKittyImagePlacementsClearsTheCellsOfThePlacements()}.
 */
public class KittyGraphicsTest extends TerminalTestCase {

    /** A valid `1x1` `PNG` image, `base64` encoded, which decodes to 70 bytes. */
    private static final String PNG_BASE64 =
        "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8z8DwHwAFAAH/q842iQAAAABJRU5ErkJggg==";

    /** The length of the image data {@link #PNG_BASE64} decodes to. */
    private static final int PNG_LENGTH = 70;

    /** The first chunk of {@link #PNG_BASE64}, whose length is a multiple of `4` as required. */
    private static final String PNG_BASE64_CHUNK_0 = PNG_BASE64.substring(0, 40);
    private static final String PNG_BASE64_CHUNK_1 = PNG_BASE64.substring(40);

    /** Raw `RGB` (`f=24`) pixel data for a `1x1` and a `2x2` image, `base64` encoded. */
    private static final String RGB_BASE64_1X1 = "AQID";
    private static final String RGB_BASE64_2X2 = "AQIDBAUGBwgJCgsM";

    /** Raw `RGBA` (`f=32`) pixel data for a `1x1` and a `2x2` image, `base64` encoded. */
    private static final String RGBA_BASE64_1X1 = "AQIDBA==";
    private static final String RGBA_BASE64_2X2 = "AQIDBAUGBwgJCgsMDQ4PEA==";



    /* Capability queries (`a=q`). */

    public void testCapabilityQueryRespondsOk() {
        // The exact string yazi and the kitty documentation use for probing terminal support.
        withTerminalSized(10, 5);
        assertEnteringStringGivesResponse(
            "\033_Gi=31,s=1,v=1,a=q,t=d,f=24;AAAA\033\\",
            "\033_Gi=31;OK\033\\");
    }

    public void testCapabilityQueryDoesNotPaintScreen() {
        withTerminalSized(2, 2);
        assertEnteringStringGivesResponse(
            "\033_Gi=31,s=1,v=1,a=q,t=d,f=24;AAAA\033\\",
            "\033_Gi=31;OK\033\\");
        assertLinesAre("  ", "  ");
    }

    public void testCapabilityQueryWithoutImageIdIsSilent() {
        // A response cannot be matched with the command that caused it without an image id.
        withTerminalSized(10, 5);
        assertEnteringStringGivesResponse("\033_Ga=q,t=d,f=24;AAAA\033\\", "");
    }

    public void testCapabilityQueryWithUnsupportedTransmissionMediumRespondsError() {
        withTerminalSized(10, 5);
        assertEnteringStringGivesResponse(
            "\033_Gi=31,s=1,v=1,a=q,t=f,f=24;L3RtcC9pbWc=\033\\",
            "\033_Gi=31;ENOTSUP:unsupported transmission medium\033\\");
    }

    public void testCapabilityQueryValidatesTheImageData() {
        // A capability query must not blindly respond with `OK`, since a client uses the response to
        // decide what it can send.
        withTerminalSized(10, 5);

        // The pixel dimensions of the raw formats must match the length of the image data.
        assertEnteringStringGivesResponse(
            "\033_Gi=31,s=2,v=2,a=q,t=d,f=24;AAAA\033\\",
            "\033_Gi=31;EINVAL:image data of length 3 does not match the 12 bytes required for pixel dimensions 2x2\033\\");

        assertEnteringStringGivesResponse(
            "\033_Gi=31,a=q,t=d,f=32;" + RGBA_BASE64_1X1 + "\033\\",
            "\033_Gi=31;EINVAL:missing pixel dimensions for image data of length 4\033\\");

        // Compressed image data is not supported.
        assertEnteringStringGivesResponse(
            "\033_Gi=31,s=1,v=1,a=q,t=d,f=24,o=z;AAAA\033\\",
            "\033_Gi=31;ENOTSUP:unsupported compression\033\\");

        // The image data must be valid `base64`.
        assertEnteringStringGivesResponse(
            "\033_Gi=31,s=1,v=1,a=q,t=d,f=24;AA!A\033\\",
            "\033_Gi=31;EBADDATA:invalid base64 image data\033\\");
    }

    public void testCapabilityQueryDoesNotStoreTheImage() {
        withTerminalSized(10, 5);
        assertEnteringStringGivesResponse(
            "\033_Gi=31,s=1,v=1,a=q,t=d,f=24;" + RGB_BASE64_1X1 + "\033\\",
            "\033_Gi=31;OK\033\\");
        assertEquals(-1, mTerminal.getKittyImageDataLength(31));
    }


    /* Quiet levels (`q=`). */

    public void testQuietLevelsForSuccessResponse() {
        withTerminalSized(10, 5);

        // `q` not passed, so respond.
        assertEnteringStringGivesResponse(
            "\033_Ga=t,f=100,t=d,i=1,m=0;" + PNG_BASE64 + "\033\\",
            "\033_Gi=1;OK\033\\");

        // `q=1` suppresses success responses.
        assertEnteringStringGivesResponse(
            "\033_Ga=t,f=100,t=d,i=1,m=0,q=1;" + PNG_BASE64 + "\033\\", "");

        // `q=2` suppresses success and error responses.
        assertEnteringStringGivesResponse(
            "\033_Ga=t,f=100,t=d,i=1,m=0,q=2;" + PNG_BASE64 + "\033\\", "");
    }

    public void testQuietLevelsForErrorResponse() {
        withTerminalSized(10, 5);

        // `q` not passed, so respond.
        assertEnteringStringGivesResponse(
            "\033_Ga=t,f=100,t=d,i=2,m=0,o=z;" + PNG_BASE64 + "\033\\",
            "\033_Gi=2;ENOTSUP:unsupported compression\033\\");

        // `q=1` only suppresses success responses, so still respond.
        assertEnteringStringGivesResponse(
            "\033_Ga=t,f=100,t=d,i=2,m=0,o=z,q=1;" + PNG_BASE64 + "\033\\",
            "\033_Gi=2;ENOTSUP:unsupported compression\033\\");

        // `q=2` suppresses success and error responses.
        assertEnteringStringGivesResponse(
            "\033_Ga=t,f=100,t=d,i=2,m=0,o=z,q=2;" + PNG_BASE64 + "\033\\", "");
    }

    public void testResponseDoesNotDependOnTheOrderOfTheControlDataKeys() {
        // All the control data is read even if a key is invalid, so that the `i` and `q` keys
        // required to send the response are read regardless of the order the keys are passed in.
        withTerminalSized(10, 5);

        assertEnteringStringGivesResponse("\033_Gq=2,o=z,i=3,a=t,f=100,m=0;" + PNG_BASE64 + "\033\\", "");
        assertEnteringStringGivesResponse("\033_Go=z,q=2,i=3,a=t,f=100,m=0;" + PNG_BASE64 + "\033\\", "");
        assertEnteringStringGivesResponse("\033_Go=z,i=3,q=2,a=t,f=100,m=0;" + PNG_BASE64 + "\033\\", "");

        assertEnteringStringGivesResponse("\033_Go=z,i=3,a=t,f=100,m=0;" + PNG_BASE64 + "\033\\",
            "\033_Gi=3;ENOTSUP:unsupported compression\033\\");
    }


    /* Transmitting images (`a=t`). */

    public void testTransmitStoresImageData() {
        withTerminalSized(10, 5);
        assertEnteringStringGivesResponse(
            "\033_Ga=t,f=100,t=d,i=424242,m=0,q=2;" + PNG_BASE64 + "\033\\", "");
        assertEquals(PNG_LENGTH, mTerminal.getKittyImageDataLength(424242));
    }

    public void testTransmitWithoutImageIdDoesNotStoreImageData() {
        withTerminalSized(10, 5);
        assertEnteringStringGivesResponse("\033_Ga=t,f=100,t=d,m=0;" + PNG_BASE64 + "\033\\", "");
        assertEquals(-1, mTerminal.getKittyImageDataLength(0));
    }

    public void testTransmitDoesNotPaintScreen() {
        withTerminalSized(4, 2)
            .enterString("\033_Ga=t,f=100,t=d,i=1,m=0,q=2;" + PNG_BASE64 + "\033\\")
            .enterString("ok")
            .assertLinesAre("ok  ", "    ");
    }

    public void testTransmitAndDisplayDoesNotCrashWhenBitmapCannotBeCreated() {
        // The android `BitmapFactory` is stubbed out in unit tests, so the image cannot be decoded.
        withTerminalSized(20, 4);
        assertEnteringStringGivesResponse(
            "\033_Ga=T,f=100,t=d,i=424242,m=0,c=4,r=2;" + PNG_BASE64 + "\033\\",
            "\033_Gi=424242;EBADPNG:displaying image failed\033\\");

        // The image data is still stored and the screen and cursor are left untouched.
        assertEquals(PNG_LENGTH, mTerminal.getKittyImageDataLength(424242));
        assertCursorAt(0, 0);
        enterString("ok").assertLinesAre("ok                  ", "                    ",
            "                    ", "                    ");
    }

    /* Raw pixel data formats (`f=24` and `f=32`). */

    public void testTransmitRawRgbAndRgbaImages() {
        withTerminalSized(20, 4);

        assertEnteringStringGivesResponse(
            "\033_Ga=t,f=24,t=d,i=100,s=1,v=1,m=0;" + RGB_BASE64_1X1 + "\033\\",
            "\033_Gi=100;OK\033\\");
        assertEquals(3, mTerminal.getKittyImageDataLength(100));

        assertEnteringStringGivesResponse(
            "\033_Ga=t,f=24,t=d,i=101,s=2,v=2,m=0;" + RGB_BASE64_2X2 + "\033\\",
            "\033_Gi=101;OK\033\\");
        assertEquals(12, mTerminal.getKittyImageDataLength(101));

        assertEnteringStringGivesResponse(
            "\033_Ga=t,f=32,t=d,i=102,s=1,v=1,m=0;" + RGBA_BASE64_1X1 + "\033\\",
            "\033_Gi=102;OK\033\\");
        assertEquals(4, mTerminal.getKittyImageDataLength(102));

        assertEnteringStringGivesResponse(
            "\033_Ga=t,f=32,t=d,i=103,s=2,v=2,m=0;" + RGBA_BASE64_2X2 + "\033\\",
            "\033_Gi=103;OK\033\\");
        assertEquals(16, mTerminal.getKittyImageDataLength(103));
    }

    public void testRawImageDataLengthMustMatchThePixelDimensions() {
        withTerminalSized(20, 4);

        // `RGB` requires 3 bytes per pixel and `RGBA` requires 4, so data that is too short is
        // rejected once all of it has been received. Data that is too long is rejected as soon as it
        // arrives instead, check
        // `testRawImageWithMoreDataThanTheDeclaredDimensionsIsRejected()`.
        assertEnteringStringGivesResponse(
            "\033_Ga=t,f=32,t=d,i=104,s=2,v=2,m=0;" + RGB_BASE64_2X2 + "\033\\",
            "\033_Gi=104;EINVAL:image data of length 12 does not match the 16 bytes required for pixel dimensions 2x2\033\\");

        // The `s` and `v` keys are required for the raw formats.
        assertEnteringStringGivesResponse(
            "\033_Ga=t,f=24,t=d,i=104,m=0;" + RGB_BASE64_2X2 + "\033\\",
            "\033_Gi=104;EINVAL:missing pixel dimensions for image data of length 12\033\\");

        assertEnteringStringGivesResponse(
            "\033_Ga=t,f=24,t=d,i=104,s=2,m=0;" + RGB_BASE64_2X2 + "\033\\",
            "\033_Gi=104;EINVAL:missing pixel dimensions for image data of length 12\033\\");

        assertEquals(-1, mTerminal.getKittyImageDataLength(104));
    }

    public void testRawImageWithoutFormatKeyUsesTheProtocolDefaultFormat() {
        // The protocol defines `f=32` (raw `RGBA`) as the default if the `f` key is not passed.
        withTerminalSized(20, 4);
        assertEnteringStringGivesResponse(
            "\033_Ga=t,t=d,i=105,s=1,v=1,m=0;" + RGBA_BASE64_1X1 + "\033\\",
            "\033_Gi=105;OK\033\\");
        assertEquals(4, mTerminal.getKittyImageDataLength(105));
    }

    public void testRawImageExceedingTheMaxImageDataLengthIsRejected() {
        withTerminalSized(20, 4);

        // The pixel dimensions are known before the image data is received for the raw formats, so
        // the limit is checked without having to receive all of the data first. The max pixel
        // dimensions of `32766x32766` `RGBA` are `4.29GB` of pixel data, which also checks that
        // calculating the length does not overflow.
        assertEnteringStringGivesResponse(
            "\033_Ga=t,f=32,t=d,i=106,s=32766,v=32766,m=0;" + RGBA_BASE64_1X1 + "\033\\",
            "\033_Gi=106;ENOSPC:image data exceeds max length " + KittyImage.IMAGE_DATA__MAX_LENGTH + "\033\\");
        assertEquals(-1, mTerminal.getKittyImageDataLength(106));
    }

    public void testMaxImageDataLengthIsDerivedFromTheMaxBitmapSize() {
        // The limit must match the largest bitmap the terminal can draw, so that an image that could
        // have been displayed is not rejected while it is being received. The limit is in bytes of
        // decoded image data, since the `base64` payload is decoded chunk by chunk instead of being
        // collected as text and decoded at the end.
        assertEquals(TerminalBitmap.MAX_BITMAP_SIZE, KittyImage.IMAGE_DATA__MAX_LENGTH);
        assertTrue(KittyImage.IMAGE_DATA__MAX_LENGTH > 0);

        // A full screen image on a high resolution device must be within the limit, which is
        // `1080x2400` raw `RGBA` for a portrait device.
        assertTrue(1080 * 2400 * 4 <= KittyImage.IMAGE_DATA__MAX_LENGTH);
    }

    /**
     * Decode a `base64` payload that is split into chunks with {@link KittyImage} directly, so that
     * the decoded bytes themselves can be checked instead of only their length. The chunks are
     * processed the same way {@link TerminalEmulator} processes the commands for them.
     *
     * @return Returns the decoded image data, or `null` if it was rejected.
     */
    private byte[] decodeChunkedPayload(String controlData, String... chunks) {
        KittyImage kittyImage = null;

        for (int i = 0; i < chunks.length; i++) {
            boolean isFinalChunk = i == chunks.length - 1;
            StringBuilder apcArgs = new StringBuilder("G" +
                (i == 0 ? controlData + "," : "") + "m=" + (isFinalChunk ? 0 : 1) + ";" + chunks[i]);

            KittyImage chunk = new KittyImage(null);
            int payloadIndex = chunk.readControlData(apcArgs, /* `G` */ 1);
            assertTrue("The control data of chunk " + i + " was rejected", payloadIndex >= 0);

            if (kittyImage == null) {
                kittyImage = chunk;
            } else {
                kittyImage.readContinuationControlData(chunk);
            }

            if (!kittyImage.startImage()) return null;
            for (int index = payloadIndex; index < apcArgs.length(); index++) {
                if (!kittyImage.readImageChar(apcArgs.charAt(index))) return null;
            }
        }

        return kittyImage.finishImage() ? kittyImage.getDecodedImage() : null;
    }

    public void testChunkBoundariesThatAreNotAMultipleOfFourDecodeToTheSameBytes() {
        // Each 4 `base64` characters decode to 3 bytes, so a chunk whose length is not a multiple of
        // 4 leaves bits that must be carried over to the next chunk. The result is compared against
        // the JDK `base64` decoder as an independent reference implementation.
        byte[] expected = Base64.getDecoder().decode(PNG_BASE64);
        assertEquals(PNG_LENGTH, expected.length);

        // The padding is only valid at the very end of the data, so the offsets that would put it in
        // a chunk that is not the final chunk are excluded, which is checked by
        // `testBase64PaddingInANonFinalChunkIsRejected()` instead.
        int paddingIndex = PNG_BASE64.indexOf('=');
        assertTrue(paddingIndex > 0);

        for (int splitIndex = 1; splitIndex < paddingIndex; splitIndex++) {
            byte[] decoded = decodeChunkedPayload("a=t,f=100,t=d",
                PNG_BASE64.substring(0, splitIndex), PNG_BASE64.substring(splitIndex));
            assertNotNull("Split at index " + splitIndex, decoded);
            assertTrue("Split at index " + splitIndex, Arrays.equals(expected, decoded));
        }

        // Three chunks, so that a chunk that is neither the first nor the last carries bits over on
        // both of its boundaries.
        for (int splitIndex = 2; splitIndex < paddingIndex; splitIndex++) {
            byte[] decoded = decodeChunkedPayload("a=t,f=100,t=d",
                PNG_BASE64.substring(0, 1), PNG_BASE64.substring(1, splitIndex), PNG_BASE64.substring(splitIndex));
            assertNotNull("Split at indexes 1 and " + splitIndex, decoded);
            assertTrue("Split at indexes 1 and " + splitIndex, Arrays.equals(expected, decoded));
        }

        // A chunk of a single character at every offset.
        for (int splitIndex = 1; splitIndex < paddingIndex; splitIndex++) {
            byte[] decoded = decodeChunkedPayload("a=t,f=100,t=d",
                PNG_BASE64.substring(0, splitIndex), PNG_BASE64.substring(splitIndex, splitIndex + 1),
                PNG_BASE64.substring(splitIndex + 1));
            assertNotNull("Single character chunk at index " + splitIndex, decoded);
            assertTrue("Single character chunk at index " + splitIndex, Arrays.equals(expected, decoded));
        }
    }

    public void testChunkBoundariesOfRawPixelDataDecodeToTheSameBytes() {
        byte[] expected = Base64.getDecoder().decode(RGB_BASE64_2X2);
        assertEquals(12, expected.length);

        // `RGB_BASE64_2X2` has no padding, so every offset can be split at.
        for (int splitIndex = 1; splitIndex < RGB_BASE64_2X2.length(); splitIndex++) {
            byte[] decoded = decodeChunkedPayload("a=t,f=24,t=d,s=2,v=2",
                RGB_BASE64_2X2.substring(0, splitIndex), RGB_BASE64_2X2.substring(splitIndex));
            assertNotNull("Split at index " + splitIndex, decoded);
            assertTrue("Split at index " + splitIndex, Arrays.equals(expected, decoded));
        }
    }

    public void testChunkBoundariesThroughTheTerminal() {
        // The same as above, but through the actual commands, which also checks that the image is
        // stored with the assembled length.
        int paddingIndex = PNG_BASE64.indexOf('=');
        for (int splitIndex = 1; splitIndex < paddingIndex; splitIndex++) {
            withTerminalSized(20, 4);
            enterString("\033_Ga=t,f=100,t=d,i=400,m=1,q=2;" + PNG_BASE64.substring(0, splitIndex) + "\033\\");
            assertEnteringStringGivesResponse(
                "\033_Gm=0,q=2;" + PNG_BASE64.substring(splitIndex) + "\033\\", "");
            assertEquals("Split at index " + splitIndex, PNG_LENGTH, mTerminal.getKittyImageDataLength(400));
        }

        for (int splitIndex = 1; splitIndex < RGBA_BASE64_2X2.indexOf('='); splitIndex++) {
            withTerminalSized(20, 4);
            enterString("\033_Ga=t,f=32,t=d,i=401,s=2,v=2,m=1,q=2;" + RGBA_BASE64_2X2.substring(0, splitIndex) + "\033\\");
            enterString("\033_Gm=0,q=2;" + RGBA_BASE64_2X2.substring(splitIndex) + "\033\\");
            assertEquals("Split at index " + splitIndex, 16, mTerminal.getKittyImageDataLength(401));
        }
    }

    public void testBase64PaddingInANonFinalChunkIsRejected() {
        withTerminalSized(20, 4);

        // The padding is only valid at the very end of the `base64` data, so a chunk that is not the
        // final chunk must not contain any.
        assertEnteringStringGivesResponse(
            "\033_Ga=t,f=32,t=d,i=402,s=1,v=1,m=1;" + RGBA_BASE64_1X1 + "\033\\",
            "\033_Gi=402;EBADDATA:base64 padding in a chunk that is not the final chunk\033\\");
        assertEquals(-1, mTerminal.getKittyImageDataLength(402));

        // The same data is accepted if it is the final chunk.
        assertEnteringStringGivesResponse(
            "\033_Ga=t,f=32,t=d,i=402,s=1,v=1,m=0;" + RGBA_BASE64_1X1 + "\033\\",
            "\033_Gi=402;OK\033\\");
        assertEquals(4, mTerminal.getKittyImageDataLength(402));
    }

    public void testBase64DataAfterThePaddingIsRejected() {
        withTerminalSized(20, 4);

        // Within a single chunk.
        assertEnteringStringGivesResponse("\033_Ga=t,f=24,t=d,i=403,s=1,v=1,m=0;QQ==QQ==\033\\",
            "\033_Gi=403;EBADDATA:base64 data after the padding\033\\");

        // And across chunks, where the first chunk ended with the padding.
        enterString("\033_Ga=t,f=24,t=d,i=403,s=1,v=1,m=1;AQI\033\\");
        assertEnteringStringGivesResponse("\033_Gm=0;D" + "\033\\", "\033_Gi=403;OK\033\\");
    }

    public void testInvalidBase64InAnyChunkIsRejected() {
        withTerminalSized(20, 4);

        // The strict validation must be kept for every chunk, not only the final one.
        assertEnteringStringGivesResponse("\033_Ga=t,f=100,t=d,i=404,m=1;AA!A\033\\",
            "\033_Gi=404;EBADDATA:invalid base64 image data\033\\");
        assertEquals(-1, mTerminal.getKittyImageDataLength(404));

        // A total number of characters that leaves a single character in the final quad.
        enterString("\033_Ga=t,f=100,t=d,i=404,m=1;AAAA\033\\");
        assertEnteringStringGivesResponse("\033_Gm=0;A\033\\",
            "\033_Gi=404;EBADDATA:invalid base64 image data\033\\");
        assertEquals(-1, mTerminal.getKittyImageDataLength(404));
    }

    public void testRawImageWithMoreDataThanTheDeclaredDimensionsIsRejected() {
        withTerminalSized(20, 4);

        // The extra data is rejected as soon as it arrives, so that the memory for it is never
        // allocated for an image that cannot be displayed anyway.
        assertEnteringStringGivesResponse(
            "\033_Ga=t,f=24,t=d,i=405,s=2,v=2,m=0;" + RGBA_BASE64_2X2 + "\033\\",
            "\033_Gi=405;EINVAL:image data exceeds the 12 bytes required for pixel dimensions 2x2\033\\");
        assertEquals(-1, mTerminal.getKittyImageDataLength(405));
    }

    public void testRawImageMultiChunkTransmitAssemblesPayload() {
        withTerminalSized(20, 4);

        // `RGB_BASE64_2X2` split at 8 characters, which is a multiple of 4 as required.
        assertEnteringStringGivesResponse(
            "\033_Ga=t,f=24,t=d,i=107,s=2,v=2,m=1;" + RGB_BASE64_2X2.substring(0, 8) + "\033\\", "");
        assertEnteringStringGivesResponse(
            "\033_Gm=0;" + RGB_BASE64_2X2.substring(8) + "\033\\", "\033_Gi=107;OK\033\\");
        assertEquals(12, mTerminal.getKittyImageDataLength(107));
    }


    public void testTransmitAndDisplayWithOnlyColumnsDoesNotHang() {
        // A `c` key without an `r` key results in the aspect ratio of the image being preserved,
        // which requires the image dimensions to be known.
        withTerminalSized(20, 4);
        assertEnteringStringGivesResponse(
            "\033_Ga=T,f=100,t=d,i=1,m=0,c=4;" + PNG_BASE64 + "\033\\",
            "\033_Gi=1;EBADPNG:displaying image failed\033\\");
        assertEnteringStringGivesResponse(
            "\033_Ga=T,f=100,t=d,i=1,m=0,r=2;" + PNG_BASE64 + "\033\\",
            "\033_Gi=1;EBADPNG:displaying image failed\033\\");
        assertEnteringStringGivesResponse(
            "\033_Ga=T,f=100,t=d,i=1,m=0;" + PNG_BASE64 + "\033\\",
            "\033_Gi=1;EBADPNG:displaying image failed\033\\");
    }


    /* Chunked transmission (`m=1`/`m=0`). */

    public void testMultiChunkTransmitAssemblesPayload() {
        withTerminalSized(20, 4);

        // No response and nothing stored until the final chunk is received.
        assertEnteringStringGivesResponse(
            "\033_Ga=t,f=100,t=d,i=7,m=1,q=2;" + PNG_BASE64_CHUNK_0 + "\033\\", "");
        assertEquals(-1, mTerminal.getKittyImageDataLength(7));

        assertEnteringStringGivesResponse("\033_Gm=0,q=2;" + PNG_BASE64_CHUNK_1 + "\033\\", "");
        assertEquals(PNG_LENGTH, mTerminal.getKittyImageDataLength(7));
    }

    public void testMultiChunkTransmitRespondsOnlyAfterFinalChunk() {
        withTerminalSized(20, 4);
        assertEnteringStringGivesResponse(
            "\033_Ga=t,f=100,t=d,i=7,m=1;" + PNG_BASE64_CHUNK_0 + "\033\\", "");
        assertEnteringStringGivesResponse(
            "\033_Gm=0;" + PNG_BASE64_CHUNK_1 + "\033\\", "\033_Gi=7;OK\033\\");
    }

    public void testMultiChunkTransmitWithoutFinalChunkIsNotProcessed() {
        withTerminalSized(20, 4);
        assertEnteringStringGivesResponse(
            "\033_Ga=t,f=100,t=d,i=7,m=1;" + PNG_BASE64_CHUNK_0 + "\033\\", "");

        // The image is never completed, so it is never stored and no response is ever sent.
        assertEquals(-1, mTerminal.getKittyImageDataLength(7));
        enterString("ok").assertLinesAre("ok                  ", "                    ",
            "                    ", "                    ");
        assertEquals("", mOutput.getOutputAndClear());
    }

    public void testInterleavedTextDuringChunkedTransferDoesNotCorruptState() {
        withTerminalSized(20, 4);
        enterString("\033_Ga=t,f=100,t=d,i=7,m=1,q=2;" + PNG_BASE64_CHUNK_0 + "\033\\");

        // Printing text between the chunks of an image is a protocol violation, but the text must
        // still be printed and the image must still be received correctly.
        enterString("hello");
        enterString("\033_Gm=0,q=2;" + PNG_BASE64_CHUNK_1 + "\033\\");

        assertLinesAre("hello               ", "                    ",
            "                    ", "                    ");
        assertEquals(PNG_LENGTH, mTerminal.getKittyImageDataLength(7));
        assertEquals("", mOutput.getOutputAndClear());
    }

    public void testInterleavedEscapeSequenceDuringChunkedTransferDoesNotCorruptState() {
        withTerminalSized(20, 4);
        enterString("\033_Ga=t,f=100,t=d,i=7,m=1,q=2;" + PNG_BASE64_CHUNK_0 + "\033\\");

        // A `CSI` sequence, an unsupported `APC` command and an `OSC` command between the chunks.
        enterString("\033[2;3H\033_unrelated\033\\\033]0;title\033\\");
        enterString("\033_Gm=0,q=2;" + PNG_BASE64_CHUNK_1 + "\033\\");

        assertCursorAt(1, 2);
        assertEquals(PNG_LENGTH, mTerminal.getKittyImageDataLength(7));
        assertEquals("", mOutput.getOutputAndClear());
    }

    public void testNewCommandDuringChunkedTransferDropsIncompleteImage() {
        withTerminalSized(20, 4);
        enterString("\033_Ga=t,f=100,t=d,i=7,m=1,q=2;" + PNG_BASE64_CHUNK_0 + "\033\\");

        // A command that passes the `a` key is always a new command, never a continuation chunk.
        assertEnteringStringGivesResponse(
            "\033_Ga=t,f=100,t=d,i=8,m=0,q=2;" + PNG_BASE64 + "\033\\", "");

        assertEquals(-1, mTerminal.getKittyImageDataLength(7));
        assertEquals(PNG_LENGTH, mTerminal.getKittyImageDataLength(8));
    }

    public void testContinuationChunkWithUnexpectedKeysIgnoresThemGracefully() {
        withTerminalSized(20, 4);
        enterString("\033_Ga=t,f=100,t=d,i=7,m=1,q=2;" + PNG_BASE64_CHUNK_0 + "\033\\");

        // A continuation chunk that re-passes keys is a protocol violation. Since it passes the `a`
        // key it is treated as a new command, which drops the incomplete image, and the payload of
        // the new command is not valid image data on its own. The `f` key is not passed either, so
        // the protocol default of raw `RGBA` pixel data applies, whose dimensions are missing.
        assertEnteringStringGivesResponse(
            "\033_Ga=t,i=999,m=0;" + PNG_BASE64_CHUNK_1 + "\033\\",
            "\033_Gi=999;EINVAL:missing pixel dimensions for image data of length 40\033\\");

        assertEquals(-1, mTerminal.getKittyImageDataLength(7));
        assertEquals(-1, mTerminal.getKittyImageDataLength(999));
        enterString("ok").assertLinesAre("ok                  ", "                    ",
            "                    ", "                    ");
    }

    public void testContinuationChunkWithoutPendingImageIsIgnored() {
        withTerminalSized(20, 4);
        assertEnteringStringGivesResponse("\033_Gm=0,q=2;" + PNG_BASE64 + "\033\\", "");
        enterString("ok").assertLinesAre("ok                  ", "                    ",
            "                    ", "                    ");
    }


    /* Invalid payloads and control data. */

    public void testInvalidBase64PayloadRespondsError() {
        withTerminalSized(20, 4);
        assertEnteringStringGivesResponse(
            "\033_Ga=t,f=100,t=d,i=9,m=0;AA!AAAAA\033\\",
            "\033_Gi=9;EBADDATA:invalid base64 image data\033\\");
        assertEquals(-1, mTerminal.getKittyImageDataLength(9));

        // A `base64` string of a length that cannot encode any byte is invalid as well.
        assertEnteringStringGivesResponse(
            "\033_Ga=t,f=100,t=d,i=9,m=0;AAAAA\033\\",
            "\033_Gi=9;EBADDATA:invalid base64 image data\033\\");

        // An empty payload cannot be decoded into an image.
        assertEnteringStringGivesResponse(
            "\033_Ga=t,f=100,t=d,i=9,m=0;\033\\",
            "\033_Gi=9;EBADDATA:empty image data\033\\");
    }

    public void testInvalidBase64PaddingIsRejected() {
        withTerminalSized(20, 4);

        // More than two padding characters, and a total length that is not a multiple of four.
        assertEnteringStringGivesResponse("\033_Ga=t,f=24,t=d,i=9,s=1,v=1,m=0;QQ====\033\\",
            "\033_Gi=9;EBADDATA:invalid base64 image data\033\\");
        assertEnteringStringGivesResponse("\033_Ga=t,f=24,t=d,i=9,s=1,v=1,m=0;QQ=\033\\",
            "\033_Gi=9;EBADDATA:invalid base64 image data\033\\");
        assertEnteringStringGivesResponse("\033_Ga=t,f=24,t=d,i=9,s=1,v=1,m=0;=\033\\",
            "\033_Gi=9;EBADDATA:invalid base64 image data\033\\");

        // Valid padding, and the unpadded form which clients are allowed to send.
        assertEnteringStringGivesResponse("\033_Ga=t,f=32,t=d,i=9,s=1,v=1,m=0;" + RGBA_BASE64_1X1 + "\033\\",
            "\033_Gi=9;OK\033\\");
        assertEnteringStringGivesResponse("\033_Ga=t,f=32,t=d,i=9,s=1,v=1,m=0;AQIDBA\033\\",
            "\033_Gi=9;OK\033\\");
    }

    public void testMalformedControlDataIsRejected() {
        withTerminalSized(20, 4);

        // A key without a `=` and a value. The image id was read before the malformed key, so it
        // can be used to match the error response with the command.
        assertEnteringStringGivesResponse("\033_Ga=t,i=10,X;" + PNG_BASE64 + "\033\\",
            "\033_Gi=10;EINVAL:malformed control data\033\\");
        assertEquals(-1, mTerminal.getKittyImageDataLength(10));

        // The remaining keys are read even if the malformed key comes before them.
        assertEnteringStringGivesResponse("\033_GX,i=10;" + PNG_BASE64 + "\033\\",
            "\033_Gi=10;EINVAL:malformed control data\033\\");
        assertEnteringStringGivesResponse("\033_GX,i=10,q=2;" + PNG_BASE64 + "\033\\", "");

        // A key with an empty value.
        assertEnteringStringGivesResponse("\033_Ga=t,i=10,c=;" + PNG_BASE64 + "\033\\",
            "\033_Gi=10;EINVAL:invalid columns\033\\");

        enterString("ok").assertLinesAre("ok                  ", "                    ",
            "                    ", "                    ");
    }

    public void testUnknownActionRespondsError() {
        withTerminalSized(20, 4);
        assertEnteringStringGivesResponse("\033_Ga=X,i=11;\033\\",
            "\033_Gi=11;ENOTSUP:unsupported action\033\\");

        // The `a=f` (frame), `a=a` (animate) and `a=c` (compose) actions are valid protocol actions,
        // but are not supported.
        assertEnteringStringGivesResponse("\033_Ga=f,i=11;\033\\",
            "\033_Gi=11;ENOTSUP:unsupported action\033\\");
        assertEnteringStringGivesResponse("\033_Ga=a,i=11;\033\\",
            "\033_Gi=11;ENOTSUP:unsupported action\033\\");
        assertEnteringStringGivesResponse("\033_Ga=c,i=11;\033\\",
            "\033_Gi=11;ENOTSUP:unsupported action\033\\");

        enterString("ok").assertLinesAre("ok                  ", "                    ",
            "                    ", "                    ");
    }


    /* Displaying an already transmitted image (`a=p`). */

    public void testPutUsesTheStoredImageWithoutRetransmittingIt() {
        withTerminalSized(20, 4);
        enterString("\033_Ga=t,f=100,t=d,i=200,m=0,q=2;" + PNG_BASE64 + "\033\\");

        // The image data is not sent again, so a failure to display it can only come from the stored
        // image being looked up and used. Creating the bitmap always fails in unit tests.
        assertEnteringStringGivesResponse("\033_Ga=p,i=200,p=1,c=4,r=2,z=0,C=1\033\\",
            "\033_Gi=200;EBADPNG:displaying image failed\033\\");

        // The stored image is kept so that it can be displayed again.
        assertEquals(PNG_LENGTH, mTerminal.getKittyImageDataLength(200));
    }

    public void testPutForUnknownImageIdRespondsError() {
        withTerminalSized(20, 4);
        assertEnteringStringGivesResponse("\033_Ga=p,i=201,p=1,c=4,r=2,C=1\033\\",
            "\033_Gi=201;ENOENT:no image transmitted for image id\033\\");

        // The image data is freed by a `d=I` delete, after which it cannot be displayed again.
        enterString("\033_Ga=t,f=100,t=d,i=201,m=0,q=2;" + PNG_BASE64 + "\033\\");
        enterString("\033_Ga=d,d=I,i=201,q=2\033\\");
        assertEnteringStringGivesResponse("\033_Ga=p,i=201,p=1,C=1\033\\",
            "\033_Gi=201;ENOENT:no image transmitted for image id\033\\");
    }

    public void testPutWithoutImageIdIsIgnored() {
        withTerminalSized(20, 4);
        // No response can be sent without an image id, so only check that nothing breaks.
        assertEnteringStringGivesResponse("\033_Ga=p,p=1,c=4,r=2,C=1\033\\", "");
        enterString("ok").assertLinesAre("ok                  ", "                    ",
            "                    ", "                    ");
    }

    /* Source rectangles (`x=`, `y=`, `w=` and `h=`). */

    public void testSourceRectangleWithinTheImageIsAccepted() {
        withTerminalSized(20, 4);
        enterString("\033_Ga=t,f=32,t=d,i=300,s=2,v=2,m=0,q=2;" + RGBA_BASE64_2X2 + "\033\\");

        // Creating the bitmap always fails in unit tests, so reaching `EBADPNG` means the source
        // rectangle passed validation and the placement was attempted.
        assertEnteringStringGivesResponse("\033_Ga=p,i=300,p=1,x=0,y=0,w=1,h=1,c=1,r=1,C=1\033\\",
            "\033_Gi=300;EBADPNG:displaying image failed\033\\");
        assertEnteringStringGivesResponse("\033_Ga=p,i=300,p=1,x=1,y=1,w=1,h=1,c=1,r=1,C=1\033\\",
            "\033_Gi=300;EBADPNG:displaying image failed\033\\");
        assertEnteringStringGivesResponse("\033_Ga=p,i=300,p=1,x=0,y=0,w=2,h=2,c=2,r=2,C=1\033\\",
            "\033_Gi=300;EBADPNG:displaying image failed\033\\");

        // The `w` and `h` keys default to the rest of the image after the `x` and `y` offset.
        assertEnteringStringGivesResponse("\033_Ga=p,i=300,p=1,x=1,y=1,c=1,r=1,C=1\033\\",
            "\033_Gi=300;EBADPNG:displaying image failed\033\\");
        assertEnteringStringGivesResponse("\033_Ga=p,i=300,p=1,x=1,c=1,r=2,C=1\033\\",
            "\033_Gi=300;EBADPNG:displaying image failed\033\\");
    }

    public void testSourceRectangleOutsideTheImageIsRejected() {
        withTerminalSized(20, 4);
        enterString("\033_Ga=t,f=32,t=d,i=301,s=2,v=2,m=0,q=2;" + RGBA_BASE64_2X2 + "\033\\");

        // The width extends past the right edge of the image.
        assertEnteringStringGivesResponse("\033_Ga=p,i=301,p=1,x=1,y=0,w=2,h=1,c=1,r=1,C=1\033\\",
            "\033_Gi=301;EINVAL:source rectangle x 1 and width 2 exceed image width 2\033\\");

        // The height extends past the bottom edge of the image.
        assertEnteringStringGivesResponse("\033_Ga=p,i=301,p=1,x=0,y=1,w=1,h=2,c=1,r=1,C=1\033\\",
            "\033_Gi=301;EINVAL:source rectangle y 1 and height 2 exceed image height 2\033\\");

        // The offset itself is outside the image.
        assertEnteringStringGivesResponse("\033_Ga=p,i=301,p=1,x=2,y=0,c=1,r=1,C=1\033\\",
            "\033_Gi=301;EINVAL:source rectangle x 2 is outside image width 2\033\\");
        assertEnteringStringGivesResponse("\033_Ga=p,i=301,p=1,x=0,y=2,c=1,r=1,C=1\033\\",
            "\033_Gi=301;EINVAL:source rectangle y 2 is outside image height 2\033\\");

        // A source rectangle far larger than the image.
        assertEnteringStringGivesResponse("\033_Ga=p,i=301,p=1,x=0,y=0,w=100,h=100,c=4,r=4,C=1\033\\",
            "\033_Gi=301;EINVAL:source rectangle x 0 and width 100 exceed image width 2\033\\");

        // The image is not deleted by a rejected placement.
        assertEquals(16, mTerminal.getKittyImageDataLength(301));
        enterString("ok").assertLinesAre("ok                  ", "                    ",
            "                    ", "                    ");
    }

    public void testSourceRectangleIsValidatedForTransmitAndDisplayAsWell() {
        withTerminalSized(20, 4);
        assertEnteringStringGivesResponse(
            "\033_Ga=T,f=24,t=d,i=302,s=2,v=2,x=1,y=0,w=2,h=1,c=1,r=1,m=0;" + RGB_BASE64_2X2 + "\033\\",
            "\033_Gi=302;EINVAL:source rectangle x 1 and width 2 exceed image width 2\033\\");

        // The image is still transmitted and stored even though the placement was rejected.
        assertEquals(12, mTerminal.getKittyImageDataLength(302));
    }

    public void testSourceRectangleKeyValuesAreValidated() {
        withTerminalSized(20, 4);

        // The `w` and `h` keys cannot be `0`, since a source rectangle must cover at least one pixel.
        assertEnteringStringGivesResponse("\033_Ga=p,i=303,w=0\033\\",
            "\033_Gi=303;EINVAL:invalid source rectangle width\033\\");
        assertEnteringStringGivesResponse("\033_Ga=p,i=303,h=0\033\\",
            "\033_Gi=303;EINVAL:invalid source rectangle height\033\\");

        // Negative and out of range values.
        assertEnteringStringGivesResponse("\033_Ga=p,i=303,x=-1\033\\",
            "\033_Gi=303;EINVAL:invalid source rectangle x\033\\");
        assertEnteringStringGivesResponse("\033_Ga=p,i=303,y=99999\033\\",
            "\033_Gi=303;EINVAL:invalid source rectangle y\033\\");
    }

    public void testSourceRectangleIsNotValidatedIfTheImageDimensionsAreNotKnown() {
        // A `PNG` image can be transmitted without the `s` and `v` keys, in which case its dimensions
        // are only known once it has been decoded, so the source rectangle can only be validated
        // against the bitmap while creating it, which always fails in unit tests.
        withTerminalSized(20, 4);
        enterString("\033_Ga=t,f=100,t=d,i=304,m=0,q=2;" + PNG_BASE64 + "\033\\");
        assertEnteringStringGivesResponse("\033_Ga=p,i=304,p=1,x=0,y=0,w=100,h=100,c=4,r=4,C=1\033\\",
            "\033_Gi=304;EBADPNG:displaying image failed\033\\");
    }


    public void testPutDoesNotMoveTheCursorForCursorMovementPolicyOne() {
        withTerminalSized(20, 4);
        enterString("\033_Ga=t,f=100,t=d,i=202,m=0,q=2;" + PNG_BASE64 + "\033\\");
        enterString("\033[2;3H");
        assertCursorAt(1, 2);

        // This is the sequence herdr sends, which positions the cursor itself beforehand.
        enterString("\033_Ga=p,i=202,p=1,c=4,r=2,z=0,C=1,q=2\033\\");
        assertCursorAt(1, 2);
        assertEquals("", mOutput.getOutputAndClear());
    }

    public void testUnknownKeysAreIgnored() {
        withTerminalSized(20, 4);

        // The `z` (z-index), `U` (unicode placeholder), `X` and `Y` (cell pixel offset) keys are
        // ignored, as are keys that are not part of the protocol at all.
        assertEnteringStringGivesResponse(
            "\033_Ga=t,f=100,t=d,i=12,m=0,z=1,U=1,X=3,Y=4,Z=9;" + PNG_BASE64 + "\033\\",
            "\033_Gi=12;OK\033\\");
        assertEquals(PNG_LENGTH, mTerminal.getKittyImageDataLength(12));

        enterString("ok").assertLinesAre("ok                  ", "                    ",
            "                    ", "                    ");
    }

    public void testUnsupportedTransmissionMediumsAndCompressionRespondError() {
        withTerminalSized(20, 4);

        // Reading image data from a file, a temporary file or shared memory is not supported.
        assertEnteringStringGivesResponse("\033_Ga=T,f=100,t=f,i=13,m=0;L3RtcC9pbWc=\033\\",
            "\033_Gi=13;ENOTSUP:unsupported transmission medium\033\\");
        assertEnteringStringGivesResponse("\033_Ga=T,f=100,t=t,i=13,m=0;L3RtcC9pbWc=\033\\",
            "\033_Gi=13;ENOTSUP:unsupported transmission medium\033\\");
        assertEnteringStringGivesResponse("\033_Ga=T,f=100,t=s,i=13,m=0;L3RtcC9pbWc=\033\\",
            "\033_Gi=13;ENOTSUP:unsupported transmission medium\033\\");

        // `zlib` compressed image data is not supported.
        assertEnteringStringGivesResponse("\033_Ga=T,f=100,t=d,o=z,i=13,m=0;" + PNG_BASE64 + "\033\\",
            "\033_Gi=13;ENOTSUP:unsupported compression\033\\");

        // An image format that is not part of the protocol at all.
        assertEnteringStringGivesResponse("\033_Ga=T,f=50,t=d,i=13,m=0;" + PNG_BASE64 + "\033\\",
            "\033_Gi=13;EINVAL:invalid image format\033\\");

        assertEquals(-1, mTerminal.getKittyImageDataLength(13));
        enterString("ok").assertLinesAre("ok                  ", "                    ",
            "                    ", "                    ");
    }

    public void testImageIdRangeIsValidated() {
        withTerminalSized(20, 4);

        // Image ids are 32-bit unsigned integers, so the max value is accepted.
        assertEnteringStringGivesResponse(
            "\033_Ga=t,f=100,t=d,i=4294967295,m=0;" + PNG_BASE64 + "\033\\",
            "\033_Gi=4294967295;OK\033\\");
        assertEquals(PNG_LENGTH, mTerminal.getKittyImageDataLength(4294967295L));

        // One more than the max value is rejected. No response is sent since the image id required
        // to match the response with the command could not be read.
        assertEnteringStringGivesResponse(
            "\033_Ga=t,f=100,t=d,i=4294967296,m=0;" + PNG_BASE64 + "\033\\", "");
        assertEquals(-1, mTerminal.getKittyImageDataLength(4294967296L));

        // A value with more digits than any 32-bit unsigned integer must not overflow.
        assertEnteringStringGivesResponse(
            "\033_Ga=t,f=100,t=d,i=99999999999999999999,m=0;" + PNG_BASE64 + "\033\\", "");
        assertEnteringStringGivesResponse(
            "\033_Ga=t,f=100,t=d,i=-1,m=0;" + PNG_BASE64 + "\033\\", "");

        enterString("ok").assertLinesAre("ok                  ", "                    ",
            "                    ", "                    ");
    }

    public void testExtremePlacementSizeIsRejected() {
        withTerminalSized(20, 4);

        // The x and y coordinates of a bitmap cell are stored in 12 bits each, so a placement
        // cannot be larger than `4095` cells in either direction.
        assertEnteringStringGivesResponse(
            "\033_Ga=T,f=100,t=d,i=14,m=0,c=4096,r=1;" + PNG_BASE64 + "\033\\",
            "\033_Gi=14;EINVAL:invalid columns\033\\");
        assertEnteringStringGivesResponse(
            "\033_Ga=T,f=100,t=d,i=14,m=0,c=1,r=99999;" + PNG_BASE64 + "\033\\",
            "\033_Gi=14;EINVAL:invalid rows\033\\");

        // A placement that is within the limits but far larger than the screen and the max bitmap
        // size must be handled without crashing.
        assertEnteringStringGivesResponse(
            "\033_Ga=T,f=100,t=d,i=14,m=0,c=4095,r=4095;" + PNG_BASE64 + "\033\\",
            "\033_Gi=14;EBADPNG:displaying image failed\033\\");

        enterString("ok").assertLinesAre("ok                  ", "                    ",
            "                    ", "                    ");
    }

    public void testSingleCommandWithLargeImageDataIsSupported() {
        // The protocol recommends that clients split the image data into chunks of at most 4096
        // bytes, but that is not a requirement, and a client may send an entire image with a single
        // command. The image data is decoded as it is received, so the size is not limited.
        // This is `128x128` raw `RGBA`, which is `65536` bytes of image data and `87384` characters
        // of `base64`, far more than the args buffer of a command can hold.
        StringBuilder payload = new StringBuilder();
        for (int i = 0; i < 21845; i++) {
            payload.append("AAAA");
        }
        payload.append("AA==");
        assertEquals(87384, payload.length());

        withTerminalSized(20, 4);
        assertEnteringStringGivesResponse(
            "\033_Ga=t,f=32,t=d,i=500,s=128,v=128,m=0;" + payload + "\033\\",
            "\033_Gi=500;OK\033\\");
        assertEquals(128 * 128 * 4, mTerminal.getKittyImageDataLength(500));

        // Nothing of the image data was printed on the terminal.
        enterString("ok").assertLinesAre("ok                  ", "                    ",
            "                    ", "                    ");
    }

    public void testUnterminatedCommandWithLargeImageDataDoesNotPrintOnScreen() {
        withTerminalSized(4, 4);
        enterString("\033_Ga=t,f=100,t=d,i=15,m=0,q=2;");

        // The command is never terminated with `ESC \`, so all of the data is received as the image
        // data of the command and must not be printed as text.
        StringBuilder payload = new StringBuilder();
        for (int i = 0; i < 1000; i++) {
            payload.append('A');
        }
        for (int i = 0; i < 70; i++) {
            enterString(payload.toString());
        }

        assertLinesAre("    ", "    ", "    ", "    ");
        assertEquals(-1, mTerminal.getKittyImageDataLength(15));
        assertEquals("", mOutput.getOutputAndClear());

        // The terminal is not left in a broken state once the command is terminated.
        enterString("\033\\");
        enterString("ok").assertLinesAre("ok  ", "    ", "    ", "    ");
    }

    public void testCommandWithControlDataLongerThanTheMaxLengthIsDiscarded() {
        withTerminalSized(4, 4);

        // The control data of a command is stored in the args buffer, so a command that never sends
        // the `;` that ends it is discarded instead of being printed as text.
        StringBuilder controlData = new StringBuilder("\033_G");
        for (int i = 0; i < 1000; i++) {
            controlData.append("a=t,");
        }
        enterString(controlData.toString());

        assertLinesAre("    ", "    ", "    ", "    ");
        assertEquals("", mOutput.getOutputAndClear());

        // The rest of the command is discarded as well, including a payload.
        enterString(";AAAAAAAA\033\\");
        enterString("ok").assertLinesAre("ok  ", "    ", "    ", "    ");
    }


    /* Deleting images (`a=d`). */

    public void testDeleteWithoutDeleteModeRespondsOk() {
        withTerminalSized(20, 4);
        enterString("\033_Ga=t,f=100,t=d,i=16,m=0,q=2;" + PNG_BASE64 + "\033\\");

        // The `d` key defaults to `d=a`, which only deletes placements and keeps the image data.
        assertEnteringStringGivesResponse("\033_Ga=d,i=16\033\\", "\033_Gi=16;OK\033\\");
        assertEquals(PNG_LENGTH, mTerminal.getKittyImageDataLength(16));

        // This is what the herdr-browser plugin sends.
        assertEnteringStringGivesResponse("\033_Ga=d,i=16,q=2\033\\", "");
    }

    /**
     * Create a placement of a kitty graphics image directly instead of by sending an `a=T` or `a=p`
     * command, since the android `Bitmap` class required to create one is not available in unit
     * tests. The {@link TerminalBitmap} is created without a bitmap, which is enough to cover
     * everything that operates on placements after they have been created.
     *
     * @param bitmapNum The bitmap number to create the placement with, which must not already be in
     *                  use, so the placements must be created with `0`, `1`, `2`, ... in order.
     * @param isKittyImage Whether the placement is of a kitty graphics image, so that a sixel or
     *                     iTerm image can be created as well.
     */
    private void addPlacement(int bitmapNum, boolean isKittyImage, long imageId, long placementId,
                              int row, int column, int columns) {
        TerminalBuffer screen = mTerminal.getScreen();

        TerminalBitmap terminalBitmap = new TerminalBitmap(null, bitmapNum, /* bitmap */ null,
            INITIAL_CELL_WIDTH_PIXELS, INITIAL_CELL_HEIGHT_PIXELS, /* scrollLines */ 1,
            new int[] {1, columns});
        if (isKittyImage) {
            terminalBitmap.setKittyImage(imageId, placementId);
        }
        screen.addTerminalBitmap(terminalBitmap);

        for (int i = 0; i < columns; i++) {
            screen.setChar(column + i, row, '+', TextStyle.encodeTerminalBitmap(bitmapNum, i, 0));
        }
    }

    /** Whether a {@link TerminalBitmap} is still loaded for a bitmap number. */
    private boolean hasBitmap(int bitmapNum) {
        return mTerminal.getScreen().getTerminalBitmap(TextStyle.encodeTerminalBitmap(bitmapNum, 0, 0)) != null;
    }

    /** Whether the row still contains any {@link TerminalBitmap} cell. */
    private boolean hasBitmapInRow(int row) {
        TerminalBuffer screen = mTerminal.getScreen();
        return screen.mLines[screen.externalToInternalRow(row)].mHasTerminalBitmap;
    }

    public void testDeleteKittyImagePlacementsClearsTheCellsOfThePlacements() {
        withTerminalSized(6, 4);

        addPlacement(/* bitmapNum */ 0, /* isKittyImage */ true, /* imageId */ 1, /* placementId */ 1, /* row */ 0, /* column */ 0, /* columns */ 2);
        addPlacement(1, true, 1, 2, 1, 0, 2);
        addPlacement(2, true, 2, 1, 2, 0, 2);
        // A sixel or iTerm image, which a kitty graphics delete must never remove.
        addPlacement(3, false, 0, 0, 3, 0, 2);

        assertLinesAre("++    ", "++    ", "++    ", "++    ");
        assertTrue(hasBitmapInRow(0));

        // Only the placement of the placement id passed with the `p` key is deleted.
        mTerminal.getScreen().deleteKittyImagePlacements(1, 2);
        assertLinesAre("++    ", "      ", "++    ", "++    ");
        assertTrue(hasBitmap(0));
        assertFalse(hasBitmap(1));
        assertTrue(hasBitmap(2));
        assertTrue(hasBitmap(3));
        assertTrue(hasBitmapInRow(0));
        assertFalse(hasBitmapInRow(1));

        // All the placements of the image are deleted if the `p` key is not passed.
        mTerminal.getScreen().deleteKittyImagePlacements(1, KittyImage.PLACEMENT_ID__NONE);
        assertLinesAre("      ", "      ", "++    ", "++    ");
        assertFalse(hasBitmap(0));
        assertTrue(hasBitmap(2));
        assertTrue(hasBitmap(3));
        assertFalse(hasBitmapInRow(0));

        // Deleting all the kitty graphics placements must not remove the sixel or iTerm image.
        mTerminal.getScreen().deleteAllKittyImagePlacements();
        assertLinesAre("      ", "      ", "      ", "++    ");
        assertFalse(hasBitmap(2));
        assertTrue(hasBitmap(3));
        assertTrue(hasBitmapInRow(3));
    }

    /*
     * The cursor movement after an image.
     *
     * The android `Bitmap` class is not available in unit tests, so an image is never actually placed
     * and `TerminalEmulator.moveCursorAfterTerminalBitmap()` is never reached through a command.
     * It is called directly with the cursor delta a placement would have returned instead, which is
     * the number of rows the image covers as the first value and the number of columns as the second.
     */

    public void testCursorAfterImageThatEndsExactlyAtTheLastColumn() {
        // An image that ends exactly at the last column of the screen fits on the last row it covers,
        // so the cursor must be left on that row at the column after the image, which is the column
        // past the end of the screen. Note that the condition for this was `col < mColumns - 1` in the
        // sixel and iTerm image code this is shared with, which treated such an image as not fitting.
        withTerminalSized(56, 20);

        // A single row image in the columns 53 and 54 of a 56 column screen ends at the last column.
        placeCursorAndAssert(2, 53);
        mTerminal.moveCursorAfterTerminalBitmap(new int[] {1, 2});
        assertCursorAt(2, 55);

        // A two row image ends on the row below the row it started on.
        placeCursorAndAssert(8, 53);
        mTerminal.moveCursorAfterTerminalBitmap(new int[] {2, 2});
        assertCursorAt(9, 55);
    }

    public void testCursorAfterImageThatDoesNotFitOnTheLastColumn() {
        // An image that ends past the last column of the screen does not fit, so the cursor is wrapped
        // to the first column of the row after the image.
        withTerminalSized(56, 20);

        // A single row image in the columns 54 and 55 of a 56 column screen ends past the last column.
        placeCursorAndAssert(2, 54);
        mTerminal.moveCursorAfterTerminalBitmap(new int[] {1, 2});
        assertCursorAt(3, 0);

        placeCursorAndAssert(8, 54);
        mTerminal.moveCursorAfterTerminalBitmap(new int[] {2, 2});
        assertCursorAt(10, 0);
    }

    public void testCursorAfterImageThatEndsBeforeTheLastColumn() {
        // The common case, where the cursor is left on the last row the image covers.
        withTerminalSized(56, 20);

        placeCursorAndAssert(2, 0);
        mTerminal.moveCursorAfterTerminalBitmap(new int[] {1, 4});
        assertCursorAt(2, 4);

        placeCursorAndAssert(8, 10);
        mTerminal.moveCursorAfterTerminalBitmap(new int[] {3, 4});
        assertCursorAt(10, 14);
    }

    public void testUnreferencedTerminalBitmapsAreReleasedWhenTheirCellsAreOverwritten() {
        // A placement that displays a new image at the position of a previous one overwrites its
        // cells, which leaves the bitmap of the previous one unreferenced and it must be released
        // without waiting for the throttled garbage collection, otherwise a client that streams a
        // pane at frame rate keeps every frame in memory.
        withTerminalSized(6, 3);
        TerminalBuffer screen = mTerminal.getScreen();

        addPlacement(/* bitmapNum */ 0, /* isKittyImage */ true, /* imageId */ 1, /* placementId */ 1, /* row */ 0, /* column */ 0, /* columns */ 2);
        addPlacement(1, true, 1, 2, 1, 0, 2);
        addPlacement(2, false, 0, 0, 2, 0, 2);
        assertTrue(hasBitmap(0));
        assertTrue(hasBitmap(1));
        assertTrue(hasBitmap(2));

        // Nothing is released while every bitmap is still referenced by a cell.
        Set<Integer> bitmapNums = new HashSet<>(Arrays.asList(0, 1, 2));
        screen.releaseUnreferencedTerminalBitmaps(bitmapNums);
        assertTrue(hasBitmap(0));
        assertTrue(hasBitmap(1));
        assertTrue(hasBitmap(2));

        // Overwrite the cells of the first placement, like a new placement at the same position does.
        screen.setChar(0, 0, 'a', TextStyle.NORMAL);
        screen.setChar(1, 0, 'b', TextStyle.NORMAL);

        screen.releaseUnreferencedTerminalBitmaps(new HashSet<>(Arrays.asList(0, 1, 2)));
        assertFalse(hasBitmap(0));
        assertTrue(hasBitmap(1));
        assertTrue(hasBitmap(2));

        // A `null` or empty set of candidates is a no operation.
        screen.releaseUnreferencedTerminalBitmaps(null);
        screen.releaseUnreferencedTerminalBitmaps(new HashSet<Integer>());
        assertTrue(hasBitmap(1));
        assertTrue(hasBitmap(2));
    }

    public void testReferencedTerminalBitmapsInTheTranscriptAreNotReleased() {
        // A bitmap that is only referenced by a row that has scrolled into the transcript must not be
        // released, since it is still drawn when the transcript is scrolled back to.
        withTerminalSized(6, 2);
        addPlacement(0, true, 1, 1, 0, 0, 2);
        assertTrue(hasBitmap(0));

        // Scroll the placement off the screen and into the transcript.
        enterString("\033[2;1H\r\n\r\n");
        assertTrue(mTerminal.getScreen().getActiveTranscriptRows() > 0);

        mTerminal.getScreen().releaseUnreferencedTerminalBitmaps(new HashSet<>(Arrays.asList(0)));
        assertTrue(hasBitmap(0));
    }

    public void testDeleteAllKittyImagePlacementsDeletesPlacementsWithoutAnImageId() {
        // A placement created by a command that did not pass the `i` key can only be deleted with a
        // `d=a` or `d=A` delete, so it must not be treated as a placement of a non kitty image.
        withTerminalSized(6, 2);
        addPlacement(0, true, KittyImage.IMAGE_ID__NONE, KittyImage.PLACEMENT_ID__NONE, 0, 0, 2);
        addPlacement(1, false, 0, 0, 1, 0, 2);
        assertLinesAre("++    ", "++    ");

        mTerminal.getScreen().deleteAllKittyImagePlacements();
        assertLinesAre("      ", "++    ");
        assertFalse(hasBitmap(0));
        assertTrue(hasBitmap(1));
    }

    public void testDeleteAllKittyImagePlacementsWithoutAnyPlacementDoesNothing() {
        withTerminalSized(6, 2);
        addPlacement(0, false, 0, 0, 0, 0, 2);
        assertLinesAre("++    ", "      ");

        mTerminal.getScreen().deleteAllKittyImagePlacements();
        mTerminal.getScreen().deleteKittyImagePlacements(1, 1);
        assertLinesAre("++    ", "      ");
        assertTrue(hasBitmap(0));
    }

    public void testDeleteByIdKeepsOrFreesImageData() {
        withTerminalSized(20, 4);
        enterString("\033_Ga=t,f=100,t=d,i=17,m=0,q=2;" + PNG_BASE64 + "\033\\");
        enterString("\033_Ga=t,f=100,t=d,i=18,m=0,q=2;" + PNG_BASE64 + "\033\\");

        // The lowercase `d=i` only deletes the placements of the image and keeps its data.
        assertEnteringStringGivesResponse("\033_Ga=d,d=i,i=17\033\\", "\033_Gi=17;OK\033\\");
        assertEquals(PNG_LENGTH, mTerminal.getKittyImageDataLength(17));

        // The uppercase `d=I` also frees the image data, and only for the image id passed.
        assertEnteringStringGivesResponse("\033_Ga=d,d=I,i=17\033\\", "\033_Gi=17;OK\033\\");
        assertEquals(-1, mTerminal.getKittyImageDataLength(17));
        assertEquals(PNG_LENGTH, mTerminal.getKittyImageDataLength(18));
    }

    public void testDeleteAllFreesImageDataOnlyForUppercaseDeleteMode() {
        withTerminalSized(20, 4);
        enterString("\033_Ga=t,f=100,t=d,i=19,m=0,q=2;" + PNG_BASE64 + "\033\\");
        enterString("\033_Ga=t,f=100,t=d,i=20,m=0,q=2;" + PNG_BASE64 + "\033\\");

        assertEnteringStringGivesResponse("\033_Ga=d,d=a,i=19\033\\", "\033_Gi=19;OK\033\\");
        assertEquals(PNG_LENGTH, mTerminal.getKittyImageDataLength(19));
        assertEquals(PNG_LENGTH, mTerminal.getKittyImageDataLength(20));

        assertEnteringStringGivesResponse("\033_Ga=d,d=A,i=19\033\\", "\033_Gi=19;OK\033\\");
        assertEquals(-1, mTerminal.getKittyImageDataLength(19));
        assertEquals(-1, mTerminal.getKittyImageDataLength(20));
    }

    public void testDeleteOfUnknownImageIdRespondsOk() {
        withTerminalSized(20, 4);

        // Deleting an image that does not exist is not an error as per the protocol.
        assertEnteringStringGivesResponse("\033_Ga=d,d=i,i=21\033\\", "\033_Gi=21;OK\033\\");
        assertEnteringStringGivesResponse("\033_Ga=d,d=I,i=21\033\\", "\033_Gi=21;OK\033\\");
        assertEnteringStringGivesResponse("\033_Ga=d,d=i,i=21,q=2\033\\", "");

        enterString("ok").assertLinesAre("ok                  ", "                    ",
            "                    ", "                    ");
    }

    public void testDeleteByIdWithoutImageIdRespondsError() {
        withTerminalSized(20, 4);
        // No response can be sent without an image id, so only check that nothing breaks.
        assertEnteringStringGivesResponse("\033_Ga=d,d=i\033\\", "");
        enterString("ok").assertLinesAre("ok                  ", "                    ",
            "                    ", "                    ");
    }

    public void testUnsupportedDeleteModeRespondsError() {
        withTerminalSized(20, 4);

        // The delete modes that require image numbers, placement ids or coordinates.
        assertEnteringStringGivesResponse("\033_Ga=d,d=n,i=22\033\\",
            "\033_Gi=22;ENOTSUP:unsupported delete mode\033\\");
        assertEnteringStringGivesResponse("\033_Ga=d,d=z,i=22\033\\",
            "\033_Gi=22;ENOTSUP:unsupported delete mode\033\\");
    }


    /* The command sequences of actual clients. */

    public void testHerdrCommandSequence() {
        // The exact sequences the herdr client sends, which transmits an image once and then
        // displays it again for every frame with an `a=p` command. Every command passes `q=2`, so the
        // terminal must stay completely silent, and the cursor is positioned with `CUP` before every
        // placement, which `C=1` requires to be left alone.
        withTerminalSized(20, 6);

        // Transmit, in two chunks like herdr does for image data larger than one chunk.
        enterString("\033_Ga=t,t=d,f=100,s=1,v=1,i=424242,q=2,m=1;" + PNG_BASE64_CHUNK_0 + "\033\\");
        enterString("\033_Gm=0,q=2;" + PNG_BASE64_CHUNK_1 + "\033\\");
        assertEquals(PNG_LENGTH, mTerminal.getKittyImageDataLength(424242));

        // Place, for two consecutive frames without transmitting the image again.
        enterString("\033[2;1H");
        enterString("\033_Ga=p,i=424242,p=1,c=20,r=4,z=0,C=1,q=2\033\\");
        assertCursorAt(1, 0);
        enterString("\033_Ga=d,d=i,i=424242,p=1,q=2;\033\\");
        enterString("\033[2;1H");
        // The optional keys herdr only adds when the corresponding fields are non zero. The source
        // rectangle covers the entire `1x1` image that was transmitted with `s=1,v=1` above.
        enterString("\033_Ga=p,i=424242,p=1,c=20,r=4,z=0,C=1,q=2,x=0,y=0,w=1,h=1,X=0,Y=0\033\\");
        assertCursorAt(1, 0);

        // The image data is kept for every frame until it is explicitly freed.
        assertEquals(PNG_LENGTH, mTerminal.getKittyImageDataLength(424242));
        enterString("\033_Ga=d,d=I,i=424242,q=2;\033\\");
        assertEquals(-1, mTerminal.getKittyImageDataLength(424242));

        assertEquals("", mOutput.getOutputAndClear());
        assertLinesAre("                    ", "                    ", "                    ",
            "                    ", "                    ", "                    ");
    }

    public void testYaziCapabilityQueryAndRawTransmitSequence() {
        // Yazi probes with a capability query followed by `CSI c` (Primary Device Attributes), and
        // expects the graphics response before the device attributes response if the protocol is
        // supported. It transmits raw `RGB`/`RGBA` pixel data and never `PNG`.
        withTerminalSized(20, 6);
        assertEnteringStringGivesResponse("\033_Gi=31,s=1,v=1,a=q,t=d,f=24;AAAA\033\\\033[c",
            "\033_Gi=31;OK\033\\" + "\033[?64;1;2;4;6;9;15;18;21;22c");

        assertEnteringStringGivesResponse(
            "\033_Ga=T,f=32,t=d,i=31,s=2,v=2,c=2,r=1,m=0,q=1;" + RGBA_BASE64_2X2 + "\033\\",
            // Creating the bitmap always fails in unit tests, and `q=1` does not suppress errors.
            "\033_Gi=31;EBADPNG:displaying image failed\033\\");
        assertEquals(16, mTerminal.getKittyImageDataLength(31));
    }


    /* Regressions for other APC commands. */

    public void testNonKittyApcCommandsAreStillIgnoredSilently() {
        // Only `APC` commands starting with `G` are kitty graphics commands.
        withTerminalSized(12, 2);
        assertEnteringStringGivesResponse("hello \033_some\023\033_\\apc#end\033\\ world", "");
        assertLinesAre("hello  world", "            ");

        withTerminalSized(12, 2);
        assertEnteringStringGivesResponse("\033_Ha=T,f=100,t=d,i=1,m=0;" + PNG_BASE64 + "\033\\", "");
        assertLinesAre("            ", "            ");

        withTerminalSized(12, 2);
        assertEnteringStringGivesResponse("\033_\033\\", "");
        assertLinesAre("            ", "            ");
    }

    public void testOldestKittyImageDataIsEvictedWhenTooManyImagesAreTransmitted() {
        withTerminalSized(20, 4);

        // At most 32 images are stored, so transmitting 33 images must evict the oldest one.
        for (int imageId = 1; imageId <= 33; imageId++) {
            enterString("\033_Ga=t,f=100,t=d,i=" + imageId + ",m=0,q=2;" + PNG_BASE64 + "\033\\");
        }

        assertEquals(-1, mTerminal.getKittyImageDataLength(1));
        assertEquals(PNG_LENGTH, mTerminal.getKittyImageDataLength(2));
        assertEquals(PNG_LENGTH, mTerminal.getKittyImageDataLength(33));

        // Re-transmitting an image does not evict any other image.
        enterString("\033_Ga=t,f=100,t=d,i=33,m=0,q=2;" + PNG_BASE64 + "\033\\");
        assertEquals(PNG_LENGTH, mTerminal.getKittyImageDataLength(2));
        assertEquals(PNG_LENGTH, mTerminal.getKittyImageDataLength(33));
    }

    public void testKittyImageDataIsFreedOnTerminalReset() {
        withTerminalSized(20, 4);
        enterString("\033_Ga=t,f=100,t=d,i=23,m=0,q=2;" + PNG_BASE64 + "\033\\");
        assertEquals(PNG_LENGTH, mTerminal.getKittyImageDataLength(23));

        // `ESC c` - full reset (RIS).
        enterString("\033c");
        assertEquals(-1, mTerminal.getKittyImageDataLength(23));
    }

}
