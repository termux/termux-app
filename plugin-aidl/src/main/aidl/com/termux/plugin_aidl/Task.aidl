package com.termux.plugin_aidl;

import android.os.ParcelFileDescriptor;

parcelable Task {
    ParcelFileDescriptor stdout;
    ParcelFileDescriptor stderr;
    int pid;
}
