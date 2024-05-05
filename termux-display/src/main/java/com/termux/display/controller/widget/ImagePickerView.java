package com.termux.display.controller.widget;

import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.RectF;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ImageView;
import android.widget.PopupWindow;

import androidx.annotation.Nullable;

import com.termux.display.R;
import com.termux.display.controller.core.AppUtils;
import com.termux.display.controller.core.FileUtils;
import com.termux.display.controller.core.ImageUtils;
import com.termux.display.controller.core.UnitUtils;
import com.termux.display.controller.core.WineThemeManager;

import java.io.File;
import com.termux.display.MainActivity;
public class ImagePickerView extends View implements View.OnClickListener {
    private final Bitmap icon;

    public ImagePickerView(Context context) {
        this(context, null);
    }

    public ImagePickerView(Context context, @Nullable AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public ImagePickerView(Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);

        icon = BitmapFactory.decodeResource(context.getResources(), R.drawable.icon_image_picker);

        setBackgroundResource(R.drawable.combo_box);
        setClickable(true);
        setFocusable(true);
        setOnClickListener(this);
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

        Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
        Rect srcRect = new Rect(0, 0, icon.getWidth(), icon.getHeight());
        RectF dstRect = new RectF(startX, startY, startX + rectSize, startY + rectSize);
        canvas.drawBitmap(icon, srcRect, dstRect, paint);
    }

    @Override
    public void onClick(View anchor) {
        final Context context = getContext();
        final File userWallpaperFile = WineThemeManager.getUserWallpaperFile(context);

        View view = LayoutInflater.from(context).inflate(R.layout.image_picker_view, null);
        ImageView imageView = view.findViewById(R.id.ImageView);

        if (userWallpaperFile.isFile()) {
            imageView.setImageBitmap(BitmapFactory.decodeFile(userWallpaperFile.getPath()));
        }
        else imageView.setImageResource(R.drawable.wallpaper);

        final PopupWindow[] popupWindow = {null};
        View browseButton = view.findViewById(R.id.BTBrowse);
        browseButton.setOnClickListener((v) -> {
            MainActivity activity = (MainActivity)context;
            Intent intent = new Intent(Intent.ACTION_PICK);
            intent.setType("image/*");
            activity.setOpenFileCallback((data) -> {
                Bitmap bitmap = ImageUtils.getBitmapFromUri(context, data, 1280);
                if (bitmap == null) return;

                ImageUtils.save(bitmap, userWallpaperFile, Bitmap.CompressFormat.PNG, 100);
                popupWindow[0].dismiss();
            });
            activity.startActivityForResult(intent, MainActivity.OPEN_FILE_REQUEST_CODE);
        });

        View removeButton = view.findViewById(R.id.BTRemove);
        if (userWallpaperFile.isFile()) {
            removeButton.setVisibility(View.VISIBLE);
            removeButton.setOnClickListener((v) -> {
                FileUtils.delete(userWallpaperFile);
                popupWindow[0].dismiss();
            });
        }

        popupWindow[0] = AppUtils.showPopupWindow(anchor, view, 200, 240);
    }
}
