package com.termux.terminal;

import java.util.Arrays;

/**
 * A kitty graphics protocol image received via an `APC _G` command.
 *
 * The command is in the format `APC _ G <control data> [; <payload>] ST`, where `control data` is a
 * comma separated list of `<key>=<value>` pairs with single character keys, and `payload` is the
 * `base64` encoded image data.
 *
 * Only a minimal subset of the protocol is supported, check
 * {@link TerminalEmulator#doApcKittyGraphics(KittyImage)} for details of what is supported and what
 * is not.
 *
 * A single image may be transmitted with multiple commands by setting the `m` key to `1` for all
 * commands except the last one, in which case the same {@link KittyImage} instance is kept by
 * {@link TerminalEmulator} until the final command is received.
 *
 * The `base64` payload is not collected as text. Each character of it is decoded by
 * {@link #readImageChar(char)} as it is received and the bytes are accumulated in
 * {@link #mImageData}, since a `char` is stored on 2 bytes and `base64` requires 4 characters for
 * every 3 bytes, so collecting the payload as text would require `2.66` times as much memory as the
 * image data itself. The payload of a command does not have to be a multiple of the 4 characters
 * that decode to 3 bytes, so the bits that do not complete a byte are carried over to the next
 * command of the image in {@link #mBase64Buffer} and {@link #mBase64BufferBits}. The combined
 * length of the image data of all the commands for a single image is bound by
 * {@link #IMAGE_DATA__MAX_LENGTH}.
 *
 * - https://sw.kovidgoyal.net/kitty/graphics-protocol/
 */
public class KittyImage {

    public static final String LOG_TAG = "KittyImage";



    /** The {@link Enum} that defines {@link KittyImage} state. */
    public enum ImageState {

        INIT("init", 0),
        ARGUMENTS_READ("arguments_read", 1),
        IMAGE_READING("image_reading", 2),
        IMAGE_READ("image_read", 3),
        IMAGE_DECODED("image_decoded", 4),
        FAILED("failed", 5);

        private final String name;
        private final int value;

        ImageState(final String name, final int value) {
            this.name = name;
            this.value = value;
        }

        public String getName() {
            return name;
        }

        public int getValue() {
            return value;
        }

    }



    /* The supported actions for the `a` key. */

    /** `a=t` - Transmit the image data and store it for the image id passed with the `i` key. */
    public static final char ACTION__TRANSMIT = 't';
    /** `a=T` - Like {@link #ACTION__TRANSMIT}, but also display the image at the cursor position. */
    public static final char ACTION__TRANSMIT_AND_DISPLAY = 'T';
    /** `a=p` - Display an image that was already transmitted for the image id passed with the `i` key. */
    public static final char ACTION__PUT = 'p';
    /** `a=q` - Query the terminal for kitty graphics protocol support without storing anything. */
    public static final char ACTION__QUERY = 'q';
    /** `a=d` - Delete image placements as per the {@link #mDeleteMode} passed with the `d` key. */
    public static final char ACTION__DELETE = 'd';

    /** The default action if the `a` key is not passed. */
    public static final char ACTION__DEFAULT = ACTION__TRANSMIT;


    /* The delete modes for the `d` key of an {@link #ACTION__DELETE} command. */

    /** The `d` key was not passed, which is equivalent to `d=a`. */
    public static final char DELETE_MODE__NONE = 0;
    /** `d=a` - Delete all placements. The uppercase `d=A` also frees the image data. */
    public static final char DELETE_MODE__ALL = 'a';
    public static final char DELETE_MODE__ALL_AND_FREE_DATA = 'A';
    /**
     * `d=i` - Delete the placements of the image id passed with the `i` key, and only the placement
     * for the placement id passed with the `p` key if it is passed. The uppercase `d=I` also frees
     * the image data.
     */
    public static final char DELETE_MODE__ID = 'i';
    public static final char DELETE_MODE__ID_AND_FREE_DATA = 'I';


    /* The transmission mediums for the `t` key. */

    /** `t=d` - The image data is passed directly in the `base64` encoded command payload. */
    public static final char MEDIUM__DIRECT = 'd';

    /** The default transmission medium if the `t` key is not passed. */
    public static final char MEDIUM__DEFAULT = MEDIUM__DIRECT;


    /* The image formats for the `f` key. */

    /** `f=24` - Raw `RGB` pixel data. */
    public static final int FORMAT__RGB = 24;
    /** `f=32` - Raw `RGBA` pixel data. */
    public static final int FORMAT__RGBA = 32;
    /** `f=100` - `PNG` image data. */
    public static final int FORMAT__PNG = 100;

    /** The default image format if the `f` key is not passed as per the protocol. */
    public static final int FORMAT__DEFAULT = FORMAT__RGBA;


    /* The quiet levels for the `q` key. */

