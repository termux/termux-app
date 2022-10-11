package com.termux.app.voice;

import android.content.Context;
import android.os.Handler;
import android.service.voice.VoiceInteractionSession;

public class TermuxVoiceInteractionSession extends VoiceInteractionSession {
    public TermuxVoiceInteractionSession(Context context) {
        super(context);
    }

    public TermuxVoiceInteractionSession(Context context, Handler handler) {
        super(context, handler);
    }
}
