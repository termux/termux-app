package com.termux.dom;

import android.content.Context;
import android.util.Log;

/**
 * Created by andrzej on 26.01.18.
 */


public class AisCommandLog {
    public static final String TYPE_IN = "TYPE_IN";
    public static final String TYPE_OUT = "TYPE_OUT";
    public static final String TYPE_ERROR = "TYPE_ERROR";
    public static final String TAG = AisCommandLog.class.getName();

    public static void show(Context context, String text, String type) {
        Log.i(TAG, "AisCommandLog -> show, type: " + type + ", text: " + text);

        // TODO style the text
        //Spannable spannable = new SpannableString(text);
        //spannable.setSpan(new ForegroundColorSpan(Color.BLUE), 0, text.length(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);


//        if (type == TYPE_IN) {
//            BrowserActivity.chatTextSpeak.append("Ty: " + text +"\n");
//        }
//        else if (type == TYPE_OUT) {
//            BrowserActivity.chatTextSpeak.append("Asystent: " +text +"\n");
//        }
//        else if (type == TYPE_ERROR) {
//            BrowserActivity.chatTextSpeak.append("System: " +text +"\n");
//        }
//
//
//        // autoscroll
//        final int scrollAmount = BrowserActivity.chatTextSpeak.getLayout().getLineTop(BrowserActivity.chatTextSpeak.getLineCount())
//                - BrowserActivity.chatTextSpeak.getHeight();
//        // if there is no need to scroll, scrollAmount will be <=0
//        if (scrollAmount > 0)
//            BrowserActivity.chatTextSpeak.scrollTo(0, scrollAmount);
//        else
//            BrowserActivity.chatTextSpeak.scrollTo(0, 0);

    }
}
