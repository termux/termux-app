package com.termux.x11.controller.xserver;

public class Viewport {
    public int x;
    public int y;
    public int width;
    public int height;
    public float aspect;

    public void update(int outerWidth, int outerHeight, int innerWidth, int innerHeight) {
        aspect = Math.min((float)outerWidth / innerWidth, (float)outerHeight / innerHeight);
        width = (int)Math.ceil(innerWidth * aspect);
        height = (int)Math.ceil(innerHeight * aspect);
        x = (int)((outerWidth - innerWidth * aspect) * 0.5f);
        y = (int)((outerHeight - innerHeight * aspect) * 0.5f);
    }
}
