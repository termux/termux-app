package com.termux.app;

import android.os.ParcelFileDescriptor;
import androidx.annotation.Keep;
import java.io.*;
import java.util.*;

/** Runs as Shizuku shell user. Read-only access to the MLBB audio folder. */
public class MLBBFilesService extends IMLBBFiles.Stub {
    private static final File BASE = new File("/storage/emulated/0/Android/data/com.mobile.legends/files/dragon2017/assets/Audio/android");
    @Keep public MLBBFilesService() {}
    @Override public String[] listBanks(){
        File[] fs=BASE.listFiles((dir,name)->name.endsWith(".bnk"));
        if(fs==null)throw new IllegalStateException("Cannot access "+BASE);
        String[] names=new String[fs.length];
        for(int i=0;i<fs.length;i++)names[i]=fs[i].getName();
        Arrays.sort(names);return names;
    }
    private File resolve(String name)throws IOException{
        if(name==null||!name.matches("[A-Za-z0-9_.-]+\\.bnk")||name.contains(".."))throw new SecurityException("Invalid name");
        File f=new File(BASE,name).getCanonicalFile();
        if(!BASE.getCanonicalFile().equals(f.getParentFile()))throw new SecurityException("Escaped game folder");
        return f;
    }
    @Override public ParcelFileDescriptor openBank(String name){
        try{return ParcelFileDescriptor.open(resolve(name),ParcelFileDescriptor.MODE_READ_ONLY);}
        catch(IOException e){throw new IllegalStateException(e);}
    }
    @Override public boolean backupExists(String name){
        try{return new File(resolve(name).getPath()+".bak").isFile();}
        catch(IOException e){throw new IllegalStateException(e);}
    }
    @Override public void destroy(){System.exit(0);}
}
