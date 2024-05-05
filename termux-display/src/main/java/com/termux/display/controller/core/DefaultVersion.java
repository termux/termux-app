package com.termux.display.controller.core;

import android.content.Context;

public abstract class DefaultVersion {
    public static final String BOX86 = "0.3.0";
    public static final String BOX64 = "0.2.6";
    public static final String ZINK = "22.2.2";
    public static final String VIRGL = "22.1.7";
    public static final String DXVK = "1.10.3";

    public static String TURNIP(Context context) {
        return GPUInformation.isAdreno6xx(context) ? "23.0.0" : "24.1.0";
    }
}