    /** `q=0` - Send both success and error responses. */
    public static final int QUIET__NONE = 0;
    /** `q=1` - Suppress success responses. */
    public static final int QUIET__SUCCESS = 1;
    /** `q=2` - Suppress both success and error responses. */
    public static final int QUIET__ALL = 2;


    /* The error codes sent in error responses. */

    /** The control data of the command was malformed or out of range. */
    public static final String ERROR__EINVAL = "EINVAL";
    /** The command requires a feature of the protocol that is not supported. */
    public static final String ERROR__ENOTSUP = "ENOTSUP";
    /** No image data has been transmitted for the image id passed with the `i` key. */
    public static final String ERROR__ENOENT = "ENOENT";
    /** The `base64` encoded payload of the command could not be decoded. */
    public static final String ERROR__EBADDATA = "EBADDATA";
    /** The image data could not be decoded into a bitmap for display. */
    public static final String ERROR__EBADPNG = "EBADPNG";
    /** The image data exceeded the size limits of the terminal. */
    public static final String ERROR__ENOSPC = "ENOSPC";


    /** The image id value for the `i` key if it was not passed. Image id `0` is not valid. */
    public static final long IMAGE_ID__NONE = 0;

    /** The max value for the `i` key, since image ids are 32-bit unsigned integers. */
    public static final long IMAGE_ID__MAX = 4294967295L;

    /** The placement id value for the `p` key if it was not passed. Placement id `0` is not valid. */
    public static final long PLACEMENT_ID__NONE = 0;

    /** The max value for the `p` key, since placement ids are 32-bit unsigned integers. */
    public static final long PLACEMENT_ID__MAX = 4294967295L;

    /**
     * The max value for the `c` and `r` keys. The x and y coordinates of a bitmap cell are stored
     * in 12 bits each by {@link TextStyle#encodeTerminalBitmap(int, int, int)}.
     */
    public static final int PLACEMENT_CELLS__MAX = 4095;

    /**
     * The max value for the `s` and `v` keys, which is the max dimension of a bitmap supported by
     * skia. Check {@link TerminalBitmap#MAX_BITMAP_SIZE} for more info.
     */
    public static final int PIXEL_DIMENSION__MAX = 32766;

    /**
     * The max length in bytes of the image data of a single image after it has been decoded from
     * `base64`, which for the raw {@link #FORMAT__RGB} and {@link #FORMAT__RGBA} formats is
     * `<pixel width> * <pixel height> * <bytes per pixel>`.
     *
     * The limit is derived from {@link TerminalBitmap#MAX_BITMAP_SIZE}, which is the max size of the
     * pixels of a bitmap the terminal can actually draw, since accepting image data that is larger
     * than that would only waste memory on an image that cannot be displayed, while accepting less
     * would reject images that could have been displayed.
     *
     * A bitmap stores each pixel on 4 bytes as {@link android.graphics.Bitmap.Config#ARGB_8888}, so
     * the max number of pixels is `MAX_BITMAP_SIZE / 4`, and the raw {@link #FORMAT__RGBA} format
     * transmits the same 4 bytes per pixel, which makes the max image data length
     * `(MAX_BITMAP_SIZE / 4) * 4`, so equal to `MAX_BITMAP_SIZE` itself. The raw
     * {@link #FORMAT__RGB} format transmits only 3 bytes per pixel, so its data will be under this
     * limit before its bitmap reaches `MAX_BITMAP_SIZE`, which is checked separately while creating
     * the bitmap. The `PNG` format is compressed, so its data is far under this limit in practice.
     */
    public static final int IMAGE_DATA__MAX_LENGTH = TerminalBitmap.MAX_BITMAP_SIZE;

    /** The initial capacity for {@link #mImageData}. */
    private static final int IMAGE_DATA__INITIAL_CAPACITY = 4096;



    private static final int BASE64__INVALID = -1;

    /** The max number of `=` padding characters a `base64` encoded string can end with. */
    private static final int BASE64__MAX_PADDING = 2;

    private static final int[] BASE64__DECODE_TABLE = createBase64DecodeTable();



    protected final TerminalSessionClient mClient;

    /** The action passed with the `a` key. */
    protected char mAction = ACTION__DEFAULT;
    /** Whether the `a` key was passed. A continuation chunk of an image does not pass it. */
    protected boolean mHasAction = false;

    /** The delete mode passed with the `d` key. */
    protected char mDeleteMode = DELETE_MODE__NONE;

    /** The transmission medium passed with the `t` key. */
    protected char mMedium = MEDIUM__DEFAULT;

    /** The image format passed with the `f` key. */
    protected int mFormat = FORMAT__DEFAULT;

    /** The image id passed with the `i` key. */
    protected long mImageId = IMAGE_ID__NONE;

    /** The placement id passed with the `p` key. */
    protected long mPlacementId = PLACEMENT_ID__NONE;

