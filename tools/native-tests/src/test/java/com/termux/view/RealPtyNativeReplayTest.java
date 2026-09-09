package com.termux.view;

import android.graphics.*;
import android.os.Build;
import com.termux.terminal.*;
import org.json.*;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.*;
import java.io.*;
import java.lang.reflect.Proxy;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.nio.file.Path;
import java.security.MessageDigest;
import java.util.*;
import static org.junit.Assert.*;

/** Recorded real process PTY bytes -> actual TerminalEmulator -> Android native Canvas.
 * No shell screenshot, device, or authenticated Claude is implied. */
@RunWith(RobolectricTestRunner.class)
@Config(sdk={28,34}, manifest=Config.NONE)
@GraphicsMode(GraphicsMode.Mode.NATIVE)
public class RealPtyNativeReplayTest {
    final Path fixtures=Paths.get(System.getProperty("fixtures", "fixtures"));
    final TerminalRenderer renderer=new TerminalRenderer(28, Typeface.MONOSPACE, true);
    final TerminalRenderer nativeRenderer=new TerminalRenderer(28, Typeface.MONOSPACE, false);
    int cols, rows;

    TerminalEmulator emulator() {
        TerminalSessionClient client=(TerminalSessionClient)Proxy.newProxyInstance(getClass().getClassLoader(),new Class[]{TerminalSessionClient.class},(p,m,a)->m.getReturnType()==boolean.class?false:null);
        TerminalOutput output=new TerminalOutput() {
            public void write(byte[] d,int o,int n) {} public void titleChanged(String a,String b) {}
            public void onCopyTextToClipboard(String s) {} public void onPasteTextFromClipboard() {}
            public void onBell() {} public void onColorsChanged() {}
        };
        return new TerminalEmulator(output,cols,rows,Math.round(renderer.mFontWidth),renderer.mFontLineSpacing,200,client);
    }
    void feed(TerminalEmulator e,byte[] b) { e.append(b,b.length); }
    TerminalRow line(TerminalEmulator e,int row) {return e.getScreen().allocateFullLineIfNecessary(e.getScreen().externalToInternalRow(row));}
    String rowText(TerminalEmulator e,int row) {TerminalRow l=line(e,row);return new String(l.mText,0,l.getSpaceUsed()).replaceFirst(" +$", "");}
    String state(TerminalEmulator e) {
        StringBuilder b=new StringBuilder(); b.append(e.getCursorRow()).append(',').append(e.getCursorCol()).append(',').append(e.isAlternateBufferActive());
        b.append(',').append(e.isMouseTrackingActive()).append(',').append(e.isCursorKeysApplicationMode());
        for(int row=0;row<rows;row++) {TerminalRow l=line(e,row); b.append('\n').append(rowText(e,row));for(int c=0;c<cols;c++)b.append('/').append(l.getStyle(c));}
        return b.toString();
    }
    Bitmap blank() {Bitmap b=Bitmap.createBitmap((int)Math.ceil(renderer.mFontWidth*cols),renderer.mFontLineSpacing*rows+renderer.mFontLineSpacingAndAscent,Bitmap.Config.ARGB_8888);b.eraseColor(Color.BLACK);return b;}
    Bitmap draw(TerminalEmulator e,boolean enabled) {Bitmap b=blank();(enabled?renderer:nativeRenderer).render(e,new Canvas(b),0,-1,-1,-1,-1);return b;}
    int[] pixels(Bitmap b) {int[] p=new int[b.getWidth()*b.getHeight()];b.getPixels(p,0,b.getWidth(),0,0,b.getWidth(),b.getHeight());return p;}
    void png(Path file,Bitmap b)throws Exception {try(OutputStream out=Files.newOutputStream(file)){assertTrue(b.compress(Bitmap.CompressFormat.PNG,100,out));}}
    String sha(byte[] b)throws Exception {StringBuilder s=new StringBuilder();for(byte x:MessageDigest.getInstance("SHA-256").digest(b))s.append(String.format("%02x",x&255));return s.toString();}
    void equalBytes(String label,byte[] a,byte[] b){assertArrayEquals(label,a,b);}

    @Test public void realNano83()throws Exception {replay("nano-8.3",13);}
    @Test public void realNano91()throws Exception {replay("nano-9.1",13);}
    @Test public void realHtop321()throws Exception {replay("htop",8);}
    @Test public void syntheticClaudeStyleNotActualClaude()throws Exception {replay("synthetic-claude-style",3);}

