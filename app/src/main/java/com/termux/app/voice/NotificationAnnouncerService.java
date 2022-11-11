package com.termux.app.voice;

import android.app.Notification;
import android.content.Context;
import android.service.notification.NotificationListenerService;
import android.service.notification.StatusBarNotification;
import android.speech.tts.TextToSpeech;

public class NotificationAnnouncerService extends NotificationListenerService implements TextToSpeech.OnInitListener {

    private TextToSpeech tts;

    @Override
    protected void attachBaseContext(Context base) {
        super.attachBaseContext(base);
        tts = new TextToSpeech(base, this);
    }

    @Override
    public void onNotificationPosted(StatusBarNotification sbn) {
        super.onNotificationPosted(sbn);
        Notification notification = sbn.getNotification();
        tts.speak(notification.extras.getString("android.title"), TextToSpeech.QUEUE_ADD, null, sbn.getKey());
        tts.speak(notification.extras.getString("android.text"), TextToSpeech.QUEUE_ADD, null, sbn.getKey());
    }

    @Override
    public void onListenerConnected() {
        super.onListenerConnected();
    }

    @Override
    public void onInit(int status) {

    }
}
