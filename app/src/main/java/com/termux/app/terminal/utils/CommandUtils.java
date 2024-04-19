package com.termux.app.terminal.utils;

import static com.termux.shared.termux.TermuxConstants.TERMUX_FILES_DIR_PATH;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;

import com.termux.app.TermuxService;
import com.termux.shared.termux.TermuxConstants;

import java.util.ArrayList;

public class CommandUtils {
    public static void  exec(Activity activity, String cmd, ArrayList<String> args){
        String fullCmd = TERMUX_FILES_DIR_PATH+"/usr/bin/"+cmd;
        Intent executeIntent = new Intent(TermuxConstants.TERMUX_APP.TERMUX_SERVICE.ACTION_SERVICE_EXECUTE);
        executeIntent.setFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        executeIntent.setFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
        Uri uri = Uri.parse(fullCmd);
        executeIntent.setData(uri);
        if (args!=null){
            executeIntent.putStringArrayListExtra(TermuxConstants.TERMUX_APP.TERMUX_SERVICE.EXTRA_ARGUMENTS, args);
        }
        executeIntent.putExtra(TermuxConstants.TERMUX_APP.TERMUX_SERVICE.EXTRA_BACKGROUND,true);
        executeIntent.putExtra(TermuxConstants.TERMUX_APP.TERMUX_SERVICE.EXTRA_WORKDIR, TERMUX_FILES_DIR_PATH+"/usr/bin/");
        executeIntent.setClass(activity, TermuxService.class);
        activity.startService(executeIntent);
    }
    public static void  execInPath(Activity activity, String cmd, ArrayList<String> args,String path){
        String fullCmd = TERMUX_FILES_DIR_PATH+path+cmd;
        Intent executeIntent = new Intent(TermuxConstants.TERMUX_APP.TERMUX_SERVICE.ACTION_SERVICE_EXECUTE);
        executeIntent.setFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        executeIntent.setFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
        Uri uri = Uri.parse(fullCmd);
        executeIntent.setData(uri);
        if (args!=null){
            executeIntent.putStringArrayListExtra(TermuxConstants.TERMUX_APP.TERMUX_SERVICE.EXTRA_ARGUMENTS, args);
        }
        executeIntent.putExtra(TermuxConstants.TERMUX_APP.TERMUX_SERVICE.EXTRA_BACKGROUND,true);
        executeIntent.putExtra(TermuxConstants.TERMUX_APP.TERMUX_SERVICE.EXTRA_WORKDIR, TERMUX_FILES_DIR_PATH+path);
        executeIntent.setClass(activity, TermuxService.class);
        activity.startService(executeIntent);
    }
}
