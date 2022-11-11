package com.termux.app.voice;

import android.app.Notification;
import android.content.Context;
import android.service.notification.NotificationListenerService;
import android.service.notification.StatusBarNotification;
import android.speech.tts.TextToSpeech;

import java.util.Locale;

public class NotificationAnnouncerService extends NotificationListenerService implements TextToSpeech.OnInitListener {

    private static final Locale RU = new Locale("ru", "RU");
    private TextToSpeech tts;

    @Override
    protected void attachBaseContext(Context base) {
        super.attachBaseContext(base);
        tts = new TextToSpeech(base, this);
    }

    @Override
    public void onNotificationPosted(StatusBarNotification sbn) {
        super.onNotificationPosted(sbn);
        final Notification notification = sbn.getNotification();
        final String title = notification.extras.getString("android.title");
        final String text = sanitize(notification.extras.getString("android.text"));
        tts.setLanguage(detectLocale(title));
        tts.speak(title, TextToSpeech.QUEUE_ADD, null, sbn.getKey());
        tts.setLanguage(detectLocale(text));
        tts.speak(text, TextToSpeech.QUEUE_ADD, null, sbn.getKey());
    }

    @Override
    public void onListenerConnected() {
        super.onListenerConnected();
    }

    @Override
    public void onInit(int status) {

    }

    protected Locale detectLocale(String text) {
        if (text.matches(".*[а-яА-Я].*")) {
            return RU;
        }

        return Locale.ENGLISH;
    }

    protected String sanitize(String text) {
        return text.replaceAll("^(https?|ftp|file)://[-a-zA-Z0-9+&@#/%?=~_|!:,.;]*[-a-zA-Z0-9+&@#/%=~_|]", "[link]");
    }
}
