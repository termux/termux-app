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
    public interface FileSlectedAdapter{
        public void onFileSelected(FileInfo fileInfo);
    }
    private View mFileBrowserView;
    private TextView mSearchEditText;
    private RecyclerView mRecyclerView;
    private FileAdapter mFileAdapter;
    private ArrayList<FileInfo> mFileList;
    private TermuxActivity mTermuxActivity;
    private PopupWindow mPopupWindow;
    private int popWindowHeight, mPopWindowWidth;
    private String mCurrentPath = TERMUX_FILES_DIR_PATH;
    private FileSlectedAdapter fileSlectedAdapter;

    private FileBrowser() {
    }

    public FileBrowser(TermuxActivity activity, FileSlectedAdapter ada) {
        this.mTermuxActivity = activity;
        this.fileSlectedAdapter = ada;
        mPopWindowWidth = 210;
    }

    public void init() {
        mFileBrowserView = LayoutInflater.from(mTermuxActivity).inflate(R.layout.file_bowser, null);
        mRecyclerView = mFileBrowserView.findViewById(R.id.recyclerView);
        mSearchEditText = mFileBrowserView.findViewById(R.id.ESearchEditText);
        mFileList = new ArrayList<>();

        mFileAdapter = new FileAdapter(mFileList, this);
        mRecyclerView.setAdapter(mFileAdapter);
        mRecyclerView.setLayoutManager(new LinearLayoutManager(mTermuxActivity));

        mSearchEditText.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {
            }

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
            }

            @Override
            public void afterTextChanged(Editable s) {
                String keyword = s.toString();
                if (s.length() < 1) {
                    return;
                }
                mFileAdapter.filter(keyword, mCurrentPath);
            }
        });

        loadFiles(TERMUX_FILES_DIR_PATH);
    }

    private void loadFiles(String path) {
        List<FileInfo> fileList = getFileList(path);
        this.mFileList.clear();
        this.mFileList.addAll(fileList);
        mFileAdapter.notifyDataSetChanged();
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
        if (mPopupWindow == null) {
            popWindowHeight = Math.min(AppUtils.getScreenWidth(), AppUtils.getScreenHeight());
            popWindowHeight = (int) UnitUtils.pxToDp(popWindowHeight * 4 / 5);
            mPopupWindow = AppUtils.showPopupWindow(view, mFileBrowserView, mPopWindowWidth, popWindowHeight);
        }
        mPopupWindow.showAsDropDown(view);
    }

    public void hideFileBrowser() {
        if (mPopupWindow == null || !mPopupWindow.isShowing()) {
            return;
        }
        mPopupWindow.dismiss();
    }

    @Override
    public void onItemClick(FileInfo fileInfo) {
        if (fileInfo.getName().equals("..")) {
//            Log.d("FileBrowser.onItemClick", ".. : " + fileInfo.getPath());
            if (fileInfo.getPath().equals(TERMUX_FILES_DIR_PATH)) {
                return;
            }
            int pathIdx = fileInfo.getPath().lastIndexOf("/");
            String path = fileInfo.getPath().substring(0, pathIdx);
//            Log.d("FileBrowser.onItemClick", "path: " + path);
            loadFiles(path);
            mCurrentPath = path;
        } else if (fileInfo.isDirectory()) {
//            Log.d("FileBrowser.onItemClick", "dir : " + fileInfo.getPath());
            loadFiles(fileInfo.getPath());
            mCurrentPath = fileInfo.getPath();
        } else {
//            Log.d("FileBrowser.onItemClick", fileInfo.getName() + ": " + fileInfo.getPath());
            File currentFile = new File(fileInfo.getPath());
            if (!currentFile.canExecute()) {
                Toast.makeText(mTermuxActivity, fileInfo.getName() + "is not executable file", Toast.LENGTH_LONG).show();
                return;
            }
            mCurrentPath = fileInfo.getPath();
            if (this.fileSlectedAdapter!=null){
                fileSlectedAdapter.onFileSelected(fileInfo);
            }
        }
    }
}
