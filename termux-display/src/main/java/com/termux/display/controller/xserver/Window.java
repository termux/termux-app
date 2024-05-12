package com.termux.display.controller.xserver;

import android.util.SparseArray;

import androidx.annotation.NonNull;

import com.termux.display.controller.xserver.events.Event;
import com.termux.display.controller.xserver.events.InputDeviceEvent;
import com.termux.display.controller.xserver.events.MappingNotify;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class Window extends XResource {
    public static final int FLAG_X = 1;
    public static final int FLAG_Y = 1<<1;
    public static final int FLAG_WIDTH = 1<<2;
    public static final int FLAG_HEIGHT = 1<<3;
    public static final int FLAG_BORDER_WIDTH = 1<<4;
    public static final int FLAG_SIBLING = 1<<5;
    public static final int FLAG_STACK_MODE = 1<<6;

    public void sendEvent(Bitmask eventMask, Event event) {
    }
    public void sendEvent(int eventId, Event event) {
    }
    public void sendEvent(int eventId, InputDeviceEvent event) {
    }

    public void sendEvent(Event event) {
    }

    public enum StackMode {ABOVE, BELOW, TOP_IF, BOTTOM_IF, OPPOSITE}
    public enum MapState {UNMAPPED, UNVIEWABLE, VIEWABLE}
    private Drawable content;
    private short x;
    private short y;
    private short width;
    private short height;
    private short borderWidth;
    private Window parent;
    private final SparseArray<Property> properties = new SparseArray<>();
    private final ArrayList<Window> children = new ArrayList<>();
    private final List<Window> immutableChildren = Collections.unmodifiableList(children);
    public Window(int id, Drawable content, int x, int y, int width, int height) {
        super(id);
        this.content = content;
        this.x = (short)x;
        this.y = (short)y;
        this.width = (short)width;
        this.height = (short)height;
    }

    public short getX() {
        return x;
    }

    public void setX(short x) {
        this.x = x;
    }

    public short getY() {
        return y;
    }

    public void setY(short y) {
        this.y = y;
    }

    public short getWidth() {
        return width;
    }

    public void setWidth(short width) {
        this.width = width;
    }

    public short getHeight() {
        return height;
    }

    public void setHeight(short height) {
        this.height = height;
    }

    public short getBorderWidth() {
        return borderWidth;
    }

    public void setBorderWidth(short borderWidth) {
        this.borderWidth = borderWidth;
    }

    public Drawable getContent() {
        return content;
    }

    public void setContent(Drawable content) {
        this.content = content;
    }

    public Window getParent() {
        return parent;
    }

    public void setParent(Window parent) {
        this.parent = parent;
    }

    public Property getProperty(int id) {
        return properties.get(id);
    }

    @NonNull
    public String getPropertyValue(String name) {
        Property property = getProperty(Atom.getId(name));
        return property != null ? property.toString() : "";
    }

    public void addProperty(Property property) {
        properties.put(property.name, property);
    }

    public String getName() {
        return getPropertyValue("WM_NAME");
    }

    public String getClassName() {
        return getPropertyValue("WM_CLASS");
    }

    public boolean isInputOutput() {
        return content != null;
    }

    public void addChild(Window child) {
        if (child == null || child.parent == this) return;
        child.parent = this;
        children.add(child);
    }

    public void removeChild(Window child) {
        if (child == null || child.parent != this) return;
        child.parent = null;
        children.remove(child);
    }

    public Window previousSibling() {
        if (parent == null) return null;
        int index = parent.children.indexOf(this);
        return index > 0 ? parent.children.get(index - 1) : null;
    }

    public void moveChildAbove(Window child, Window sibling) {
        children.remove(child);
        if (sibling != null && children.contains(sibling)) {
            children.add(children.indexOf(sibling) + 1, child);
            return;
        }
        children.add(child);
    }

    public void moveChildBelow(Window child, Window sibling) {
        children.remove(child);
        if (sibling != null && children.contains(sibling)) {
            children.add(children.indexOf(sibling), child);
            return;
        }
        children.add(0, child);
    }

    public List<Window> getChildren() {
        return immutableChildren;
    }

    public int getChildCount() {
        return children.size();
    }


}
