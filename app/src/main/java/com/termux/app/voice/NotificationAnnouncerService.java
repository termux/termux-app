package com.termux.app.voice;

import android.app.Notification;
import android.app.NotificationManager;
import android.content.Context;
import android.content.Intent;
import android.os.IBinder;
import android.provider.Settings;
import android.service.notification.NotificationListenerService;
import android.service.notification.StatusBarNotification;
import android.speech.tts.TextToSpeech;

import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Set;
import java.util.Timer;
import java.util.TimerTask;

public class NotificationAnnouncerService extends NotificationListenerService implements TextToSpeech.OnInitListener {

    private static final Locale RU = new Locale("ru", "RU");
    private TextToSpeech tts;

    private Set<String> ongoing = new HashSet<>();

    @Override
    public IBinder onBind(Intent intent) {
        IBinder result = super.onBind(intent);
        tts = new TextToSpeech(getBaseContext(), this);
        return result;
    }

    @Override
    public boolean onUnbind(Intent intent) {
        tts.stop();
        tts = null;
        return false;
    }

    @Override
    public void onNotificationPosted(StatusBarNotification sbn) {
        super.onNotificationPosted(sbn);
        final Notification notification = sbn.getNotification();
        try {
            int filter = Settings.Global.getInt(getContentResolver(), "zen_mode");
            if (notification.priority < filter) {
                return;
            }
        } catch (Settings.SettingNotFoundException e) {
            e.printStackTrace();
        }

        final String title = sanitize(notification.extras.getString("android.title"));
        final String text = sanitize(notification.extras.getString("android.text"));
        final String key = sbn.getKey();

        tts.setLanguage(detectLocale(title));
        tts.speak(title, TextToSpeech.QUEUE_ADD, null, key);
        tts.setLanguage(detectLocale(text));
        tts.speak(text, TextToSpeech.QUEUE_ADD, null, key);
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
        return text == null ? "" : text.replaceAll("^(https?|ftp|file)://[-a-zA-Z0-9+&@#/%?=~_|!:,.;]*[-a-zA-Z0-9+&@#/%=~_|]", "[link]");
    }
}
