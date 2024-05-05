package com.termux.display.controller.xserver;

import java.util.ArrayList;

public abstract class XResourceManager {
    private final ArrayList<OnResourceLifecycleListener> onResourceLifecycleListeners = new ArrayList<>();

    public interface OnResourceLifecycleListener {
        default void onCreateResource(XResource resource) {}

        default void onFreeResource(XResource resource) {}
    }

    public void addOnResourceLifecycleListener(OnResourceLifecycleListener OnResourceLifecycleListener) {
        onResourceLifecycleListeners.add(OnResourceLifecycleListener);
    }

    public void removeOnResourceLifecycleListener(OnResourceLifecycleListener OnResourceLifecycleListener) {
        onResourceLifecycleListeners.remove(OnResourceLifecycleListener);
    }

    public void triggerOnCreateResourceListener(XResource resource) {
        for (int i = onResourceLifecycleListeners.size()-1; i >= 0; i--) {
            onResourceLifecycleListeners.get(i).onCreateResource(resource);
        }
    }

    public void triggerOnFreeResourceListener(XResource resource) {
        for (int i = onResourceLifecycleListeners.size()-1; i >= 0; i--) {
            onResourceLifecycleListeners.get(i).onFreeResource(resource);
        }
    }
}
