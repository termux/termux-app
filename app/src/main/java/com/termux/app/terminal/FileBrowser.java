package com.termux.app.terminal;

import static com.termux.shared.termux.TermuxConstants.TERMUX_FILES_DIR_PATH;

import android.text.Editable;
import android.text.TextWatcher;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.PopupWindow;
import android.widget.TextView;
import android.widget.Toast;

import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.termux.R;
import com.termux.app.TermuxActivity;
import com.termux.x11.controller.core.AppUtils;
import com.termux.x11.controller.core.UnitUtils;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

public class FileBrowser implements FileAdapter.OnItemClickListener {
    private View fileBrowserView;
    private TextView searchEditText;
    private RecyclerView recyclerView;
    private FileAdapter fileAdapter;
    private ArrayList<FileInfo> fileList;
    private TermuxActivity termuxActivity;
    private PopupWindow popupWindow;
    private StartEntryClient startEntryClient;
    private int popWindowHeight, popWindowWidth;
    private String currentPath = TERMUX_FILES_DIR_PATH;

    private FileBrowser() {
    }

    public FileBrowser(TermuxActivity activity, StartEntryClient entryClien) {
        this.termuxActivity = activity;
        this.startEntryClient = entryClien;
        popWindowWidth = 210;
    }

    public void init() {
        fileBrowserView = LayoutInflater.from(termuxActivity).inflate(R.layout.file_bowser, null);
        recyclerView = fileBrowserView.findViewById(R.id.recyclerView);
        searchEditText = fileBrowserView.findViewById(R.id.ESearchEditText);
        fileList = new ArrayList<>();

        fileAdapter = new FileAdapter(fileList, this);
        recyclerView.setAdapter(fileAdapter);
        recyclerView.setLayoutManager(new LinearLayoutManager(termuxActivity));

        searchEditText.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {
            }

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
            }

            @Override
            public void afterTextChanged(Editable s) {
                String keyword = s.toString();
                if (s.length()<1){
                    return;
                }
                fileAdapter.filter(keyword,currentPath);
            }
        });

        loadFiles(TERMUX_FILES_DIR_PATH);
    }

    private void loadFiles(String path) {
        List<FileInfo> fileList = getFileList(path);
        this.fileList.clear();
        this.fileList.addAll(fileList);
        fileAdapter.notifyDataSetChanged();
    }

    private List<FileInfo> getFileList(String path) {
        List<FileInfo> fileInfoList = new ArrayList<>();
        File parent = new File(path);
        File[] files = parent.listFiles();
        FileInfo parentInfo = new FileInfo("..", parent.getAbsolutePath(), true);
        fileInfoList.add(parentInfo);
        for (int i = 0; i < files.length; i++) {
            FileInfo f = new FileInfo(files[i].getName(), files[i].getAbsolutePath(), files[i].isDirectory());
            fileInfoList.add(f);
        }
        return fileInfoList;
    }

    public void showFileBrowser(View view) {
        if (popupWindow == null) {
            popWindowHeight = Math.min(AppUtils.getScreenWidth(), AppUtils.getScreenHeight());
            popWindowHeight = (int) UnitUtils.pxToDp(popWindowHeight * 4 / 5);
            popupWindow = AppUtils.showPopupWindow(view, fileBrowserView, popWindowWidth, popWindowHeight);
        }
        popupWindow.showAsDropDown(view);
    }

    public void hideFileBrowser() {
        if (popupWindow == null || !popupWindow.isShowing()) {
            return;
        }
        popupWindow.dismiss();
    }

    @Override
    public void onItemClick(FileInfo fileInfo) {
        if (fileInfo.getName().equals("..")) {
            if (fileInfo.getPath().equals(TERMUX_FILES_DIR_PATH)) {
                return;
            }
            int pathIdx = fileInfo.getPath().lastIndexOf("/");
            String path = fileInfo.getPath().substring(0, pathIdx);
            loadFiles(path);
            currentPath = path;
        } else if (fileInfo.isDirectory()) {
            loadFiles(fileInfo.getPath());
            currentPath = fileInfo.getPath();
        } else {
            File currentFile = new File(fileInfo.getPath());
            if (!currentFile.canExecute()) {
                Toast.makeText(termuxActivity, fileInfo.getName() + "is not executable file", Toast.LENGTH_LONG).show();
                return;
            }
            currentPath = fileInfo.getPath();
            startEntryClient.addStartEntry(fileInfo);
        }
    }
}