    /** Whether more chunks of the image will follow as per the `m` key. */
    protected boolean mMoreChunks = false;

    /** The quiet level passed with the `q` key. */
    protected int mQuiet = QUIET__NONE;
    /** Whether the `q` key was passed. */
    protected boolean mHasQuiet = false;

    /** The number of columns and rows to display the image in as passed with the `c` and `r` keys. */
    protected int mColumns = -1;
    protected int mRows = -1;

    /**
     * The width and height in pixels of the image data as passed with the `s` and `v` keys, which
     * are required for the raw {@link #FORMAT__RGB} and {@link #FORMAT__RGBA} formats.
     */
    protected int mPixelWidth = -1;
    protected int mPixelHeight = -1;

    /**
     * The source rectangle of the transmitted image to display as passed with the `x`, `y`, `w` and
     * `h` keys, where `x` and `y` are the top left corner in pixels and `w` and `h` are the size in
     * pixels. The `x` and `y` keys default to `0` and the `w` and `h` keys default to the rest of
     * the image after the offset, so `-1` means that they were not passed.
     */
    protected int mSourceX = 0;
    protected int mSourceY = 0;
    protected int mSourceWidth = -1;
    protected int mSourceHeight = -1;

    /** Whether the cursor must not be moved after displaying the image as per the `C` key. */
    protected boolean mDoNotMoveCursor = false;

    /** The error code and message to send in an error response if the command failed. */
    protected String mErrorCode;
    protected String mErrorMessage;

    /**
     * The image data decoded from the `base64` payload of all the chunks of the image received so
     * far, which may be longer than {@link #mImageDataLength} since it is grown in advance.
     *
     * The payload of each chunk is decoded as it is received instead of the `base64` data of all the
     * chunks being collected first and decoded at the end, since a `char` is stored on 2 bytes and
     * `base64` requires 4 characters for every 3 bytes, so collecting it would require `2.66` times
     * as much memory as the image data itself.
     */
    protected byte[] mImageData;

    /** The number of bytes of {@link #mImageData} that have been decoded. */
    protected int mImageDataLength;

    /**
     * The exact length in bytes the image data must have, which is only known in advance for the raw
     * formats, otherwise `0`. It is used to allocate {@link #mImageData} exactly once and to reject
     * a client that sends more data than it declared before the memory for it is allocated.
     */
    protected int mImageDataExpectedLength;

    /**
     * The bits of the `base64` payload that did not complete a byte yet, which are carried over to
     * the next chunk of the image since the payload of a chunk does not have to be a multiple of the
     * 4 characters that decode to 3 bytes.
     */
    protected int mBase64Buffer;
    protected int mBase64BufferBits;

    /** The number of `=` padding characters received at the end of the `base64` payload. */
    protected int mBase64Padding;

    /** The total number of `base64` payload characters received, including the padding. */
    protected int mBase64Length;

    /** The current state of the {@link ImageState}. */
    protected ImageState mCurrentState = ImageState.INIT;
    /** The previous state of the {@link ImageState}. */
    protected ImageState mPreviousState = ImageState.INIT;



    protected KittyImage(TerminalSessionClient client) {
        mClient = client;
    }



    public char getAction() {
        return mAction;
    }

    public boolean hasAction() {
        return mHasAction;
    }


    public char getDeleteMode() {
        return mDeleteMode;
    }


    public int getFormat() {
        return mFormat;
    }

    /** Whether the image data is raw pixel data instead of an encoded image like `PNG`. */
    public boolean isRawFormat() {
        return mFormat == FORMAT__RGB || mFormat == FORMAT__RGBA;
    }

    /** Get the number of bytes each pixel of raw pixel data is stored on, or `0` for other formats. */
    public static int getBytesPerPixel(int format) {
        switch (format) {
            case FORMAT__RGB: return 3;
            case FORMAT__RGBA: return 4;
            default: return 0;
        }
    }


    public long getImageId() {
        return mImageId;
    }

    public long getPlacementId() {
        return mPlacementId;
    }


    public boolean hasMoreChunks() {
        return mMoreChunks;
    }


    public int getQuiet() {
        return mQuiet;
    }


    /**
     * Get the width in pixels to display the image in, or `-1` if the `c` key was not passed, in
     * which case the native width of the image is used.
     */
    public int getWidthPixels(int cellWidthPixels) {
        return mColumns > 0 ? mColumns * cellWidthPixels : -1;
    }

    /**
     * Get the height in pixels to display the image in, or `-1` if the `r` key was not passed, in
     * which case the native height of the image is used.
     */
    public int getHeightPixels(int cellHeightPixels) {
        return mRows > 0 ? mRows * cellHeightPixels : -1;
    }

