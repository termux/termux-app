package com.termux.floatball.utils;

import android.content.Context;
import android.content.res.ColorStateList;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.NinePatch;
import android.graphics.Rect;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.NinePatchDrawable;
import android.graphics.drawable.StateListDrawable;

import java.io.IOException;
import java.io.InputStream;

public class BackGroudSeletor {
    static int[] PRESSED_ENABLED_STATE_SET = {16842910, 16842919};// debug出的该变量在view中的值
    static int[] ENABLED_STATE_SET = {16842910};
    static int[] EMPTY_STATE_SET = {};
    //private static String pix = ""; // 屏幕分辨率 例如 480x800;

    private BackGroudSeletor() {
    }

    /**
     * 该方法主要是构建StateListDrawable对象，以StateListDrawable来设置图片状态，来表现View的各中状态：未选中，按下
     * ，选中效果
     *
     * @param imagename 选中和未选中使用的两张图片名称
     * @param context   上下文
     */
    public static StateListDrawable createBgByImagedrawble(String[] imagename,
                                                           Context context) {
        StateListDrawable bg = new StateListDrawable();
        Drawable normal = getdrawble(imagename[0], context);
        Drawable pressed = getdrawble(imagename[1], context);
        bg.addState(PRESSED_ENABLED_STATE_SET, pressed);
        bg.addState(ENABLED_STATE_SET, normal);
        bg.addState(EMPTY_STATE_SET, normal);
        return bg;
    }

    public static StateListDrawable createBg(Drawable normal, Drawable pressed) {
        StateListDrawable bg = new StateListDrawable();
        bg.addState(PRESSED_ENABLED_STATE_SET, pressed);
        bg.addState(ENABLED_STATE_SET, normal);
        bg.addState(EMPTY_STATE_SET, normal);
        return bg;
    }
    public static ColorStateList createColorStateList(int normal, int pressed) {
        int[] colors = new int[] { pressed, pressed, normal, pressed, pressed, normal };
        int[][] states = new int[6][];
        states[0] = new int[] { android.R.attr.state_pressed, android.R.attr.state_enabled };
        states[1] = new int[] { android.R.attr.state_enabled, android.R.attr.state_focused };
        states[2] = new int[] { android.R.attr.state_enabled };
        states[3] = new int[] { android.R.attr.state_focused };
        states[4] = new int[] { android.R.attr.state_window_focused };
        states[5] = new int[] {};
        ColorStateList colorList = new ColorStateList(states, colors);
        return colorList;
    }
    /**
     * 该方法主要是构建StateListDrawable对象，以StateListDrawable来设置图片状态，来表现View的各中状态：未选中，按下
     * ，选中效果
     *
     * @param imagename 选中和未选中使用的两张图片名称
     * @param context   上下文
     */
    public static StateListDrawable createBgByImage9png(String[] imagename,
                                                        Context context) {
        StateListDrawable bg = new StateListDrawable();
        NinePatchDrawable normal = get9png(imagename[0], context);
        NinePatchDrawable pressed = get9png(imagename[1], context);
        bg.addState(PRESSED_ENABLED_STATE_SET, pressed);
        bg.addState(ENABLED_STATE_SET, normal);
        bg.addState(EMPTY_STATE_SET, normal);
        return bg;
    }

    public static Bitmap getBitmap(String imagename, Context context) {
        Bitmap bitmap = null;
        try {
//			File file = new File("image/" + imagename  + ".png");
            String imagePath = imagename + ".png";
//			if (!file.isFile()) {
//				imagePath = "image/" + imagename + "480x800" + ".png";
//			}
            bitmap = BitmapFactory.decodeStream(context.getAssets().open(imagePath));

        } catch (IOException e) {
            if (null != bitmap) {
                bitmap.recycle();
            }
            e.printStackTrace();
        }
        return bitmap;
    }

    /**
     * 该方法主要根据图片名称获取可用的 Drawable
     *
     * @param imagename 选中和未选中使用的两张图片名称
     * @param context   上下文
     * @return 可用的Drawable
     */
    public static Drawable getdrawble(String imagename, Context context) {
        Drawable drawable = null;
        Bitmap bitmap = null;
        try {
            String imagePath = "image/" + imagename + ".png";
            bitmap = BitmapFactory.decodeStream(context.getAssets().open(imagePath));
            BitmapDrawable bitmapDrawable = new BitmapDrawable(bitmap);
            drawable = bitmapDrawable;
        } catch (IOException e) {
            if (null != bitmap) {
                bitmap.recycle();
            }
            e.printStackTrace();
        }
        return drawable;
    }


    /**
     * 获取asset下面的.9 png
     *
     * @param imagename 图片名
     * @param context   上下文对象
     */
    public static NinePatchDrawable get9png(String imagename, Context context) {
        Bitmap toast_bitmap;
        try {
            toast_bitmap = BitmapFactory.decodeStream(context.getAssets().open("image/" + imagename + ".9.png"));
            byte[] temp = toast_bitmap.getNinePatchChunk();
            boolean is_nine = NinePatch.isNinePatchChunk(temp);
            if (is_nine) {
                NinePatchDrawable nine_draw = new NinePatchDrawable(context.getResources(), toast_bitmap, temp, new Rect(), null);
                return nine_draw;
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
        return null;
    }

    public static InputStream zipPic(InputStream is) {

        return null;
    }

    /**
     * 获取pix（分辨率）
     *
     * @return pix （分辨率）
     */
//	public static String getPix() {
//		return pix;
//	}

    /**
     * 设置pix（分辨率）
     *
     * @param pix
     *            （分辨率）
     */
//	public static void setPix(String pix) {
//		BackGroudSeletor.pix = pix;
//	}

}
