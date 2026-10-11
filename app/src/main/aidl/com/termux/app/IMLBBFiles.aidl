package com.termux.app;
import android.os.ParcelFileDescriptor;
interface IMLBBFiles {
    String[] listBanks();
    ParcelFileDescriptor openBank(String name);
    boolean backupExists(String name);
    void destroy();
}
