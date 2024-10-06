package com.termux.app.terminal;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.LinearLayout;

public class DisplayWindowLinearLayout extends LinearLayout {
    public DisplayWindowLinearLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
//		Log.e("TAG", "MyLinearLayout");
        setChildrenDrawingOrderEnabled(true);
    }

    @Override
    protected int getChildDrawingOrder(int childCount, int i) {
//        Log.e("tag", "getChildDrawingOrder" + i + " , " + childCount);
        if (i == 0)
            return 1;
        if (i == 2)
            return 2;
        if (i == 1)
            return 0;
        return super.getChildDrawingOrder(childCount, i);
    }
}
