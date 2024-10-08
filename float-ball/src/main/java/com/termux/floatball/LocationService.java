package com.termux.floatball;

import android.content.Context;

public interface LocationService {
    /**
     * 当悬浮球位置改变会回调这个方法，重写此方法来保存位置
     *
     * @param x
     * @param y
     */
    void onLocationChanged(int x, int y);

    /**
     * 提供在{@link LocationService#onLocationChanged(int, int)}中保存的位置,可以让悬浮球在上次记录的位置显示
     *
     * @return
     */
    int[] onRestoreLocation();

    /**
     * 可以用这个方法来传递context，可选的。
     * @param context
     */
    void start(Context context);
}
