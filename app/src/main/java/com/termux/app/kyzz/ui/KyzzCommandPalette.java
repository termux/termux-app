package com.termux.app.kyzz.ui;

import android.app.AlertDialog;
import android.content.Context;
import android.widget.ArrayAdapter;

import java.util.ArrayList;
import java.util.List;

public class KyzzCommandPalette {
    public interface CommandCallback {
        void onExecute(String commandId);
    }

    public static class CommandItem {
        public final String id;
        public final String title;
        public final String category;

        public CommandItem(String id, String title, String category) {
            this.id = id;
            this.title = title;
            this.category = category;
        }

        @Override
        public String toString() {
            return "[" + category + "] " + title;
        }
    }

    public static void show(Context context, final CommandCallback callback) {
        final List<CommandItem> commands = new ArrayList<>();
        commands.add(new CommandItem("term_new", "New Terminal Session", "Terminal"));
        commands.add(new CommandItem("term_clear", "Clear Terminal Screen", "Terminal"));
        commands.add(new CommandItem("term_rename", "Rename Current Session", "Terminal"));
        commands.add(new CommandItem("file_new", "New File in Workspace", "Editor"));
        commands.add(new CommandItem("file_open", "Open File Explorer", "Editor"));
        commands.add(new CommandItem("file_save", "Save Active File", "Editor"));
        commands.add(new CommandItem("workspace_open", "Open Workspace Folder", "Workspace"));
        commands.add(new CommandItem("module_center", "Open Module Center", "Modules"));
        commands.add(new CommandItem("pkg_update", "Run 'pkg update'", "Modules"));
        commands.add(new CommandItem("appearance_theme", "Change Theme & Colors", "Appearance"));
        commands.add(new CommandItem("appearance_wallpaper", "Choose Wallpaper & Overlay", "Appearance"));
        commands.add(new CommandItem("appearance_font", "Manage Font & Size", "Appearance"));
        commands.add(new CommandItem("app_settings", "Open App Settings", "Settings"));

        ArrayAdapter<CommandItem> adapter = new ArrayAdapter<>(context, android.R.layout.simple_list_item_1, commands);

        new AlertDialog.Builder(context)
            .setTitle("Kyzz Command Palette (Ctrl+Shift+P)")
            .setAdapter(adapter, (dialog, which) -> {
                CommandItem selected = commands.get(which);
                if (callback != null) {
                    callback.onExecute(selected.id);
                }
            })
            .setNegativeButton("Close", null)
            .show();
    }
}
