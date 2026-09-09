package com.termux.view;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Typeface;
import com.termux.terminal.*;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.*;
import java.lang.reflect.Proxy;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import static org.junit.Assert.*;

/** Native Skia/font regression coverage: source glyphs must never be rasterized then erased. */
@RunWith(RobolectricTestRunner.class)
@Config(sdk={28,34}, manifest=Config.NONE)
@GraphicsMode(GraphicsMode.Mode.NATIVE)
public class ReplacementRasterizationTest {
    final TerminalRenderer renderer = new TerminalRenderer(32, Typeface.MONOSPACE, true);
    TerminalEmulator emulator(String text) {
        TerminalSessionClient client = (TerminalSessionClient) Proxy.newProxyInstance(getClass().getClassLoader(),
            new Class[]{TerminalSessionClient.class}, (p,m,a) -> m.getReturnType() == boolean.class ? false : null);
        TerminalOutput output = new TerminalOutput() {
            public void write(byte[] b,int o,int n) {} public void titleChanged(String a,String b) {}
            public void onCopyTextToClipboard(String s) {} public void onPasteTextFromClipboard() {}
            public void onBell() {} public void onColorsChanged() {}
        };
        TerminalEmulator e = new TerminalEmulator(output,20,3,19,42,100,client);
        feed(e,text); return e;
    }
    void feed(TerminalEmulator e,String text) { byte[] bytes=text.getBytes(StandardCharsets.UTF_8); e.append(bytes,bytes.length); }
    TerminalRow line(TerminalEmulator e,int row) { return e.getScreen().allocateFullLineIfNecessary(e.getScreen().externalToInternalRow(row)); }
    Bitmap blank(TerminalEmulator e) {
        Bitmap b=Bitmap.createBitmap((int)Math.ceil(renderer.mFontWidth*e.mColumns),
            renderer.mFontLineSpacing*4,Bitmap.Config.ARGB_8888); b.eraseColor(Color.BLACK); return b;
    }
    Bitmap draw(TerminalEmulator e,int first,int last) {
        Bitmap b=blank(e); renderer.render(e,new Canvas(b),0,first<0?-1:2,first<0?-1:2,first,last); return b;
    }
    int[] pixels(Bitmap b) { int[] p=new int[b.getWidth()*b.getHeight()]; b.getPixels(p,0,b.getWidth(),0,0,b.getWidth(),b.getHeight()); return p; }
    void assertOnlyBoundsChanged(Bitmap a,Bitmap b,float left,float right) {
        int changed=0;
        for(int y=0;y<a.getHeight();y++) for(int x=0;x<a.getWidth();x++) if(a.getPixel(x,y)!=b.getPixel(x,y)) {
            changed++;
            assertTrue("stray recolored pixel x="+x+" y="+y+" bounds="+left+","+right,
                x>=Math.floor(left)-1 && x<=Math.ceil(right)+1);
        }
        assertTrue("cursor/selection must have visible pixels",changed>0);
    }

    @Test public void sourceGlyphsNeverReachCanvasAndNativeAnchorsStillDo() {
        for(boolean flow:new boolean[]{false,true}) {
            TerminalEmulator e=emulator((flow?"":"\033[?1049h")+"\033[3;1H"+
                (flow?"":"  ")+"\033[3mمرحبا\033[0m"+(flow?"":"  OK")+"\033[3;7H");
            TerminalRow row=line(e,2);
            int start=flow?0:2, end=flow?row.getSpaceUsed():7;
            final int[] shaped={0},anchors={0};
            renderer.render(e,new Canvas(blank(e)) {
                @Override public void drawTextRun(char[] text,int index,int count,int ci,int cc,float x,float y,boolean rtl,Paint p) {
                    if(text==row.mText && !rtl && index<end && index+count>start)
                        fail("source font rasterization submitted for replacement: "+new String(text,index,count));
                    if(rtl) shaped[0]++;
                    if(!flow && text==row.mText && new String(text,index,count).contains("OK")) anchors[0]++;
                    super.drawTextRun(text,index,count,ci,cc,x,y,rtl,p);
                }
            },0,-1,-1,-1,-1);
            assertTrue(shaped[0]>0);
            if(!flow) assertTrue("OK must still use the native pass",anchors[0]>0);
        }
    }

    @Test public void bottomRowCursorBlinkAndSelectionOnlyTouchMappedCells() {
        for(boolean flow:new boolean[]{false,true}) for(String style:new String[]{"","\033[3m","\033[1;3m"}) {
            String prefix=flow?"":"  ";
            TerminalEmulator e=emulator((flow?"":"\033[?1049h")+"\033[3;1H"+prefix+style+"مرحبا\033[0m"+(flow?"":"  OK"));
            e.setCursorBlinkingEnabled(true);
            for(int cursorStyle:new int[]{2,4,6}) for(int c=prefix.length();c<prefix.length()+5;c++) {
                feed(e,"\033["+cursorStyle+" q\033[3;"+(c+1)+"H");
                e.setCursorBlinkState(true); Bitmap on=draw(e,-1,-1);
                e.setCursorBlinkState(false); Bitmap off=draw(e,-1,-1);
                float a=renderer.logicalBoundaryX(e,2,c,false),b=renderer.logicalBoundaryX(e,2,c,true);
                assertOnlyBoundsChanged(on,off,Math.min(a,b),Math.max(a,b));
                Bitmap selected=draw(e,c,c);
                assertOnlyBoundsChanged(selected,off,Math.min(a,b),Math.max(a,b));
                renderer.render(e,new Canvas(on),0,-1,-1,-1,-1);
                assertArrayEquals("reusing bitmap must remove old cursor",pixels(off),pixels(on));
                on.recycle();off.recycle();selected.recycle();
            }
        }
    }

    @Test public void optionalMaskPreservesBackgroundsWithoutSourceGlyphCursorOrSelection() {
        TerminalEmulator e=emulator("\033[?1049h\033[3;1H  \033[41;3mمرحبا\033[0m  OK\033[3;7H");
        boolean[][] mask=new boolean[3][]; mask[2]=new boolean[20]; Arrays.fill(mask[2],2,7,true);
        NativeTerminalRenderer nativeRenderer=new NativeTerminalRenderer(32,Typeface.MONOSPACE);
        Bitmap masked=blank(e),unselected=blank(e);
        nativeRenderer.renderRows(e,new Canvas(masked),0,3,2,2,2,6,mask);
        feed(e,"\033[?25l");
        nativeRenderer.renderRows(e,new Canvas(unselected),0,3,-1,-1,-1,-1,mask);
        assertArrayEquals("masked cursor/selection must not be painted",pixels(unselected),pixels(masked));
        int y=renderer.mFontLineSpacingAndAscent+3*renderer.mFontLineSpacing-2;
        for(int c=2;c<7;c++) assertEquals(e.mColors.mCurrentColors[1],masked.getPixel((int)((c+.5f)*renderer.mFontWidth),y));
    }

    @Test public void enabledAllEnglishRetainsEveryNativePixelAndMetric() {
        for(String mode:new String[]{"","\033[?1049h","\033[?1000h","\033[?1h"}) {
            TerminalEmulator e=emulator(mode+"\033[31;1mCPU 42%\033[0m\r\n\033[3mTIME+ 0:00.12\033[0m\r\n界 abc");
            Bitmap enabled=draw(e,-1,-1);
            renderer.setRtlTextShapingEnabled(false); Bitmap nativeFrame=draw(e,-1,-1);
            assertArrayEquals(pixels(nativeFrame),pixels(enabled));
            renderer.setRtlTextShapingEnabled(true);
        }
    }
}
