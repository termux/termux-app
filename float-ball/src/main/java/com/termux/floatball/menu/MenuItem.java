package com.termux.floatball.menu;

import android.graphics.drawable.Drawable;

public abstract class MenuItem {
    /**
     * menu icon
     */
    public Drawable mDrawable;

    public MenuItem(Drawable drawable) {
        this.mDrawable = drawable;
    }

    /**
     * menu item action
     */
    public abstract void action();
}
