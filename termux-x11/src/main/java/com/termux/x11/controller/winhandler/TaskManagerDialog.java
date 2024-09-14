package com.termux.x11.controller.winhandler;

import android.app.ActivityManager;
import android.content.Context;
import android.os.Build;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.PopupMenu;
import android.widget.TextView;

import com.termux.x11.MainActivity;
import com.termux.x11.R;
import com.termux.x11.controller.contentdialog.ContentDialog;
import com.termux.x11.controller.core.CPUStatus;
import com.termux.x11.controller.core.ProcessHelper;
import com.termux.x11.controller.core.StringUtils;
import com.termux.x11.controller.widget.CPUListView;

import java.util.List;
import java.util.Timer;
import java.util.TimerTask;

public class TaskManagerDialog extends ContentDialog implements OnGetProcessInfoListener {
    private final MainActivity activity;
    private final LayoutInflater inflater;
    private Timer timer;
    private final Object lock = new Object();

    public TaskManagerDialog(MainActivity activity) {
        super(activity, R.layout.task_manager_dialog);
        this.activity = activity;
        setCancelable(false);
        setTitle(R.string.task_manager);
        setIcon(R.drawable.icon_task_manager);

        Button cancelButton = findViewById(R.id.BTCancel);
        cancelButton.setText(R.string.new_task);
        cancelButton.setOnClickListener((v) -> {
            dismiss();
            ContentDialog.prompt(activity, R.string.new_task, "taskmgr.exe", (command) -> activity.getWinHandler().exec(command));
        });

        setOnDismissListener((dialog) -> {
            if (timer != null) {
                timer.cancel();
                timer = null;
            }
            activity.getTermuxProcessorInfo("1");
            final LinearLayout container = findViewById(R.id.LLProcessList);
            container.removeAllViews();
            activity.getWinHandler().setOnGetProcessInfoListener(null);
        });
        inflater = LayoutInflater.from(activity);
    }

    private void update() {
        synchronized (lock) {
            activity.getWinHandler().listProcesses();

            final LinearLayout container = findViewById(R.id.LLProcessList);
            if (container.getChildCount() == 0) {
                listAndroidProcess();
                if (container.getChildCount() == 0) {
                    findViewById(R.id.TVEmptyText).setVisibility(View.VISIBLE);
                }
            }

        }

        updateCPUInfoView();
        updateMemoryInfoView();
    }

    private void listAndroidProcess() {
        List<ProcessInfo> processInfoList = activity.getTermuxProcessorInfo("0");
        if (processInfoList == null) {
            return;
        }
        int idx = 0;
        for (ProcessInfo processInfo : processInfoList) {
            onGetProcessInfo(idx, processInfoList.size(), processInfo);
            idx++;
        }
//        ActivityManager activityManager = (ActivityManager) activity.getSystemService(Context.ACTIVITY_SERVICE);
//        List<ActivityManager.RunningAppProcessInfo> runningAppProcessInfoList = activityManager.getRunningAppProcesses();
//        int idx = 0;
//        for (ActivityManager.RunningAppProcessInfo process : runningAppProcessInfoList) {
//            ProcessInfo processInfo = new ProcessInfo(process.pid, process.processName, activityManager.getProcessMemoryInfo(new int[]{process.pid})[0].getTotalPrivateDirty(), 15, true);
//            onGetProcessInfo(idx, runningAppProcessInfoList.size(), processInfo);
//            idx++;
//        }
    }

    private void showListItemMenu(final View anchorView, final ProcessInfo processInfo) {
        PopupMenu listItemMenu = new PopupMenu(activity, anchorView);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) listItemMenu.setForceShowIcon(true);

