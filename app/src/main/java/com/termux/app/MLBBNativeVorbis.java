package com.termux.app;

import java.io.IOException;

/** JNI facade for the offline arm64-v8a VorbisLib codec. */
public final class MLBBNativeVorbis {
    private static final boolean AVAILABLE;
    static {
        boolean loaded;
        try { System.loadLibrary("mlbb_vorbis_jni"); loaded=true; }
        catch (UnsatisfiedLinkError | SecurityException ex) { loaded=false; }
        AVAILABLE=loaded;
    }
    private MLBBNativeVorbis() {}
    public static boolean available() { return AVAILABLE; }
    public static byte[] encode(byte[] wavOrOgg, float quality, int maxFrames) throws IOException {
        if(!AVAILABLE)throw new IOException("ARM64 native VorbisLib is not bundled in this APK");
        if(wavOrOgg==null||wavOrOgg.length==0||maxFrames<=0)throw new IOException("Invalid audio input");
        return nativeEncode(wavOrOgg,quality,maxFrames);
    }
    public static byte[] decodeToOgg(byte[] wem, boolean legacyCodebooks) throws IOException {
        if(!AVAILABLE)throw new IOException("ARM64 native VorbisLib is not bundled in this APK");
        if(wem==null||wem.length==0)throw new IOException("Invalid WEM");
        return nativeToOgg(wem,legacyCodebooks);
    }
    private static native byte[] nativeEncode(byte[] input, float quality, int maxFrames) throws IOException;
    private static native byte[] nativeToOgg(byte[] wem, boolean legacyCodebooks) throws IOException;
}
