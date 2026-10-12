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
    private static final int OPEN_BNK=100, SAVE_WEM=101, IMPORT_WEM=102, SAVE_BNK=103, IMPORT_AUDIO=104, IMPORT_OGG_TS=105, SAVE_TS_WEM=106, IMPORT_NATIVE_AUDIO=107, SAVE_NATIVE_WEM=108, SAVE_NATIVE_OGG=109;
    private final java.util.concurrent.ExecutorService worker=Executors.newSingleThreadExecutor();
    private LinearLayout root;
    private TextView status;
    private MLBBBankReader bank;
    private final java.util.Map<Long,byte[]> replacements=new java.util.HashMap<>();
    private IMLBBFiles shizukuFiles;
    private final android.content.ServiceConnection shizukuConnection=new android.content.ServiceConnection(){
        @Override public void onServiceConnected(android.content.ComponentName name,android.os.IBinder binder){
            shizukuFiles=IMLBBFiles.Stub.asInterface(binder);message("Layanan berkas game Shizuku terhubung");
        }
        @Override public void onServiceDisconnected(android.content.ComponentName name){shizukuFiles=null;message("Layanan Shizuku terputus");}
    };
    private MLBBBankReader.Entry selected;
    private String bankName="soundbank";
    private String gameBankName=null;
    private boolean autoInstallAfterConversion=false;
    private android.media.MediaPlayer previewPlayer;
    private File previewFile;
    private volatile int bankPreviewTicket=0;
    private String previewBankName=null;
    private byte[] nativeConvertedWem=null;
    private byte[] nativeDecodedOgg=null;
    private byte[] experimentalWem=null;
    private long experimentalId=0;
    private int dp(float n){return (int)(n*getResources().getDisplayMetrics().density+0.5f);}
    private android.graphics.drawable.GradientDrawable shape(int fill,int stroke,int radius){
        android.graphics.drawable.GradientDrawable d=new android.graphics.drawable.GradientDrawable();
        d.setColor(fill);d.setCornerRadius(dp(radius));if(stroke!=0)d.setStroke(dp(1),stroke);return d;
    }
    private void label(String s,float size){
        TextView t=new TextView(this);t.setText(s);t.setTextSize(size);
        t.setTextColor(Color.rgb(233,238,253));t.setPadding(dp(3),dp(11),dp(3),dp(11));root.addView(t);
    }
    private void hint(String s){
        TextView t=new TextView(this);t.setText(s);t.setTextSize(13);
        t.setTextColor(Color.rgb(172,183,206));t.setLineSpacing(dp(3),1f);
        t.setPadding(dp(6),dp(4),dp(6),dp(14));root.addView(t);
    }
    private void section(String title,String description){
        label(title,19);if(description!=null)hint(description);
    }
    private void button(String s,Runnable action){
        Button b=new Button(this);b.setAllCaps(false);b.setText(s);b.setTextSize(15);
        b.setTextColor(Color.WHITE);
        b.setBackground(shape(Color.rgb(39,53,84),Color.rgb(74,91,131),15));
        b.setPadding(dp(14),dp(9),dp(14),dp(9));
        LinearLayout.LayoutParams lp=new LinearLayout.LayoutParams(-1,dp(54));
        lp.setMargins(0,dp(5),0,dp(7));root.addView(b,lp);
        b.setOnClickListener(v->action.run());
    }
    private void page(String title){
        ScrollView scroll=new ScrollView(this);
        scroll.setFillViewport(true);
        root=new LinearLayout(this);root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(dp(20),dp(21),dp(20),dp(30));
        root.setBackgroundColor(Color.rgb(13,20,35));
        scroll.addView(root);setContentView(scroll);
        TextView heading=new TextView(this);heading.setText(title);heading.setTextSize(27);
        heading.setTypeface(null,android.graphics.Typeface.BOLD);
        heading.setTextColor(Color.WHITE);heading.setPadding(0,dp(8),0,dp(12));
        root.addView(heading);
        status=new TextView(this);status.setText("Siap digunakan.");
        status.setTextSize(13);status.setTextColor(Color.rgb(203,219,255));
        status.setPadding(dp(14),dp(13),dp(14),dp(13));
        status.setBackground(shape(Color.rgb(27,44,76),Color.rgb(66,89,139),14));
        root.addView(status);
    }
    private void message(String s){runOnUiThread(()->{if(status!=null)status.setText(s);});}
    private void category(String title){
        button(title+" · Segera hadir",()->new AlertDialog.Builder(this)
            .setTitle(title).setMessage("Fitur ini masih dalam pengembangan.")
            .setPositiveButton("OK",null).show());
    }
    @Override protected void onCreate(Bundle state){super.onCreate(state);home();}
    @Override protected void onDestroy(){stopPreview();worker.shutdown();super.onDestroy();}
    private void stopPreview(){
        if(previewPlayer!=null){try{previewPlayer.stop();}catch(Exception ignored){}previewPlayer.release();previewPlayer=null;}
        if(previewFile!=null){previewFile.delete();previewFile=null;}
    }
    private void previewWem(MLBBBankReader.Entry e){
        if(!MLBBNativeVorbis.available()){message("VorbisLib tidak tersedia di perangkat ini");return;}
        message("Mengurai audio "+e.id+" untuk pratinjau...");
        worker.execute(()->{
            try{
                byte[] source=replacements.containsKey(e.id)?replacements.get(e.id):bank.extract(e);
                byte[] ogg=MLBBNativeVorbis.decodeToOgg(source,false);
                File f=File.createTempFile("mlbb-preview-", ".ogg",getCacheDir());
                try(FileOutputStream out=new FileOutputStream(f)){out.write(ogg);}
                runOnUiThread(()->{
                    try{
                        stopPreview();previewFile=f;
                        previewPlayer=new android.media.MediaPlayer();
                        previewPlayer.setDataSource(f.getAbsolutePath());
                        previewPlayer.setOnCompletionListener(p->{stopPreview();message("Pratinjau selesai");});
                        previewPlayer.prepare();previewPlayer.start();
                        message("Memutar "+e.id+" (ketuk Hentikan pratinjau untuk berhenti)");
                    }catch(Exception ex){stopPreview();message("Gagal memutar audio: "+ex.getMessage());}
                });
            }catch(Exception ex){message("Gagal mengurai audio untuk pratinjau: "+ex.getMessage());}
        });
    }
    private void home(){
        page("MLBB Resource Editor");
        section("Ubah suara Mobile Legends sesukamu", "Ganti musik dan efek suara langsung dari HP. Tidak perlu PC atau Wwise Authoring.");
        section("Panduan memulai", "1. Jalankan Shizuku dan izinkan aplikasi ini.\n2. Buka Editor Audio, lalu pilih bank suara game.\n3. Pilih suara, impor audio, kemudian dengarkan pratinjau.\n4. Pasang perubahan setelah memeriksa hasilnya.");
        button("1  Hubungkan Shizuku",this::checkShizuku);
        button("2  Buka Editor Audio",this::audio);
        section("Perlu diketahui","Aplikasi membuat cadangan asli .bnk.bak sebelum mengganti bank suara. Tutup MLBB sebelum memasang atau memulihkan audio.");
        button("Bantuan dan pertanyaan umum",this::showHelp);
        button("Alat eksperimental",()->new AlertDialog.Builder(this).setTitle("Fitur eksperimental")
            .setItems(new String[]{"Model (segera hadir)","Emote (segera hadir)","Gambar (segera hadir)","Sumber daya lain (segera hadir)","Terminal pengembang (belum tersedia)"},
                (d,i)->new AlertDialog.Builder(this).setMessage(i==4?"Terminal mandiri memerlukan bootstrap yang dibangun ulang untuk aplikasi ini.": "Fitur editor ini direncanakan untuk pembaruan berikutnya.")
                    .setPositiveButton("OK",null).show()).show());
    }
    private void showHelp(){
        new AlertDialog.Builder(this).setTitle("Panduan singkat")
            .setMessage("Apa itu BNK?\nBank suara Wwise yang berisi audio game.\n\nApa itu WEM?\nSatu file suara di dalam BNK.\n\nMengapa perlu Shizuku?\nAndroid membatasi akses ke folder audio game. Shizuku memberikan akses sesuai izin pengguna.\n\nBisakah perubahan dibatalkan?\nBisa. Pilih bank suara yang sama, lalu ketuk Pulihkan bank suara asli.\n\nFormat apa yang didukung?\nFormat audio umum yang dapat dibaca dekoder Android. Audio akan dikonversi menjadi WEM yang sesuai.\n\nCara mencoba dengan aman?\nImpor audio, dengarkan pratinjau, lalu ekspor BNK hasil edit sebelum memasangnya.")
            .setPositiveButton("Mengerti",null).show();
    }
    private void checkShizuku(){
        try {
            if(!Shizuku.pingBinder()){message("Jalankan Shizuku terlebih dahulu");return;}
            if(Shizuku.checkSelfPermission()==PackageManager.PERMISSION_GRANTED){
                Shizuku.UserServiceArgs args=new Shizuku.UserServiceArgs(new android.content.ComponentName(this,MLBBFilesService.class))
                    .daemon(false).processNameSuffix("mlbb_files").debuggable(false).version(1);
                Shizuku.bindUserService(args,shizukuConnection);
                message("Menghubungkan layanan akses berkas game melalui Shizuku");return;
            }
            Shizuku.requestPermission(4238);
            message("Izin Shizuku telah diminta");
        }catch(Exception e){message("Shizuku: "+e.getMessage());}
    }
    private void audio(){
        page("Editor Audio");
        button("Kembali ke beranda",this::home);
        section("Langkah 1 · Pilih bank suara", "Bank suara (BNK) berisi musik atau efek suara yang dapat diganti.");
        if(shizukuFiles==null)button("Hubungkan Shizuku dahulu",this::checkShizuku);
        button("Jelajahi bank suara Mobile Legends",this::browseGameBanks);
        button("Buka berkas BNK dari penyimpanan",()->{
            Intent i=new Intent(Intent.ACTION_OPEN_DOCUMENT);
            i.addCategory(Intent.CATEGORY_OPENABLE);i.setType("*/*");
            startActivityForResult(i,OPEN_BNK);
        });
        if(bank!=null)showEntries();
        else hint("Tips: Cari nama seperti lobby atau BGM untuk menemukan musik utama. BNK yang tersimpan di HP juga bisa dibuka tanpa Shizuku.");
    }
    private void browseGameBanks(){
        if(shizukuFiles==null){checkShizuku();message("Hubungkan Shizuku, lalu ketuk Jelajahi lagi");return;}
        message("Memuat bank suara MLBB...");
        worker.execute(()->{
            try{
                String[] names=shizukuFiles.listBanks();
                runOnUiThread(()->showBankPicker(names));
            }catch(Exception e){message("Gagal menjelajah: "+e.getMessage());}
        });
    }
    private void showBankPicker(String[] names){
        page("Daftar Bank Suara MLBB");
        button("Kembali ke Editor Audio",this::audio);
        hint("Cari bank suara, ketuk Buka untuk mengedit, atau Putar untuk mendengarkan audio pertama di dalam BNK. Pratinjau tidak mengubah berkas game.");
        EditText query=new EditText(this);
        query.setSingleLine(true);
        query.setHint("Cari bank suara, misalnya bgm");
        query.setTextColor(Color.WHITE);
        query.setHintTextColor(Color.rgb(164,177,203));
        root.addView(query);
        TextView count=new TextView(this);
        count.setTextColor(Color.rgb(191,204,230));
        count.setPadding(dp(4),dp(8),dp(4),dp(8));
        root.addView(count);
        final java.util.ArrayList<String> filtered=new java.util.ArrayList<>();
        ListView list=new ListView(this);
        list.setDividerHeight(dp(5));
        list.setBackgroundColor(Color.rgb(13,20,35));
        // Use one scroll container: a bounded list inside the page scroll.
        root.addView(list,new LinearLayout.LayoutParams(-1,dp(510)));
        final android.widget.BaseAdapter adapter=new android.widget.BaseAdapter(){
            @Override public int getCount(){return filtered.size();}
            @Override public Object getItem(int i){return filtered.get(i);}
            @Override public long getItemId(int i){return i;}
            @Override public View getView(int index,View old,android.view.ViewGroup parent){
                final String name=filtered.get(index);
                LinearLayout item=new LinearLayout(MLBBLauncherActivity.this);
                item.setOrientation(LinearLayout.VERTICAL);
                item.setPadding(dp(12),dp(10),dp(12),dp(10));
                item.setBackground(shape(Color.rgb(29,43,69),Color.rgb(63,83,120),14));
                TextView title=new TextView(MLBBLauncherActivity.this);
                title.setText(name);
                title.setTextColor(Color.WHITE);
                title.setTextSize(14);
                title.setMaxLines(2);
                item.addView(title);
                LinearLayout actions=new LinearLayout(MLBBLauncherActivity.this);
                actions.setOrientation(LinearLayout.HORIZONTAL);
                Button open=new Button(MLBBLauncherActivity.this);
                open.setAllCaps(false);open.setText("Buka");
                actions.addView(open,new LinearLayout.LayoutParams(0,dp(51),1));
                Button play=new Button(MLBBLauncherActivity.this);
                play.setAllCaps(false);
                play.setText(name.equals(previewBankName)?"Hentikan":"Putar");
                actions.addView(play,new LinearLayout.LayoutParams(0,dp(51),1));
                item.addView(actions);
                open.setOnClickListener(v->{bankPreviewTicket++;stopPreview();previewBankName=null;openGameBank(name);});
                play.setOnClickListener(v->{
                    if(name.equals(previewBankName)){
                        bankPreviewTicket++;stopPreview();previewBankName=null;
                        notifyDataSetChanged();message("Pratinjau dihentikan");
                    }else{
                        previewBank(name);
                        notifyDataSetChanged();
                    }
                });
                return item;
            }
        };
        list.setAdapter(adapter);
        Runnable refresh=()->{
            String q=query.getText().toString().trim().toLowerCase(java.util.Locale.ROOT);
            filtered.clear();
            int total=0;
            for(String name:names){
                if(name.toLowerCase(java.util.Locale.ROOT).contains(q)){
                    total++;
                    if(filtered.size()<200)filtered.add(name);
                }
            }
            count.setText(total+" dari "+names.length+" cocok. Menampilkan "+filtered.size()+" berkas.");
            adapter.notifyDataSetChanged();
        };
        query.addTextChangedListener(new android.text.TextWatcher(){
            public void beforeTextChanged(CharSequence s,int start,int count,int after){}
            public void onTextChanged(CharSequence s,int start,int before,int count){refresh.run();}
            public void afterTextChanged(android.text.Editable e){}
        });
        refresh.run();
    }
    private void previewBank(String name){
        if(shizukuFiles==null){message("Hubungkan Shizuku terlebih dahulu");return;}
        if(!MLBBNativeVorbis.available()){message("VorbisLib tidak tersedia untuk pratinjau");return;}
        final int ticket=++bankPreviewTicket;
        stopPreview();
        previewBankName=name;
        message("Menyiapkan pratinjau "+name+"...");
        worker.execute(()->{
            File temp=null;
            try(android.os.ParcelFileDescriptor fd=shizukuFiles.openBank(name);
                InputStream in=new android.os.ParcelFileDescriptor.AutoCloseInputStream(fd)){
                MLBBBankReader source=new MLBBBankReader(readLimited(in,Integer.MAX_VALUE));
                if(source.entries.isEmpty())throw new IOException("BNK tidak memiliki audio untuk diputar");
                byte[] wem=source.extract(source.entries.get(0));
                byte[] ogg=MLBBNativeVorbis.decodeToOgg(wem,false);
                temp=File.createTempFile("mlbb-bank-preview-", ".ogg",getCacheDir());
                try(FileOutputStream out=new FileOutputStream(temp)){out.write(ogg);}
                final File ready=temp;
                runOnUiThread(()->{
                    if(ticket!=bankPreviewTicket){ready.delete();return;}
                    try{
                        stopPreview();previewFile=ready;
                        previewPlayer=new android.media.MediaPlayer();
                        previewPlayer.setDataSource(ready.getAbsolutePath());
                        previewPlayer.setOnCompletionListener(p->{
                            stopPreview();previewBankName=null;message("Pratinjau selesai");
                        });
                        previewPlayer.prepare();previewPlayer.start();
                        message("Memutar audio pertama dari "+name);
                    }catch(Exception ex){
                        stopPreview();previewBankName=null;
                        message("Gagal memutar pratinjau: "+ex.getMessage());
                    }
                });
            }catch(Exception ex){
                if(temp!=null)temp.delete();
                runOnUiThread(()->{
                    if(ticket==bankPreviewTicket){
                        previewBankName=null;
                        message("Gagal memuat pratinjau "+name+": "+ex.getMessage());
                    }
                });
            }
        });
    }
    private void openGameBank(String name){
        worker.execute(()->{
            try(android.os.ParcelFileDescriptor fd=shizukuFiles.openBank(name);
                InputStream in=new android.os.ParcelFileDescriptor.AutoCloseInputStream(fd)){
                MLBBBankReader parsed=new MLBBBankReader(readLimited(in,Integer.MAX_VALUE));
                runOnUiThread(()->{bank=parsed;replacements.clear();gameBankName=name;bankName=name;selected=null;audio();message("Berhasil memuat "+parsed.entries.size()+" suara melalui Shizuku");});
            }catch(Exception e){message("Tidak dapat membuka "+name+": "+e.getMessage());}
        });
    }
    private void showEntries(){
        section("Langkah 2 · Pilih suara", bankName);
        hint(replacements.size()+" audio pengganti disiapkan. Belum ada perubahan yang dipasang tanpa konfirmasi.");
        button("Simpan hasil edit BNK sebagai berkas (aman)",this::savePatchedBank);
        if(previewPlayer!=null)button("Hentikan pratinjau",()->{stopPreview();audio();message("Pratinjau dihentikan");});
        if(gameBankName!=null){
            if(!replacements.isEmpty())button("Langkah 3 · Pasang bank suara hasil edit",this::confirmInstall);
            button("Pulihkan bank suara asli",this::confirmRestore);
        }
        for(MLBBBankReader.Entry e:bank.entries){
            String codec; try{codec=MLBBBankReader.codecName(bank.originalCodec(e));}catch(IOException ex){codec="Kodek tidak dikenal";}
            hint("ID suara: "+e.id+"  |  Jenis WEM: "+codec+(replacements.containsKey(e.id)?"  |  Pengganti disiapkan":""));
            LinearLayout soundActions=new LinearLayout(this);
            soundActions.setOrientation(LinearLayout.HORIZONTAL);
            Button previewButton=new Button(this);
            previewButton.setAllCaps(false);previewButton.setText("Putar");
            Button editButton=new Button(this);
            editButton.setAllCaps(false);editButton.setText("Edit Suara...");
            LinearLayout.LayoutParams side=new LinearLayout.LayoutParams(0,dp(54),1);
            soundActions.addView(previewButton,side);
            soundActions.addView(editButton,new LinearLayout.LayoutParams(0,dp(54),2));
            root.addView(soundActions);
            previewButton.setOnClickListener(v->{
                if(previewPlayer!=null){stopPreview();message("Pratinjau dihentikan");previewButton.setText("Putar");}
                else {previewWem(e);previewButton.setText("Hentikan");}
            });
            editButton.setOnClickListener(v->{
                selected=e;
                new AlertDialog.Builder(this).setTitle("Edit suara "+e.id)
                    .setItems(new String[]{"Ekspor suara asli (.wem)","Lanjutan: ganti menggunakan WEM","Impor musik atau audio (disarankan)","Impor audio dan pasang setelah konfirmasi","Batalkan perubahan yang disiapkan","Dengarkan suara ini","Lanjutan: konverter OGG lama (uji coba)","Lanjutan: ekspor WAV/OGG native","Lanjutan: ekspor OGG hasil dekode"},(d,option)->{
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
                        } else if(option==5){
                            previewWem(e);
                        } else if(option==4){
                            replacements.remove(e.id);
                            audio();message("Penggantian dibatalkan untuk "+e.id);
                        } else if(option==7){
                            if(!MLBBNativeVorbis.available()){message("VorbisLib native belum disertakan. Tambahkan pustaka ARM64 dan bangun ulang APK.");return;}
                            Intent i=new Intent(Intent.ACTION_OPEN_DOCUMENT);
                            i.addCategory(Intent.CATEGORY_OPENABLE);i.setType("*/*");
                            startActivityForResult(i,IMPORT_NATIVE_AUDIO);
                        } else if(option==8){
                            if(!MLBBNativeVorbis.available()){message("VorbisLib native tidak tersedia");return;}
                            worker.execute(()->{
                                try{
                                    byte[] ogg=MLBBNativeVorbis.decodeToOgg(bank.extract(e),false);
                                    runOnUiThread(()->{
                                        nativeDecodedOgg=ogg;
                                        Intent i=new Intent(Intent.ACTION_CREATE_DOCUMENT);
                                        i.addCategory(Intent.CATEGORY_OPENABLE);
                                        i.setType("application/ogg");
                                        i.putExtra(Intent.EXTRA_TITLE,e.id+"_decoded.ogg");
                                        startActivityForResult(i,SAVE_NATIVE_OGG);
                                    });
                                }catch(Exception ex){message("Dekoder native gagal: "+ex.getMessage());}
                            });
                        } else if(option==6){
                            Intent i=new Intent(Intent.ACTION_OPEN_DOCUMENT);
                            i.addCategory(Intent.CATEGORY_OPENABLE);
                            i.setType("*/*");
                            startActivityForResult(i,IMPORT_OGG_TS);
                        } else {
                            new AlertDialog.Builder(this).setMessage("Wwise Custom Vorbis memerlukan enkoder yang kompatibel dan validasi vgmstream. Konverter TypeScript eksperimental belum mendukung format paket modern MLBB.")
                                .setPositiveButton("OK",null).show();
                        }
                    }).show();
            });
        }
    }
    private void confirmInstall(){
        final String name=gameBankName;
        if(name==null||shizukuFiles==null){message("Pilih bank suara game melalui Shizuku dahulu");return;}
        if(replacements.isEmpty()){message("Siapkan minimal satu WEM pengganti");return;}
        try {bank.validateReplacementCodecs(new java.util.HashMap<>(replacements));}
        catch(IOException incompatible){
            new AlertDialog.Builder(this).setTitle("Kodek Wwise tidak kompatibel")
                .setMessage(incompatible.getMessage()+"\n\nBank suara asli tidak berubah. Gunakan enkoder Wwise dengan kodek yang sesuai sebelum memasang.")
                .setPositiveButton("OK",null).show();return;
        }
        new AlertDialog.Builder(this).setTitle("Pasang hasil edit "+name+"?")
            .setMessage("Tutup Mobile Legends sebelum melanjutkan. Cadangan asli .bak tetap disimpan dan tidak ditimpa. Kodek Wwise yang tidak cocok mungkin gagal diputar di game.")
            .setNegativeButton("Batal",null)
            .setPositiveButton("Pasang", (dialog,which)->worker.execute(()->{
                try{
                    byte[] encoded=bank.repack(new java.util.HashMap<>(replacements));
                    java.io.File stage=new java.io.File(getCacheDir(),"mlbb-patch.bnk");
                    try(java.io.FileOutputStream out=new java.io.FileOutputStream(stage)){out.write(encoded);out.getFD().sync();}
                    try(android.os.ParcelFileDescriptor fd=android.os.ParcelFileDescriptor.open(stage,android.os.ParcelFileDescriptor.MODE_READ_ONLY)){
                        message(shizukuFiles.installBank(name,fd));
                    }finally{stage.delete();}
                }catch(Exception ex){message("Pemasangan gagal: "+ex.getMessage());}
            })).show();
    }
    private void confirmRestore(){
        final String name=gameBankName;
        if(name==null||shizukuFiles==null){message("Pilih bank suara game melalui Shizuku");return;}
        new AlertDialog.Builder(this).setTitle("Pulihkan versi asli "+name+"?")
            .setMessage("Pulihkan dari cadangan asli .bak dan tetap simpan berkasnya? Tutup MLBB sebelum memulihkan.")
            .setNegativeButton("Batal",null)
            .setPositiveButton("Pulihkan",(dialog,which)->worker.execute(()->{
                try{message(shizukuFiles.restoreBank(name));}
                catch(Exception ex){message("Pemulihan gagal: "+ex.getMessage());}
            })).show();
    }
    private void savePatchedBank(){
        if(bank==null){message("Pilih berkas BNK");return;}
        Intent i=new Intent(Intent.ACTION_CREATE_DOCUMENT);
        i.addCategory(Intent.CATEGORY_OPENABLE);i.setType("application/octet-stream");
        i.putExtra(Intent.EXTRA_TITLE,"patched_"+bankName);
        startActivityForResult(i,SAVE_BNK);
    }
    private static byte[] readLimited(InputStream in,int limit) throws IOException{
        ByteArrayOutputStream out=new ByteArrayOutputStream();
        byte[] tmp=new byte[65536];int n;
        while((n=in.read(tmp))!=-1){
            if(out.size()+(long)n>limit)throw new IOException("Berkas melebihi kapasitas array Java; diperlukan pemrosesan berkas bertahap");
            out.write(tmp,0,n);
        }
        return out.toByteArray();
    }
    @Override @Deprecated protected void onActivityResult(int request,int result,Intent data){
        super.onActivityResult(request,result,data);
        if(result!=RESULT_OK || data==null || data.getData()==null)return;
        Uri uri=data.getData();
        if(request==OPEN_BNK){
            message("Membaca BNK...");
            worker.execute(()->{
                try{
                    byte[] bytes;
                    try(InputStream in=getContentResolver().openInputStream(uri)){bytes=readLimited(in,128*1024*1024);}
                    MLBBBankReader parsed=new MLBBBankReader(bytes);
                    runOnUiThread(()->{
                        bank=parsed;replacements.clear();gameBankName=null;selected=null;bankName="BNK yang diimpor.bnk";audio();
                        message("Berhasil membaca "+parsed.entries.size()+" entri WEM");
                    });
                }catch(Exception e){message("Impor gagal: "+e.getMessage());}
            });
        }else if(request==IMPORT_NATIVE_AUDIO && selected!=null){
            final long id=selected.id;
            worker.execute(()->{
                try(InputStream in=getContentResolver().openInputStream(uri)){
                    byte[] audioBytes=readLimited(in,Integer.MAX_VALUE);
                    message("Mengenkode Wwise Custom Vorbis menggunakan pustaka ARM64 native...");
                    byte[] wem=MLBBNativeVorbis.encode(audioBytes,0.4f,Integer.MAX_VALUE);
                    if(MLBBBankReader.wemCodec(wem)!=0xffff)
                        throw new IOException("Hasil enkode memiliki kodek yang tidak sesuai");
                    runOnUiThread(()->{
                        nativeConvertedWem=wem;
                        new AlertDialog.Builder(this)
                            .setTitle("Custom Vorbis berhasil dienkode")
                            .setMessage("Menghasilkan WEM sebesar "+wem.length+" byte untuk ID "+id+". Berkas ini BELUM disiapkan atau dipasang. Ekspor dan uji menggunakan vgmstream sebelum mengganti audio game.")
                            .setNegativeButton("Batal",null)
                            .setPositiveButton("Ekspor WEM uji coba",(d,w)->{
                                Intent i=new Intent(Intent.ACTION_CREATE_DOCUMENT);
                                i.addCategory(Intent.CATEGORY_OPENABLE);
                                i.setType("application/octet-stream");
                                i.putExtra(Intent.EXTRA_TITLE,id+"_native_custom_vorbis.wem");
                                startActivityForResult(i,SAVE_NATIVE_WEM);
                            }).show();
                    });
                }catch(Exception ex){message("Enkoder native gagal: "+ex.getMessage());}
            });
        }else if(request==SAVE_NATIVE_WEM && nativeConvertedWem!=null){
            final byte[] bytes=nativeConvertedWem;
            worker.execute(()->{
                try(OutputStream out=getContentResolver().openOutputStream(uri)){
                    out.write(bytes);message("WEM uji coba berhasil diekspor; berkas MLBB tidak berubah.");
                }catch(Exception ex){message("Ekspor gagal: "+ex.getMessage());}
            });
        }else if(request==SAVE_NATIVE_OGG && nativeDecodedOgg!=null){
            final byte[] bytes=nativeDecodedOgg;
            worker.execute(()->{
                try(OutputStream out=getContentResolver().openOutputStream(uri)){
                    out.write(bytes);message("OGG hasil dekode berhasil diekspor.");
                }catch(Exception ex){message("Ekspor OGG gagal: "+ex.getMessage());}
            });
        }else if(request==IMPORT_OGG_TS && selected!=null){
            final long id=selected.id;
            message("Menjalankan enkoder Ogg Vorbis TypeScript eksperimental...");
            MLBBOggWemBridge.encode(this,uri,new MLBBOggWemBridge.Listener(){
                @Override public void completed(byte[] wem){
                    experimentalWem=wem;experimentalId=id;
                    message("WEM Vorbis lama berhasil dibuat ("+wem.length+" byte). Ekspor untuk pengujian dekoder luar. Tidak disiapkan untuk pemasangan.");
                    new AlertDialog.Builder(MLBBLauncherActivity.this)
                        .setTitle("WEM eksperimental berhasil dibuat")
                        .setMessage("Berkas ini BELUM lolos pemeriksaan vgmstream atau dekode MLBB. Simpan untuk pengujian dan jangan pasang ke game.")
                        .setNegativeButton("Batal",null)
                        .setPositiveButton("Ekspor WEM",(d,w)->{
                            Intent i=new Intent(Intent.ACTION_CREATE_DOCUMENT);
                            i.addCategory(Intent.CATEGORY_OPENABLE);
                            i.setType("application/octet-stream");
                            i.putExtra(Intent.EXTRA_TITLE,id+"_ts_experimental.wem");
                            startActivityForResult(i,SAVE_TS_WEM);
                        }).show();
                }
                @Override public void failed(String reason){message(reason);}
            });
        }else if(request==SAVE_TS_WEM && experimentalWem!=null){
            final byte[] candidate=experimentalWem;
            worker.execute(()->{
                try(OutputStream out=getContentResolver().openOutputStream(uri)){
                    out.write(candidate);message("WEM eksperimental diekspor untuk pemeriksaan. Game tidak berubah.");
                }catch(Exception ex){message("Tidak dapat mengekspor WEM: "+ex.getMessage());}
            });
        }else if(request==IMPORT_AUDIO && bank!=null && selected!=null){
            final long id=selected.id;
            final MLBBBankReader.Entry targetEntry=selected;
            final MLBBBankReader targetBank=bank;
            final boolean install=autoInstallAfterConversion;
            if(install && (gameBankName==null || shizukuFiles==null)){
                message("Pemasangan memerlukan BNK game yang dibuka melalui Shizuku");
                return;
            }
            Runnable convert=()->worker.execute(()->{
                try{
                    if(!MLBBNativeVorbis.available())throw new IOException("VorbisLib native hanya mendukung ARM64 dan tidak ditemukan di APK ini");
                    message("Mendekode audio impor ke WAV dengan kodek Android...");
                    byte[] wav=MLBBAudioConverter.decodeToWav(this,uri);
                    message("Mengenkode WAV ke Wwise Custom Vorbis...");
                    byte[] wem=MLBBNativeVorbis.encode(wav,0.4f,Integer.MAX_VALUE);
                    if(bank!=targetBank)throw new IOException("Bank suara telah berubah selama konversi; ulangi impor");
                    if(MLBBBankReader.wemCodec(wem)!=targetBank.originalCodec(targetEntry))
                        throw new IOException("Kodek WEM hasil enkode tidak cocok dengan kodek bank suara asli");
                    replacements.put(id,wem);
                    runOnUiThread(()->{
                        audio();
                        message("Audio Custom Vorbis disiapkan untuk "+id+" ("+(wem.length/1024)+" KiB). Dengarkan, ekspor, atau konfirmasi pemasangan.");
                        if(install)confirmInstall();
                    });
                }catch(Exception ex){message("Konversi audio gagal: "+ex.getMessage());}
            });
            if(install){
                new AlertDialog.Builder(this)
                    .setTitle("Konversi dan pasang audio?")
                    .setMessage("Audio akan didekode menggunakan Android MediaCodec, lalu dienkode oleh VorbisLib ke Custom Vorbis dan dimasukkan ke BNK. Sebelum dipasang melalui Shizuku, Anda tetap diminta mengonfirmasi. Pastikan cadangan asli .bak tersimpan.")
                    .setNegativeButton("Batal",null).setPositiveButton("Konversi", (d,w)->convert.run()).show();
            } else convert.run();
        }else if(request==IMPORT_WEM && bank!=null && selected!=null){
            final long id=selected.id;
            final MLBBBankReader targetBank=bank;
            worker.execute(()->{
                try(InputStream in=getContentResolver().openInputStream(uri)){
                    if(bank!=targetBank)throw new IOException("Bank suara berubah, buka kembali berkas WEM");
                    byte[] wem=readLimited(in,Integer.MAX_VALUE);
                    if(wem.length<12 || !(new String(wem,0,4,java.nio.charset.StandardCharsets.US_ASCII).equals("RIFF")))
                        throw new IOException("Bukan berkas RIFF/WEM");
                    replacements.put(id,wem);
                    runOnUiThread(()->{audio();message("Audio pengganti disiapkan untuk ID "+id);});
                }catch(Exception ex){message("Impor WEM gagal: "+ex.getMessage());}
            });
        }else if(request==SAVE_BNK && bank!=null){
            worker.execute(()->{
                try{
                    byte[] rebuilt=bank.repack(new java.util.HashMap<>(replacements));
                    try(OutputStream out=getContentResolver().openOutputStream(uri)){out.write(rebuilt);}
                    message("BNK hasil edit berhasil diekspor. Berkas asli game tidak diubah.");
                }catch(Exception ex){message("Gagal mengemas ulang BNK: "+ex.getMessage());}
            });
        }else if(request==SAVE_WEM && bank!=null && selected!=null){
            MLBBBankReader.Entry e=selected;
            worker.execute(()->{
                try(OutputStream out=getContentResolver().openOutputStream(uri)){
                    out.write(bank.extract(e));message("Berhasil mengekspor "+e.id+".wem");
                }catch(Exception ex){message("Ekspor gagal: "+ex.getMessage());}
            });
        }
    }
}