        listItemMenu.inflate(R.menu.process_popup_menu);
        listItemMenu.setOnMenuItemClickListener((menuItem) -> {
            int itemId = menuItem.getItemId();
            final WinHandler winHandler = activity.getWinHandler();
            if (itemId == R.id.process_affinity) {
                showProcessorAffinityDialog(processInfo);
            } else if (itemId == R.id.bring_to_front) {
                winHandler.bringToFront(processInfo.name);
                dismiss();
            } else if (itemId == R.id.process_end) {
                ContentDialog.confirm(activity, R.string.do_you_want_to_end_this_process, () -> {
                    winHandler.killProcess(processInfo.name);
                });
            }
            return true;
        });
        listItemMenu.show();
    }

    private void showProcessorAffinityDialog(final ProcessInfo processInfo) {
        ContentDialog dialog = new ContentDialog(activity, R.layout.cpu_list_dialog);
        dialog.setTitle(processInfo.name);
        dialog.setIcon(R.drawable.icon_cpu);
        final CPUListView cpuListView = dialog.findViewById(R.id.CPUListView);
        cpuListView.setCheckedCPUList(processInfo.getCPUList());
        dialog.setOnConfirmCallback(() -> {
            WinHandler winHandler = activity.getWinHandler();
            winHandler.setProcessAffinity(processInfo.pid, ProcessHelper.getAffinityMask(cpuListView.getCheckedCPUList()));
            update();
        });
        dialog.show();
    }

    @Override
    public void show() {
        update();
        activity.getWinHandler().setOnGetProcessInfoListener(this);

        timer = new Timer();
        timer.schedule(new TimerTask() {
            @Override
            public void run() {
                activity.runOnUiThread(TaskManagerDialog.this::update);
            }
        }, 0, 1000);
        super.show();
    }

    @Override
    public void onGetProcessInfo(int index, int numProcesses, ProcessInfo processInfo) {
        activity.runOnUiThread(() -> {
            synchronized (lock) {
                final LinearLayout container = findViewById(R.id.LLProcessList);
                setBottomBarText(activity.getString(R.string.processes) + ": " + numProcesses);

                if (numProcesses == 0) {
                    container.removeAllViews();
                    findViewById(R.id.TVEmptyText).setVisibility(View.VISIBLE);
                    return;
                }

                findViewById(R.id.TVEmptyText).setVisibility(View.GONE);

                int childCount = container.getChildCount();
                View itemView = index < childCount ? container.getChildAt(index) : inflater.inflate(R.layout.process_info_list_item, container, false);
                ((TextView) itemView.findViewById(R.id.TVName)).setText(processInfo.name + (processInfo.wow64Process ? " *32" : ""));
                ((TextView) itemView.findViewById(R.id.TVPID)).setText(String.valueOf(processInfo.pid));
                ((TextView) itemView.findViewById(R.id.TVMemoryUsage)).setText(processInfo.getFormattedMemoryUsage());
                itemView.findViewById(R.id.BTMenu).setOnClickListener((v) -> showListItemMenu(v, processInfo));

                ImageView ivIcon = itemView.findViewById(R.id.IVIcon);
                if (ivIcon != null)
                    ivIcon.setImageResource(R.drawable.taskmgr_process);

                if (index >= childCount) container.addView(itemView);

                if (index == numProcesses - 1 && childCount > numProcesses) {
                    for (int i = childCount - 1; i >= numProcesses; i--) container.removeViewAt(i);
                }
            }
        });
    }

    private void updateCPUInfoView() {
        LinearLayout llCPUInfo = findViewById(R.id.LLCPUInfo);
        llCPUInfo.removeAllViews();
        short[] clockSpeeds = CPUStatus.getCurrentClockSpeeds();
        int totalClockSpeed = 0;
        short maxClockSpeed = 0;

        for (int i = 0; i < clockSpeeds.length; i++) {
            TextView textView = new TextView(activity);
            textView.setTextSize(TypedValue.COMPLEX_UNIT_DIP, 14);
            short clockSpeed = CPUStatus.getMaxClockSpeed(i);
            textView.setText(clockSpeeds[i] + "/" + clockSpeed + " MHz");
            llCPUInfo.addView(textView);
            totalClockSpeed += clockSpeeds[i];
            maxClockSpeed = (short) Math.max(maxClockSpeed, clockSpeed);
        }

        int avgClockSpeed = totalClockSpeed / clockSpeeds.length;
        TextView tvCPUTitle = findViewById(R.id.TVCPUTitle);
        byte cpuUsagePercent = (byte) (((float) avgClockSpeed / maxClockSpeed) * 100.0f);
        tvCPUTitle.setText("CPU (" + cpuUsagePercent + "%)");
    }

    private void updateMemoryInfoView() {
        ActivityManager activityManager = (ActivityManager) activity.getSystemService(Context.ACTIVITY_SERVICE);
        ActivityManager.MemoryInfo memoryInfo = new ActivityManager.MemoryInfo();
        activityManager.getMemoryInfo(memoryInfo);
        long usedMem = memoryInfo.totalMem - memoryInfo.availMem;
        byte memUsagePercent = (byte) (((double) usedMem / memoryInfo.totalMem) * 100.0f);

        TextView tvMemoryTitle = findViewById(R.id.TVMemoryTitle);
        tvMemoryTitle.setText(activity.getString(R.string.memory) + " (" + memUsagePercent + "%)");

        TextView tvMemoryInfo = findViewById(R.id.TVMemoryInfo);
        tvMemoryInfo.setText(StringUtils.formatBytes(usedMem, false) + "/" + StringUtils.formatBytes(memoryInfo.totalMem));
    }
}
