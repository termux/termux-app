package com.termux.app;

import android.os.ParcelFileDescriptor;
import androidx.annotation.Keep;
import java.io.*;
import java.util.*;
import java.nio.file.*;
import java.security.*;

/** Runs as Shizuku shell user. Constrained access to the MLBB audio folder, with backup-verified writes. */
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

    private static void copy(InputStream input,File dest)throws IOException{
        try(FileOutputStream out=new FileOutputStream(dest)){
            byte[] buffer=new byte[131072];int n;
            while((n=input.read(buffer))!=-1)out.write(buffer,0,n);
            out.getFD().sync();
        }
    }
    private static byte[] sha(File file)throws Exception{
        MessageDigest digest=MessageDigest.getInstance("SHA-256");
        try(InputStream in=new FileInputStream(file)){
            byte[] bytes=new byte[65536];int n;
            while((n=in.read(bytes))!=-1)digest.update(bytes,0,n);
        }
        return digest.digest();
    }
    private static void verifiedCopy(File source,File dest)throws Exception{
        try(InputStream in=new FileInputStream(source)){copy(in,dest);}
        if(!Arrays.equals(sha(source),sha(dest)))throw new IOException("Backup hash verification failed");
    }
    private static void atomicInstall(File stage,File destination)throws IOException{
        try{
            Files.move(stage.toPath(),destination.toPath(),
                StandardCopyOption.ATOMIC_MOVE,StandardCopyOption.REPLACE_EXISTING);
        }catch(AtomicMoveNotSupportedException ex){
            throw new IOException("Storage does not support safe atomic replace. Original left unchanged.",ex);
        }
    }
    @Override public String installBank(String name,ParcelFileDescriptor replacement){
        File stage=null,backupStage=null;
        try{
            File dest=resolve(name);
            if(!dest.isFile())throw new IOException("Original game BNK not found");
            File backup=new File(dest.getPath()+".bak");
            if(!backup.exists()){
                backupStage=new File(dest.getPath()+".bak-pending");
                verifiedCopy(dest,backupStage);
                if(!backupStage.renameTo(backup))throw new IOException("Cannot commit original backup");
            }
            if(!backup.isFile()||backup.length()<16)throw new IOException("Backup missing or invalid");
            stage=new File(dest.getPath()+".new-pending");
            try(InputStream in=new FileInputStream(replacement.getFileDescriptor())){copy(in,stage);}
            if(stage.length()<16)throw new IOException("Empty replacement");
            try(InputStream in=new FileInputStream(stage)){
                byte[] header=new byte[4];
                if(in.read(header)!=4||!Arrays.equals(header,new byte[]{'B','K','H','D'}))
                    throw new IOException("Replacement is not a BNK");
            }
            byte[] expected=sha(stage);
            atomicInstall(stage,dest);
            if(!Arrays.equals(expected,sha(dest)))throw new IOException("Installed BNK checksum mismatch; restore .bak immediately");
            return "Installed "+name+"; original .bak retained and installed SHA-256 verified";
        }catch(Exception ex){throw new IllegalStateException("Install refused: "+ex.getMessage(),ex);}
        finally{
            try{replacement.close();}catch(Exception ignored){}
            if(stage!=null)stage.delete();
            if(backupStage!=null)backupStage.delete();
        }
    }
    @Override public String restoreBank(String name){
        File stage=null;
        try{
            File dest=resolve(name);
            File backup=new File(dest.getPath()+".bak");
            if(!backup.isFile())throw new IOException("No original .bak exists");
            stage=new File(dest.getPath()+".restore-pending");
            verifiedCopy(backup,stage);
            atomicInstall(stage,dest);
            if(!Arrays.equals(sha(backup),sha(dest)))throw new IOException("Restore hash mismatch");
            return "Original "+name+" restored from verified backup";
        }catch(Exception ex){throw new IllegalStateException("Restore failed: "+ex.getMessage(),ex);}
        finally{if(stage!=null)stage.delete();}
    }
    @Override public void destroy(){System.exit(0);}

}
