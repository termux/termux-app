package com.termux.x11.controller.xserver;

import androidx.annotation.NonNull;

import java.util.Iterator;

public class Bitmask implements Iterable<Integer> {
    private int bits = 0;

    public Bitmask() {}

    public Bitmask(int bits) {
        this.bits = bits;
    }

    public boolean isSet(int flag) {
        return (flag & this.bits) != 0;
    }

    public boolean intersects(Bitmask mask) {
        return (mask.bits & this.bits) != 0;
    }

    public void set(int flag) {
        bits |= flag;
    }

    public void set(int flag, boolean value) {
        if (value) {
            set(flag);
        }
        else unset(flag);
    }

    public void unset(int flag) {
        bits &= ~flag;
    }

    public boolean isEmpty() {
        return bits == 0;
    }

    public int getBits() {
        return bits;
    }

    public void join(Bitmask mask) {
        this.bits = mask.bits | this.bits;
    }

    @NonNull
    @Override
    public Iterator<Integer> iterator() {
        final int[] bits = {this.bits};
        return new Iterator<Integer>() {
            @Override
            public boolean hasNext() {
                return bits[0] != 0;
            }

            @Override
            public Integer next() {
                int index = Integer.lowestOneBit(bits[0]);
                bits[0] &= ~index;
                return index;
            }
        };
    }

    @Override
    public int hashCode() {
        return bits;
    }
}
