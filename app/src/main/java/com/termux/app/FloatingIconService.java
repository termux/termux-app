package com.termux.app;

import android.app.Service;
import android.content.Intent;
import android.graphics.PixelFormat;
import android.os.IBinder;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.ImageView;
import android.widget.PopupMenu;
import android.view.ContextThemeWrapper;

public class FloatingIconService extends Service {
    private WindowManager windowManager;
    private ImageView floatingIcon;
    private WindowManager.LayoutParams iconParams;

    @Override
    public IBinder onBind(Intent intent) { return null; }

    @Override
    public void onCreate() {
        super.onCreate();
        windowManager = (WindowManager) getSystemService(WINDOW_SERVICE);

        // 1. Create the main floating icon
        floatingIcon = new ImageView(this);
        android.graphics.drawable.GradientDrawable shape = new android.graphics.drawable.GradientDrawable();
        shape.setShape(android.graphics.drawable.GradientDrawable.OVAL);
        shape.setColor(0xFF000000); // Black background
        shape.setStroke(4, 0xFF00FF00); // Thinner green border
        floatingIcon.setBackground(shape);
        // Use a terminal icon instead of the agenda icon
        floatingIcon.setImageResource(android.R.drawable.ic_input_add); 
        floatingIcon.setPadding(25, 25, 25, 25);
        floatingIcon.setElevation(10f);

        // Set size to a standard app icon size (around 60dp)
        int iconSize = (int) (60 * getResources().getDisplayMetrics().density);

        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.O) {
            iconParams = new WindowManager.LayoutParams(
                iconSize, iconSize,
                WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY,
                WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE,
                PixelFormat.TRANSLUCENT);
        } else {
            iconParams = new WindowManager.LayoutParams(
                iconSize, iconSize,
                WindowManager.LayoutParams.TYPE_PHONE,
                WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE,
                PixelFormat.TRANSLUCENT);
        }

        iconParams.gravity = Gravity.TOP | Gravity.LEFT;
        iconParams.x = 20;
        iconParams.y = 300;

        // 2. Handle dragging and tapping
        floatingIcon.setOnTouchListener(new View.OnTouchListener() {
            private int initialX, initialY;
            private float initialTouchX, initialTouchY;
            private boolean isDragging = false;

            @Override
            public boolean onTouch(View v, MotionEvent event) {
                switch (event.getAction()) {
                    case MotionEvent.ACTION_DOWN:
                        initialX = iconParams.x;
                        initialY = iconParams.y;
                        initialTouchX = event.getRawX();
                        initialTouchY = event.getRawY();
                        isDragging = false;
                        return true;
                    case MotionEvent.ACTION_MOVE:
                        // Only consider it a drag if moved more than 10 pixels
                        if (Math.abs(event.getRawX() - initialTouchX) > 10 || Math.abs(event.getRawY() - initialTouchY) > 10) {
                            isDragging = true;
                            iconParams.x = initialX + (int) (event.getRawX() - initialTouchX);
                            iconParams.y = initialY + (int) (event.getRawY() - initialTouchY);
                            windowManager.updateViewLayout(floatingIcon, iconParams);
                        }
                        return true;
                    case MotionEvent.ACTION_UP:
                        if (!isDragging) {
                            showPopupMenu(v);
                        }
                        return true;
                }
                return false;
            }
        });

        windowManager.addView(floatingIcon, iconParams);
    }

    // 3. The Pop-up Menu Logic
    private void showPopupMenu(View v) {
        // Use a dark theme for the popup menu
        ContextThemeWrapper wrapper = new ContextThemeWrapper(this, android.R.style.Theme_DeviceDefault_DayNight);
        PopupMenu popup = new PopupMenu(wrapper, v);
        
        // Add menu items programmatically
        popup.getMenu().add(0, 1, 0, "Maximize Termux");
        // We can't trigger stealth directly from here easily, so we'll make it a shortcut to open the app first
        // popup.getMenu().add(0, 2, 0, "Stealth Mode"); 
        popup.getMenu().add(0, 3, 0, "Close Bubble");

        popup.setOnMenuItemClickListener(item -> {
            switch (item.getItemId()) {
                case 1: // Maximize
                    Intent intent = new Intent(FloatingIconService.this, TermuxActivity.class);
                    intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_SINGLE_TOP);
                    startActivity(intent);
                    stopSelf();
                    return true;
                case 3: // Close Bubble
                    stopSelf();
                    // Force kill the app process to ensure it stops running
                    android.os.Process.killProcess(android.os.Process.myPid());
                    return true;
                default:
                    return false;
            }
        });
        popup.show();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (floatingIcon != null) windowManager.removeView(floatingIcon);
    }
}