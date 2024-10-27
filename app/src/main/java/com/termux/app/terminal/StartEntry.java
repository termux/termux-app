package com.termux.app.terminal;

import static com.termux.shared.termux.TermuxConstants.TERMUX_HOME_DIR_PATH;

import com.termux.x11.controller.core.FileUtils;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.util.ArrayList;
import java.util.HashSet;

public class StartEntry {

    public static class Entry {
        private String path;
        private String fileName;
        private String iconPath;

        public String getPath() {
            return path;
        }

        public void setPath(String mPath) {
            this.path = mPath;
        }

        public String getFileName() {
            return fileName;
        }

        public void setFileName(String mFileName) {
            this.fileName = mFileName;
        }

        public String getIconPath() {
            return iconPath;
        }

        public void setIconPath(String mIconPath) {
            this.iconPath = mIconPath;
        }

        public JSONObject toJSONObject() {
            try {
                JSONObject elementJSONObject = new JSONObject();
                elementJSONObject.put("path", path);
                elementJSONObject.put("fileName", fileName);
                elementJSONObject.put("iconPath", iconPath);
                return elementJSONObject;
            } catch (JSONException e) {
                return null;
            }
        }
    }

    private static final ArrayList<Entry> mStartEntryList = new ArrayList<>();
    private static final HashSet<String> mStartSet = new HashSet<>();
    private static int mCurrentStartItemIdx = 0;

    public static ArrayList<Entry> getStartItemList() {
        return mStartEntryList;
    }

    public static int getCurrentStartItemIdx() {
        return mCurrentStartItemIdx;
    }

    public static void setCurrentStartItemIdx(int startItemIdx) {
        mCurrentStartItemIdx = startItemIdx;
    }

    public static void saveStartItems() {
        File file = new File(TERMUX_HOME_DIR_PATH, ".startItemEntries");
        try {
            JSONObject data = new JSONObject();
            data.put("version", "1.0");
            data.put("name", "startItemEntries");
            data.put("currentStartItem", mCurrentStartItemIdx);

            JSONArray elementsJSONArray = new JSONArray();
            for (Entry itm : mStartEntryList) {
                elementsJSONArray.put(itm.toJSONObject());
                if (!mStartSet.contains(itm.getPath())) {
                    mStartSet.add(itm.getPath());
                }
            }
            data.put("elements", elementsJSONArray);
            FileUtils.writeString(file, data.toString());
        } catch (JSONException e) {
            e.printStackTrace();
        }
    }

    public static void loadStartItems() {
        File file = new File(TERMUX_HOME_DIR_PATH, ".startItemEntries");
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
                Entry entry = new Entry();
                entry.setPath(path);
                entry.setFileName(fileName);
                entry.setIconPath(iconPath);
                String key = entry.getPath();
                if (!mStartSet.contains(key)) {
                    mStartSet.add(key);
                    mStartEntryList.add(entry);
                }
            }
            if (startItemEntriesJSONObject.has("currentStartItem")) {
                mCurrentStartItemIdx = startItemEntriesJSONObject.getInt("currentStartItem");
            }
        } catch (JSONException e) {
            e.printStackTrace();
        }
    }

    public static boolean addStartEntry(Entry item) {
        String key = item.getPath();
        if (!mStartSet.contains(key)) {
            mStartSet.add(key);
            mStartEntryList.add(item);
            return true;
        }
        return false;
    }

    public static void deleteStartEntry(Entry currentCommand) {
        if (mStartSet.contains(currentCommand.getPath())) {
            mStartEntryList.remove(currentCommand);
            mStartSet.remove(currentCommand.getPath());
        }
    }
}
