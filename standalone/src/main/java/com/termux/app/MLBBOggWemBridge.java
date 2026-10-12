package com.termux.app;

import android.app.Activity;
import android.net.Uri;
import android.util.Base64;
import android.webkit.WebView;
import org.json.JSONTokener;
import java.io.*;
import java.nio.charset.StandardCharsets;

/** Executes the bundled, compiled TypeScript encoder inside Android WebView. */
public final class MLBBOggWemBridge {
    public interface Listener {
        void completed(byte[] wem);
        void failed(String reason);
    }
    private static final int MAX_OGG=4*1024*1024;
    private static final int MAX_WEM=8*1024*1024;
    private static final int CHUNK=24576;
    private MLBBOggWemBridge(){}

    public static void encode(Activity activity,Uri uri,Listener callback){
        final byte[] input;
        final String script;
        try(InputStream in=activity.getContentResolver().openInputStream(uri);
            InputStream asset=activity.getAssets().open("wwise-legacy-vorbis.js")){
            input=read(in,MAX_OGG);
            script=new String(read(asset,512*1024),StandardCharsets.UTF_8);
        }catch(Exception ex){callback.failed("Input or encoder unavailable: "+ex.getMessage());return;}
        WebView web=new WebView(activity);
        web.getSettings().setJavaScriptEnabled(true);
        web.getSettings().setAllowFileAccess(false);
        web.getSettings().setAllowContentAccess(false);
        String base64=Base64.encodeToString(input,Base64.NO_WRAP);
        String js="(function(){try{var exports={};"+script+
            ";var src=atob('"+base64+"');var input=new Uint8Array(src.length);"+
            "for(var i=0;i<src.length;i++)input[i]=src.charCodeAt(i);"+
            "window.__wwiseResult=exports.oggVorbisToWem(input);"+
            "return 'OK:'+window.__wwiseResult.length;"+
            "}catch(e){return 'ERROR:'+String(e.message||e)}})()";
        web.evaluateJavascript(js,response->{
            try {
                String result=new JSONTokener(response).nextValue().toString();
                if(!result.startsWith("OK:"))throw new IOException(result);
                int length=Integer.parseInt(result.substring(3));
                if(length<=0||length>MAX_WEM)throw new IOException("Encoded WEM outside 8 MiB limit");
                collect(web,new ByteArrayOutputStream(length),0,length,callback);
            }catch(Exception e){web.destroy();callback.failed("TypeScript encoder: "+e.getMessage());}
        });
    }

    private static void collect(WebView web,ByteArrayOutputStream out,int offset,int length,Listener callback){
        if(offset>=length){
            byte[] result=out.toByteArray();
            web.evaluateJavascript("delete window.__wwiseResult",ignored->web.destroy());
            callback.completed(result);
            return;
        }
        int take=Math.min(CHUNK,length-offset);
        String js="(function(){var a=window.__wwiseResult.subarray("+offset+","+(offset+take)+");"+
            "var s='';for(var i=0;i<a.length;i++)s+=String.fromCharCode(a[i]);return btoa(s)})()";
        web.evaluateJavascript(js,response->{
            try{
                String encoded=new JSONTokener(response).nextValue().toString();
                byte[] bytes=Base64.decode(encoded,Base64.DEFAULT);
                if(bytes.length!=take)throw new IOException("Incomplete JavaScript chunk");
                out.write(bytes);collect(web,out,offset+take,length,callback);
            }catch(Exception ex){web.destroy();callback.failed("WEM transfer failed: "+ex.getMessage());}
        });
    }

    private static byte[] read(InputStream in,int cap)throws IOException{
        if(in==null)throw new IOException("No input stream");
        ByteArrayOutputStream out=new ByteArrayOutputStream();byte[] tmp=new byte[32768];int count;
        while((count=in.read(tmp))!=-1){
            if((long)out.size()+count>cap)throw new IOException("File too large for experimental encoder");
            out.write(tmp,0,count);
        }
        return out.toByteArray();
    }
}
