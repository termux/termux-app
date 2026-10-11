package com.termux.app;

import org.junit.Test;
import java.io.ByteArrayOutputStream;
import java.io.OutputStream;
import java.util.*;
import static org.junit.Assert.*;

public class MLBBBankReaderTest {
    private static void i32(OutputStream out,int x)throws Exception {
        out.write(x&255);out.write(x>>>8&255);out.write(x>>>16&255);out.write(x>>>24&255);
    }
    private static void chunk(OutputStream out,String tag,byte[] data)throws Exception {
        out.write(tag.getBytes("US-ASCII"));i32(out,data.length);out.write(data);
    }
    private static byte[] wem(int n)throws Exception {
        ByteArrayOutputStream b=new ByteArrayOutputStream();
        b.write("RIFF".getBytes("US-ASCII"));i32(b,4);b.write("WAVE".getBytes("US-ASCII"));
        for(int i=0;i<n;i++)b.write(17+i);
        return b.toByteArray();
    }
    private static byte[] soundbank()throws Exception {
        byte[] a=wem(5),b=wem(3);
        ByteArrayOutputStream index=new ByteArrayOutputStream();
        i32(index,100);i32(index,0);i32(index,a.length);
        i32(index,200);i32(index,32);i32(index,b.length);
        ByteArrayOutputStream audio=new ByteArrayOutputStream();
        audio.write(a);while(audio.size()<32)audio.write(0);audio.write(b);
        ByteArrayOutputStream out=new ByteArrayOutputStream();
        chunk(out,"BKHD",new byte[]{1,2,3,4});
        chunk(out,"DIDX",index.toByteArray());
        chunk(out,"DATA",audio.toByteArray());
        chunk(out,"HIRC",new byte[]{9,8,7});
        return out.toByteArray();
    }
    @Test public void readsMultipleEmbeddedSounds()throws Exception{
        MLBBBankReader reader=new MLBBBankReader(soundbank());
        assertEquals(2,reader.entries.size());
        assertEquals(100,reader.entries.get(0).id);
        assertArrayEquals(wem(5),reader.extract(reader.entries.get(0)));
        assertArrayEquals(wem(3),reader.extract(reader.entries.get(1)));
    }
    @Test public void rebuildKeepsOtherSoundsAndTrailingChunks()throws Exception{
        byte[] original=soundbank(),replaced=wem(25);
        MLBBBankReader reader=new MLBBBankReader(original);
        Map<Long,byte[]> changes=new HashMap<>();changes.put(200L,replaced);
        MLBBBankReader next=new MLBBBankReader(reader.repack(changes));
        assertArrayEquals(wem(5),next.extract(next.entries.get(0)));
        assertArrayEquals(replaced,next.extract(next.entries.get(1)));
        byte[] rebuilt=reader.repack(changes);
        assertArrayEquals(new byte[]{9,8,7},Arrays.copyOfRange(rebuilt,rebuilt.length-3,rebuilt.length));
    }
    @Test public void refusesUnknownMedia()throws Exception{
        MLBBBankReader reader=new MLBBBankReader(soundbank());
        Map<Long,byte[]> changes=new HashMap<>();changes.put(300L,wem(5));
        try{reader.repack(changes);fail("Must reject unknown WEM ID");}
        catch(java.io.IOException expected){}
    }
}