    /** Independent semantic-column oracle: TIME+ is not part of the command field. */
    @Test public void realHtopTimeColumnMustRemainNative()throws Exception {
        cols=80;rows=24;TerminalEmulator e=emulator();
        feed(e,Files.readAllBytes(fixtures.resolve("htop/00-initialScreen.replay.ansi")));
        String commandRow=rowText(e,8);int time=commandRow.indexOf("0:00.00");int command=commandRow.indexOf("مرحبا");
        assertEquals(59,time);assertEquals(67,command);
        assertEquals("REAL htop TIME+ column must not move with Arabic command: expected logical column 59 at native x",time*renderer.mFontWidth,renderer.logicalBoundaryX(e,8,time,false),.01f);
    }

    void replay(String name,int expectedBoundaries)throws Exception {
        Path directory=fixtures.resolve(name);
        JSONObject manifest=new JSONObject(new String(Files.readAllBytes(directory.resolve("manifest.json")),StandardCharsets.UTF_8));
        cols=manifest.getJSONObject("dimensions").getInt("cols");rows=manifest.getJSONObject("dimensions").getInt("rows");
        assertEquals(80,cols);assertEquals(24,rows);
        byte[] stream=Files.readAllBytes(directory.resolve(manifest.getString("streamFile")));
        assertEquals(manifest.getString("streamSha256"),sha(stream));
        assertEquals(manifest.getInt("streamBytes"),stream.length);
        JSONArray actions=manifest.getJSONArray("actions");assertEquals(expectedBoundaries,actions.length());
        Path out=Paths.get(System.getProperty("evidence", "target/evidence"),"api"+Build.VERSION.SDK_INT,name);Files.createDirectories(out);
        JSONObject report=new JSONObject();JSONArray results=new JSONArray();report.put("fixture",name).put("kind",manifest.getString("kind")).put("api",Build.VERSION.SDK_INT).put("nativeGraphics",true).put("boundaries",results);
        TerminalEmulator e=emulator();Bitmap reused=blank();int previous=0;
        try {
            if(name.startsWith("nano-")) {
                byte[] saved=Files.readAllBytes(directory.resolve(manifest.getString("savedFile")));
                byte[] expected=Files.readAllBytes(directory.resolve(manifest.getString("expectedSavedFile")));
                equalBytes("real nano saved UTF-8 bytes",expected,saved);
                assertEquals(manifest.getJSONObject("saveVerification").getString("actualSha256"),sha(saved));
                assertEquals(manifest.getJSONObject("saveVerification").getString("expectedSha256"),sha(expected));
                assertEquals(manifest.getJSONObject("saveVerification").getString("actualUtf8"),new String(saved,StandardCharsets.UTF_8));
                assertEquals("مربا بالعالم",new String(saved,StandardCharsets.UTF_8).split("\n")[0]);
                report.put("realSavedFileBytesVerified",true);
            }
            for(int i=0;i<actions.length();i++) {
                JSONObject action=actions.getJSONObject(i);String label=action.getString("label");
                JSONObject result=new JSONObject().put("index",i).put("label",label);results.put(result);
                String context=name+"/"+i+"/"+label+" API"+Build.VERSION.SDK_INT;
                byte[] prefix=Files.readAllBytes(directory.resolve(action.getString("replayPrefixFile")));
                assertEquals(action.getInt("offsetEndExclusive"),prefix.length);assertEquals(previous,action.getInt("offsetStart"));
                assertEquals(action.getInt("byteCount"),prefix.length-previous);
                equalBytes(context+" prefix",Arrays.copyOf(stream,prefix.length),prefix);
                byte[] delta=Arrays.copyOfRange(prefix,previous,prefix.length);
                equalBytes(context+" delta",Files.readAllBytes(directory.resolve(action.getString("deltaFile"))),delta);
                // Preserve this emulator and feed ONLY newly emitted bytes from each cumulative prefix.
                // Fragment at 7 bytes deliberately, including possible UTF-8 and CSI boundaries.
                for(int p=0;p<delta.length;p+=7)feed(e,Arrays.copyOfRange(delta,p,Math.min(p+7,delta.length)));
                previous=prefix.length;
                TerminalEmulator fresh=emulator();feed(fresh,prefix);
                assertEquals(context+" cumulative-prefix/fresh logical state",state(fresh),state(e));
                result.put("freshPrefixStateEqualsIncremental",true).put("cursorRow",e.getCursorRow()).put("cursorCol",e.getCursorCol()).put("alternate",e.isAlternateBufferActive()).put("mouse",e.isMouseTrackingActive()).put("appKeys",e.isCursorKeysApplicationMode());
                if(!action.isNull("expectedLogicalCursor")) {
                    JSONObject cursor=action.getJSONObject("expectedLogicalCursor");
                    assertEquals(context+" cursor row",cursor.getInt("terminalRow0Based"),e.getCursorRow());
                    assertEquals(context+" cursor column",cursor.getInt("terminalCol0Based"),e.getCursorCol());
                    result.put("manifestCursorAsserted",true);
                }else result.put("manifestCursorAsserted",false);
                List<String> logicalRows=new ArrayList<>();for(int row=0;row<rows;row++)logicalRows.add(rowText(e,row));
                Files.write(out.resolve(String.format("%02d-%s.logical.txt",i,label)),logicalRows,StandardCharsets.UTF_8);
                assertLogicalContent(name,i,manifest,e,label);
                String beforeState=state(e);
                final int[] rtlCalls={0};
                Canvas recording=new Canvas(reused) {
                    @Override public void drawTextRun(char[] text,int index,int count,int ci,int cc,float x,float y,boolean rtl,Paint p) {
                        if(rtl) {
                            rtlCalls[0]++;String submitted=new String(text,index,count);boolean found=false;
                            for(String row:logicalRows)if((row+" ").contains(submitted)){found=true;break;}
                            assertTrue("native drawTextRun receives original logical run, never reversed: "+submitted,found);
                            assertEquals("whole RTL run context index",index,ci);assertEquals("whole RTL run context count",count,cc);
                        }
                        super.drawTextRun(text,index,count,ci,cc,x,y,rtl,p);
                    }
                };
                renderer.render(e,recording,0,-1,-1,-1,-1);
                Bitmap newFrame=draw(e,true),nativeFrame=draw(e,false);
                assertArrayEquals(context+" no old cursor/glyph trails: reused/fresh",pixels(newFrame),pixels(reused));
                assertEquals(context+" display must not change logical row/style/cursor/mode state",beforeState,state(e));
                result.put("reusedBitmapEqualsFresh",true).put("renderLogicalStateUnchanged",true).put("rtlDrawTextRunCalls",rtlCalls[0]);
                verifyBoundsAndPixels(name,e,newFrame,nativeFrame,result,context);
                if(!label.equals("exit")) {
                    assertTrue(context+" must issue real RTL native text runs",rtlCalls[0]>0);
                    boolean savePngs=Boolean.getBoolean("evidence.png");
                    if(savePngs) {
                        png(out.resolve(String.format("%02d-%s-new.png",i,label)),newFrame);
                        png(out.resolve(String.format("%02d-%s-native-disabled.png",i,label)),nativeFrame);
                    }
                    result.put("snapshotSaved",savePngs);
                } else {
                    assertFalse("full exit restores primary",e.isAlternateBufferActive());
                    result.put("snapshotSaved",false);
                }
                result.put("passed",true);
                newFrame.recycle();nativeFrame.recycle();
            }
            assertEquals(stream.length,previous);report.put("passed",true);
        } catch(Throwable t) {report.put("passed",false).put("failure",t.toString());throw t;}
        finally {Files.write(out.resolve("report.json"),report.toString(2).getBytes(StandardCharsets.UTF_8));reused.recycle();}
    }

