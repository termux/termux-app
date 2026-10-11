package com.termux.app;

import android.app.Activity;
import android.net.Uri;
import android.util.Base64;
import android.webkit.WebView;
import java.io.*;
import java.nio.charset.StandardCharsets;

/**
 * Executes the exact compiled TypeScript legacy Vorbis writer inside Android WebView.
 * Standalone from Termux. Experimental output MUST NOT auto-install into MLBB.
 */
public final class MLBBOggWemBridge {
    public interface Listener {
        void completed(byte[] wem);
        void failed(String reason);
    }
    private static final int MAX_OGG=4*1024*1024;
    private MLBBOggWemBridge(){}
    public static void encode(Activity activity,Uri uri,Listener callback){
        byte[] source,js;
        try(InputStream in=activity.getContentResolver().openInputStream(uri);
            InputStream asset=activity.getAssets().open("wwise-legacy-vorbis.js")){
            source=read(in,MAX_OGG);
            js=read(asset,512*1024);
            if(source.length<28)throw new IOException("Input OGG too short");
        }catch(Exception e){callback.failed("Load error: "+e.getMessage());return;}
        WebView view=new WebView(activity);
        view.getSettings().setJavaScriptEnabled(true);
        view.getSettings().setAllowFileAccess(false);
        view.getSettings().setAllowContentAccess(false);
        final String payload=Base64.encodeToString(source,Base64.NO_WRAP);
        // The bundled JS is compiled from wwise-ts/src/vorbis-encoder.ts in CI.
        final String script=new String(js,StandardCharsets.UTF_8);
        view.evaluateJavascript("(function(){var exports={};"+script+
            ";return typeof exports.oggVorbisToWem === 'function';})()",boot->{
                if(!"true".equals(boot)){
                    view.destroy();callback.failed("TypeScript encoder bundle failed to load");return;
                }
                // Re-evaluate in the same JS context because exports is local to the first call.
                String expression="(function(){try{var exports={};"+script+
                    ";var input=atob('"+payload+"');var bytes=new Uint8Array(input.length);"+
                    "for(var i=0;i<input.length;i++)bytes[i]=input.charCodeAt(i);"+
                    "var output=exports.oggVorbisToWem(bytes);"+
                    "if(output.length>8000000)throw Error('Output larger than 8 MB');"+
                    "var parts=[];for(var i=0;i<output.length;i+=8192)"+
                    "parts.push(String.fromCharCode.apply(null,output.subarray(i,i+8192)));"+
                    "return JSON.stringify({ok:true,data:btoa(parts.join(''))});"+
                    "}catch(e){return JSON.stringify({ok:false,error:String(e.message||e)})}})()";
                view.evaluateJavascript(expression,result->{
                    view.destroy();
                    try{
                        // WebView returns a JSON-quoted JavaScript string.
                        String raw=new org.json.JSONTokener(result).nextValue().toString();
                        org.json.JSONObject value=new org.json.JSONObject(raw);
                        if(!value.getBoolean("ok"))throw new IOException(value.getString("error"));
                        callback.completed(Base64.decode(value.getString("data"),Base64.DEFAULT));
                    }catch(Exception e){callback.failed("Encoding failed: "+e.getMessage());}
                });
            });
    }
    private static byte[] read(InputStream in,int max)throws IOException{
        if(in==null)throw new IOException("No input stream");
        ByteArrayOutputStream out=new ByteArrayOutputStream();byte[] buffer=new byte[32768];int n;
        while((n=in.read(buffer))!=-1){
            if((long)out.size()+n>max)throw new IOException("Experimental encoder input too large");
            out.write(buffer,0,n);
        }
        return out.toByteArray();
    }
}
