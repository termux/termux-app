package com.termux.x11.controller.widget;

import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.PointF;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.TextView;

import androidx.annotation.Nullable;
import androidx.preference.PreferenceManager;

import com.termux.x11.R;
import com.termux.x11.controller.core.Callback;
import com.termux.x11.controller.core.UnitUtils;
import com.termux.x11.controller.math.Mathf;

public class MagnifierView extends FrameLayout {
    private final SharedPreferences preferences;
    private boolean restoreSavedPosition = true;
    private short lastX = 0;
    private short lastY = 0;
    private Callback<Float> zoomButtonCallback;
    private Runnable hideButtonCallback;
    private TextView textView;

    public MagnifierView(Context context) {
        this(context, null);
    }

    public MagnifierView(Context context, @Nullable AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public MagnifierView(Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
        this(context, attrs, defStyleAttr, 0);
    }

    public MagnifierView(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);

        preferences = PreferenceManager.getDefaultSharedPreferences(context);
        setLayoutParams(new LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        View contentView = LayoutInflater.from(context).inflate(R.layout.magnifier_view, this, false);

        final PointF startPoint = new PointF();
        final boolean[] isActionDown = {false};
        contentView.findViewById(R.id.BTMove).setOnTouchListener((v, event) -> {
            switch (event.getAction()) {
                case MotionEvent.ACTION_DOWN:
                    startPoint.x = event.getX();
                    startPoint.y = event.getY();
                    isActionDown[0] = true;
                    break;
                case MotionEvent.ACTION_MOVE:
                    if (isActionDown[0]) {
                        float newX = getX() + (event.getX() - startPoint.x);
                        float newY = getY() + (event.getY() - startPoint.y);
                        movePanel(newX, newY);
                    }
                    break;
                case MotionEvent.ACTION_UP:
                    if (isActionDown[0] && lastX > 0 && lastY > 0) {
                        preferences.edit().putString("magnifier_view", lastX+"|"+lastY).apply();
                    }
                    lastX = 0;
                    lastY = 0;
                    isActionDown[0] = false;
                    break;
            }
            return true;
        });

        contentView.findViewById(R.id.BTZoomPlus).setOnClickListener((v) -> {
            if (zoomButtonCallback != null) zoomButtonCallback.call(0.25f);
        });

        contentView.findViewById(R.id.BTZoomMinus).setOnClickListener((v) -> {
            if (zoomButtonCallback != null) zoomButtonCallback.call(-0.25f);
        });

        contentView.findViewById(R.id.BTHide).setOnClickListener((v) -> {
            if (hideButtonCallback != null) hideButtonCallback.run();
        });

        textView = contentView.findViewById(R.id.TextView);
        addView(contentView);
    }

    public void setZoomValue(float value) {
        textView.setText((int)(value * 100)+"%");
    }

    @Override
    protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
        super.onLayout(changed, left, top, right, bottom);

        if (restoreSavedPosition) {
            float x = 1e6f;
            float y = 1e6f;

            String config = preferences.getString("magnifier_view", null);
            if (config != null) {
                try {
                    String[] parts = config.split("\\|");
                    x = Short.parseShort(parts[0]);
                    y = Short.parseShort(parts[1]);
                }
                catch (NumberFormatException e) {}
            }

            movePanel(x, y);
            restoreSavedPosition = false;
        }
    }

    private void movePanel(float x, float y) {
        final int padding = (int)UnitUtils.dpToPx(8);
        ViewGroup parent = (ViewGroup)getParent();
        int width = getWidth();
        int height = getHeight();
        int parentWidth = parent.getWidth();
        int parentHeight = parent.getHeight();
        x = Mathf.clamp(x, padding, parentWidth - padding - width);
        y = Mathf.clamp(y, padding, parentHeight - padding - height);
        setX(x);
        setY(y);
        lastX = (short)x;
        lastY = (short)y;
    }

    public Callback<Float> getZoomButtonCallback() {
        return zoomButtonCallback;
    }

    public void setZoomButtonCallback(Callback<Float> zoomButtonCallback) {
        this.zoomButtonCallback = zoomButtonCallback;
    }

    public Runnable getHideButtonCallback() {
        return hideButtonCallback;
    }

    public void setHideButtonCallback(Runnable hideButtonCallback) {
        this.hideButtonCallback = hideButtonCallback;
    }
}