    void assertLogicalContent(String name,int index,JSONObject manifest,TerminalEmulator e,String label)throws Exception {
        if(label.equals("exit"))return;
        if(name.startsWith("nano-")) {
            String[] original=new String(Files.readAllBytes(fixtures.resolve(name).resolve(manifest.getString("originalFile"))),StandardCharsets.UTF_8).split("\n");
            original[0]=index<2?"مرحبا بالعالم":index==2?"مرتحبا بالعالم":index==3?"مرتبا بالعالم":"مربا بالعالم";
            for(int j=0;j<original.length;j++)assertEquals("nano exact logical row "+j+" at action "+index,original[j],rowText(e,j+1));
            // Selection extraction stays logical and retains combining marks.
            assertEquals("combining marks retained by logical selection","بِسْمِ الله",e.getSelectedText(0,2,79,2));
        }else if(name.startsWith("synthetic")) {
            JSONObject lines=manifest.getJSONObject(index==0?"initialLines1Based":"finalLines1Based");
            for(Iterator<String> it=lines.keys();it.hasNext();) {String key=it.next();assertEquals("synthetic logical continuation",lines.getString(key),rowText(e,Integer.parseInt(key)-1));}
        }else {
            assertFalse("htop cursor is hidden",e.shouldCursorBeVisible());
            boolean arabic=false;for(int row=0;row<rows;row++) {String text=rowText(e,row);if(index<4?(text.contains("مرحبا")||text.contains("بِسْمِ")):text.contains("م"))arabic=true;}
            // Real tree-toggle indents this captured command to column 78: only first م remains visible.
            assertTrue("real htop command contains captured logical Arabic (possibly tree-clipped)",arabic);
        }
    }

