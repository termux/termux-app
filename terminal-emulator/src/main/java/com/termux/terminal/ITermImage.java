package com.termux.terminal;

import android.util.Base64;

import java.util.Arrays;

/**
 * An iTerm image received via `OSC 1337`.
 *
 * - https://iterm2.com/documentation-images.html
 */
public class ITermImage {

    public static final String LOG_TAG = "ITermImage";



    /** The {@link Enum} that defines {@link ITermImage} state. */
    public enum ImageState {

        INIT("init", 0),
        ARGUMENTS_READ("arguments_read", 1),
        IMAGE_READING("image_reading", 2),
        IMAGE_READ("image_read", 3),
        IMAGE_DECODED("image_decoded", 4),
        FAILED("Failed", 5);

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



    protected final TerminalSessionClient mClient;

    protected final boolean mIsMultipart;

    protected int mWidth = -1;
    protected int mHeight = -1;

    protected boolean mInline = false;

    protected boolean mPreserveAspectRatio = true;

    protected final StringBuilder mEncodedImage = new StringBuilder(/* Initial capacity. */ 4096);
    protected byte[] mDecodedImage;

    /** The current state of the {@link ImageState}. */
    protected ImageState mCurrentState = ImageState.INIT;
    /** The previous state of the {@link ImageState}. */
    protected ImageState mPreviousState = ImageState.INIT;



    protected ITermImage(TerminalSessionClient client, boolean isMultiPart) {
        mClient = client;

        mIsMultipart = isMultiPart;
    }



    public TerminalSessionClient getClient() {
        return mClient;
    }


    public boolean isMultipart() {
        return mIsMultipart;
    }


    public int getWidth() {
        return mWidth;
    }

    public int getHeight() {
        return mHeight;
    }


    public boolean isInline() {
        return mInline;
    }


    public boolean shouldPreserveAspectRatio() {
        return mPreserveAspectRatio;
    }


    public String getEncodedImage() {
        return mEncodedImage.toString();
    }

    public byte[] getDecodedImage() {
        return mDecodedImage;
    }


    public synchronized ImageState getCurrentState() {
        return mCurrentState;
    }

    public synchronized ImageState getPreviousState() {
        return mPreviousState;
    }


    protected synchronized boolean setState(ImageState newState) {
        // The state transition cannot go back or change if already at `ImageState.IMAGE_DECODED`
        if (newState.getValue() < mCurrentState.getValue() || mCurrentState == ImageState.IMAGE_DECODED) {
            Logger.logError(mClient, LOG_TAG,
                "Invalid image state transition from \"" + mCurrentState.getName() + "\" to " +  "\"" + newState.getName() + "\"");
            return false;
        }

        // The `ImageState.FAILED` can be set again, like to add more errors, but we don't update
        // `mPreviousState` with the `mCurrentState` value if its at `ImageState.FAILED` to
        // preserve the last valid state.
        if (mCurrentState != ImageState.FAILED)
            mPreviousState = mCurrentState;

        mCurrentState = newState;
        return  true;
    }


    protected synchronized boolean setStateFailed(String error) {
        if (error != null) {
            Logger.logError(mClient, LOG_TAG, error);
        }
        return setState(ImageState.FAILED);
    }


    protected synchronized boolean ensureState(ImageState expectedState) {
        return ensureState(expectedState, null);
    }

    protected synchronized boolean ensureState(ImageState expectedState, String functionName) {
        if (mCurrentState != expectedState) {
            Logger.logError(mClient, LOG_TAG,
                "The current image state is \"" + mCurrentState.getName() + "\" but expected \"" + expectedState.getName() + "\"" +
                (functionName != null ? " while calling '" + functionName : "'") +
                " for " + (!mIsMultipart ? "singlepart" : "multipart") + " image");
            return false;
        }
        return true;
    }


    public synchronized boolean isArgumentsRead() {
        return mCurrentState == ImageState.ARGUMENTS_READ;
    }