    /**
     * Whether the aspect ratio of the image should be preserved. If both the `c` and `r` keys are
     * passed, then the image is scaled to exactly fill the requested cells as per the protocol.
     */
    public boolean shouldPreserveAspectRatio() {
        return !(mColumns > 0 && mRows > 0);
    }


    public int getPixelWidth() {
        return mPixelWidth;
    }

    public int getPixelHeight() {
        return mPixelHeight;
    }


    public int getSourceX() {
        return mSourceX;
    }

    public int getSourceY() {
        return mSourceY;
    }

    public int getSourceWidth() {
        return mSourceWidth;
    }

    public int getSourceHeight() {
        return mSourceHeight;
    }

    /**
     * Validate the source rectangle passed with the `x`, `y`, `w` and `h` keys against the pixel
     * dimensions of the image it is for, so that a client that requests a rectangle outside the
     * image is sent an error response instead of the request being silently clipped.
     *
     * The dimensions of an image are only known before it is decoded into a bitmap if the client
     * passed the `s` and `v` keys when transmitting it, which is always required for the raw formats
     * but is optional for `PNG`, so the source rectangle is validated against the actual dimensions
     * of the bitmap again by
     * {@link TerminalBitmap#buildForKittyImage(TerminalBuffer, int, int, byte[], int, int, int, int, int, int, int, int, int, int, int, int, boolean)}.
     *
     * @param imageWidth The width in pixels of the image, or a value `< 1` if it is not known.
     * @param imageHeight The height in pixels of the image, or a value `< 1` if it is not known.
     * @return Returns `true` if the source rectangle is valid, otherwise `false`, in which case
     * {@link #getErrorCode()} will be set.
     */
    public synchronized boolean validateSourceRectangle(int imageWidth, int imageHeight) {
        if (imageWidth < 1 || imageHeight < 1) return true;

        if (mSourceX >= imageWidth) {
            return setStateFailed(ERROR__EINVAL, "source rectangle x " + mSourceX +
                " is outside image width " + imageWidth);
        }

        if (mSourceWidth > 0 && mSourceX + mSourceWidth > imageWidth) {
            return setStateFailed(ERROR__EINVAL, "source rectangle x " + mSourceX +
                " and width " + mSourceWidth + " exceed image width " + imageWidth);
        }

        if (mSourceY >= imageHeight) {
            return setStateFailed(ERROR__EINVAL, "source rectangle y " + mSourceY +
                " is outside image height " + imageHeight);
        }

        if (mSourceHeight > 0 && mSourceY + mSourceHeight > imageHeight) {
            return setStateFailed(ERROR__EINVAL, "source rectangle y " + mSourceY +
                " and height " + mSourceHeight + " exceed image height " + imageHeight);
        }

        return true;
    }


    /** Whether the cursor should be moved to after the image after displaying it. */
    public boolean shouldMoveCursor() {
        return !mDoNotMoveCursor;
    }


    public String getErrorCode() {
        return mErrorCode;
    }

    public String getErrorMessage() {
        return mErrorMessage;
    }

    public boolean isFailed() {
        return mErrorCode != null;
    }


    /** Get the decoded image data, which is only valid after {@link #finishImage()} succeeded. */
    public byte[] getDecodedImage() {
        return mImageData;
    }


    protected synchronized boolean setState(ImageState newState) {
        // The `ImageState.FAILED` state can always be set, since a command can fail after its image
        // data was decoded successfully, like if it requests a source rectangle that is outside the
        // image or if the bitmap for it cannot be created.
        // Any other state transition cannot go back or change if already at `ImageState.IMAGE_DECODED`
        if (newState != ImageState.FAILED &&
            (newState.getValue() < mCurrentState.getValue() || mCurrentState == ImageState.IMAGE_DECODED)) {
            Logger.logError(mClient, LOG_TAG,
                "Invalid image state transition from \"" + mCurrentState.getName() + "\" to " + "\"" + newState.getName() + "\"");
            return false;
        }

        // The `ImageState.FAILED` can be set again, like to add more errors, but we don't update
        // `mPreviousState` with the `mCurrentState` value if its at `ImageState.FAILED` to
        // preserve the last valid state.
        if (mCurrentState != ImageState.FAILED)
            mPreviousState = mCurrentState;

        mCurrentState = newState;
        return true;
    }


    /**
     * Set {@link ImageState#FAILED} state with the error code and message to send in the error
     * response for the command.
     *
     * @return Returns `false` always so that callers can return the value directly.
     */
    protected synchronized boolean setStateFailed(String errorCode, String errorMessage) {
        // Only keep the first error so that the error response is for the actual cause of failure.
        if (mErrorCode == null) {
            mErrorCode = errorCode;
            mErrorMessage = errorMessage;
        }

        Logger.logError(mClient, LOG_TAG, "Kitty graphics command failed with " + errorCode + ": " + errorMessage);
        setState(ImageState.FAILED);
        return false;
    }


