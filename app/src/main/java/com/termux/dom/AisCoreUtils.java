package com.termux.dom;

import java.io.File;

import static com.termux.app.TermuxService.HOME_PATH;

public class AisCoreUtils {
    private static String AIS_DOM_URL = "http://192.168.1.54:8180";

    // STT
    public static final String BROADCAST_ON_START_SPEECH_TO_TEXT = "BROADCAST_ON_START_SPEECH_TO_TEXT";
    public static final String BROADCAST_ON_END_SPEECH_TO_TEXT = "BROADCAST_ON_END_SPEECH_TO_TEXT";
    public static final String BROADCAST_ON_SPEECH_TO_TEXT_ERROR = "BROADCAST_ON_SPEECH_TO_TEXT_ERROR";
    public static final String SPEECH_TO_TEXT_ERROR_VALUE = "SPEECH_TO_TEXT_ERROR_VALUE";
    public static final String BROADCAST_EVENT_ON_SPEECH_COMMAND = "BROADCAST_EVENT_ON_SPEECH_COMMAND";
    public static final String BROADCAST_EVENT_ON_SPEECH_COMMAND_TEXT = "BROADCAST_EVENT_ON_SPEECH_COMMAND_TEXT";
    //
    // TTS
    public static final String BROADCAST_ON_END_TEXT_TO_SPEECH = "BROADCAST_ON_END_TEXT_TO_SPEECH";
    public static final String BROADCAST_ON_START_TEXT_TO_SPEECH = "BROADCAST_ON_START_TEXT_TO_SPEECH";
    public static final String COMMAND_LOG_TEXT = "COMMAND_LOG_TEXT";
    public static final String COMMAND_LOG_TYPE = "COMMAND_LOG_TYPE";

    public static String getAisDomUrl(){
        return AIS_DOM_URL;
    }

    public static Boolean getAppPreventSleep(){
        return true;
    }

    public static boolean onBox() {
        // default for box and phone
        File conf = new File(HOME_PATH + "/AIS/configuration.yaml");
        // during the first start on box only bootstrap exists
        File bootstrap = new File("/data/data/pl.sviete.dom/files.tar.7z");
        // during the first start on box the bootstrap can be on /sdcard
        File sd_bootstrap = new File("/sdcard/files.tar.7z");
        if (conf.exists() || bootstrap.exists() || sd_bootstrap.exists()) {
            return true;
        } else {
            return false;
        }

    }
}
