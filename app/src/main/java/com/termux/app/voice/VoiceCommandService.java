package com.termux.app.voice;

import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;
import android.service.voice.VoiceInteractionService;

import androidx.annotation.Nullable;

import java.util.Arrays;
import java.util.List;

public class VoiceCommandService extends VoiceInteractionService {
    private static final String TAG = VoiceCommandService.class.getSimpleName();

    private static final List<String> SUPPORTED_ACTIONS = Arrays.asList(
    );

    class LocalBinder extends Binder {
        public final VoiceCommandService service = VoiceCommandService.this;
    }

    private final IBinder mBinder = new LocalBinder();

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }
}
