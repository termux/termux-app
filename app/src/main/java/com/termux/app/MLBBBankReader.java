package com.termux.app;

import java.io.*;
import java.nio.*;
import java.nio.charset.StandardCharsets;
import java.util.*;

/** Minimal Wwise BNK reader/repacker for standard BKHD/DIDX/DATA media banks. */
public final class MLBBBankReader {
    public static final class Entry {
        public final long id;
        public final int offset, size;
        Entry(long id,int offset,int size){this.id=id;this.offset=offset;this.size=size;}
    }
    public final List<Entry> entries=new ArrayList<>();
    private final byte[] source;
    private final int dataStart;

    public MLBBBankReader(byte[] bytes) throws IOException {
        source=bytes;
        if(bytes.length<16 || !ascii(bytes,0).equals("BKHD"))throw new IOException("Not a Wwise BKHD soundbank");
        int didx=-1,data=-1,indexSize=0,dataSize=0,position=0;
        while(position+8<=bytes.length){
            int len=integer(bytes,position+4);
            if(len<0||(long)position+8+len>bytes.length)throw new IOException("Invalid BNK chunk length");
            String tag=ascii(bytes,position);
            if(tag.equals("DIDX")){didx=position+8;indexSize=len;}
            if(tag.equals("DATA")){data=position+8;dataSize=len;}
            position+=8+len;
        }
        if(didx<0||data<0||indexSize%12!=0)throw new IOException("Missing/invalid DIDX or DATA");
        dataStart=data;
        for(int i=didx;i<didx+indexSize;i+=12){
            long id=integer(bytes,i)&0xffffffffL;
            int off=integer(bytes,i+4),size=integer(bytes,i+8);
            if(off<0||size<0||(long)off+size>dataSize)throw new IOException("Invalid WEM index");
            entries.add(new Entry(id,off,size));
        }
    }
    private static String ascii(byte[] b,int at){return new String(b,at,4,StandardCharsets.US_ASCII);}
    private static int integer(byte[] b,int at){return ByteBuffer.wrap(b,at,4).order(ByteOrder.LITTLE_ENDIAN).getInt();}
    private static void little32(OutputStream out,int n)throws IOException {
        out.write(n&255);out.write(n>>>8&255);out.write(n>>>16&255);out.write(n>>>24&255);
    }
    public byte[] extract(Entry e){return Arrays.copyOfRange(source,dataStart+e.offset,dataStart+e.offset+e.size);}
    public static int wemCodec(byte[] wem) throws IOException {
        if(wem.length<24 || !ascii(wem,0).equals("RIFF") || !ascii(wem,8).equals("WAVE"))
            throw new IOException("Invalid WEM RIFF/WAVE header");
        int pos=12;
        while(pos+8<=wem.length) {
            String tag=ascii(wem,pos);
            int size=integer(wem,pos+4);
            if(size<0 || (long)pos+8+size>wem.length)throw new IOException("Corrupt WEM chunk");
            if(tag.equals("fmt ")) {
                if(size<16)throw new IOException("Short WEM fmt chunk");
                return (wem[pos+8]&255)|((wem[pos+9]&255)<<8);
            }
            pos+=8+size+(size&1);
        }
        throw new IOException("WEM fmt chunk missing");
    }
    public static String codecName(int codec) {
        if(codec==0xffff)return "Wwise Custom Vorbis";
        if(codec==1 || codec==0xfffe)return "PCM";
        if(codec==2 || codec==0x69)return "IMA ADPCM";
        return String.format(java.util.Locale.ROOT,"Codec 0x%04X",codec);
    }
    public int originalCodec(Entry e)throws IOException{return wemCodec(extract(e));}
    public void validateReplacementCodecs(Map<Long,byte[]> changes)throws IOException {
        for(Entry e:entries)if(changes.containsKey(e.id)){
            int original=originalCodec(e),incoming=wemCodec(changes.get(e.id));
            if(original!=incoming)
                throw new IOException("Audio "+e.id+" uses "+codecName(original)+
                    ", but replacement is "+codecName(incoming)+". Game playback is not reliable. Export only; encode compatible Wwise media first.");
        }
    }


    /**
     * Creates a separate BNK. Preserves unknown chunks and their order as-is.
     * Does not guarantee that a game accepts a changed WEM codec or event length.
     */
    public byte[] repack(Map<Long,byte[]> replacements) throws IOException {
        if(replacements.isEmpty())return source.clone();
        Set<Long> known=new HashSet<>();
        for(Entry e:entries)known.add(e.id);
        if(!known.containsAll(replacements.keySet()))throw new IOException("Replacement has unknown audio ID");
        ByteArrayOutputStream index=new ByteArrayOutputStream();
        ByteArrayOutputStream media=new ByteArrayOutputStream();
        for(Entry e:entries){
            while(media.size()%16!=0)media.write(0);
            byte[] wem=replacements.containsKey(e.id)?replacements.get(e.id):extract(e);
            if(wem==null||wem.length<12||!ascii(wem,0).equals("RIFF"))
                throw new IOException("Entry "+e.id+" is not RIFF WEM");
            little32(index,(int)e.id);little32(index,media.size());little32(index,wem.length);
            media.write(wem);
        }
        ByteArrayOutputStream out=new ByteArrayOutputStream();
        int pos=0;
        while(pos+8<=source.length){
            int size=integer(source,pos+4);
            if(size<0||(long)pos+8+size>source.length)break;
            String tag=ascii(source,pos);
            byte[] chunk=tag.equals("DIDX")?index.toByteArray():
                         tag.equals("DATA")?media.toByteArray():
                         Arrays.copyOfRange(source,pos+8,pos+8+size);
            out.write(tag.getBytes(StandardCharsets.US_ASCII));
            little32(out,chunk.length);out.write(chunk);
            pos+=8+size;
        }
        out.write(source,pos,source.length-pos);
        byte[] result=out.toByteArray();
        MLBBBankReader verified=new MLBBBankReader(result);
        if(verified.entries.size()!=entries.size())throw new IOException("Repack verification: incorrect entry count");
        for(Entry e:verified.entries)if(replacements.containsKey(e.id)&&
                !Arrays.equals(verified.extract(e),replacements.get(e.id)))
            throw new IOException("Repack verification failed for "+e.id);
        return result;
    }
}