    public synchronized boolean isImageReading() {
        return mCurrentState == ImageState.IMAGE_READING;
    }

    public synchronized boolean isImageRead() {
        return mCurrentState == ImageState.IMAGE_READ;
    }

    public synchronized boolean isImageDecoded() {
        return mCurrentState == ImageState.IMAGE_DECODED;
    }



    public synchronized int readArguments(TerminalEmulator terminalEmulator, StringBuilder oscArgs, int index) {
        if (!ensureState(ImageState.INIT, "ImageState.readArguments()")) {
            return -1;
        }

        boolean lastParam = false;
        while (index < oscArgs.length()) {
            char ch = oscArgs.charAt(index);
            // End of optional arguments.
            if (ch == ':' && !mIsMultipart) {
                break;
            } else if (ch == ' ') {
                index++;
                continue;
            }

            int keyEndIndex = oscArgs.indexOf("=", index);
            if (keyEndIndex == -1) {
                setStateFailed("The key for an argument not found after index " + index + " in osc argument string: " + oscArgs);
                return -1;
            }
            String argKey = oscArgs.substring(index, keyEndIndex);

            int valueEndIndex = oscArgs.indexOf(";", keyEndIndex);
            if (valueEndIndex == -1) {
                if (!mIsMultipart) {
                    // The last key value for `File=` command arguments may end with a colon `:` instead of a semi colon `;`.
                    valueEndIndex = oscArgs.indexOf(":", keyEndIndex);
                    if (valueEndIndex == -1) {
                        setStateFailed("The value for an argument not found after index " + index + " in osc argument string: " + oscArgs);
                        return -1;
                    } else {
                        index = valueEndIndex;
                        lastParam = true;
                    }
                } else {
                    // The last key value for `MultipartFile=` command arguments may end without a semi colon `;`.
                    valueEndIndex = oscArgs.length();
                    index = valueEndIndex;
                }
            } else {
                index = valueEndIndex + 1;
            }

            if (valueEndIndex <= keyEndIndex) {
                setStateFailed("The argument key end index " + keyEndIndex +
                    " is <= value end index " + valueEndIndex + " in osc argument string: " + oscArgs);
                return -1;
            }

            String argValue = oscArgs.substring(keyEndIndex + 1, valueEndIndex);

            if (argKey.equalsIgnoreCase("inline")) {
                mInline = argValue.equals("1");
            }
            else if (argKey.equalsIgnoreCase("preserveAspectRatio")) {
                mPreserveAspectRatio = !argValue.equals("0");
            }
            else if (argKey.equalsIgnoreCase("width")) {
                double factor = terminalEmulator.getCellWidthPixels();
                int intValueEndIndex = argValue.length();
                if (argValue.endsWith("px")) {
                    factor = 1;
                    intValueEndIndex -= 2;
                } else if (argValue.endsWith("%")) {
                    factor = 0.01 * terminalEmulator.getCellWidthPixels() * terminalEmulator.getColumns();
                    intValueEndIndex -= 1;
                }
                try {
                    mWidth = (int) (factor * Integer.parseInt(argValue.substring(0, intValueEndIndex)));
                } catch (Exception e) {
                }
            }
            else if (argKey.equalsIgnoreCase("height")) {
                double factor = terminalEmulator.getCellHeightPixels();
                int intValueEndIndex = argValue.length();
                if (argValue.endsWith("px")) {
                    factor = 1;
                    intValueEndIndex -= 2;
                } else if (argValue.endsWith("%")) {
                    factor = 0.01 * terminalEmulator.getCellHeightPixels() * terminalEmulator.getRows();
                    intValueEndIndex -= 1;
                }
                try {
                    mHeight = (int) (factor * Integer.parseInt(argValue.substring(0, intValueEndIndex)));
                } catch (Exception e) {
                }
            } else {
                // `name` and `size` keys are not supported.
            }

            if (lastParam) {
                break;
            }
        }

        setState(ImageState.ARGUMENTS_READ);

        return index;
    }


