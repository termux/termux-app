package com.termux.x11.controller.inputcontrols;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class GamepadState {
    public float thumbLX = 0;
    public float thumbLY = 0;
    public float thumbRX = 0;
    public float thumbRY = 0;
    public final boolean[] dpad = new boolean[4];
    public short buttons = 0;

    public byte getPovHat() {
        byte povHat = -1;
        if (dpad[0] && dpad[1]) povHat = 1;
        else if (dpad[1] && dpad[2]) povHat = 3;
        else if (dpad[2] && dpad[3]) povHat = 5;
        else if (dpad[3] && dpad[0]) povHat = 7;
        else if (dpad[0]) povHat = 0;
        else if (dpad[1]) povHat = 2;
        else if (dpad[2]) povHat = 4;
        else if (dpad[3]) povHat = 6;
        return povHat;
    }

    public void writeTo(ByteBuffer buffer) {
        buffer.putShort(buttons);
        buffer.put(getPovHat());
        buffer.putShort((short)(thumbLX * Short.MAX_VALUE));
        buffer.putShort((short)(thumbLY * Short.MAX_VALUE));
        buffer.putShort((short)(thumbRX * Short.MAX_VALUE));
        buffer.putShort((short)(thumbRY * Short.MAX_VALUE));
    }

    public void setPressed(int buttonIdx, boolean pressed) {
        int flag = 1<<buttonIdx;
        if (pressed) {
            buttons |= flag;
        }
        else buttons &= ~flag;
    }

    public boolean isPressed(int buttonIdx) {
        return (buttons & (1<<buttonIdx)) != 0;
    }

    public byte getDPadX() {
        return (byte)(dpad[1] ? 1 : (dpad[3] ? -1 : 0));
    }

    public byte getDPadY() {
        return (byte)(dpad[0] ? -1 : (dpad[2] ? 1 : 0));
    }

    public void copy(GamepadState other) {
        this.thumbLX = other.thumbLX;
        this.thumbLY = other.thumbLY;
        this.thumbRX = other.thumbRX;
        this.thumbRY = other.thumbRY;
        this.buttons = other.buttons;
        System.arraycopy(other.dpad, 0, this.dpad, 0, 4);
    }
    public byte[] toByteArray() {
        ByteBuffer buffer = ByteBuffer.allocate(11).order(ByteOrder.LITTLE_ENDIAN);
        writeTo(buffer);
        return buffer.array();
    }
}
