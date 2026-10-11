package com.termux.app;

import android.app.*;
import android.content.*;
import android.database.Cursor;
import android.graphics.Color;
import android.net.Uri;
import android.os.Bundle;
import android.provider.OpenableColumns;
import android.view.*;
import android.widget.*;
import java.io.*;
import java.util.concurrent.Executors;

/** Local BNK explorer. The protected-game Shizuku broker is a separate milestone. */
public class MLBBLauncherActivity extends Activity {
    private static final int OPEN_BNK=100, SAVE_WEM=101;
    private final java.util.concurrent.ExecutorService worker=Executors.newSingleThreadExecutor();
    private LinearLayout root;
    private TextView status;
    private MLBBBankReader bank;
    private MLBBBankReader.Entry selected;
    private String bankName="soundbank";
    private void label(String s,float size){
        TextView t=new TextView(this);t.setText(s);t.setTextSize(size);
        t.setTextColor(Color.WHITE);t.setPadding(12,16,12,16);root.addView(t);
    }
    private void button(String s,Runnable action){
        Button b=new Button(this);b.setAllCaps(false);b.setText(s);root.addView(b);
        b.setOnClickListener(v->action.run());
    }
    private void page(String title){
        ScrollView scroll=new ScrollView(this);
        root=new LinearLayout(this);root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(20,24,20,24);root.setBackgroundColor(Color.rgb(15,17,29));
        scroll.addView(root);setContentView(scroll);
        label(title,25);status=new TextView(this);status.setTextColor(Color.LTGRAY);
        status.setPadding(12,8,12,16);root.addView(status);
    }
    private void message(String s){runOnUiThread(()->{if(status!=null)status.setText(s);});}
    private void category(String title){
        button(title+" · Coming soon",()->new AlertDialog.Builder(this)
            .setTitle(title).setMessage("This editor is planned but not yet implemented.")
            .setPositiveButton("OK",null).show());
    }
    @Override protected void onCreate(Bundle state){super.onCreate(state);home();}
    @Override protected void onDestroy(){worker.shutdown();super.onDestroy();}
    private void home(){
        page("MLBB Resource Editor");
        label("Resource explorer · Development build",15);
        button("🎵 Audio Editor",this::audio);
        category("Models");category("Emotes");category("Images");category("Other Resources");
        button("Open Developer Terminal",()->startActivity(new Intent(this,TermuxActivity.class)));
        label("This build still uses the original Termux package prefix. The custom app ID and Shizuku integration require additional work.",13);
    }
    private void audio(){
        page("Audio Editor");
        button("← Home",this::home);
        button("Open BNK using Android file picker",()->{
            Intent i=new Intent(Intent.ACTION_OPEN_DOCUMENT);
            i.addCategory(Intent.CATEGORY_OPENABLE);i.setType("*/*");
            startActivityForResult(i,OPEN_BNK);
        });
        if(bank!=null)showEntries();
        else label("Import a BNK from shared storage. Shizuku game-folder browsing is not yet available in this build.",14);
    }
    private void showEntries(){
        label(bankName+" · "+bank.entries.size()+" audio entries",16);
        for(MLBBBankReader.Entry e:bank.entries){
            button("WEM ID "+e.id+" ("+e.size+" bytes)",()->{
                selected=e;
                new AlertDialog.Builder(this).setTitle("Audio ID "+e.id)
                    .setItems(new String[]{"Export original WEM","About audio preview"},(d,option)->{
                        if(option==0){
                            Intent i=new Intent(Intent.ACTION_CREATE_DOCUMENT);
                            i.addCategory(Intent.CATEGORY_OPENABLE);
                            i.setType("application/octet-stream");
                            i.putExtra(Intent.EXTRA_TITLE,e.id+".wem");
                            startActivityForResult(i,SAVE_WEM);
                        } else {
                            new AlertDialog.Builder(this).setMessage("Wwise Custom Vorbis requires a native vgmstream decoder. The current milestone implements BNK inspection and export only.")
                                .setPositiveButton("OK",null).show();
                        }
                    }).show();
            });
        }
    }
    private static byte[] readLimited(InputStream in,int limit) throws IOException{
        ByteArrayOutputStream out=new ByteArrayOutputStream();
        byte[] tmp=new byte[65536];int n;
        while((n=in.read(tmp))!=-1){
            if(out.size()+(long)n>limit)throw new IOException("BNK too large for in-memory preview (128 MB cap)");
            out.write(tmp,0,n);
        }
        return out.toByteArray();
    }
    @Override @Deprecated protected void onActivityResult(int request,int result,Intent data){
        super.onActivityResult(request,result,data);
        if(result!=RESULT_OK || data==null || data.getData()==null)return;
        Uri uri=data.getData();
        if(request==OPEN_BNK){
            message("Reading BNK...");
            worker.execute(()->{
                try{
                    byte[] bytes;
                    try(InputStream in=getContentResolver().openInputStream(uri)){bytes=readLimited(in,128*1024*1024);}
                    MLBBBankReader parsed=new MLBBBankReader(bytes);
                    runOnUiThread(()->{
                        bank=parsed;selected=null;bankName="Imported BNK";audio();
                        message("Parsed "+parsed.entries.size()+" WEM entries");
                    });
                }catch(Exception e){message("Import failed: "+e.getMessage());}
            });
        }else if(request==SAVE_WEM && bank!=null && selected!=null){
            MLBBBankReader.Entry e=selected;
            worker.execute(()->{
                try(OutputStream out=getContentResolver().openOutputStream(uri)){
                    out.write(bank.extract(e));message("Exported "+e.id+".wem");
                }catch(Exception ex){message("Export failed: "+ex.getMessage());}
            });
        }
    }
}
