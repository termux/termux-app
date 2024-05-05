package com.termux.display.controller.widget;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.RectF;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.PopupWindow;

import androidx.annotation.Nullable;

import com.termux.display.R;
import com.termux.display.controller.core.AppUtils;
import com.termux.display.controller.core.UnitUtils;

import java.util.Locale;

public class ColorPickerView extends View implements View.OnClickListener {
    private static final int[] colors = {0xff8f00, 0xd32f2f, 0x9575cd, 0x2e7d32, 0x00838f, 0x0277bd, 0x607d8b, 0x000000};
    private int currentColor = 0xffffff;
    private final Bitmap colorFrame;

    public ColorPickerView(Context context) {
        this(context, null);
    }

    public ColorPickerView(Context context, @Nullable AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public ColorPickerView(Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);

        colorFrame = BitmapFactory.decodeResource(context.getResources(), R.drawable.color_frame);

        setBackgroundResource(R.drawable.combo_box);
        setClickable(true);
        setFocusable(true);
        setOnClickListener(this);
    }

    public int getColor() {
        return toARGB(currentColor);
    }

    public void setColor(int color) {
        currentColor = toRGB(color);
        invalidate();
    }

    public String getColorAsString() {
        return String.format(Locale.ENGLISH, "#%06X", (0x00ffffff & currentColor));
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);

        int width = getWidth();
        int height = getHeight();
        if (width == 0 || height == 0) return;

        float rectSize = height - UnitUtils.dpToPx(12);
        float startX = (width - rectSize) * 0.5f - UnitUtils.dpToPx(16);
        float startY = (height - rectSize) * 0.5f;

        Paint paint = new Paint();
        paint.setColor(toARGB(currentColor));
        paint.setStyle(Paint.Style.FILL);
        paint.setAntiAlias(false);
        paint.setFilterBitmap(false);
        canvas.drawRect(startX, startY, startX + rectSize, startY + rectSize, paint);

        Rect srcRect = new Rect(0, 0, colorFrame.getWidth(), colorFrame.getHeight());
        RectF dstRect = new RectF(startX, startY, startX + rectSize, startY + rectSize);
        canvas.drawBitmap(colorFrame, srcRect, dstRect, paint);
    }

    public static int toARGB(int rgb) {
        return Color.argb(255, Color.red(rgb), Color.green(rgb), Color.blue(rgb));
    }

    public static int toRGB(int argb) {
        return Color.argb(0, Color.red(argb), Color.green(argb), Color.blue(argb));
    }

    @Override
    public void onClick(View anchor) {
        Context context = getContext();
        final int popupHeight = 60;
        LinearLayout container = new LinearLayout(context);
        container.setLayoutParams(new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, (int)UnitUtils.dpToPx(popupHeight)));
        container.setOrientation(LinearLayout.HORIZONTAL);
        container.setGravity(Gravity.CENTER_VERTICAL);
        container.setPadding(0, 0, (int)UnitUtils.dpToPx(4), 0);

        Bitmap colorFrameSelected = BitmapFactory.decodeResource(context.getResources(), R.drawable.color_frame_selected);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams((int)UnitUtils.dpToPx(32), (int)UnitUtils.dpToPx(32));
        params.setMargins((int)UnitUtils.dpToPx(4), 0, 0, 0);
        final PopupWindow[] popupWindow = {null};

        for (final int color : colors) {
            ImageView imageView = new ImageView(context);
            imageView.setLayoutParams(params);
            imageView.setImageBitmap(color == currentColor ? colorFrameSelected : colorFrame);
            imageView.setBackgroundColor(toARGB(color));
            imageView.setOnClickListener((v) -> {
                currentColor = color;
                invalidate();
                if (popupWindow[0] != null) popupWindow[0].dismiss();
            });
            container.addView(imageView);
        }
        popupWindow[0] = AppUtils.showPopupWindow(anchor, container, 0, popupHeight);
    }
}
