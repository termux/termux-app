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
    private ArrayList<FileInfo> fileList;
    private List<FileInfo> entireFileList = new ArrayList<>();
    private OnItemClickListener listener;
    private String lastPath = TERMUX_FILES_DIR_PATH;

    public FileAdapter(ArrayList<FileInfo> fileList, OnItemClickListener listener) {
        this.fileList = fileList;
        this.listener = listener;
        entireFileList.clear();
        for(FileInfo fileInfo : fileList){
            entireFileList.add(fileInfo);
        }
    }

    @NonNull
    @Override
    public FileViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        View view = LayoutInflater.from(parent.getContext()).inflate(R.layout.item_file, parent, false);
        return new FileViewHolder(view);
    }

    @Override
    public void onBindViewHolder(@NonNull FileViewHolder holder, int position) {
        FileInfo fileInfo = fileList.get(position);
        holder.bind(fileInfo);
    }

    @Override
    public int getItemCount() {
        return fileList.size();
    }

    public interface OnItemClickListener {
        void onItemClick(FileInfo fileInfo);
    }

    public void filter(String keyword, String path) {
        ArrayList<FileInfo> filteredList = new ArrayList<>();
        if (!path.equals(lastPath)) {
            entireFileList.clear();
            for(FileInfo fileInfo : fileList){
                entireFileList.add(fileInfo);
            }
            lastPath = path;
        }
        for (FileInfo fileInfo : entireFileList) {
            if (fileInfo.getName().equals("..") ||
                fileInfo.getName().toLowerCase().contains(keyword.toLowerCase())) {
                filteredList.add(fileInfo);
            }
        }
        fileList = filteredList;
        notifyDataSetChanged();
    }


    public class FileViewHolder extends RecyclerView.ViewHolder implements View.OnClickListener {
        private TextView fileNameTextView;

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
                FileInfo fileInfo = fileList.get(position);
                listener.onItemClick(fileInfo);
            }
        }
    }
}
