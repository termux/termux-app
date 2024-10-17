package com.termux.x11.controller.core;

import com.termux.x11.LorieView;
import com.termux.x11.controller.math.Mathf;

import java.util.Timer;
import java.util.TimerTask;

public class CursorLocker extends TimerTask {
    private final LorieView xServer;
    private float damping = 0.25f;
    private short maxDistance;
    private boolean enabled = false;
    private final Object pauseLock = new Object();

    public CursorLocker(LorieView xServer) {
        this.xServer = xServer;
        maxDistance = (short)(xServer.screenInfo.screenWidth * 0.05f);
        Timer timer = new Timer();
        timer.schedule(this, 0, 1000 / 60);
    }

    public short getMaxDistance() {
        return maxDistance;
    }

    public void setMaxDistance(short maxDistance) {
        this.maxDistance = maxDistance;
    }

    public float getDamping() {
        return damping;
    }

    public void setDamping(float damping) {
        this.damping = damping;
    }

    public boolean isEnabled() {
        return enabled;
    }

    public void setEnabled(boolean enabled) {
        if (enabled) {
            synchronized (pauseLock) {
                this.enabled = true;
                pauseLock.notifyAll();
            }
        }
        else this.enabled = enabled;
    }

    @Override
    public void run() {
        synchronized (pauseLock) {
            if (!enabled) {
                try {
                    pauseLock.wait();
                }
                catch (InterruptedException e) {}
            }
        }

        short x = (short) Mathf.clamp(xServer.pointer.getX(), -maxDistance, xServer.screenInfo.screenWidth + maxDistance);
        short y = (short)Mathf.clamp(xServer.pointer.getY(), -maxDistance, xServer.screenInfo.screenHeight + maxDistance);

        if (x < 0) {
            xServer.pointer.setX((short)Math.ceil(x * damping));
        }
        else if (x >= xServer.screenInfo.screenWidth) {
            xServer.pointer.setX((short)Math.floor(xServer.screenInfo.screenWidth + (x - xServer.screenInfo.screenWidth) * damping));
        }
        if (y < 0) {
            xServer.pointer.setY((short)Math.ceil(y * damping));
        }
        else if (y >= xServer.screenInfo.screenHeight) {
            xServer.pointer.setY((short)Math.floor(xServer.screenInfo.screenHeight + (y - xServer.screenInfo.screenHeight) * damping));
        }
    }
}
