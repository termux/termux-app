package com.termux.display.controller.winhandler;

import com.termux.display.controller.core.StringUtils;

import java.util.ArrayList;

public class ProcessInfo {
    public final int pid;
    public final String name;
    public final long memoryUsage;
    public final int affinityMask;

    public ProcessInfo(int pid, String name, long memoryUsage, int affinityMask) {
        this.pid = pid;
        this.name = name;
        this.memoryUsage = memoryUsage;
        this.affinityMask = affinityMask;
    }

    public String getFormattedMemoryUsage() {
        return StringUtils.formatBytes(memoryUsage);
    }

    public String getCPUList() {
        int numProcessors = Runtime.getRuntime().availableProcessors();
        ArrayList<String> cpuList = new ArrayList<>();
        for (byte i = 0; i < numProcessors; i++) {
            if ((affinityMask & (1 << i)) != 0) cpuList.add(String.valueOf(i));
        }
        return String.join(",", cpuList.toArray(new String[0]));
    }
}