    public synchronized boolean readImage(StringBuilder oscArgs, int index) {
        if (!mIsMultipart) {
            if (!ensureState(ImageState.ARGUMENTS_READ, "ImageState.readImage()")) {
                return false;
            }

            if (index < oscArgs.length()) {
                int colonIndex = oscArgs.indexOf(":", index);
                if (colonIndex >= 0 && colonIndex + 1 < oscArgs.length()) {
                    setState(ImageState.IMAGE_READING);
                    int imageStartIndex = colonIndex + 1;

                    try {
                        // Appending can cause an increase in capacity and cause an OOM.
                        mEncodedImage.append(oscArgs.substring(imageStartIndex));
                    } catch (Throwable t) {
                        if (t instanceof OutOfMemoryError) System.gc();
                        setStateFailed("Collecting singlepart image" + " in osc argument string failed: " + t.getMessage());
                        return false;
                    }

                    setState(ImageState.IMAGE_READ);
                    return true;
                }
            }

            setStateFailed("Failed to read singlepart image from index " + index + " in osc argument string: " + oscArgs);
            return false;
        } else {
            if (mCurrentState != ImageState.IMAGE_READING &&
                !ensureState(ImageState.ARGUMENTS_READ, "ImageState.readImage()")) {
                return false;
            }

            // An empty `FilePart=` command could be received as well, so change state before `if` below.
            setState(ImageState.IMAGE_READING);

            if (index < oscArgs.length()) {
                try {
                    // Appending can cause an increase in capacity and cause an OOM.
                    mEncodedImage.append(oscArgs.substring(index));
                } catch (Throwable t) {
                    if (t instanceof OutOfMemoryError) System.gc();
                    setStateFailed("Collecting multipart image" + " in osc argument string failed: " + t.getMessage());
                    return false;
                }
                return true;
            }

            setStateFailed("Failed to read multipart image" + " in osc argument string: " + oscArgs);
            return false;
        }
    }

    public synchronized boolean setMultiPartImageRead() {
        if (!mIsMultipart) {
            Logger.logError(mClient, LOG_TAG, "Attempting to call 'ImageState.setMultiPartImageRead()' for a singlepart image");
            return false;
        }

        // A `FileEnd` command may have been received without a `FilePart=` command preceding it.
        if (!ensureState(ImageState.IMAGE_READING, "ImageState.setMultiPartImageRead()")) {
            return false;
        }

        setState(ImageState.IMAGE_READ);
        return true;
    }


    public synchronized boolean decodeImage() {
        if (!ensureState(ImageState.IMAGE_READ, "ImageState.decodeImage()")) {
            return false;
        }

        String encodedImageString = null;
        try {
            if (mEncodedImage.length() < 1) {
                setStateFailed("Cannot decoded an empty image");
                return false;
            }

            while (mEncodedImage.length() % 4 != 0) {
                mEncodedImage.append('=');
            }

            encodedImageString = mEncodedImage.toString();

            // Clear original encoded image from memory as it is no longer needed.
            mEncodedImage.setLength(0);
            mEncodedImage.trimToSize();

            mDecodedImage = Base64.decode(encodedImageString, Base64.DEFAULT);
            if (mDecodedImage == null || mDecodedImage.length < 2) {
                setStateFailed("The decoded image is not valid: " + Arrays.toString(mDecodedImage) + "\nimage: " + encodedImageString);
                return false;
            }

            setState(ImageState.IMAGE_DECODED);
            return true;
        } catch (Throwable t) {
            if (t instanceof OutOfMemoryError) {
                Logger.logError(mClient, LOG_TAG, "Failed to decode image: " + t.getMessage());
                System.gc();
            } else {
                Logger.logStackTraceWithMessage(mClient, LOG_TAG, "Failed to decode image: " + encodedImageString, t);
            }
            setStateFailed(null);
            return false;
        }
    }

}