    protected synchronized boolean ensureState(ImageState expectedState, String functionName) {
        if (mCurrentState != expectedState) {
            Logger.logError(mClient, LOG_TAG,
                "The current image state is \"" + mCurrentState.getName() + "\" but expected \"" + expectedState.getName() + "\"" +
                    " while calling '" + functionName + "'");
            return false;
        }
        return true;
    }


    /**
     * Read the `<control data>` of an `APC _G <control data> [; <payload>] ST` command.
     *
     * All the control data is read even if a key is invalid, so that the `i` and `q` keys required
     * to send the response for the command are read regardless of the order the keys are passed in.
     *
     * @param apcArgs The `APC` command arguments received, without the leading `ESC _`.
     * @param index The index in `apcArgs` at which the control data starts.
     * @return Returns the index at which the `base64` encoded payload starts, which will be equal
     * to the length of `apcArgs` if the command has no payload, or `-1` if the control data was
     * invalid, in which case {@link #getErrorCode()} will be set.
     */
    public synchronized int readControlData(StringBuilder apcArgs, int index) {
        if (!ensureState(ImageState.INIT, "KittyImage.readControlData()")) {
            return -1;
        }

        int argsLength = apcArgs.length();
        int payloadIndex = argsLength;

        while (index < argsLength) {
            // End of control data, the rest of the command is the payload.
            if (apcArgs.charAt(index) == ';') {
                payloadIndex = index + 1;
                break;
            }

            // All control data keys are a single character followed by a `=`.
            char argKey = apcArgs.charAt(index);
            if (index + 1 >= argsLength || apcArgs.charAt(index + 1) != '=') {
                setStateFailed(ERROR__EINVAL, "malformed control data");

                // Skip the malformed key so that the remaining keys are still read.
                while (index < argsLength && apcArgs.charAt(index) != ',' && apcArgs.charAt(index) != ';') {
                    index++;
                }
                if (index < argsLength && apcArgs.charAt(index) == ',') index++;
                continue;
            }

            int valueStartIndex = index + 2;
            int valueEndIndex = valueStartIndex;
            while (valueEndIndex < argsLength) {
                char ch = apcArgs.charAt(valueEndIndex);
                if (ch == ',' || ch == ';') break;
                valueEndIndex++;
            }

            readControlDataArg(argKey, apcArgs.substring(valueStartIndex, valueEndIndex));

            index = valueEndIndex;
            // Skip the `,` separator, the `;` is handled at the start of the loop.
            if (index < argsLength && apcArgs.charAt(index) == ',') index++;
        }

        if (isFailed()) {
            return -1;
        }

        setState(ImageState.ARGUMENTS_READ);

        return payloadIndex;
    }

