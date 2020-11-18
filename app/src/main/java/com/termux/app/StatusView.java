package com.termux.app;

import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.graphics.Typeface;
import android.util.AttributeSet;
import android.view.Gravity;
import android.widget.GridLayout;
import android.widget.TextView;


import java.io.File;
import java.util.ArrayList;

public class StatusView extends GridLayout {

    public StatusView(Context context, AttributeSet attrs) {

        super(context, attrs);
        createStatusArea();
    }
    private ArrayList<TextView> lines;
    private TermuxPreferences settings;

    public void setSettings(TermuxPreferences settings){
        this.settings = settings;
        int line = 0;
        String content = getLineCache(line);
        while(!content.equals("[empty]")){
            setLineText(line,content);
            line++;
            content = getLineCache(line);
        }
    }
    public void createStatusArea(){
        lines = new ArrayList<>();
    }

    private String getLineCache(int line){
        SharedPreferences sharedPref = getContext().getSharedPreferences("lineCache",Context.MODE_PRIVATE);
        return sharedPref.getString("lineCache"+line,"[empty]");
    }
    private void setLineCache(int line, String content){
        SharedPreferences sharedPref = getContext().getSharedPreferences("lineCache",Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = sharedPref.edit();
        editor.putString("lineCache"+line, content);
        editor.apply();
    }
    private void deleteLineCache(int line){
        SharedPreferences sharedPref = getContext().getSharedPreferences("lineCache",Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = sharedPref.edit();
        editor.remove("lineCache"+line);
        editor.apply();
    }
    public void setLineText(int line, String text){
        float size = 12;
        String colorStr = "#c0b18b";
        if(settings != null){
            size = settings.getStatusTextSize();
            colorStr = settings.getStatusTextColor();
        }
       int color = Color.parseColor(colorStr);
        if(lines.size()<=line){
            for(int i=lines.size();i<=line;i++){
                TextView textView = new TextView(getContext());
                File fontFile = new File(TermuxService.HOME_PATH+"/.termux/font.ttf");
                if(fontFile.exists()){
                    Typeface font = Typeface.createFromFile(fontFile);
                    textView.setTypeface(font);
                }
                textView.setText("");
                textView.setTextColor(color);
                textView.setTextSize(size);
                addView(textView);
                lines.add(textView);
                GridLayout.LayoutParams params = new GridLayout.LayoutParams(textView.getLayoutParams());
                params.setGravity(Gravity.CENTER);
                textView.setLayoutParams(params);
            }
        }
        lines.get(line).setText(text);
        setLineCache(line,text);
    }
    public void deleteUntil(int line){
        while(lines.size()>line+1){
            TextView toRemove = lines.get(lines.size()-1);
            removeView(toRemove);
            deleteLineCache(lines.size()-1);
            lines.remove(toRemove);

        }
    }
}
