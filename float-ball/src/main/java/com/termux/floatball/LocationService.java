package com.termux.floatball;

import android.content.Context;

public interface LocationService {
    /**
     * This method will be called back when the position of the floating ball changes. Rewrite this method to save the position
     *
     * @param x
     * @param y
     */
    void onLocationChanged(int x, int y);

    /**
     * provide {@link LocationService#onLocationChanged(int, int)}The saved position can make the floating ball display at the last recorded position
     *
     * @return
     */
    int[] onRestoreLocation();

    /**
     * You can use this method to pass context, optional.
     *
     * @param context
     */
    void start(Context context);
}