    /** Read a single `<key>=<value>` pair of the control data. */
    private synchronized boolean readControlDataArg(char argKey, String argValue) {
        switch (argKey) {
            case 'a': // The action.
                if (argValue.length() != 1) return setStateFailed(ERROR__EINVAL, "invalid action");
                mAction = argValue.charAt(0);
                mHasAction = true;
                switch (mAction) {
                    case ACTION__TRANSMIT:
                    case ACTION__TRANSMIT_AND_DISPLAY:
                    case ACTION__PUT:
                    case ACTION__QUERY:
                    case ACTION__DELETE:
                        break;
                    default:
                        // The `a=f` (animation frame), `a=a` (animate) and `a=c` (compose) actions
                        // are valid protocol actions, but are not supported.
                        return setStateFailed(ERROR__ENOTSUP, "unsupported action");
                }
                break;
            case 'd': // The delete mode.
                if (argValue.length() != 1) return setStateFailed(ERROR__EINVAL, "invalid delete mode");
                mDeleteMode = argValue.charAt(0);
                break;
            case 't': // The transmission medium.
                if (argValue.length() != 1) return setStateFailed(ERROR__EINVAL, "invalid transmission medium");
                mMedium = argValue.charAt(0);
                if (mMedium != MEDIUM__DIRECT) {
                    // Reading the image data from a file, a temporary file or shared memory is not
                    // supported. Reading arbitrary files that the terminal has access to on behalf
                    // of a client is a security concern.
                    return setStateFailed(ERROR__ENOTSUP, "unsupported transmission medium");
                }
                break;
            case 'o': // The compression.
                if (argValue.length() != 1 || argValue.charAt(0) != 'z') {
                    return setStateFailed(ERROR__EINVAL, "invalid compression");
                }
                return setStateFailed(ERROR__ENOTSUP, "unsupported compression");
            case 'f': { // The image format.
                long value = readNumberArg(argValue, 0, FORMAT__PNG);
                if (value != FORMAT__RGB && value != FORMAT__RGBA && value != FORMAT__PNG) {
                    return setStateFailed(ERROR__EINVAL, "invalid image format");
                }
                mFormat = (int) value;
                break;
            }
            case 'i': { // The image id.
                long value = readNumberArg(argValue, 1, IMAGE_ID__MAX);
                if (value < 0) return setStateFailed(ERROR__EINVAL, "invalid image id");
                mImageId = value;
                break;
            }
            case 'p': { // The placement id.
                long value = readNumberArg(argValue, 1, PLACEMENT_ID__MAX);
                if (value < 0) return setStateFailed(ERROR__EINVAL, "invalid placement id");
                mPlacementId = value;
                break;
            }
            case 'm': { // Whether more chunks will follow.
                long value = readNumberArg(argValue, 0, 1);
                if (value < 0) return setStateFailed(ERROR__EINVAL, "invalid chunk flag");
                mMoreChunks = value == 1;
                break;
            }
            case 'q': { // The quiet level.
                long value = readNumberArg(argValue, 0, QUIET__ALL);
                if (value < 0) return setStateFailed(ERROR__EINVAL, "invalid quiet level");
                mQuiet = (int) value;
                mHasQuiet = true;
                break;
            }
            case 'c': { // The number of columns to display the image in.
                long value = readNumberArg(argValue, 0, PLACEMENT_CELLS__MAX);
                if (value < 0) return setStateFailed(ERROR__EINVAL, "invalid columns");
                mColumns = (int) value;
                break;
            }
            case 'r': { // The number of rows to display the image in.
                long value = readNumberArg(argValue, 0, PLACEMENT_CELLS__MAX);
                if (value < 0) return setStateFailed(ERROR__EINVAL, "invalid rows");
                mRows = (int) value;
                break;
            }
            case 's': { // The width in pixels of the raw pixel data.
                long value = readNumberArg(argValue, 0, PIXEL_DIMENSION__MAX);
                if (value < 0) return setStateFailed(ERROR__EINVAL, "invalid pixel width");
                mPixelWidth = (int) value;
                break;
            }
            case 'v': { // The height in pixels of the raw pixel data.
                long value = readNumberArg(argValue, 0, PIXEL_DIMENSION__MAX);
                if (value < 0) return setStateFailed(ERROR__EINVAL, "invalid pixel height");
                mPixelHeight = (int) value;
                break;
            }
            case 'C': { // Whether to not move the cursor after displaying the image.
                long value = readNumberArg(argValue, 0, 1);
                if (value < 0) return setStateFailed(ERROR__EINVAL, "invalid cursor movement policy");
                mDoNotMoveCursor = value == 1;
                break;
            }
            case 'x': { // The x coordinate in pixels of the source rectangle.
                long value = readNumberArg(argValue, 0, PIXEL_DIMENSION__MAX);
                if (value < 0) return setStateFailed(ERROR__EINVAL, "invalid source rectangle x");
                mSourceX = (int) value;
                break;
            }
            case 'y': { // The y coordinate in pixels of the source rectangle.
                long value = readNumberArg(argValue, 0, PIXEL_DIMENSION__MAX);
                if (value < 0) return setStateFailed(ERROR__EINVAL, "invalid source rectangle y");
                mSourceY = (int) value;
                break;
            }
            case 'w': { // The width in pixels of the source rectangle.
                long value = readNumberArg(argValue, 1, PIXEL_DIMENSION__MAX);
                if (value < 0) return setStateFailed(ERROR__EINVAL, "invalid source rectangle width");
                mSourceWidth = (int) value;
                break;
            }
            case 'h': { // The height in pixels of the source rectangle.
                long value = readNumberArg(argValue, 1, PIXEL_DIMENSION__MAX);
                if (value < 0) return setStateFailed(ERROR__EINVAL, "invalid source rectangle height");
                mSourceHeight = (int) value;
                break;
            }
            default:
                // The keys for unsupported features of the protocol are read but ignored, since a
                // client is expected to pass them for images that can still be displayed. These are
                // the `I` (image number), `z` (z-index) and `U` (unicode placeholder) keys, the `S`,
                // `O`, `P` and `Q` keys for the transmission mediums and placement features that are
                // not supported, and any key that is not part of the protocol at all.
                // The `X` and `Y` keys, which offset the image by pixels inside the first cell it is
                // displayed in, are also ignored, so an image will be aligned to the cell grid
                // instead of being offset by up to one cell width and height.
                // The values of the ignored keys are not validated since they are not used.
                break;
        }
        return true;
    }

