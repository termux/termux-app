package com.termux.app.terminal;

import static com.termux.shared.termux.TermuxConstants.TERMUX_HOME_DIR_PATH;

import com.termux.x11.controller.core.FileUtils;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.util.ArrayList;

public class MenuEntry {
    public static class Entry extends StartEntry.Entry {
        private String command;

        public String getCommand() {
            return command;
        }

        public void setCommand(String cmd) {
            this.command = cmd;
        }

        @Override
        public JSONObject toJSONObject() {
            JSONObject jsonObject = super.toJSONObject();
            try {
                jsonObject.put("command", command);
                return jsonObject;
            } catch (JSONException e) {
                throw new RuntimeException(e);
            }
        }
    }
    private static final ArrayList<Entry> mMenuEntryList = new ArrayList<>();

    public static ArrayList<Entry> getStartItemList() {
        return mMenuEntryList;
    }


    public static void saveMenuItems() {
        File file = new File(TERMUX_HOME_DIR_PATH, ".startMenuEntries");
        try {
            JSONObject data = new JSONObject();
            data.put("version", "1.0");
            data.put("name", "startMenuEntries");

            JSONArray elementsJSONArray = new JSONArray();
            for (Entry itm : mMenuEntryList) {
                elementsJSONArray.put(itm.toJSONObject());
                mMenuEntryList.add(itm);
            }
            data.put("elements", elementsJSONArray);
            FileUtils.writeString(file, data.toString());
        } catch (JSONException e) {
            e.printStackTrace();
        }
    }

    public static void loadMenuItems() {
        File file = new File(TERMUX_HOME_DIR_PATH, ".startMenuEntries");
        if (!file.exists() || !file.isFile()) {
            return;
        }

        try {
            JSONObject startItemEntriesJSONObject = new JSONObject(FileUtils.readString(file));
            JSONArray startItemsJSONArray = startItemEntriesJSONObject.getJSONArray("elements");
            for (int i = 0; i < startItemsJSONArray.length(); i++) {
                JSONObject controllerJSONObject = startItemsJSONArray.getJSONObject(i);
                String path = controllerJSONObject.getString("path");
                String fileName = controllerJSONObject.getString("fileName");
                String iconPath = controllerJSONObject.getString("iconPath");
                String cmd = controllerJSONObject.getString("command");
                Entry entry = new Entry();
                entry.setPath(path);
                entry.setFileName(fileName);
                entry.setIconPath(iconPath);
                entry.setCommand(cmd);
                String key = entry.getPath();
                mMenuEntryList.add(entry);
            }
        } catch (JSONException e) {
            e.printStackTrace();
        }
    }

    public static void addMenuEntry(Entry item) {
        mMenuEntryList.add(item);
    }

    public static void deleteMenuEntry(Entry currentCommand) {
        mMenuEntryList.remove(currentCommand);
    }
}
