package com.termux.display.controller.xserver;

import androidx.annotation.NonNull;

import com.termux.display.controller.core.ArrayUtils;
import com.termux.display.controller.core.StringUtils;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;

public class Property {
    public enum Mode {REPLACE, PREPEND, APPEND}
    public enum Format {
        BYTE_ARRAY(8), SHORT_ARRAY(16), INT_ARRAY(32);
        public final byte value;

        Format(int value) {
            this.value = (byte)value;
        }

        public static Format valueOf(int format) {
            switch (format) {
                case 8: return BYTE_ARRAY;
                case 16: return SHORT_ARRAY;
                case 32: return INT_ARRAY;
            }
            return null;
        }
    }
    public final int name;
    public final int type;
    public final Format format;
    private byte[] data;

    public Property(int name, int type, Format format, byte[] data) {
        this.name = name;
        this.type = type;
        this.format = format;
        replace(data);
    }

    public byte[] data() {
        return data;
    }

    public void replace(byte[] data) {
        this.data = data != null ? data : new byte[0];
    }

    public void prepend(byte[] values) {
        this.data = ArrayUtils.concat(values, this.data);
    }

    public void append(byte[] values) {
        this.data = ArrayUtils.concat(this.data, values);
    }

    @NonNull
    @Override
    public String toString() {
        String type = Atom.getName(this.type);
        switch (type) {
            case "UTF8_STRING":
                return StringUtils.fromANSIString(data, StandardCharsets.UTF_8);
//            case "STRING":
//                return StringUtils.fromANSIString(data, XServer.LATIN1_CHARSET);
            case "ATOM":
                return Atom.getName(ByteBuffer.wrap(data).order(ByteOrder.LITTLE_ENDIAN).getInt());
            default:
                StringBuilder sb = new StringBuilder();
                ByteBuffer buffer = ByteBuffer.wrap(data).order(ByteOrder.LITTLE_ENDIAN);
                for (int i = 0, size = data.length / (format.value >> 3); i < size; i++) {
                    if (i > 0) sb.append(",");
                    switch (format) {
                        case BYTE_ARRAY:
                            sb.append(buffer.get());
                            break;
                        case SHORT_ARRAY:
                            sb.append(buffer.getShort());
                            break;
                        case INT_ARRAY:
                            sb.append(buffer.getInt());
                            break;
                    }
                }
                return sb.toString();
        }
    }
}
