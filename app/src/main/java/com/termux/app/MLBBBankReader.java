package com.termux.app;

import java.io.*;
import java.nio.*;
import java.util.*;

/** Little-endian Wwise BNK inspector. Read-only: never modifies original game assets. */
public final class MLBBBankReader {
    public static final class Entry {
        public final long id;
        public final int offset, size;
        Entry(long id,int off,int size){this.id=id;this.offset=off;this.size=size;}
    }
    public final List<Entry> entries = new ArrayList<>();
    private byte[] source;
    private int dataStart;
    public MLBBBankReader(byte[] bytes) throws IOException {
        this.source=bytes;
        if(bytes.length<16 || !ascii(bytes,0).equals("BKHD")) throw new IOException("Not a Wwise BKHD soundbank");
        int didx=-1, data=-1, didxLength=0, dataLength=0, position=0;
        while(position+8<=bytes.length){
            int len=integer(bytes,position+4);
            if(len<0 || (long)position+8+len>bytes.length)throw new IOException("Invalid BNK chunk length");
            String tag=ascii(bytes,position);
            if(tag.equals("DIDX")){didx=position+8;didxLength=len;}
            if(tag.equals("DATA")){data=position+8;dataLength=len;}
            position+=8+len;
        }
        if(didx<0||data<0||didxLength%12!=0)throw new IOException("Missing/invalid DIDX or DATA");
        dataStart=data;
        for(int i=didx;i<didx+didxLength;i+=12){
            long id=integer(bytes,i)&0xffffffffL;
            int off=integer(bytes,i+4),size=integer(bytes,i+8);
            if(off<0||size<0||(long)off+size>dataLength)throw new IOException("Invalid WEM index");
            entries.add(new Entry(id,off,size));
        }
    }
    private static String ascii(byte[] b,int i){return new String(b,i,4,java.nio.charset.StandardCharsets.US_ASCII);}
    private static int integer(byte[] b,int p){return ByteBuffer.wrap(b,p,4).order(ByteOrder.LITTLE_ENDIAN).getInt();}
    public byte[] extract(Entry e){return Arrays.copyOfRange(source,dataStart+e.offset,dataStart+e.offset+e.size);}
}
