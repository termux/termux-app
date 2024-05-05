package com.termux.display.controller.xserver;

import android.util.SparseArray;

public class PixmapManager extends XResourceManager {
    public final Visual visual;
    public final Visual[] supportedVisuals;
    public final PixmapFormat[] supportedPixmapFormats;
    private final SparseArray<Pixmap> pixmaps = new SparseArray<>();

    public PixmapManager() {
        visual = new Visual(IDGenerator.generate(), true, 32, 24, 0xff0000, 0x00ff00, 0x0000ff);
        supportedVisuals = new Visual[]{visual, new Visual(IDGenerator.generate(), false, 1, 1, 0, 0, 0)};

        supportedPixmapFormats = new PixmapFormat[] {
            new PixmapFormat(1, 1, 32),
            new PixmapFormat(24, 32, 32),
            new PixmapFormat(32, 32, 32)
        };
    }

    public Pixmap getPixmap(int id) {
        return pixmaps.get(id);
    }

    public Pixmap createPixmap(Drawable drawable) {
        if (pixmaps.indexOfKey(drawable.id) >= 0) return null;
        Pixmap pixmap = new Pixmap(drawable);
        pixmaps.put(drawable.id, pixmap);
        triggerOnCreateResourceListener(pixmap);
        return pixmap;
    }

    public void freePixmap(int id) {
        triggerOnFreeResourceListener(pixmaps.get(id));
        pixmaps.remove(id);
    }

    public Visual getVisualForDepth(byte depth) {
        if (depth == visual.depth) return visual;
        for (Visual visual : supportedVisuals) {
            if (depth == visual.depth) return visual;
        }
        return null;
    }

    public Visual getVisual(int id) {
        if (id == visual.id) return visual;
        for (Visual visual : supportedVisuals) {
            if (id == visual.id && visual.displayable) return visual;
        }
        return null;
    }
}