    void verifyBoundsAndPixels(String name,TerminalEmulator e,Bitmap newFrame,Bitmap nativeFrame,JSONObject result,String context)throws Exception {
        List<RectF> allocations=new ArrayList<>();int mappingChecks=0,fieldCount=0;
        boolean[][] omitted=new boolean[rows][];
        for(int row=0;row<rows;row++) {
            if(!renderer.isRtlShapedRow(e,row))continue;
            assertFalse(context+" fixture Arabic rows must be bounded, not whole-screen paragraph flow",renderer.isRtlFlowRow(e,row));
            TerminalRow l=line(e,row);GridBidiLayout grid=new GridBidiLayout(l.mText,l.getSpaceUsed(),cols,row==e.getCursorRow()?e.getCursorCol():-1);
            if(name.startsWith("nano-")&&(row==4||row==5))assertEquals("nano indentation anchor",row==4?2:4,grid.fields.get(0).startColumn);
            if(name.startsWith("synthetic"))assertEquals("bullet / continuation starts remain column two",2,grid.fields.get(0).startColumn);
            omitted[row]=new boolean[cols];
            for(GridBidiLayout.Field f:grid.fields) {
                Arrays.fill(omitted[row],f.startColumn,f.endColumn,true);
                fieldCount++;float left=f.startColumn*renderer.mFontWidth,right=f.endColumn*renderer.mFontWidth;
                float bottom=renderer.mFontLineSpacingAndAscent+(row+1)*renderer.mFontLineSpacing;
                allocations.add(new RectF(left,bottom-renderer.mFontLineSpacing,right,bottom));
                for(int c=f.startColumn;c<f.endColumn;c++) {
                    float a=renderer.logicalBoundaryX(e,row,c,false),b=renderer.logicalBoundaryX(e,row,c,true);
                    assertTrue(context+" hit bound left cell "+c,a>=left-.01f&&b>=left-.01f);
                    assertTrue(context+" hit bound right cell "+c,a<=right+.01f&&b<=right+.01f);
                    assertTrue(context+" nonzero cell width "+c,Math.abs(a-b)>.001f);
                    assertEquals(context+" hit roundtrip row="+row+" logical="+c,c,renderer.logicalColumnAt(e,row,(a+b)/2));mappingChecks++;
                }
            }
            for(int c=0;c<cols;c++)if(grid.fieldAtColumn(c)==null) {
                assertEquals(context+" native anchor column",c,renderer.logicalColumnAt(e,row,(c+.5f)*renderer.mFontWidth));
                assertEquals(context+" native anchor boundary",c*renderer.mFontWidth,renderer.logicalBoundaryX(e,row,c,false),.01f);
            }
        }
        // Compare outside allocations against the original native ANCHORS, not accidental
        // source-Arabic overhang. Keeping those stray source pixels was the cursor-remnant bug.
        // This independent native-only frame has original backgrounds and no shaped overlay.
        // No expanded erasure/tolerance is permitted: every outside pixel must equal this frame.
        Bitmap anchorsFrame=blank();
        new NativeTerminalRenderer(28,Typeface.MONOSPACE).renderRows(e,new Canvas(anchorsFrame),0,rows,
            -1,-1,-1,-1,omitted);
        int[] a=pixels(newFrame),b=pixels(nativeFrame),anchors=pixels(anchorsFrame);
        int changed=0,outside=0,removedOverhang=0;String firstOutside="";int w=newFrame.getWidth();
        for(int p=0;p<a.length;p++) {
            if(a[p]!=b[p])changed++;
            int x=p%w,y=p/w;boolean inside=false;
            // Include only raster pixels intersecting the original half-open allocated cell rectangles.
            for(RectF f:allocations)if(x>=Math.floor(f.left)&&x<Math.ceil(f.right)&&y>=Math.floor(f.top)&&y<Math.ceil(f.bottom)){inside=true;break;}
            if(!inside) {
                if(b[p]!=anchors[p])removedOverhang++;
                if(a[p]!=anchors[p]){outside++;if(firstOutside.isEmpty())firstOutside=x+","+y;}
            }
        }
        anchorsFrame.recycle();
        assertEquals(context+" header/footer/ASCII/separator pixels outside bounded RTL fields, first="+firstOutside,0,outside);
        if(fieldCount>0)assertTrue(context+" shaping must change actual pixels",changed>0);
        assertEquals(cols*renderer.mFontWidth,cols*nativeRenderer.mFontWidth,0f);
        assertEquals(renderer.mFontLineSpacing,nativeRenderer.mFontLineSpacing);
        result.put("boundedFields",fieldCount).put("cellHitRoundTrips",mappingChecks).put("changedPixelsAgainstNative",changed).put("changedPixelsOutsideFields",outside).put("nativeSourceOverhangPixelsRemoved",removedOverhang);
    }
}
