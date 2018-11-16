package com.termux.dom;

import android.accessibilityservice.AccessibilityService;
import android.util.Log;
import android.view.KeyEvent;
import android.view.accessibility.AccessibilityEvent;

import org.json.JSONException;
import org.json.JSONObject;

public class DomAccessibilityService extends AccessibilityService {
    public static final String TAG = "dom";

    private String[] getKeyEvent(KeyEvent event) {
        Log.d(TAG, "getKeyEvent Called");
        JSONObject k_event = new JSONObject();
        int RepeatCount = 0;
        try{
            RepeatCount = event.getRepeatCount();
        } catch (Exception e) {
            Log.e(TAG, e.getMessage());
        }
        if (RepeatCount % 10 == 0) {
            try {
                k_event.put("KeyCode", event.getKeyCode());
            } catch (JSONException e) {
                e.printStackTrace();
            }
            try {
                k_event.put("Action", event.getAction());
            } catch (JSONException e) {
                e.printStackTrace();
            }
            try {
                k_event.put("DownTime", event.getDownTime());
            } catch (JSONException e) {
                e.printStackTrace();
            }
            try {
                k_event.put("RepeatCount", RepeatCount);
            } catch (JSONException e) {
                e.printStackTrace();
            }

            return new String[] {String.valueOf(k_event), "" + RepeatCount};
        }
        return new String[] {"", ""};

    }

    @Override
    public void onAccessibilityEvent(AccessibilityEvent accessibilityEvent) {
        Log.e(TAG, "accessibilityEvent: " +  accessibilityEvent.toString());

    }

    @Override
    public void onInterrupt() {
        Log.i(TAG, "onInterrupt");
    }

    @Override
    protected boolean onKeyEvent(KeyEvent event) {
        Log.d(TAG, "onKeyEvent" + event.toString());
        String k_result[]  = getKeyEvent(event);
        //return super.onKeyEvent(event);
        if (!k_result[0].equals("")) {
            DomWebInterface.publishMessage(k_result[0], "key_command");
        }
        return false;
    }

}
