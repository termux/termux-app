package com.termux.app;

import android.content.Context;
import android.content.Intent;
import android.graphics.drawable.Drawable;
import android.util.Log;

public class SuggetionBarAppButton implements SuggestionBarButton{
        private String text;
        private Drawable icon;
        private boolean hasIcon;
        private String packageName;
        private Context context;
        private int ratio = 0;

    public SuggetionBarAppButton(Context context, String packageName, String appName, Drawable icon) {
        hasIcon = true;
        this.text = appName;
        this.icon = icon;
        this.packageName = packageName;
        this.context = context;
    }

    public SuggetionBarAppButton(Context context, String packageName, String appName) {
        hasIcon = false;
        this.text = appName;
        this.packageName = packageName;
        this.context = context;
    }
    public int getRatio(){return ratio;}
    public void setRatio(int ratio){this.ratio=ratio;}
    public String getText() {
        return text;
    }
    public Drawable getIcon() {
        return icon;
    }
    public Boolean hasIcon(){
        return hasIcon;
    }
    public void click(){
        Intent launchIntent = context.getPackageManager().getLaunchIntentForPackage(packageName);
        if (launchIntent != null) {
            context.startActivity(launchIntent);
        }
    }

}