    /**
     * Read an unsigned decimal control data value.
     *
     * @return Returns the value read, or `-1` if it was not a decimal number or was not within the
     * `minValue` and `maxValue` range.
     */
    private static long readNumberArg(String argValue, long minValue, long maxValue) {
        // A value of more than 10 digits cannot be a valid 32-bit unsigned integer, and the check
        // also prevents an overflow of `value` below.
        if (argValue.isEmpty() || argValue.length() > 10) return -1;

        long value = 0;
        for (int i = 0; i < argValue.length(); i++) {
            char ch = argValue.charAt(i);
            if (ch < '0' || ch > '9') return -1;
            value = (value * 10) + (ch - '0');
        }

        return (value < minValue || value > maxValue) ? -1 : value;
    }


    /**
     * Read the control data of a continuation chunk of the image being received by this instance.
     * As per the protocol, a continuation chunk only passes the `m` and `q` keys, and all the other
     * keys are inherited from the first chunk of the image.
     *
     * @param continuationChunk The {@link KittyImage} whose control data was read for the
     *                          continuation chunk command.
     */
    public synchronized void readContinuationControlData(KittyImage continuationChunk) {
        mMoreChunks = continuationChunk.hasMoreChunks();

        // The quiet level of the first chunk of the image is kept if the continuation chunk did not
        // pass the `q` key, so that a response is not sent for an image whose transmission was
        // started with responses suppressed.
        if (continuationChunk.mHasQuiet) {
            mQuiet = continuationChunk.getQuiet();
            mHasQuiet = true;
        }
    }


    /**
     * Start receiving the `base64` encoded `<payload>` of an `APC _G <control data> ; <payload> ST`
     * command, which is decoded by {@link #readImageChar(char)} as it is received instead of being
     * collected first, so that the size of the payload of a single command is not limited.
     *
     * @return Returns `true` if the payload can be received, otherwise `false`, in which case
     * {@link #getErrorCode()} will be set.
     */
    public synchronized boolean startImage() {
        if (mCurrentState != ImageState.IMAGE_READING &&
            !ensureState(ImageState.ARGUMENTS_READ, "KittyImage.startImage()")) {
            return false;
        }

        // The exact length of the image data is known in advance for the raw formats, so the limit
        // can be checked before all of it has been received, and the buffer for it can be allocated
        // exactly once with the required size instead of being grown as more chunks are received.
        if (isRawFormat() && mImageData == null && mPixelWidth > 0 && mPixelHeight > 0) {
            long imageDataLength = (long) mPixelWidth * mPixelHeight * getBytesPerPixel(mFormat);
            if (imageDataLength > IMAGE_DATA__MAX_LENGTH) {
                return setStateFailed(ERROR__ENOSPC, "image data exceeds max length " + IMAGE_DATA__MAX_LENGTH);
            }

            mImageDataExpectedLength = (int) imageDataLength;
            if (!ensureImageDataCapacity(mImageDataExpectedLength)) return false;
        }

        // An empty payload can be received as well, so the state is changed even if no character of
        // the payload is received at all.
        setState(ImageState.IMAGE_READING);

        return true;
    }

    /**
     * Decode a single `base64` character of the payload of the current chunk of the image.
     *
     * The payload of a chunk does not have to be a multiple of the 4 `base64` characters that decode
     * to 3 bytes, so the bits that do not complete a byte are carried over to the next chunk.
     *
     * @return Returns `true` if the character was decoded, otherwise `false`, in which case
     * {@link #getErrorCode()} will be set. The remaining characters of the payload must still be
     * passed to this method so that they are not printed on the terminal as text, and they will be
     * discarded.
     */
    public synchronized boolean readImageChar(char ch) {
        // The rest of the payload is discarded once the command has failed.
        if (isFailed()) return false;

        if (ch == '=') {
            // The padding is only valid at the very end of the `base64` data, so it can only be
            // received in the final chunk of the image.
            if (mMoreChunks) {
                return setStateFailed(ERROR__EBADDATA, "base64 padding in a chunk that is not the final chunk");
            }

            mBase64Padding++;
            mBase64Length++;
            return true;
        }

        if (mBase64Padding > 0) {
            return setStateFailed(ERROR__EBADDATA, "base64 data after the padding");
        }

        int value = (ch < 128) ? BASE64__DECODE_TABLE[ch] : BASE64__INVALID;
        if (value < 0) {
            return setStateFailed(ERROR__EBADDATA, "invalid base64 image data");
        }

        mBase64Length++;
        mBase64Buffer = (mBase64Buffer << 6) | value;
        mBase64BufferBits += 6;

        if (mBase64BufferBits >= 8) {
            mBase64BufferBits -= 8;

            // A client that sends more data than the dimensions it declared is rejected before the
            // memory for the extra data is allocated.
            if (mImageDataExpectedLength > 0 && mImageDataLength >= mImageDataExpectedLength) {
                return setStateFailed(ERROR__EINVAL, "image data exceeds the " + mImageDataExpectedLength +
                    " bytes required for pixel dimensions " + mPixelWidth + "x" + mPixelHeight);
            }

            if (mImageData == null || mImageDataLength >= mImageData.length) {
                if (!ensureImageDataCapacity(mImageDataLength + 1)) return false;
            }

            mImageData[mImageDataLength++] = (byte) (mBase64Buffer >>> mBase64BufferBits);
        }

        return true;
    }

