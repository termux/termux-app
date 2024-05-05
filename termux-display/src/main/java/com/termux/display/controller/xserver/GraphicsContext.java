package com.termux.display.controller.xserver;

public class GraphicsContext extends XResource {
    public static final int FLAG_FUNCTION = 1<<0;
    public static final int FLAG_PLANE_MASK = 1<<1;
    public static final int FLAG_FOREGROUND = 1<<2;
    public static final int FLAG_BACKGROUND = 1<<3;
    public static final int FLAG_LINE_WIDTH = 1<<4;
    public static final int FLAG_LINE_STYLE = 1<<5;
    public static final int FLAG_CAP_STYLE = 1<<6;
    public static final int FLAG_JOIN_STYLE = 1<<7;
    public static final int FLAG_FILL_STYLE = 1<<8;
    public static final int FLAG_FILL_RULE = 1<<9;
    public static final int FLAG_TILE = 1<<10;
    public static final int FLAG_STIPPLE = 1<<11;
    public static final int FLAG_TILE_STIPPLE_X_ORIGIN = 1<<12;
    public static final int FLAG_TILE_STIPPLE_Y_ORIGIN	= 1<<13;
    public static final int FLAG_FONT = 1<<14;
    public static final int FLAG_SUBWINDOW_MODE	= 1<<15;
    public static final int FLAG_GRAPHICS_EXPOSURES = 1<<16;
    public static final int FLAG_CLIP_X_ORIGIN = 1<<17;
    public static final int FLAG_CLIP_Y_ORIGIN = 1<<18;
    public static final int FLAG_CLIP_MASK = 1<<19;
    public static final int FLAG_DASH_OFFSET = 1<<20;
    public static final int FLAG_DASHES = 1<<21;
    public static final int FLAG_ARC_MODE = 1<<22;
    public enum Function {CLEAR, AND, AND_REVERSE, COPY, AND_INVERTED, NO_OP, XOR, OR, NOR, EQUIV, INVERT, OR_REVERSE, COPY_INVERTED, OR_INVERTED, NAND, SET}
    public enum SubwindowMode {CLIP_BY_CHILDREN, INCLUDE_INFERIORS}
    public final Drawable drawable;
    private Function function = Function.COPY;
    private int background = 0xffffff;
    private int foreground = 0x000000;
    private int lineWidth = 1;
    private int planeMask = -1;
    private SubwindowMode subwindowMode = SubwindowMode.CLIP_BY_CHILDREN;

    public GraphicsContext(int id, Drawable drawable) {
        super(id);
        this.drawable = drawable;
    }

    public int getForeground() {
        return foreground;
    }

    public void setForeground(int foreground) {
        this.foreground = foreground;
    }

    public int getBackground() {
        return this.background;
    }

    public void setBackground(int background) {
        this.background = background;
    }

    public int getLineWidth() {
        return lineWidth;
    }

    public void setLineWidth(int lineWidth) {
        this.lineWidth = lineWidth;
    }

    public int getPlaneMask() {
        return planeMask;
    }

    public void setPlaneMask(int planeMask) {
        this.planeMask = planeMask;
    }

    public Function getFunction() {
        return function;
    }

    public void setFunction(Function function) {
        this.function = function;
    }

    public SubwindowMode getSubwindowMode() {
        return subwindowMode;
    }

    public void setSubwindowMode(SubwindowMode subwindowMode) {
        this.subwindowMode = subwindowMode;
    }
}
