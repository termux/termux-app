package com.termux.app.terminal;

import static com.termux.shared.termux.TermuxConstants.TERMUX_HOME_DIR_PATH;

import android.util.Log;

import com.termux.x11.controller.core.FileUtils;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.util.ArrayList;

public class MenuEntry {
    public static class Entry extends StartEntry.Entry {
        private String command;
        private String type;
        private String titlle;

        public String getCommand() {
            return command;
        }

        public void setCommand(String cmd) {
            this.command = cmd;
        }

        public void setType(String t) {
            this.type = t;
        }

        public String getType() {
            return this.type;
        }
        public void setTitlle(String t){
            this.titlle =t;
        }
        public String getTitlle(){
            return this.titlle;
        }

        @Override
        public JSONObject toJSONObject() {
            JSONObject jsonObject = super.toJSONObject();
            try {
                jsonObject.put("command", command);
                jsonObject.put("type", type);
                jsonObject.put("title", titlle);
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
            }
            data.put("elements", elementsJSONArray);
            FileUtils.writeString(file, data.toString());
        } catch (JSONException e) {
            e.printStackTrace();
        }
    }

    public static void loadMenuItems() {
        mMenuEntryList.clear();
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
                String type = controllerJSONObject.getString("type");
                String title = controllerJSONObject.getString("title");
                Entry entry = new Entry();
                entry.setPath(path);
                entry.setFileName(fileName);
                entry.setIconPath(iconPath);
                entry.setCommand(cmd);
                entry.setType(type);
                entry.setTitlle(title);
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
