package com.termux.display.controller.core;

import android.graphics.Bitmap;
import android.graphics.Color;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public abstract class MSBitmap {
    public static boolean create(Bitmap bitmap, File outputFile) {
        int width = bitmap.getWidth();
        int height = bitmap.getHeight();

        int[] pixels = new int[width * height];
        bitmap.getPixels(pixels, 0, width, 0, 0, width, height);

        int extraBytes = width % 4;
        int imageSize = height * (3 * width + extraBytes);
        int infoHeaderSize = 40;
        int dataOffset = 54;
        int bitCount = 24;
        int planes = 1;
        int compression = 0;
        int hr = 0;
        int vr = 0;
        int colorsUsed = 0;
        int colorsImportant = 0;
        
        ByteBuffer buffer = ByteBuffer.allocate(dataOffset + imageSize).order(ByteOrder.LITTLE_ENDIAN);

        buffer.put(new byte[]{'B', 'M'});
        buffer.putInt(dataOffset + imageSize);
        buffer.putInt(0);
        buffer.putInt(dataOffset);

        buffer.putInt(infoHeaderSize);
        buffer.putInt(width);
        buffer.putInt(height);
        buffer.putShort((short)planes);
        buffer.putShort((short)bitCount);
        buffer.putInt(compression);
        buffer.putInt(imageSize);
        buffer.putInt(hr);
        buffer.putInt(vr);
        buffer.putInt(colorsUsed);
        buffer.putInt(colorsImportant);

        int rowBytes = 3 * width + extraBytes;
        for (int y = height - 1, i = 0, j; y >= 0; y--) {
            for (int x = 0; x < width; x++) {
                j = dataOffset + y * rowBytes + x * 3;
                int pixel = pixels[i++];
                buffer.put(j+0, (byte)Color.blue(pixel));
                buffer.put(j+1, (byte)Color.green(pixel));
                buffer.put(j+2, (byte)Color.red(pixel));
            }

            if (extraBytes > 0) {
                int fillOffset = dataOffset + y * rowBytes + width * 3;
                for (j = fillOffset; j < fillOffset + extraBytes; j++) buffer.put(j, (byte)255);
            }
        }

        try (FileOutputStream fos = new FileOutputStream(outputFile)) {
            fos.write(buffer.array());
            return true;
        }
        catch (IOException e) {
            return false;
        }
    }
}
