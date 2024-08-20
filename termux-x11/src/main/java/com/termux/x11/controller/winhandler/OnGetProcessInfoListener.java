package com.termux.x11.controller.winhandler;

public interface OnGetProcessInfoListener {
    void onGetProcessInfo(int index, int count, ProcessInfo processInfo);
}
