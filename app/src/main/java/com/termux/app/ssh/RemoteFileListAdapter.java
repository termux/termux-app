package com.termux.app.ssh;

import android.content.Context;
import android.graphics.Typeface;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;

import com.termux.R;
import com.termux.shared.theme.NightMode;
import com.termux.shared.theme.ThemeUtils;

import java.util.List;

/**
 * ArrayAdapter for displaying RemoteFile items in a ListView.
 *
 * Binds RemoteFile data to the item_remote_file layout, showing:
 * - File type icon (folder/file/symlink)
 * - File name with styling for directories
 * - File size (for files only)
 * - Permissions and modification time
 * - Symlink target (for symlinks)
 */
public class RemoteFileListAdapter extends ArrayAdapter<RemoteFile> {

    private final Context mContext;
    private final int mResourceId;

    /**
     * Create a new RemoteFileListAdapter.
     *
     * @param context Application context
     * @param files List of RemoteFile items to display
     */
    public RemoteFileListAdapter(@NonNull Context context, @NonNull List<RemoteFile> files) {
        super(context, R.layout.item_remote_file, files);
        this.mContext = context;
        this.mResourceId = R.layout.item_remote_file;
    }

    @NonNull
    @Override
    public View getView(int position, @Nullable View convertView, @NonNull ViewGroup parent) {
        View itemView = convertView;
        ViewHolder holder;

        if (itemView == null) {
            LayoutInflater inflater = LayoutInflater.from(mContext);
            itemView = inflater.inflate(mResourceId, parent, false);

            holder = new ViewHolder();
            holder.iconView = itemView.findViewById(R.id.file_icon);
            holder.nameView = itemView.findViewById(R.id.file_name);
            holder.sizeView = itemView.findViewById(R.id.file_size);
            holder.permissionsView = itemView.findViewById(R.id.file_permissions);
            holder.dateView = itemView.findViewById(R.id.file_date);
            holder.symlinkTargetView = itemView.findViewById(R.id.symlink_target);

            itemView.setTag(holder);
        } else {
            holder = (ViewHolder) itemView.getTag();
        }

        RemoteFile file = getItem(position);
        if (file == null) {
            return itemView;
        }

        // Set file icon based on type
        setFileIcon(holder.iconView, file);

        // Set file name with Morandi theme colors
        holder.nameView.setText(file.getName());
        if (file.isDirectoryOrSymlinkToDirectory()) {
            // Symlink to directory shows bold + directory color
            holder.nameView.setTypeface(holder.nameView.getTypeface(), Typeface.BOLD);
            holder.nameView.setTextColor(ContextCompat.getColor(mContext, R.color.morandi_directory_name));
        } else {
            holder.nameView.setTypeface(Typeface.create(holder.nameView.getTypeface(), Typeface.NORMAL), Typeface.NORMAL);
            holder.nameView.setTextColor(ContextCompat.getColor(mContext, R.color.morandi_file_name));
        }

        // Set file size (show "-" for directories and symlinks to directories)
        if (file.isDirectoryOrSymlinkToDirectory()) {
            holder.sizeView.setText(mContext.getString(R.string.directory_size));
        } else {
            holder.sizeView.setText(file.getSizeFormatted());
        }

        // Set permissions
        holder.permissionsView.setText(file.getPermissions());

        // Set modification time
        holder.dateView.setText(file.getModifyTime());

        // Show symlink target if applicable
        if (file.isSymlink() && file.getSymlinkTarget() != null) {
            holder.symlinkTargetView.setText("→ " + file.getSymlinkTarget());
            holder.symlinkTargetView.setVisibility(View.VISIBLE);
        } else {
            holder.symlinkTargetView.setVisibility(View.GONE);
        }

        return itemView;
    }

    /**
     * Set the appropriate icon for the file type.
     *
     * @param iconView ImageView to set icon on
     * @param file RemoteFile to determine icon type
     */
    private void setFileIcon(ImageView iconView, RemoteFile file) {
        int iconResId;
        if (file.isDirectoryOrSymlinkToDirectory()) {
            // Symlink to directory shows folder icon
            iconResId = R.drawable.ic_folder;
        } else if (file.isSymlink()) {
            // Symlink to file shows symlink icon
            iconResId = R.drawable.ic_symlink;
        } else {
            iconResId = R.drawable.ic_file;
        }
        iconView.setImageResource(iconResId);

        // Set icon tint using Morandi theme color
        iconView.setColorFilter(ContextCompat.getColor(mContext, R.color.morandi_icon_primary));
    }

    /**
     * ViewHolder for efficient view recycling.
     */
    private static class ViewHolder {
        ImageView iconView;
        TextView nameView;
        TextView sizeView;
        TextView permissionsView;
        TextView dateView;
        TextView symlinkTargetView;
    }

    /**
     * Update the list with new files and refresh the view.
     *
     * @param files New list of RemoteFile items
     */
    public void updateFiles(@NonNull List<RemoteFile> files) {
        clear();
        addAll(files);
        notifyDataSetChanged();
    }
}