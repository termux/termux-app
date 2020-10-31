package com.termux.app;

import android.content.Context;
import android.util.AttributeSet;
import android.view.Gravity;
import android.widget.GridLayout;
import android.widget.TextView;


import java.util.ArrayList;

public class StatusView extends GridLayout {

    public StatusView(Context context, AttributeSet attrs) {

        super(context, attrs);
        createStatusArea();
    }
    private ArrayList<TextView> lines;
    public void createStatusArea(){
        lines = new ArrayList<>();
    }

    public void setLineText(int line, String text){
        if(lines.size()<=line){
            for(int i=lines.size();i<=line;i++){
                TextView textView = new TextView(getContext());
                textView.setText("");
                addView(textView);
                lines.add(textView);
                GridLayout.LayoutParams params = new GridLayout.LayoutParams(textView.getLayoutParams());
                params.setGravity(Gravity.CENTER);
                textView.setLayoutParams(params);
            }
        }
        lines.get(line).setText(text);
    }
    public void deleteUntil(int line){
        while(lines.size()>line+1){
            TextView toRemove = lines.get(lines.size()-1);
            removeView(toRemove);
            lines.remove(toRemove);
        }
    }
}
