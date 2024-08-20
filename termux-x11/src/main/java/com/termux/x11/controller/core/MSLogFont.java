package com.termux.x11.controller.core;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class MSLogFont {
    private int height = -11;
    private int width = 0;
    private int escapement = 0;
    private int orientation = 0;
    private int weight = 400;
    private byte italic = 0;
    private byte underline = 0;
    private byte strikeOut = 0;
    private byte charSet = 0;
    private byte outPrecision = 0;
    private byte clipPrecision = 0;
    private byte quality = 0;
    private byte pitchAndFamily = 34;
    private String faceName = "Tahoma";

    public int getHeight() {
        return height;
    }

    public MSLogFont setHeight(int height) {
        this.height = height;
        return this;
    }

    public int getWidth() {
        return width;
    }

    public MSLogFont setWidth(int width) {
        this.width = width;
        return this;
    }

    public int getEscapement() {
        return escapement;
    }

    public MSLogFont setEscapement(int escapement) {
        this.escapement = escapement;
        return this;
    }

    public int getOrientation() {
        return orientation;
    }

    public MSLogFont setOrientation(int orientation) {
        this.orientation = orientation;
        return this;
    }

    public int getWeight() {
        return weight;
    }

    public MSLogFont setWeight(int weight) {
        this.weight = weight;
        return this;
    }

    public byte getItalic() {
        return italic;
    }

    public MSLogFont setItalic(byte italic) {
        this.italic = italic;
        return this;
    }

    public byte getUnderline() {
        return underline;
    }

    public MSLogFont setUnderline(byte underline) {
        this.underline = underline;
        return this;
    }

    public byte getStrikeOut() {
        return strikeOut;
    }

    public MSLogFont setStrikeOut(byte strikeOut) {
        this.strikeOut = strikeOut;
        return this;
    }

    public byte getCharSet() {
        return charSet;
    }

    public MSLogFont setCharSet(byte charSet) {
        this.charSet = charSet;
        return this;
    }

    public byte getOutPrecision() {
        return outPrecision;
    }

    public MSLogFont setOutPrecision(byte outPrecision) {
        this.outPrecision = outPrecision;
        return this;
    }

    public byte getClipPrecision() {
        return clipPrecision;
    }

    public MSLogFont setClipPrecision(byte clipPrecision) {
        this.clipPrecision = clipPrecision;
        return this;
    }

    public byte getQuality() {
        return quality;
    }

    public MSLogFont setQuality(byte quality) {
        this.quality = quality;
        return this;
    }

    public byte getPitchAndFamily() {
        return pitchAndFamily;
    }

    public MSLogFont setPitchAndFamily(byte pitchAndFamily) {
        this.pitchAndFamily = pitchAndFamily;
        return this;
    }

    public String getFaceName() {
        return faceName;
    }

    public MSLogFont setFaceName(String faceName) {
        this.faceName = faceName;
        return this;
    }

    public byte[] toByteArray() {
        ByteBuffer data = ByteBuffer.allocate(92).order(ByteOrder.LITTLE_ENDIAN);
        data.putInt(height);
        data.putInt(width);
        data.putInt(escapement);
        data.putInt(orientation);
        data.putInt(weight);
        data.put(italic);
        data.put(underline);
        data.put(strikeOut);
        data.put(charSet);
        data.put(outPrecision);
        data.put(clipPrecision);
        data.put(quality);
        data.put(pitchAndFamily);

        for (int i = 0; i < faceName.length(); i++) data.putChar(faceName.charAt(i));
        return data.array();
    }
}
