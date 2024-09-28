package com.termux.x11.controller.widget;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.res.TypedArray;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.RectF;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.HorizontalScrollView;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.PopupWindow;

import androidx.annotation.Nullable;

import com.termux.x11.R;
import com.termux.x11.controller.ControlsEditorActivity;
import com.termux.x11.controller.core.AppUtils;
import com.termux.x11.controller.core.FileUtils;
import com.termux.x11.controller.core.ImageUtils;
import com.termux.x11.controller.core.UnitUtils;
import com.termux.x11.controller.core.WineThemeManager;

import java.io.File;
import java.io.FileInputStream;
import java.io.FilenameFilter;
import java.io.IOException;
import java.io.InputStream;
import java.util.Arrays;

import com.termux.x11.MainActivity;

public class ImagePickerView extends View implements View.OnClickListener {
    private final Bitmap icon;
    private int activityType;
    private String imageId;
    private String buttonIconDirectoryPath;

    public void setActivityType(int activityType) {
        this.activityType = activityType;
    }

    public void setImageId(String imageId) {
        this.imageId = imageId;
    }

    public String getImageId() {
        return this.imageId;
    }

    public ImagePickerView(Context context) {
        this(context, null);
    }

    public ImagePickerView(Context context, @Nullable AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public ImagePickerView(Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        TypedArray typedArray = getContext().obtainStyledAttributes(attrs, R.styleable.ImagePickerView);
        int n = typedArray.getIndexCount();
        for (int i = 0; i < n; i++) {
            int attr = typedArray.getIndex(i);
            if (attr == R.styleable.ImagePickerView_activityTypeCode) {
                activityType = (int) typedArray.getInt(attr, MainActivity.OPEN_FILE_REQUEST_CODE);
                break;
            }
        }
        typedArray.recycle();
        icon = BitmapFactory.decodeResource(context.getResources(), R.drawable.icon_image_picker);

        setBackgroundResource(R.drawable.combo_box);
        setClickable(true);
        setFocusable(true);
        setOnClickListener(this);
        if (activityType == getResources().getInteger(R.integer.load_button_icon_code)) {
            buttonIconDirectoryPath = getContext().getFilesDir().getPath() + "/home/.buttonIcons";
        } else {
            buttonIconDirectoryPath = getContext().getFilesDir().getPath() + "/usr/glibc";
        }
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
        if (activityType == getResources().getInteger(R.integer.load_button_icon_code)) {
            setButtonIcon(anchor);
            createButtonImageList(anchor);
        } else {
            setWineWallPaper(anchor);
        }
    }

    private void setButtonIcon(View anchor) {
        final Context context = getContext();
        final File buttonIconFile = new File(buttonIconDirectoryPath, getImageId() + ".png");

        View view = LayoutInflater.from(context).inflate(R.layout.image_picker_view, null);
        ImageView imageView = view.findViewById(R.id.ImageView);

        if (buttonIconFile.isFile()) {
            imageView.setImageBitmap(BitmapFactory.decodeFile(buttonIconFile.getPath()));
        } else imageView.setImageResource(R.drawable.ic_x11_icon);

        final PopupWindow[] popupWindow = {null};
        View browseButton = view.findViewById(R.id.BTBrowse);
        browseButton.setOnClickListener((v) -> {
            ControlsEditorActivity activity = (ControlsEditorActivity) context;
            Intent intent = new Intent(Intent.ACTION_PICK);
            intent.setType("image/*");
            activity.setOpenFileCallback((data) -> {
                Bitmap bitmap = ImageUtils.getBitmapFromUri(context, data, 1280);
                if (bitmap == null) return null;
                String md5 = ImageUtils.getFileMD5(getContext(), data);
                File iconFile = new File(buttonIconDirectoryPath, md5 + ".png");
                setImageId(md5);
                if (!iconFile.exists()) {
                    File saveFile = saveButtonIcon(md5);
                    ImageUtils.save(bitmap, saveFile, Bitmap.CompressFormat.PNG, 100);
                }
                imageView.setImageBitmap(BitmapFactory.decodeFile(iconFile.getPath()));
                popupWindow[0].dismiss();
                return md5;
            });
            activity.startActivityForResult(intent, activityType);
        });

        View removeButton = view.findViewById(R.id.BTRemove);
        if (buttonIconFile.isFile()) {
            removeButton.setVisibility(View.VISIBLE);
            removeButton.setOnClickListener((v) -> {
                popupWindow[0].dismiss();
                ControlsEditorActivity activity = (ControlsEditorActivity) context;
                boolean delTag = activity.unLoadCustomIcon();
                if (delTag) {
                    FileUtils.delete(buttonIconFile);
                }
            });
        }

        popupWindow[0] = AppUtils.showPopupWindow(anchor, view, 200, 240);
    }

    private void setWineWallPaper(View anchor) {
        final Context context = getContext();
        final File userWallpaperFile = WineThemeManager.getUserWallpaperFile(context);

        View view = LayoutInflater.from(context).inflate(R.layout.image_picker_view, null);
        ImageView imageView = view.findViewById(R.id.ImageView);

        if (userWallpaperFile.isFile()) {
            imageView.setImageBitmap(BitmapFactory.decodeFile(userWallpaperFile.getPath()));
        } else imageView.setImageResource(R.drawable.wallpaper);

        final PopupWindow[] popupWindow = {null};
        View browseButton = view.findViewById(R.id.BTBrowse);
        browseButton.setOnClickListener((v) -> {
            MainActivity activity = (MainActivity) context;
            Intent intent = new Intent(Intent.ACTION_PICK);
            intent.setType("image/*");
            activity.setOpenFileCallback((data) -> {
                Bitmap bitmap = ImageUtils.getBitmapFromUri(context, data, 1280);
                if (bitmap == null) return;

                ImageUtils.save(bitmap, userWallpaperFile, Bitmap.CompressFormat.PNG, 100);
                popupWindow[0].dismiss();
            });
            activity.startActivityForResult(intent, activityType);
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

    public void createButtonImageList(View anchor) {
        Context context = getContext();
        final int popupHeight = 60;
        HorizontalScrollView horizontalScrollView = new HorizontalScrollView(context);
        LinearLayout container = new LinearLayout(context);
        container.setLayoutParams(new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, (int) UnitUtils.dpToPx(popupHeight)));
        container.setOrientation(LinearLayout.HORIZONTAL);
        container.setGravity(Gravity.CENTER_VERTICAL);
        container.setPadding(0, 0, (int) UnitUtils.dpToPx(4), 0);

        Bitmap colorFrameSelected = BitmapFactory.decodeResource(context.getResources(), R.drawable.color_frame_selected);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams((int) UnitUtils.dpToPx(32), (int) UnitUtils.dpToPx(32));
        params.setMargins((int) UnitUtils.dpToPx(4), 0, 0, 0);
        final PopupWindow[] popupWindow = {null,};

        ImageView delete = new ImageView(context);
        delete.setLayoutParams(params);
        String[] filenames = null;
        String prefix = null;
        try (InputStream is = context.getAssets().open("inputcontrols/icons/" + "0.png")) {
            delete.setImageBitmap(BitmapFactory.decodeStream(is));
            File buttonIconsDir = new File(context.getFilesDir(), "home/.buttonIcons");
            prefix = buttonIconsDir.getPath();
            filenames = buttonIconsDir.list(new FilenameFilter() {
                @Override
                public boolean accept(File dir, String name) {
                    return name.toLowerCase().endsWith(".png");
                }
            });
        } catch (IOException e) {
        }
        delete.setOnClickListener((v) -> {
            ControlsEditorActivity activity = (ControlsEditorActivity) context;
            activity.setCustomButtonIcon(null);
            invalidate();
            if (popupWindow[0] != null) popupWindow[0].dismiss();
        });
        container.addView(delete);
        if (filenames == null) {
            return;
        }
        for (final String name : filenames) {
            ImageView imageView = new ImageView(context);
            imageView.setLayoutParams(params);
            imageView.setImageBitmap(BitmapFactory.decodeFile(prefix + "/" + name));
            imageView.setOnClickListener((v) -> {
                ControlsEditorActivity activity = (ControlsEditorActivity) context;
                String baseName = name.replace(".png", "");
                activity.setCustomButtonIcon(baseName);
                invalidate();
                if (popupWindow[0] != null) popupWindow[0].dismiss();
            });
            container.addView(imageView);
        }
        horizontalScrollView.addView(container);
        popupWindow[0] = AppUtils.showPopupWindow(anchor, horizontalScrollView, 0, popupHeight);
    }

    private File saveButtonIcon(String iconId) {
        File iconFile = new File(buttonIconDirectoryPath, iconId + ".png");
        if (!iconFile.exists()) {
            try {
                File directory = iconFile.getParentFile();
                if (!directory.exists()) {
                    directory.mkdirs();
                }
                iconFile.createNewFile();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        return iconFile;
    }
}
