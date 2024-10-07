package com.termux.app.terminal;

import static com.termux.shared.termux.TermuxConstants.TERMUX_FILES_DIR_PATH;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import com.termux.R;

import java.util.ArrayList;
import java.util.List;

public class FileAdapter extends RecyclerView.Adapter<FileAdapter.FileViewHolder> {
    private ArrayList<FileInfo> mFileList;
    private final List<FileInfo> mEntireFileList = new ArrayList<>();
    private final OnItemClickListener mListener;
    private String mLastPath = TERMUX_FILES_DIR_PATH;

    public FileAdapter(ArrayList<FileInfo> fileList, OnItemClickListener listener) {
        this.mFileList = fileList;
        this.mListener = listener;
        mEntireFileList.clear();
        mEntireFileList.addAll(fileList);
    }

    @NonNull
    @Override
    public FileViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        View view = LayoutInflater.from(parent.getContext()).inflate(R.layout.item_file, parent, false);
        return new FileViewHolder(view);
    }

    @Override
    public void onBindViewHolder(@NonNull FileViewHolder holder, int position) {
        FileInfo fileInfo = mFileList.get(position);
        holder.bind(fileInfo);
    }

    @Override
    public int getItemCount() {
        return mFileList.size();
    }

    public interface OnItemClickListener {
        void onItemClick(FileInfo fileInfo);
    }

    public void filter(String keyword, String path) {
//        Log.d("FileAdapter.filter",keyword+": "+path);
        ArrayList<FileInfo> filteredList = new ArrayList<>();
        if (!path.equals(mLastPath)) {
            mEntireFileList.clear();
            mEntireFileList.addAll(mFileList);
            mLastPath = path;
        }
        for (FileInfo fileInfo : mEntireFileList) {
            if (fileInfo.getName().equals("..") ||
                fileInfo.getName().toLowerCase().contains(keyword.toLowerCase())) {
                filteredList.add(fileInfo);
            }
        }
        mFileList.clear();
        mFileList.addAll(filteredList);
        notifyDataSetChanged();
    }


    public class FileViewHolder extends RecyclerView.ViewHolder implements View.OnClickListener {
        private final TextView fileNameTextView;

        public FileViewHolder(@NonNull View itemView) {
            super(itemView);
            fileNameTextView = itemView.findViewById(R.id.file_item);
            itemView.setOnClickListener(this);
        }

        public void bind(FileInfo fileInfo) {
            fileNameTextView.setText(fileInfo.getName());
        }

        @Override
        public void onClick(View v) {
            int position = getAdapterPosition();
            if (position != RecyclerView.NO_POSITION) {
                FileInfo fileInfo = mFileList.get(position);
                mListener.onItemClick(fileInfo);
            }
        }
    }
}
