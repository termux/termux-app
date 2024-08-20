package com.termux.x11.controller.widget;

import android.content.Context;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.CheckBox;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.annotation.Nullable;

import com.termux.x11.R;

import java.util.Arrays;
import java.util.List;

public class CPUListView extends LinearLayout {
    private List<String> checkedCPUList;
    private final byte numProcessors;

    public CPUListView(Context context) {
        this(context, null);
    }

    public CPUListView(Context context, @Nullable AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public CPUListView(Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        setOrientation(HORIZONTAL);
        numProcessors = (byte)Runtime.getRuntime().availableProcessors();
        refreshContent();
    }

    private void refreshContent() {
        removeAllViews();
        LayoutInflater inflater = LayoutInflater.from(getContext());

        for (int i = 0; i < numProcessors; i++) {
            View itemView = inflater.inflate(R.layout.cpu_list_item, this, false);
            String tag = "CPU"+i;
            CheckBox checkBox = itemView.findViewById(R.id.CheckBox);
            checkBox.setTag(tag);
            checkBox.setChecked(checkedCPUList != null ? checkedCPUList.contains(String.valueOf(i)) : i > 0);

            ((TextView)itemView.findViewById(R.id.TextView)).setText(tag);
            addView(itemView);
        }
    }

    public void setCheckedCPUList(String checkedCPUList) {
        this.checkedCPUList = Arrays.asList(checkedCPUList.split(","));
        refreshContent();
    }

    public String getCheckedCPUListAsString() {
        String cpuList = "";

        for (int i = 0; i < numProcessors; i++) {
            CheckBox checkBox = findViewWithTag("CPU"+i);
            if (checkBox.isChecked()) cpuList += (!cpuList.isEmpty() ? "," : "")+i;
        }
        return cpuList;
    }

    public boolean[] getCheckedCPUList() {
        boolean[] cpuList = new boolean[numProcessors];
        for (int i = 0; i < numProcessors; i++) {
            CheckBox checkBox = findViewWithTag("CPU"+i);
            cpuList[i] = checkBox.isChecked();
        }
        return cpuList;
    }
}
