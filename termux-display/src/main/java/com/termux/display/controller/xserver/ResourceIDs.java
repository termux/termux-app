package com.termux.display.controller.xserver;

import androidx.collection.ArraySet;

import java.util.Iterator;

public class ResourceIDs {
    private final ArraySet<Integer> idBases = new ArraySet<>();
    public final int idMask;

    public ResourceIDs(int maxClients) {
        int clientsBits = 32 - Integer.numberOfLeadingZeros(maxClients);
        clientsBits = Integer.bitCount(maxClients) == 1 ? clientsBits - 1 : clientsBits;
        int base = 29 - clientsBits;
        idMask = (1 << base) - 1;
        for (int i = 1; i < maxClients; i++) idBases.add(i << base);
    }

    public synchronized Integer get() {
        if (idBases.isEmpty()) return -1;
        Iterator<Integer> iter = idBases.iterator();
        int idBase = iter.next();
        iter.remove();
        return idBase;
    }

    public boolean isInInterval(int value, int idBase) {
        return (value | idMask) == (idBase | idMask);
    }

    public synchronized void free(Integer idBase) {
        idBases.add(idBase);
    }
}
