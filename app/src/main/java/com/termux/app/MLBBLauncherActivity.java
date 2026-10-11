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
import rikka.shizuku.Shizuku;
import android.content.pm.PackageManager;

/** Local BNK explorer. The protected-game Shizuku broker is a separate milestone. */
public class MLBBLauncherActivity extends Activity {
    private static final int OPEN_BNK=100, SAVE_WEM=101, IMPORT_WEM=102, SAVE_BNK=103, IMPORT_AUDIO=104;
    private final java.util.concurrent.ExecutorService worker=Executors.newSingleThreadExecutor();
    private LinearLayout root;
    private TextView status;
    private MLBBBankReader bank;
    private final java.util.Map<Long,byte[]> replacements=new java.util.HashMap<>();
    private IMLBBFiles shizukuFiles;
    private final android.content.ServiceConnection shizukuConnection=new android.content.ServiceConnection(){
        @Override public void onServiceConnected(android.content.ComponentName name,android.os.IBinder binder){
            shizukuFiles=IMLBBFiles.Stub.asInterface(binder);message("Shizuku game file service connected");
        }
        @Override public void onServiceDisconnected(android.content.ComponentName name){shizukuFiles=null;message("Shizuku service disconnected");}
    };
    private MLBBBankReader.Entry selected;
    private String bankName="soundbank";
    private String gameBankName=null;
    private boolean autoInstallAfterConversion=false;
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
        button("Shizuku: check / request permission",this::checkShizuku);
        category("Models");category("Emotes");category("Images");category("Other Resources");
        button("Developer Terminal (requires custom bootstrap)",()->new AlertDialog.Builder(this).setTitle("Terminal not available").setMessage("The standalone app has its own package ID. A matching com.farizgd.mlbbeditor Termux bootstrap must be rebuilt before enabling the shell. Audio BNK and Shizuku features do not need the terminal.").setPositiveButton("OK",null).show());
        label("This standalone build uses com.farizgd.mlbbeditor. Termux shell runtime is temporarily disabled until the custom bootstrap is rebuilt.",13);
    }
    private void checkShizuku(){
        try {
            if(!Shizuku.pingBinder()){message("Start Shizuku first");return;}
            if(Shizuku.checkSelfPermission()==PackageManager.PERMISSION_GRANTED){
                Shizuku.UserServiceArgs args=new Shizuku.UserServiceArgs(new android.content.ComponentName(this,MLBBFilesService.class))
                    .daemon(false).processNameSuffix("mlbb_files").debuggable(false).version(1);
                Shizuku.bindUserService(args,shizukuConnection);
                message("Connecting to Shizuku game file service");return;
            }
            Shizuku.requestPermission(4238);
            message("Shizuku permission requested");
        }catch(Exception e){message("Shizuku: "+e.getMessage());}
    }
    private void audio(){
        page("Audio Editor");
        button("← Home",this::home);
        button("Browse MLBB game files via Shizuku",this::browseGameBanks);
        button("Open BNK using Android file picker",()->{
            Intent i=new Intent(Intent.ACTION_OPEN_DOCUMENT);
            i.addCategory(Intent.CATEGORY_OPENABLE);i.setType("*/*");
            startActivityForResult(i,OPEN_BNK);
        });
        if(bank!=null)showEntries();
        else label("Browse game BNKs through Shizuku or import a local BNK file.",14);
    }
    private void browseGameBanks(){
        if(shizukuFiles==null){checkShizuku();message("Connect Shizuku, then tap Browse again");return;}
        message("Loading MLBB soundbanks...");
        worker.execute(()->{
            try{
                String[] names=shizukuFiles.listBanks();
                runOnUiThread(()->showBankPicker(names));
            }catch(Exception e){message("Browse failed: "+e.getMessage());}
        });
    }
    private void showBankPicker(String[] names){
        LinearLayout content=new LinearLayout(this);
        content.setOrientation(LinearLayout.VERTICAL);
        int pad=(int)(16*getResources().getDisplayMetrics().density);
        content.setPadding(pad,0,pad,0);
        EditText query=new EditText(this);
        query.setSingleLine(true);
        query.setHint("Filter by filename, e.g. bgm_intro");
        content.addView(query);
        TextView count=new TextView(this);
        count.setTextColor(Color.LTGRAY);
        content.addView(count);
        ListView list=new ListView(this);
        content.addView(list,new LinearLayout.LayoutParams(-1,0,1));
        final java.util.ArrayList<String> filtered=new java.util.ArrayList<>();
        final ArrayAdapter<String> adapter=new ArrayAdapter<>(this,android.R.layout.simple_list_item_1,filtered);
        list.setAdapter(adapter);
        AlertDialog dlg=new AlertDialog.Builder(this)
            .setTitle("MLBB Soundbanks")
            .setView(content)
            .setNegativeButton("Close",null).create();
        Runnable refresh=()->{
            String q=query.getText().toString().trim().toLowerCase(java.util.Locale.ROOT);
            filtered.clear();
            int found=0;
            for(String name:names){
                if(name.toLowerCase(java.util.Locale.ROOT).contains(q)){
                    found++;
                    if(filtered.size()<200)filtered.add(name);
                }
            }
            count.setText(found+" of "+names.length+" matching (showing first "+filtered.size()+")");
            adapter.notifyDataSetChanged();
        };
        query.addTextChangedListener(new android.text.TextWatcher(){
            public void beforeTextChanged(CharSequence x,int start,int count,int after){}
            public void onTextChanged(CharSequence x,int start,int before,int count){refresh.run();}
            public void afterTextChanged(android.text.Editable e){}
        });
        list.setOnItemClickListener((parent,view,index,id)->{
            String picked=filtered.get(index);
            dlg.dismiss();
            openGameBank(picked);
        });
        dlg.setOnShowListener(d->{
            int height=(int)(440*getResources().getDisplayMetrics().density);
            list.getLayoutParams().height=height;
            list.requestLayout();
        });
        refresh.run();
        dlg.show();
    }
    private void openGameBank(String name){
        worker.execute(()->{
            try(android.os.ParcelFileDescriptor fd=shizukuFiles.openBank(name);
                InputStream in=new android.os.ParcelFileDescriptor.AutoCloseInputStream(fd)){
                MLBBBankReader parsed=new MLBBBankReader(readLimited(in,128*1024*1024));
                runOnUiThread(()->{bank=parsed;replacements.clear();gameBankName=name;bankName=name;selected=null;audio();message("Loaded "+parsed.entries.size()+" sounds via Shizuku");});
            }catch(Exception e){message("Cannot open "+name+": "+e.getMessage());}
        });
    }
    private void showEntries(){
        label(bankName+" · "+bank.entries.size()+" audio entries",16);
        label(replacements.size()+" replacements queued",14);
        button("Export modified BNK (does not touch game)",this::savePatchedBank);
        if(gameBankName!=null){
            button("Install modified BNK with Shizuku",this::confirmInstall);
            button("Restore original .bak with Shizuku",this::confirmRestore);
        }
        for(MLBBBankReader.Entry e:bank.entries){
            button("WEM ID "+e.id+" ("+e.size+" bytes)"+(replacements.containsKey(e.id)?"  • REPLACED":""),()->{
                selected=e;
                new AlertDialog.Builder(this).setTitle("Audio ID "+e.id)
                    .setItems(new String[]{"Export original WEM","Replace from WEM","Replace from MP3/OGG/WAV","Replace and auto-install MP3/OGG/WAV","Clear replacement","About audio preview"},(d,option)->{
                        if(option==0){
                            Intent i=new Intent(Intent.ACTION_CREATE_DOCUMENT);
                            i.addCategory(Intent.CATEGORY_OPENABLE);
                            i.setType("application/octet-stream");
                            i.putExtra(Intent.EXTRA_TITLE,e.id+".wem");
                            startActivityForResult(i,SAVE_WEM);
                        } else if(option==1){
                            Intent i=new Intent(Intent.ACTION_OPEN_DOCUMENT);
                            i.addCategory(Intent.CATEGORY_OPENABLE);i.setType("*/*");
                            startActivityForResult(i,IMPORT_WEM);
                        } else if(option==2 || option==3){
                            autoInstallAfterConversion=(option==3);
                            Intent i=new Intent(Intent.ACTION_OPEN_DOCUMENT);
                            i.addCategory(Intent.CATEGORY_OPENABLE);i.setType("audio/*");
                            i.putExtra(Intent.EXTRA_MIME_TYPES,new String[]{"audio/mpeg","audio/ogg","audio/wav","audio/x-wav","audio/flac","audio/mp4","application/ogg"});
                            startActivityForResult(i,IMPORT_AUDIO);
                        } else if(option==4){
                            replacements.remove(e.id);
                            audio();message("Cleared replacement for "+e.id);
                        } else {
                            new AlertDialog.Builder(this).setMessage("Wwise Custom Vorbis requires native vgmstream. Replacing audio with a different codec may not work in game.")
                                .setPositiveButton("OK",null).show();
                        }
                    }).show();
            });
        }
    }
    private void confirmInstall(){
        final String name=gameBankName;
        if(name==null||shizukuFiles==null){message("Select game bank through Shizuku first");return;}
        if(replacements.isEmpty()){message("Select at least one replacement WEM");return;}
        try {bank.validateReplacementCodecs(new java.util.HashMap<>(replacements));}
        catch(IOException incompatible){
            new AlertDialog.Builder(this).setTitle("Incompatible Wwise codec")
                .setMessage(incompatible.getMessage()+"\n\nOriginal soundbank left unchanged. Use a matching Wwise encoder before installing.")
                .setPositiveButton("OK",null).show();return;
        }
        new AlertDialog.Builder(this).setTitle("Install edited "+name+"?")
            .setMessage("Close Mobile Legends before continuing. Original .bak is retained and never overwritten. Different Wwise codecs may fail in-game.")
            .setNegativeButton("Cancel",null)
            .setPositiveButton("Install", (dialog,which)->worker.execute(()->{
                try{
                    byte[] encoded=bank.repack(new java.util.HashMap<>(replacements));
                    java.io.File stage=new java.io.File(getCacheDir(),"mlbb-patch.bnk");
                    try(java.io.FileOutputStream out=new java.io.FileOutputStream(stage)){out.write(encoded);out.getFD().sync();}
                    try(android.os.ParcelFileDescriptor fd=android.os.ParcelFileDescriptor.open(stage,android.os.ParcelFileDescriptor.MODE_READ_ONLY)){
                        message(shizukuFiles.installBank(name,fd));
                    }finally{stage.delete();}
                }catch(Exception ex){message("Install failed: "+ex.getMessage());}
            })).show();
    }
    private void confirmRestore(){
        final String name=gameBankName;
        if(name==null||shizukuFiles==null){message("Select a game bank through Shizuku");return;}
        new AlertDialog.Builder(this).setTitle("Restore original "+name+"?")
            .setMessage("Restore the .bak original and retain backup file? Close MLBB before restoring.")
            .setNegativeButton("Cancel",null)
            .setPositiveButton("Restore",(dialog,which)->worker.execute(()->{
                try{message(shizukuFiles.restoreBank(name));}
                catch(Exception ex){message("Restore failed: "+ex.getMessage());}
            })).show();
    }
    private void savePatchedBank(){
        if(bank==null){message("Select a BNK");return;}
        Intent i=new Intent(Intent.ACTION_CREATE_DOCUMENT);
        i.addCategory(Intent.CATEGORY_OPENABLE);i.setType("application/octet-stream");
        i.putExtra(Intent.EXTRA_TITLE,"patched_"+bankName);
        startActivityForResult(i,SAVE_BNK);
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
                        bank=parsed;replacements.clear();gameBankName=null;selected=null;bankName="Imported BNK.bnk";audio();
                        message("Parsed "+parsed.entries.size()+" WEM entries");
                    });
                }catch(Exception e){message("Import failed: "+e.getMessage());}
            });
        }else if(request==IMPORT_AUDIO && bank!=null && selected!=null){
            final long id=selected.id;
            final boolean install=autoInstallAfterConversion;
            if(install && (gameBankName==null || shizukuFiles==null)){
                message("Auto-install needs a game BNK opened via Shizuku");
                return;
            }
            Runnable convert=()->worker.execute(()->{
                try{
                    message("Converting audio to 16-bit PCM WEM...");
                    byte[] wem=MLBBAudioConverter.toPcmWem(this,uri);
                    replacements.put(id,wem);
                    runOnUiThread(()->{
                        audio();
                        message("Converted audio for "+id+" ("+(wem.length/1024)+" KiB)");
                        if(install)confirmInstall();
                    });
                }catch(Exception ex){message("Audio conversion failed: "+ex.getMessage());}
            });
            if(install){
                new AlertDialog.Builder(this)
                    .setTitle("Convert and install audio?")
                    .setMessage("The app converts to PCM WEM. If the original WEM uses Wwise Custom Vorbis, installation will be blocked instead of silently replacing it with incompatible PCM. The converted media can still be exported.")
                    .setNegativeButton("Cancel",null).setPositiveButton("Convert", (d,w)->convert.run()).show();
            } else convert.run();
        }else if(request==IMPORT_WEM && bank!=null && selected!=null){
            final long id=selected.id;
            worker.execute(()->{
                try(InputStream in=getContentResolver().openInputStream(uri)){
                    byte[] wem=readLimited(in,64*1024*1024);
                    if(wem.length<12 || !(new String(wem,0,4,java.nio.charset.StandardCharsets.US_ASCII).equals("RIFF")))
                        throw new IOException("Not a RIFF/WEM");
                    replacements.put(id,wem);
                    runOnUiThread(()->{audio();message("Queued replacement for ID "+id);});
                }catch(Exception ex){message("Import WEM failed: "+ex.getMessage());}
            });
        }else if(request==SAVE_BNK && bank!=null){
            worker.execute(()->{
                try{
                    byte[] rebuilt=bank.repack(new java.util.HashMap<>(replacements));
                    try(OutputStream out=getContentResolver().openOutputStream(uri)){out.write(rebuilt);}
                    message("Repacked BNK exported. Original game file was not modified.");
                }catch(Exception ex){message("Repack failed: "+ex.getMessage());}
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
