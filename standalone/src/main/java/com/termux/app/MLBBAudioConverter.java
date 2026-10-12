package com.termux.app;

import android.content.Context;
import android.media.AudioFormat;
import android.media.MediaCodec;
import android.media.MediaExtractor;
import android.media.MediaFormat;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
import java.io.*;
import java.nio.*;
import java.nio.charset.StandardCharsets;

/**
 * Android system-decoder-backed MP3 / Ogg Vorbis / AAC / FLAC / WAV importer.
 * Output: Wwise-compatible RIFF PCM16 WEM. This is not Custom Vorbis encoding:
 * games which require the original Wwise codec may not play the replacement.
 */
public final class MLBBAudioConverter {
    private MLBBAudioConverter(){}
    private static void u16(OutputStream o,int v)throws IOException{
        o.write(v&255);o.write(v>>>8&255);
    }
    private static void u32(OutputStream o,long v)throws IOException{
        o.write((int)v&255);o.write((int)(v>>>8)&255);
        o.write((int)(v>>>16)&255);o.write((int)(v>>>24)&255);
    }
    public static byte[] toPcmWem(Context context,Uri source)throws Exception {
        return decodeToWav(context, source);
    }
    /** Decode via Android MediaCodec to a standard RIFF PCM16 WAV for native VorbisLib. */
    public static byte[] decodeToWav(Context context,Uri source)throws Exception {
        MediaExtractor extractor=new MediaExtractor();
        MediaCodec codec=null;
        try(ParcelFileDescriptor fd=context.getContentResolver().openFileDescriptor(source,"r")){
            if(fd==null)throw new IOException("Cannot open audio file");
            extractor.setDataSource(fd.getFileDescriptor());
            int track=-1;
            for(int i=0;i<extractor.getTrackCount();i++){
                String mime=extractor.getTrackFormat(i).getString(MediaFormat.KEY_MIME);
                if(mime!=null&&mime.startsWith("audio/")){track=i;break;}
            }
            if(track<0)throw new IOException("No audio track found");
            extractor.selectTrack(track);
            MediaFormat initial=extractor.getTrackFormat(track);
            String mime=initial.getString(MediaFormat.KEY_MIME);
            codec=MediaCodec.createDecoderByType(mime);
            codec.configure(initial,null,null,0);
            codec.start();
            boolean inputEnded=false, outputEnded=false;
            int channels=initial.containsKey(MediaFormat.KEY_CHANNEL_COUNT)?initial.getInteger(MediaFormat.KEY_CHANNEL_COUNT):2;
            int rate=initial.containsKey(MediaFormat.KEY_SAMPLE_RATE)?initial.getInteger(MediaFormat.KEY_SAMPLE_RATE):48000;
            ByteArrayOutputStream audio=new ByteArrayOutputStream();
            MediaCodec.BufferInfo info=new MediaCodec.BufferInfo();
            while(!outputEnded){
                if(!inputEnded){
                    int input=codec.dequeueInputBuffer(10000);
                    if(input>=0){
                        ByteBuffer target=codec.getInputBuffer(input);
                        if(target==null)throw new IOException("Audio input buffer missing");
                        target.clear();
                        int count=extractor.readSampleData(target,0);
                        if(count<0){
                            codec.queueInputBuffer(input,0,0,0,MediaCodec.BUFFER_FLAG_END_OF_STREAM);
                            inputEnded=true;
                        }else{
                            codec.queueInputBuffer(input,0,count,extractor.getSampleTime(),0);
                            extractor.advance();
                        }
                    }
                }
                int index=codec.dequeueOutputBuffer(info,10000);
                if(index==MediaCodec.INFO_OUTPUT_FORMAT_CHANGED){
                    MediaFormat out=codec.getOutputFormat();
                    if(out.containsKey(MediaFormat.KEY_CHANNEL_COUNT))channels=out.getInteger(MediaFormat.KEY_CHANNEL_COUNT);
                    if(out.containsKey(MediaFormat.KEY_SAMPLE_RATE))rate=out.getInteger(MediaFormat.KEY_SAMPLE_RATE);
                    if(out.containsKey(MediaFormat.KEY_PCM_ENCODING) && out.getInteger(MediaFormat.KEY_PCM_ENCODING)!=AudioFormat.ENCODING_PCM_16BIT)
                        throw new IOException("Decoder output is not PCM16; cannot encode safely");
                }else if(index>=0){
                    ByteBuffer output=codec.getOutputBuffer(index);
                    if(output!=null && info.size>0){
                        output.position(info.offset); output.limit(info.offset+info.size);
                        if((long)audio.size()+info.size>Integer.MAX_VALUE-44L)throw new IOException("Decoded PCM exceeds Java array address space; streaming conversion is required");
                        byte[] block=new byte[info.size]; output.get(block);
                        audio.write(block);
                    }
                    codec.releaseOutputBuffer(index,false);
                    if((info.flags&MediaCodec.BUFFER_FLAG_END_OF_STREAM)!=0)outputEnded=true;
                }
            }
            if(channels<1||channels>2||rate<8000||rate>48000)throw new IOException("VorbisLib requires mono/stereo audio at 8–48 kHz; convert sample rate/channels before importing.");
            byte[] pcm=audio.toByteArray();
            int align=channels*2;
            if(pcm.length%align!=0)throw new IOException("Misaligned PCM samples");
            return makeWem(pcm,channels,rate);
        }finally{
            extractor.release();
            if(codec!=null){try{codec.stop();}catch(Exception ignored){}codec.release();}
        }
    }
    /** Wwise RIFF PCM container; identical bytes to PCM WAV except optional Wwise extensions. */
    public static byte[] makeWem(byte[] pcm,int channels,int sampleRate)throws IOException{
        if(channels<=0||channels>8||pcm.length% (channels*2)!=0)throw new IOException("Invalid PCM frames");
        ByteArrayOutputStream out=new ByteArrayOutputStream(pcm.length+64);
        out.write("RIFF".getBytes(StandardCharsets.US_ASCII));
        u32(out,36L+pcm.length);
        out.write("WAVEfmt ".getBytes(StandardCharsets.US_ASCII));
        u32(out,16);u16(out,1);u16(out,channels);
        u32(out,sampleRate);u32(out,(long)sampleRate*channels*2);
        u16(out,channels*2);u16(out,16);
        out.write("data".getBytes(StandardCharsets.US_ASCII));u32(out,pcm.length);out.write(pcm);
        return out.toByteArray();
    }
}
