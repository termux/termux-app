package com.termux.filepicker;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.ListActivity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.view.MenuItem;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.ListView;

import com.termux.R;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

/** Activity allowing picking files from the $HOME folder. */
public class TermuxFilePickerActivity extends ListActivity {

    @SuppressLint("SdCardPath")
    final String TERMUX_HOME = "/data/data/com.termux/files/home";

    private File mCurrentDirectory;
    private final List<File> mFiles = new ArrayList<>();
    private final List<String> mFileNames = new ArrayList<>();
    private ArrayAdapter mAdapter;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.file_picker);

        mAdapter = new ArrayAdapter<>(this, android.R.layout.simple_list_item_1, mFileNames);

        enterDirectory(new File(TERMUX_HOME));
        setListAdapter(mAdapter);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        int id = item.getItemId();
        if (id == android.R.id.home) {
            enterDirectory(mCurrentDirectory.getParentFile());
            return true;
        } else {
            return super.onOptionsItemSelected(item);
        }
    }

    @Override
    protected void onListItemClick(ListView l, View v, int position, long id) {
        super.onListItemClick(l, v, position, id);
        File requestFile = mFiles.get(position);
        if (requestFile.isDirectory()) {
            enterDirectory(requestFile);
        } else {
            Uri returnUri = Uri.withAppendedPath(Uri.parse("content://com.termux.filepicker.provider/"), requestFile.getAbsolutePath());
            Intent returnIntent = new Intent().setData(returnUri);
            returnIntent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
            setResult(Activity.RESULT_OK, returnIntent);
            finish();
        }
    }

    void enterDirectory(File directory) {
        getActionBar().setDisplayHomeAsUpEnabled(!directory.getAbsolutePath().equals(TERMUX_HOME));

        String title = directory.getAbsolutePath() + "/";
        if (title.startsWith(TERMUX_HOME)) {
            title = "~" + title.substring(TERMUX_HOME.length(), title.length());
        }
        setTitle(title);

        mCurrentDirectory = directory;
        mFiles.clear();
        mFileNames.clear();
        mFiles.addAll(Arrays.asList(mCurrentDirectory.listFiles()));

        Collections.sort(mFiles, new Comparator<File>() {
            @Override
            public int compare(File f1, File f2) {
                final String n1 = f1.getName();
                final String n2 = f2.getName();
                // Display dot folders last:
                if (n1.startsWith(".") && !n2.startsWith(".")) {
                    return 1;
                } else if (n2.startsWith(".") && !n1.startsWith(".")) {
                    return -1;
                }
                return n1.compareToIgnoreCase(n2);
            }
        });

        for (File file : mFiles) {
            mFileNames.add(file.getName() + (file.isDirectory() ? "/" : ""));
        }
        mAdapter.notifyDataSetChanged();
    }

}