    /**
     * Ensure {@link #mImageData} can hold `capacity` bytes.
     *
     * @return Returns `true` if the buffer can hold `capacity` bytes, otherwise `false`, in which
     * case {@link #getErrorCode()} will be set.
     */
    private synchronized boolean ensureImageDataCapacity(int capacity) {
        if (capacity > IMAGE_DATA__MAX_LENGTH) {
            return setStateFailed(ERROR__ENOSPC, "image data exceeds max length " + IMAGE_DATA__MAX_LENGTH);
        }

        if (mImageData != null && mImageData.length >= capacity) return true;

        // The buffer is grown by doubling its size so that it is not reallocated and copied for
        // every chunk received, since the total length of the image data of a compressed image
        // format is not known in advance.
        long newCapacity = Math.max(capacity, IMAGE_DATA__INITIAL_CAPACITY);
        if (mImageData != null) {
            newCapacity = Math.max(newCapacity, Math.min((long) mImageData.length * 2, IMAGE_DATA__MAX_LENGTH));
        }

        try {
            // Allocating the new array and copying the old one into it can cause an OOM.
            mImageData = (mImageData == null) ? new byte[(int) newCapacity] : Arrays.copyOf(mImageData, (int) newCapacity);
        } catch (Throwable t) {
            if (t instanceof OutOfMemoryError) System.gc();
            return setStateFailed(ERROR__ENOSPC, "allocating " + newCapacity + " bytes for image data failed: " + t.getMessage());
        }

        return true;
    }


    /**
     * Finish decoding the image data received for all the chunks of the image and validate it
     * against the image format and dimensions passed in the control data.
     *
     * @return Returns `true` if the image data is valid, otherwise `false`, in which case
     * {@link #getErrorCode()} will be set.
     */
    public synchronized boolean finishImage() {
        if (!ensureState(ImageState.IMAGE_READING, "KittyImage.finishImage()")) {
            return setStateFailed(ERROR__EBADDATA, "incomplete image data");
        }

        setState(ImageState.IMAGE_READ);

        // A single remaining `base64` character only holds 6 bits and cannot encode a byte.
        if (mBase64BufferBits == 6) {
            return setStateFailed(ERROR__EBADDATA, "invalid base64 image data");
        }

        // The padding does not add any bits, so it is not required to decode the data, but if it is
        // present, then there can be at most two padding characters and the total number of
        // characters must be a multiple of four.
        if (mBase64Padding > 0 && (mBase64Padding > BASE64__MAX_PADDING || (mBase64Length % 4) != 0)) {
            return setStateFailed(ERROR__EBADDATA, "invalid base64 image data");
        }

        if (mImageDataLength < 1) {
            mImageData = null;
            return setStateFailed(ERROR__EBADDATA, "empty image data");
        }

        // The pixel dimensions of raw image data are not part of the data itself, so they must be
        // passed with the `s` and `v` keys and must match the length of the data exactly.
        if (isRawFormat()) {
            if (mPixelWidth < 1 || mPixelHeight < 1) {
                int imageDataLength = mImageDataLength;
                mImageData = null;
                return setStateFailed(ERROR__EINVAL, "missing pixel dimensions for" +
                    " image data of length " + imageDataLength);
            }

            if (mImageDataLength != mImageDataExpectedLength) {
                int imageDataLength = mImageDataLength;
                mImageData = null;
                return setStateFailed(ERROR__EINVAL, "image data of length " + imageDataLength +
                    " does not match the " + mImageDataExpectedLength + " bytes required for" +
                    " pixel dimensions " + mPixelWidth + "x" + mPixelHeight);
            }
        }

        // The buffer is grown in advance, so the extra bytes at its end must be released. This never
        // creates a copy of the image data of a raw format image, since its buffer was allocated with
        // the exact length required.
        if (mImageData.length != mImageDataLength) {
            try {
                mImageData = Arrays.copyOf(mImageData, mImageDataLength);
            } catch (Throwable t) {
                if (t instanceof OutOfMemoryError) System.gc();
                return setStateFailed(ERROR__ENOSPC, "trimming image data to" +
                    " length " + mImageDataLength + " failed: " + t.getMessage());
            }
        }

        setState(ImageState.IMAGE_DECODED);
        return true;
    }


    private static int[] createBase64DecodeTable() {
        int[] table = new int[128];
        Arrays.fill(table, BASE64__INVALID);

        String alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        for (int i = 0; i < alphabet.length(); i++) {
            table[alphabet.charAt(i)] = i;
        }
        return table;
    }

}
