package com.termux.app.terminal;

import static com.termux.shared.termux.TermuxConstants.TERMUX_BIN_PREFIX_DIR_PATH;

import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.Spinner;
import android.widget.Toast;

import com.termux.R;
import com.termux.app.TermuxActivity;

import java.io.File;
import java.util.ArrayList;

public class StartEntryClient implements FileBrowser.FileSlectedAdapter{
    private TermuxActivity mTermuxActivity;
    private TermuxTerminalSessionActivityClient mTermuxTerminalSessionActivityClient;
    private LinearLayout mStartItemEntriesConfig;
    private Button mLaunchButton;
    private ImageButton mSelectConfigButton;
    private ImageButton mAddConfigButton;
    private ImageButton mRemoveConfigButton;
    private Spinner mLaunchItemSpinner;
    private ArrayList<StartEntry.Entry> mEntries;
    private StartEntryArrayAdapter mAdapter;
    private int mCurrentCommand;
    private FileBrowser mFileBrowser;


    private StartEntryClient() {
    }

    public StartEntryClient(TermuxActivity activity, TermuxTerminalSessionActivityClient termuxTerminalSessionActivityClient) {
        this.mTermuxActivity = activity;
        this.mTermuxTerminalSessionActivityClient = termuxTerminalSessionActivityClient;
    }

    public void init() {
        StartEntry.loadStartItems();
        mStartItemEntriesConfig = mTermuxActivity.findViewById(com.termux.x11.R.id.LConfigStartItems);
        mLaunchButton = mTermuxActivity.findViewById(com.termux.x11.R.id.BLaunch_item);
        mSelectConfigButton = mTermuxActivity.findViewById(com.termux.x11.R.id.BAdd_item);
        mAddConfigButton = mTermuxActivity.findViewById(com.termux.x11.R.id.BConfig_item);
        mRemoveConfigButton = mTermuxActivity.findViewById(com.termux.x11.R.id.BRemove_item);
        mLaunchItemSpinner = mTermuxActivity.findViewById(com.termux.x11.R.id.SLaunchItemList);
        mLaunchButton.setVisibility(View.VISIBLE);
        mStartItemEntriesConfig.setVisibility(View.GONE);
        mCurrentCommand = StartEntry.getCurrentStartItemIdx();
        mEntries = StartEntry.getStartItemList();
        if (mCurrentCommand < StartEntry.getStartItemList().size()) {
            mLaunchButton.setText(StartEntry.getStartItemList().get(mCurrentCommand).getFileName());
        }

        mAdapter = new StartEntryArrayAdapter(mTermuxActivity, StartEntry.getStartItemList());
        mAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mLaunchItemSpinner.setAdapter(mAdapter);
        mLaunchItemSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                mLaunchButton.setText(mAdapter.getItem(position).getFileName());
                mCurrentCommand = position;
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                mLaunchButton.setText(mTermuxActivity.getResources().getString(com.termux.x11.R.string.launch_button_text));
            }
        });

        mLaunchButton.setOnClickListener((v) -> {
            if(StartEntry.getStartItemList().isEmpty()){
                Toast.makeText(mTermuxActivity, mTermuxActivity.getResources().getString(R.string.select_command_first), Toast.LENGTH_SHORT).show();
                mLaunchButton.setVisibility(View.GONE);
                mStartItemEntriesConfig.setVisibility(View.VISIBLE);
                return;
            }
            if (mCurrentCommand >= StartEntry.getStartItemList().size()) {
                Toast.makeText(mTermuxActivity, mTermuxActivity.getResources().getString(R.string.no_such_file) + ": " + "empty command", Toast.LENGTH_SHORT).show();
                return;
            }
            File file = new File(StartEntry.getStartItemList().get(mCurrentCommand).getPath());
            if (!file.exists()) {
                Toast.makeText(mTermuxActivity, mTermuxActivity.getResources().getString(R.string.no_such_file) + ": " + StartEntry.getStartItemList().get(mCurrentCommand).getFileName(), Toast.LENGTH_SHORT).show();
                return;
            }
            if (!file.canExecute()) {
                Toast.makeText(mTermuxActivity, mTermuxActivity.getResources().getString(R.string.not_executable) + ": " + StartEntry.getStartItemList().get(mCurrentCommand).getFileName(), Toast.LENGTH_SHORT).show();
                return;
            }
            String command = "^c";
            if (StartEntry.getStartItemList().get(mCurrentCommand).getPath().contains(TERMUX_BIN_PREFIX_DIR_PATH)) {
                command = StartEntry.getStartItemList().get(mCurrentCommand).getFileName() + "\n";
            } else {
                command = StartEntry.getStartItemList().get(mCurrentCommand).getPath() + "\n";
            }

            mTermuxTerminalSessionActivityClient.getCurrentStoredSessionOrLast().write(command);
        });
        mLaunchButton.setOnLongClickListener(l -> {
            mLaunchButton.setVisibility(View.GONE);
            mStartItemEntriesConfig.setVisibility(View.VISIBLE);
            return true;
        });
        mSelectConfigButton.setOnClickListener((v) -> {
            if (mLaunchButton.getVisibility() == View.VISIBLE) {
                mLaunchButton.setVisibility(View.GONE);
                mStartItemEntriesConfig.setVisibility(View.VISIBLE);
            } else {
                mLaunchButton.setVisibility(View.VISIBLE);
                mStartItemEntriesConfig.setVisibility(View.GONE);
                StartEntry.setCurrentStartItemIdx(mCurrentCommand);
                StartEntry.saveStartItems();
            }
        });

        mFileBrowser = new FileBrowser(mTermuxActivity, this);
        mFileBrowser.init();
        mAddConfigButton.setOnClickListener(v -> {
            mFileBrowser.showFileBrowser(mStartItemEntriesConfig);
        });
        mRemoveConfigButton.setOnClickListener(v -> {
            if(StartEntry.getStartItemList().isEmpty()){
                return;
            }
            StartEntry.deleteStartEntry(StartEntry.getStartItemList().get(mCurrentCommand));
            mAdapter.notifyDataSetChanged();
            StartEntry.saveStartItems();
        });
    }

    public void addStartEntry(FileInfo fileInfo) {
        StartEntry.Entry entry = new StartEntry.Entry();
        entry.setIconPath("default");
        entry.setFileName(fileInfo.getName());
        entry.setPath(fileInfo.getPath());
        boolean added = StartEntry.addStartEntry(entry);
        StartEntry.setCurrentStartItemIdx(mCurrentCommand);
        StartEntry.saveStartItems();
        if (added) {
            mAdapter.notifyDataSetChanged();
            mLaunchItemSpinner.setSelection(mAdapter.getCount() - 1);
        }
    }

    @Override
    public void onFileSelected(FileInfo fileInfo) {
        addStartEntry(fileInfo);
        mFileBrowser.hideFileBrowser();
    }
}
