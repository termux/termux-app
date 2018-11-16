package com.termux.dom;

import android.app.Activity;
import android.app.KeyguardManager;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.media.AudioManager;
import android.media.ToneGenerator;
import android.net.Uri;
import android.net.wifi.WifiManager;
import android.os.Binder;
import android.os.Handler;
import android.os.IBinder;
import android.os.PowerManager;
import android.speech.tts.TextToSpeech;
import android.speech.tts.UtteranceProgressListener;
import android.speech.tts.Voice;
import android.support.v4.content.LocalBroadcastManager;
import android.util.Log;
import android.widget.Toast;

import com.google.android.exoplayer2.DefaultLoadControl;
import com.google.android.exoplayer2.ExoPlaybackException;
import com.google.android.exoplayer2.ExoPlayer;
import com.google.android.exoplayer2.ExoPlayerFactory;
import com.google.android.exoplayer2.LoadControl;
import com.google.android.exoplayer2.PlaybackParameters;
import com.google.android.exoplayer2.SimpleExoPlayer;
import com.google.android.exoplayer2.extractor.DefaultExtractorsFactory;
import com.google.android.exoplayer2.extractor.ExtractorsFactory;
import com.google.android.exoplayer2.source.ExtractorMediaSource;
import com.google.android.exoplayer2.source.MediaSource;
import com.google.android.exoplayer2.source.TrackGroupArray;
import com.google.android.exoplayer2.trackselection.AdaptiveTrackSelection;
import com.google.android.exoplayer2.trackselection.DefaultTrackSelector;
import com.google.android.exoplayer2.trackselection.TrackSelection;
import com.google.android.exoplayer2.trackselection.TrackSelectionArray;
import com.google.android.exoplayer2.trackselection.TrackSelector;
import com.google.android.exoplayer2.upstream.BandwidthMeter;
import com.google.android.exoplayer2.upstream.DataSource;
import com.google.android.exoplayer2.upstream.DefaultBandwidthMeter;
import com.google.android.exoplayer2.upstream.DefaultDataSourceFactory;
import com.koushikdutta.async.http.AsyncHttpClient;
import com.koushikdutta.async.http.AsyncHttpGet;
import com.koushikdutta.async.http.AsyncHttpPost;
import com.koushikdutta.async.http.AsyncHttpRequest;
import com.koushikdutta.async.http.body.JSONObjectBody;
import com.koushikdutta.async.http.server.AsyncHttpServer;
import com.koushikdutta.async.http.server.AsyncHttpServerRequest;
import com.koushikdutta.async.http.server.AsyncHttpServerResponse;
import com.koushikdutta.async.http.server.HttpServerRequestCallback;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.math.BigDecimal;
import java.security.KeyManagementException;
import java.security.cert.CertificateException;
import java.security.cert.X509Certificate;
import java.util.ArrayList;
import java.util.Locale;

import javax.net.ssl.HostnameVerifier;
import javax.net.ssl.HttpsURLConnection;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSession;
import javax.net.ssl.TrustManager;
import javax.net.ssl.X509TrustManager;

import com.termux.dom.api.MountReceiver;
import com.termux.dom.api.WifiAPI;
import com.termux.app.TermuxService;
import com.termux.dom.api.WifiReceiver;

import static com.termux.dom.AisCoreUtils.BROADCAST_ON_END_SPEECH_TO_TEXT;
import static com.termux.dom.AisCoreUtils.BROADCAST_ON_END_TEXT_TO_SPEECH;
import static com.termux.dom.AisCoreUtils.BROADCAST_ON_START_SPEECH_TO_TEXT;
import static com.termux.dom.AisCoreUtils.BROADCAST_ON_START_TEXT_TO_SPEECH;
import static com.termux.dom.AisCoreUtils.getAppPreventSleep;

public class AisPanelService extends Service implements TextToSpeech.OnInitListener, ExoPlayer.EventListener {
    private static final int ONGOING_NOTIFICATION_ID = 1;
    public static final String BROADCAST_EVENT_URL_CHANGE = "BROADCAST_EVENT_URL_CHANGE";
    public static final String BROADCAST_EVENT_DO_STOP_TTS = "BROADCAST_EVENT_DO_STOP_TTS";
    public static final String BROADCAST_EVENT_ON_SPEECH_COMMAND = "BROADCAST_EVENT_ON_SPEECH_COMMAND";
    public static final String BROADCAST_EVENT_ON_SPEECH_COMMAND_TEXT = "BROADCAST_EVENT_ON_SPEECH_COMMAND_TEXT";
    public static final String BROADCAST_ON_SPEECH_TO_TEXT_ERROR = "BROADCAST_ON_SPEECH_TO_TEXT_ERROR";
    public static final String SPEECH_TO_TEXT_ERROR_VALUE = "SPEECH_TO_TEXT_ERROR_VALUE";
    public static final String BROADCAST_EVENT_KEY_BUTTON_PRESSED = "BROADCAST_EVENT_KEY_BUTTON_PRESSED";
    public static final String EVENT_KEY_BUTTON_PRESSED_VALUE = "EVENT_KEY_BUTTON_PRESSED_VALUE";
    public static final String EVENT_KEY_BUTTON_PRESSED_SOUND = "EVENT_KEY_BUTTON_PRESSED_SOUND";
    private final String TAG = AisPanelService.class.getName();

    private PowerManager.WakeLock fullWakeLock;
    private PowerManager.WakeLock partialWakeLock;
    private WifiManager.WifiLock wifiLock;
    private KeyguardManager.KeyguardLock keyguardLock;

    private AsyncHttpServer mHttpServer;
    private static String currentUrl;

    private final IBinder mBinder = new AisPanelServiceBinder();

    private TextToSpeech mTts;
    private String mReadThisTextWhenReady;

    private ToneGenerator toneGenerator;

    //ExoPlayer -start
    private Handler mainHandler;
    private BandwidthMeter bandwidthMeter;
    private LoadControl loadControl;
    private DataSource.Factory dataSourceFactory;
    private ExtractorsFactory extractorsFactory;
    private MediaSource mediaSource;
    private TrackSelection.Factory trackSelectionFactory;
    private SimpleExoPlayer exoPlayer;
    private TrackSelector trackSelector;


    @Override
    public void onTracksChanged(TrackGroupArray trackGroups, TrackSelectionArray trackSelections) {

    }

    @Override
    public void onLoadingChanged(boolean isLoading) {

    }

    @Override
    public void onPlayerStateChanged(boolean playWhenReady, int playbackState) {
        JSONObject jState = new JSONObject();
        if (playbackState == ExoPlayer.STATE_ENDED) {
            // inform client that next song can be played
            try {
                jState.put("currentStatus", playbackState);
            } catch(JSONException e) { e.printStackTrace(); }
            try {
                jState.put("currentMedia", "Piosenka");
            } catch(JSONException e) { e.printStackTrace(); }
            try {
                jState.put("playing", false);
            } catch(JSONException e) { e.printStackTrace(); }
            try {
                jState.put("giveMeNextOne", true);
            } catch(JSONException e) { e.printStackTrace(); }
            try {
                jState.put("currentVolume", exoPlayer.getVolume());
            } catch(JSONException e) { e.printStackTrace(); }
            try {
                jState.put("duration", exoPlayer.getDuration());
            } catch(JSONException e) { e.printStackTrace(); }
            try {
                jState.put("currentPosition", exoPlayer.getCurrentPosition());
            } catch(JSONException e) { e.printStackTrace(); }
            try {
                jState.put("currentSpeed", exoPlayer.getPlaybackParameters().speed);
            } catch(JSONException e) { e.printStackTrace(); }
            publishAudioPlayerStatus(jState.toString());
        } else {
            // inform client about status change
            jState = getAudioStatus();
            publishAudioPlayerStatus(jState.toString());
        }
    }

    @Override
    public void onRepeatModeChanged(int repeatMode) {

    }

    @Override
    public void onPlayerError(ExoPlaybackException error) {

    }

    @Override
    public void onPlaybackParametersChanged(PlaybackParameters playbackParameters) {

    }

    //ExoPlayer - end


    //    Speech To Text
    public class AisPanelServiceBinder extends Binder {
        AisPanelService getService() {
            return AisPanelService.this;
        }
    }

    public void stopSpeechToText(){
        Log.i(TAG, "Speech started, stoping the tts");
        try {
            mTts.stop();
        } catch (Exception e) {
            Log.e(TAG, e.toString());
        }
    }

    public void createTTS() {
        Log.i(TAG, "starting TTS initialization");
        mTts = new TextToSpeech(this,this);
        mTts.setSpeechRate(1.0f);
    }


    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        super.onStartCommand(intent, flags, startId);
        Log.i(TAG, "AisPanelService onStartCommand");
        try {
            String action = intent.getAction();
            Log.i(TAG, "AisPanelService onStartCommand, action: " + action);
        } catch (Exception e) {
            Log.i(TAG, "AisPanelService onStartCommand no action");
        }
        return START_STICKY;
    }


    @Override
    public void onInit(int status) {
        Log.v(TAG, "AisPanelService onInit");
        if (status != TextToSpeech.ERROR) {
            int result = mTts.setLanguage(new Locale("pl_PL"));
            if (result == TextToSpeech.LANG_MISSING_DATA ||
                    result == TextToSpeech.LANG_NOT_SUPPORTED) {
                Log.e(TAG, "Language is not available.");
                Toast.makeText(getApplicationContext(), "TTS język polski nie jest obsługiwany",Toast.LENGTH_SHORT).show();
            }

            if(result == TextToSpeech.SUCCESS) {
                mTts.setOnUtteranceProgressListener(new UtteranceProgressListener() {
                    @Override
                    public void onDone(String utteranceId) {
                        Log.d(TAG, "TTS finished");
                        Intent intent = new Intent(BROADCAST_ON_END_TEXT_TO_SPEECH);
                        LocalBroadcastManager bm = LocalBroadcastManager.getInstance(getApplicationContext());
                        bm.sendBroadcast(intent);
                        publishSpeechStatus("DONE");
                    }

                    @Override
                    public void onError(String utteranceId) {
                        Log.d(TAG, "TTS onError");
                        Intent intent = new Intent(BROADCAST_ON_END_TEXT_TO_SPEECH);
                        LocalBroadcastManager bm = LocalBroadcastManager.getInstance(getApplicationContext());
                        bm.sendBroadcast(intent);
                        publishSpeechStatus("ERROR");
                    }

                    @Override
                    public void onStart(String utteranceId) {
                        publishSpeechStatus("START");
                    }
                });

                if (mReadThisTextWhenReady != null){
                    processTTS(mReadThisTextWhenReady, AisCommandLog.TYPE_OUT);
                    mReadThisTextWhenReady = null;
                }

            };
        } else {
            Log.e(TAG, "Could not initialize TextToSpeech.");
        }

    }

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        Log.d(TAG, "onCreate Called");

        currentUrl = AisCoreUtils.getAisDomUrl();
        // prepare the lock types we may use
        PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
        fullWakeLock = pm.newWakeLock(PowerManager.FULL_WAKE_LOCK |
                PowerManager.ON_AFTER_RELEASE |
                PowerManager.ACQUIRE_CAUSES_WAKEUP, "dom:fullWakeLock");
        partialWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "dom:partialWakeLock");
        WifiManager wifiManager = (WifiManager) getApplicationContext().getSystemService(Context.WIFI_SERVICE);
        wifiLock = wifiManager.createWifiLock(WifiManager.WIFI_MODE_FULL, "wifiLock");

        KeyguardManager km = (KeyguardManager) getSystemService(Activity.KEYGUARD_SERVICE);
        keyguardLock = km.newKeyguardLock(KEYGUARD_SERVICE);

        IntentFilter filter = new IntentFilter();
        filter.addAction(BROADCAST_EVENT_URL_CHANGE);
        filter.addAction(BROADCAST_EVENT_DO_STOP_TTS);
        filter.addAction(BROADCAST_EVENT_ON_SPEECH_COMMAND);;
        filter.addAction(BROADCAST_ON_SPEECH_TO_TEXT_ERROR);
        filter.addAction(BROADCAST_EVENT_KEY_BUTTON_PRESSED);
        filter.addAction(BROADCAST_ON_END_SPEECH_TO_TEXT);
        filter.addAction(BROADCAST_ON_START_SPEECH_TO_TEXT);
        filter.addAction(BROADCAST_ON_END_TEXT_TO_SPEECH);
        filter.addAction(BROADCAST_ON_START_TEXT_TO_SPEECH);

        LocalBroadcastManager bm = LocalBroadcastManager.getInstance(this);
        bm.registerReceiver(mBroadcastReceiver, filter);

        configurePowerOptions();

        // http api server
        configureHttp();

        //ExoPlayer
        bandwidthMeter = new DefaultBandwidthMeter();
        trackSelectionFactory = new AdaptiveTrackSelection.Factory(bandwidthMeter);
        trackSelector = new DefaultTrackSelector(trackSelectionFactory);
        loadControl = new DefaultLoadControl();
        exoPlayer = ExoPlayerFactory.newSimpleInstance(getApplicationContext());
        exoPlayer.addListener(this);

        dataSourceFactory = new DefaultDataSourceFactory(getApplicationContext(), "AisDom");
        extractorsFactory = new DefaultExtractorsFactory();


        createTTS();

        toneGenerator = new ToneGenerator(AudioManager.STREAM_SYSTEM, 100);


        startForeground();
    }


    @Override
    public void onDestroy() {
        super.onDestroy();
        Log.d(TAG, "onDestroy Called");

        stopPowerOptions();

        if (mTts != null) {
            mTts.stop();
            mTts.shutdown();
            mTts = null;
        }
        //mMediaNotificationManager.stopNotification();

        toneGenerator.release();

        Log.i(TAG, "destroy");
    }

    private void startForeground(){
        Log.d(TAG, "startForeground Called");
    }


    private final BroadcastReceiver mBroadcastReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (intent.getAction().equals(BROADCAST_EVENT_URL_CHANGE)) {
                final String url = intent.getStringExtra(BROADCAST_EVENT_URL_CHANGE);
                if (!url.equals(currentUrl)) {
                    Log.d(TAG, "Url changed to " + url);
                    currentUrl = url;
                }
            } else if (intent.getAction().equals(BROADCAST_EVENT_DO_STOP_TTS)) {
                Log.d(TAG, "Speech started, stoping the tts");
                stopSpeechToText();
            } else if (intent.getAction().equals(BROADCAST_EVENT_ON_SPEECH_COMMAND)) {
                Log.d(TAG, BROADCAST_EVENT_ON_SPEECH_COMMAND + " going to publishSpeechCommand");
                final String command = intent.getStringExtra(BROADCAST_EVENT_ON_SPEECH_COMMAND_TEXT);
                publishSpeechCommand(command);
            } else if (intent.getAction().equals(BROADCAST_ON_SPEECH_TO_TEXT_ERROR)) {
                Log.d(TAG, BROADCAST_ON_SPEECH_TO_TEXT_ERROR + " going to processTTS");
                final String errorMessage = intent.getStringExtra(SPEECH_TO_TEXT_ERROR_VALUE);
                processTTS(errorMessage, AisCommandLog.TYPE_ERROR);
                turnUpVolumeForSTT();
            } else if (intent.getAction().equals(BROADCAST_EVENT_KEY_BUTTON_PRESSED)) {
                Log.d(TAG, BROADCAST_EVENT_KEY_BUTTON_PRESSED + " going to publishKeyEvent");
                final String key_event = intent.getStringExtra(EVENT_KEY_BUTTON_PRESSED_VALUE);
                final String sound = intent.getStringExtra(EVENT_KEY_BUTTON_PRESSED_SOUND);
                publishKeyEvent(key_event, sound);
            } else if (intent.getAction().equals(BROADCAST_ON_START_SPEECH_TO_TEXT)) {
                Log.d(TAG, BROADCAST_ON_START_SPEECH_TO_TEXT + " turnDownVolume");
                turnDownVolumeForSTT();
            } else if (intent.getAction().equals(BROADCAST_ON_END_SPEECH_TO_TEXT)) {
                Log.d(TAG, BROADCAST_ON_END_SPEECH_TO_TEXT + " turnUpVolume");
                turnUpVolumeForSTT();
            } else if (intent.getAction().equals(BROADCAST_ON_START_TEXT_TO_SPEECH)) {
                Log.d(TAG, BROADCAST_ON_START_TEXT_TO_SPEECH + " turnDownVolume");
                turnDownVolumeForTTS();
            } else if (intent.getAction().equals(BROADCAST_ON_END_TEXT_TO_SPEECH)) {
                Log.d(TAG, BROADCAST_ON_END_TEXT_TO_SPEECH + " turnUpVolume");
                turnUpVolumeForTTS();
            }
        }
    };


    //******** Power Related Functions

    private void configurePowerOptions() {
        Log.d(TAG, "configurePowerOptions Called");

        // We always grab partialWakeLock & WifiLock
        Log.i(TAG, "Acquiring Partial Wake Lock and WiFi Lock");
        if (!partialWakeLock.isHeld()) partialWakeLock.acquire();
        if (!wifiLock.isHeld()) wifiLock.acquire();

        if (getAppPreventSleep())
        {
            Log.i(TAG, "Acquiring WakeLock to prevent screen sleep");
            if (!fullWakeLock.isHeld()) {
                fullWakeLock.acquire();
                if (AisCoreUtils.onBox()) {
                    // Lock wake in console
                    Intent wakeLockIntent = new Intent(getApplicationContext(), TermuxService.class);
                    wakeLockIntent.setAction(TermuxService.ACTION_LOCK_WAKE);
                    startService(wakeLockIntent);
                }

            }
        }
        else
        {
            Log.i(TAG, "Will not prevent screen sleep");
            if (fullWakeLock.isHeld()){
                fullWakeLock.release();
                if (AisCoreUtils.onBox()) {
                    // Unlock wake in console
                    Intent wakeUnlockIntent = new Intent(getApplicationContext(), TermuxService.class);
                    wakeUnlockIntent.setAction(TermuxService.ACTION_UNLOCK_WAKE);
                    startService(wakeUnlockIntent);
                }
            }
        }

        try {
            keyguardLock.disableKeyguard();
        }
        catch (Exception ex) {
            Log.i(TAG, "Disabling keyguard didn't work");
            ex.printStackTrace();
        }
    }

    private void stopPowerOptions() {
        Log.i(TAG, "Releasing Screen/WiFi Locks");
        if (partialWakeLock.isHeld()) partialWakeLock.release();
        if (fullWakeLock.isHeld()) fullWakeLock.release();
        if (wifiLock.isHeld()) wifiLock.release();

        try {
            keyguardLock.reenableKeyguard();
        }
        catch (Exception ex) {
            Log.i(TAG, "Reenabling keyguard didn't work");
            ex.printStackTrace();
        }
    }

    //******** HTTP Related Functions

    private void configureHttp(){
        mHttpServer = new AsyncHttpServer();

        mHttpServer.get("/", new HttpServerRequestCallback() {
            @Override
            public void onRequest(AsyncHttpServerRequest request, AsyncHttpServerResponse response) {
                Log.d(TAG, "request: " + request);
                JSONObject json = new JSONObject();
                 try {
                        // the seme structure like in sonoff http://<device-ip>/cm?cmnd=status%205
                        json.put("Hostname", AisNetUtils.getHostName());
                        json.put("MacWlan0", AisNetUtils.getMACAddress("wlan0"));
                        json.put("MacEth0", AisNetUtils.getMACAddress("eth0"));
                        json.put("IPAddressIPv4", AisNetUtils.getIPAddress(true));
                        json.put("IPAddressIPv6", AisNetUtils.getIPAddress(false));
                        json.put("ApiLevel", AisNetUtils.getApiLevel());
                        json.put("Device", AisNetUtils.getDevice());
                        json.put("OsVersion", AisNetUtils.getOsVersion());
                        json.put("Model", AisNetUtils.getModel());
                        json.put("Product", AisNetUtils.getProduct());
                        json.put("Manufacturer", AisNetUtils.getManufacturer());
                    } catch (JSONException e) {
                        e.printStackTrace();
                    }
                response.send(json);
            }
        });

        mHttpServer.get("/audio_status", new HttpServerRequestCallback() {
            @Override
            public void onRequest(AsyncHttpServerRequest request, AsyncHttpServerResponse response) {
                Log.d(TAG, "request: " + request);
                JSONObject json = new JSONObject();
                json = getAudioStatus();
                response.send(json);
            }
        });

        mHttpServer.post("/command", new HttpServerRequestCallback() {
            @Override
            public void onRequest(AsyncHttpServerRequest request, AsyncHttpServerResponse response) {
                Log.d(TAG, "command: " + request);
                JSONObject body = ((JSONObjectBody)request.getBody()).get();
                processCommand(body.toString());
                response.send("ok");
            }
        });

        mHttpServer.post("/text_to_speech", new HttpServerRequestCallback() {
            @Override
            public void onRequest(AsyncHttpServerRequest request, AsyncHttpServerResponse response) {
                Log.d(TAG, "text_to_speech: " + request);
                JSONObject body = ((JSONObjectBody)request.getBody()).get();
                processTTS(body.toString(), AisCommandLog.TYPE_OUT);
                response.send("ok");
            }
        });

        // listen on port 8122
        mHttpServer.listen(8122);
    }



    public static void publishMessage(String message, String topicPostfix){
        Log.d("publishMessage", "publishMessage Called, topic: " + topicPostfix + " message: " + message);
        // publish via http rest to local instance
        // url is the URL to home assistant
        JSONObject json = new JSONObject();
        try {
            json.put("topic", "ais/" + topicPostfix);
            json.put("payload", message);
            json.put("callback", AisNetUtils.getIPAddress(true));
        } catch (JSONException e) {
            Log.e("publishMessage", e.toString());
        }
        currentUrl = AisCoreUtils.getAisDomUrl();
        String url = currentUrl + "/api/services/ais_ai_service/process_command_from_frame";
        if (currentUrl.startsWith("https://")){
            // always trust the localhost
            AsyncHttpRequest request = new AsyncHttpRequest(Uri.parse(url), "POST");
            AsyncHttpClient asyncHttpClient = AsyncHttpClient.getDefaultInstance();
            SSLContext sslContext = null;
            TrustManager[] trustManagers = null;
            try {
                HttpsURLConnection.setDefaultHostnameVerifier(new HostnameVerifier(){
                    public boolean verify(String hostname, SSLSession session) {
                        return true;
                    }});
                sslContext = SSLContext.getInstance("TLS");
                trustManagers = new TrustManager[] {
                        new X509TrustManager(){
                    public void checkClientTrusted(X509Certificate[] chain,
                                                   String authType) throws CertificateException {}
                    public void checkServerTrusted(X509Certificate[] chain,
                                                   String authType) throws CertificateException {}
                    public X509Certificate[] getAcceptedIssuers() {
                        return new X509Certificate[0];
                    }}};
                try {
                    sslContext.init(null, trustManagers, null);
                } catch (KeyManagementException e) {
                    e.printStackTrace();
                }
            } catch (Exception e) { // should never happen
                e.printStackTrace();
            }
            asyncHttpClient.getSSLSocketMiddleware().setSSLContext(sslContext);
            asyncHttpClient.getSSLSocketMiddleware().setTrustManagers(trustManagers);
            asyncHttpClient.getSSLSocketMiddleware().setHostnameVerifier(new HostnameVerifier() {
                @Override
                public boolean verify(String s, SSLSession sslSession) {
                    return true;
                }
            });
            request.addHeader("Content-Type", "application/json");
            JSONObjectBody body = new JSONObjectBody(json);
            request.setBody(body);
            asyncHttpClient.executeJSONObject(request, null);



        } else {
            // do the simple HTTP post
            AsyncHttpPost post = new AsyncHttpPost(url);
            JSONObjectBody body = new JSONObjectBody(json);
            post.addHeader("Content-Type", "application/json");
            post.setBody(body);
            AsyncHttpClient.getDefaultInstance().executeJSONObject(post, null);
        }

    }

    public static void publishSettingsToIot(String url){
        Log.d("publishSettingsToIot", "publishSettingsToIot Called, topic: " + url);
        // publish via http rest to local instance
        // url is the URL to sonoff IOT
        // http://192.168.4.1/cm?cmnd=Power%20TOGGLE
        AsyncHttpGet get = new AsyncHttpGet(url);
        //get.addHeader("Content-Type", "application/json");
        AsyncHttpClient.getDefaultInstance().executeJSONObject(get, null);
    }


    //******** API Functions

    private boolean processCommand(JSONObject commandJson) {
        Log.d(TAG, "processCommand Called ");
        try {
            if(commandJson.has("playAudio")) {
                playAudio(commandJson.getString("playAudio"));
            }
              else if(commandJson.has("playAudioList")) {
                playAudioList(commandJson.getJSONArray("playAudioList"));
            }
              else if(commandJson.has("stopAudio")) {
                stopAudio();
            }
              else if(commandJson.has("pauseAudio")) {
                pauseAudio(commandJson.getBoolean("pauseAudio"));
            }
              else if(commandJson.has("getAudioStatus")) {
                JSONObject jState = getAudioStatus();
                publishAudioPlayerStatus(jState.toString());
            }
              else if(commandJson.has("setVolume")) {
                setVolume(commandJson.getLong("setVolume"));
            }
              else if(commandJson.has("upVolume")) {
                float vol = Math.min(getVolume() + 0.1f, 1.0f);
                setVolume(vol);
            }
              else if(commandJson.has("downVolume")) {
                float vol = Math.max(getVolume() - 0.1f, 0.0f);
                setVolume(vol);
            }
              else if(commandJson.has("setPlaybackPitch")) {
                setPlaybackPitch(Float.parseFloat(commandJson.getString("setPlaybackPitch")));
            }
              else if(commandJson.has("setPlaybackSpeed")) {
                setPlaybackSpeed(Float.parseFloat(commandJson.getString("setPlaybackSpeed")));
            }
              else if(commandJson.has("increasePlaybackSpeed")) {
                increasePlaybackSpeed(Float.parseFloat(commandJson.getString("increasePlaybackSpeed")));
            }
              else if(commandJson.has("seekTo")) {
                seekTo(commandJson.getLong("seekTo"));
            }
              else if(commandJson.has("skipTo")) {
                skipTo(commandJson.getLong("skipTo"));
            }
              else if(commandJson.has("WifiConnectionInfo")) {
                  WifiAPI.onReceiveWifiConnectionInfo(getApplicationContext());
            }
              else if(commandJson.has("WifiScanInfo")) {
                WifiAPI.onReceiveWifiScanInfo(getApplicationContext(), "wifi_scan_info");
            }
              else if(commandJson.has("WifiEnable")) {
                WifiAPI.onReceiveWifiEnable(getApplicationContext(), commandJson.getBoolean("WifiEnable"));
            } else if (commandJson.has("IotScanInfo")){
                WifiAPI.onReceiveIotScanInfo(getApplicationContext());
            }
              else if(commandJson.has("WifiConnectToSid")) {
                String networkPass = null;
                String networkType = null;
                if(commandJson.has("WifiNetworkPass")){
                    networkPass = commandJson.getString("WifiNetworkPass");
                }
                if(commandJson.has("WifiNetworkType")){
                    networkType = commandJson.getString("WifiNetworkType");
                }
                WifiReceiver.IotDeviceSsidToConnect = "";
                WifiAPI.onReceiveWifiConnectToSid(
                        getApplicationContext(),
                        commandJson.getString("WifiConnectToSid"),
                        networkPass,
                        networkType);
            }
              else if(commandJson.has("WifiConnectTheDevice")) {
                String networkPass = "";
                if(commandJson.has("WifiNetworkPass")){
                    networkPass = commandJson.getString("WifiNetworkPass");
                }
                WifiReceiver.IotDeviceSsidToConnect = commandJson.getString("WifiConnectTheDevice");
                WifiReceiver.IotSSIdToSettings = commandJson.getString("WifiNetworkSsid");
                WifiReceiver.IotPassToSettings = networkPass;
                WifiReceiver.IotNameToSettings = commandJson.getString("IotName");
                WifiReceiver.appContext = getApplicationContext();
                WifiReceiver.IotConnectionNumOfTry = 5;
                WifiAPI.onReceiveWifiConnectTheDevice(
                        getApplicationContext(),
                        commandJson.getString("WifiConnectTheDevice"),
                        commandJson.getString("WifiNetworkSsid"),
                        networkPass,
                        commandJson.getString("IotName"));
                Log.d(TAG, "WifiConnectTheDevice: " + commandJson.getString("WifiConnectTheDevice"));
                Log.d(TAG, "WifiNetworkSsid: " + commandJson.getString("WifiNetworkSsid"));
                Log.d(TAG, "networkPass: " + networkPass);
                Log.d(TAG, "IotName: " + commandJson.getString("IotName"));
            }
              else if(commandJson.has("tone")){
                    onTone(commandJson.getInt("tone"));
            }
              else if(commandJson.has("setupStorageSymlinks")){
                MountReceiver.setupStorageSymlinks(getApplicationContext());
            }



        }
        catch (JSONException ex) {
            Log.e(TAG, "Invalid JSON passed as a command: " + commandJson.toString());
            return false;
        }
        return true;
    }

    private boolean processCommand(String command) {
        Log.d(TAG, "processCommand Called");
        try {
            return processCommand(new JSONObject(command));
        }
        catch (JSONException ex) {
            Log.e(TAG, "Invalid JSON passed as a command: " + command);
            return false;
        }
    }

    private boolean processTTS(String text, String type) {
        Log.d(TAG, "processTTS Called: " + text);
        String textForReading = "";
        String voice = "";
        float pitch = 1;
        float rate = 1;

        // speak failed: not bound to TTS engine
        if (mTts == null){
            Log.w(TAG, "mTts == null");
            try {
                createTTS();
                mReadThisTextWhenReady = text;
                return true;
            }
            catch (Exception e) {
                Log.e(TAG, e.toString());
            }
        }



        try {
            JSONObject textJson = new JSONObject(text);
            try {
                if (textJson.has("text")) {
                    textForReading = textJson.getString("text");
                }
                if (textJson.has("pitch")) {
                    pitch = BigDecimal.valueOf(textJson.getDouble("pitch")).floatValue();
                    mTts.setPitch(pitch);
                }
                if (textJson.has("rate")) {
                    rate = BigDecimal.valueOf(textJson.getDouble("rate")).floatValue();
                    mTts.setSpeechRate(rate);
                }
                if (textJson.has("voice")) {
                    voice = textJson.getString("voice");
                    Voice voiceobj = new Voice(
                            voice, new Locale("pl_PL"),
                            Voice.QUALITY_HIGH,
                            Voice.LATENCY_NORMAL,
                            false,
                            null);
                    mTts.setVoice(voiceobj);
                }

            }
            catch (JSONException ex) {
                Log.e(TAG, "Invalid JSON passed as a text: " + text);
                return false;
            }

        }
        catch (JSONException ex) {
            textForReading = text;
        }

        //textToSpeech can only cope with Strings with < 4000 characters
        int dividerLimit = 3900;
        if(textForReading.length() >= dividerLimit) {
            int textLength = textForReading.length();
            ArrayList<String> texts = new ArrayList<String>();
            int count = textLength / dividerLimit + ((textLength % dividerLimit == 0) ? 0 : 1);
            int start = 0;
            int end = textForReading.indexOf(" ", dividerLimit);
            for(int i = 1; i<=count; i++) {
                texts.add(textForReading.substring(start, end));
                start = end;
                if((start + dividerLimit) < textLength) {
                    end = textForReading.indexOf(" ", start + dividerLimit);
                } else {
                    end = textLength;
                }
            }
            for(int i=0; i<texts.size(); i++) {
                mTts.speak(texts.get(i), TextToSpeech.QUEUE_ADD, null,"123");
            }
        } else {
            mTts.speak(textForReading, TextToSpeech.QUEUE_FLUSH, null,"456");
        }

        Intent intent = new Intent(BROADCAST_ON_START_TEXT_TO_SPEECH);
        intent.putExtra(AisCoreUtils.COMMAND_LOG_TEXT, textForReading);
        intent.putExtra(AisCoreUtils.COMMAND_LOG_TYPE, type);
        LocalBroadcastManager bm = LocalBroadcastManager.getInstance(getApplicationContext());
        bm.sendBroadcast(intent);

        return true;

    }

    // publish the text to hass - this text will be displayed in app
    // and send back to frame to read...
    public static void publishSpeechText(String message) {
        Log.d("TTS", "publishSpeechText " + message);
        publishMessage(message, "speech_text");
    }


    private void publishSpeechCommand(String message) {
        Log.d(TAG, "publishSpeechCommand " + message);
        publishMessage(message, "speech_command");
    }

    private void publishKeyEvent(String event, String sound) {
        Log.d(TAG, "publishKeyEvent " + event);
        // sample message: {"KeyCode":24,"Action":0,"DownTime":5853415}
            if (!sound.equals("0")){
                Log.i(TAG, "long press - play sound efect");
                toneGenerator.startTone(ToneGenerator.TONE_CDMA_PIP,150);
            }
        publishMessage(event, "key_command");
    }

    private void publishAudioPlayerStatus(String status) {
        Log.d(TAG, "publishAudioPlayerStatus: " + status);
        publishMessage(status, "player_status");
    }

    private void publishAudioPlayerSpeed(String speed) {
        Log.d(TAG, "publishAudioPlayerSpeed: " + speed);
        publishMessage(speed, "player_speed");
    }


    private void publishSpeechStatus(String status) {
        Log.d(TAG, "publishSpeechStatus: " + status);
        publishMessage(status, "speech_status");
    }


    // AUDIO
    private void playAudio(String streamUrl){
        Log.d(TAG, "playAudio Called: " + streamUrl);
        try {
            exoPlayer.stop();
            mediaSource = new ExtractorMediaSource(Uri.parse(streamUrl),
                    dataSourceFactory,
                    extractorsFactory,
                    mainHandler,
                    null);
            exoPlayer.prepare(mediaSource);
            exoPlayer.setPlayWhenReady(true);
        } catch(Exception e) {
            Log.e(TAG,"Error playAudio: " + e.getMessage());
        }
    }

    // ConcatenatingMediaSource
    private void playAudioList(JSONArray streamUrlsList){
        Log.d(TAG, "playAudioList Called: " + streamUrlsList.toString());
        // TODO ConcatenatingMediaSource
        // https://github.com/vrazumovsky/ExoPlayerDemo/blob/ee66be6e0badf5356177105522ab47c9f68fc094/app/src/main/java/razumovsky/ru/exoplayerdemo/MainActivity.java
    }

    private void pauseAudio(Boolean pause){
        Log.d(TAG, "pauseAudio Called: " + pause.toString());
        try {
            exoPlayer.setPlayWhenReady(!pause);
        } catch(Exception e) {
            Log.e(TAG,"Error pauseAudio: " + e.getMessage());
        }

    }

    private void stopAudio(){
        Log.d(TAG, "stopAudio Called ");
        try {
            exoPlayer.stop();
        } catch(Exception e) {
            Log.e(TAG,"Error stopAudio: " + e.getMessage());
        }
    }

    private void setVolume(float vol){
        Log.d(TAG, "setVolume Called: " + vol);
        exoPlayer.setVolume(vol);
    }

    private void turnDownVolumeForSTT(){
        Log.d(TAG, "turnDownVolumeForSTT Called ");
//        float vol = exoPlayer.getVolume();
//        vol = vol / 5f;
//        Log.d(TAG, "turnDownVolumeForSTT: " + vol);
        float vol = 0.2f;
        setVolume(vol);

    }

    private void turnUpVolumeForSTT(){
        Log.d(TAG, "turnUpVolumeForSTT Called ");
//        float vol = exoPlayer.getVolume();
//        vol = vol * 5f;
//        vol = Math.min(vol, 1.0f);
//        Log.d(TAG, "turnUpVolumeForSTT: " + vol);
        float vol = 0.6f;
        setVolume(vol);

    }

    private void turnDownVolumeForTTS(){
        Log.d(TAG, "turnDownVolumeForTTS Called ");
//        float vol = exoPlayer.getVolume();
//        vol = vol / 2f;
//        Log.d(TAG, "turnDownVolumeForTTS: " + vol);
        float vol = 0.2f;
        setVolume(vol);

    }

    private void turnUpVolumeForTTS(){
        Log.d(TAG, "turnUpVolumeForTTS Called ");
//        float vol = exoPlayer.getVolume();
//        vol = vol * 2f;
//        vol = Math.min(vol, 1.0f);
//        Log.d(TAG, "turnUpVolumeForTTS: " + vol);
        float vol = 1.0f;
        setVolume(vol);

    }

    private float getVolume(){
        Log.d(TAG, "getVolume Called ");
        return exoPlayer.getVolume();
    }

    private void setPlaybackPitch(float pitch){
        Log.d(TAG, "setPlaybackPitch Called ");
        PlaybackParameters pp = new  PlaybackParameters(1.0f, pitch);
        exoPlayer.setPlaybackParameters(pp);
    }

    private void setPlaybackSpeed(float speed){
        PlaybackParameters pp = new  PlaybackParameters(speed, 1.0f);
        exoPlayer.setPlaybackParameters(pp);
        Log.d(TAG, "setPlaybackSpeed speed: " + speed);
        JSONObject jState = new JSONObject();
        try {
            jState.put("currentSpeed", speed);
        } catch(JSONException e) { e.printStackTrace(); }
        publishAudioPlayerSpeed(jState.toString());
    }

    private void increasePlaybackSpeed(float speed){
        Log.d(TAG, "increasePlaybackSpeed Called ");
        PlaybackParameters old_pp = exoPlayer.getPlaybackParameters();
        float new_speed = old_pp.speed + speed;
        PlaybackParameters pp = new  PlaybackParameters(new_speed, 1.0f);
        exoPlayer.setPlaybackParameters(pp);
        Log.d(TAG, "increasePlaybackSpeed new_speed: " + new_speed);
        JSONObject jState = new JSONObject();
        try {
            jState.put("currentSpeed", new_speed);
        } catch(JSONException e) { e.printStackTrace(); }
        publishAudioPlayerSpeed(jState.toString());
    }

    private void seekTo(long positionMs){
        exoPlayer.seekTo(exoPlayer.getCurrentPosition() + positionMs);
    }

    private void skipTo(long positionMs){
        exoPlayer.seekTo(positionMs);
    }

    private JSONObject getAudioStatus(){
        Log.d(TAG, "getAudioStatus Called ");
        int state = 0;

        try {
            state = exoPlayer.getPlaybackState();
        } catch(Exception e) {
            Log.e(TAG, "Error getAudioStatus: " + e.getMessage());
        }

        Log.d(TAG, "getAudioStatus -> " + state);

        JSONObject jState = new JSONObject();
        try {
            jState.put("currentStatus", state);
        } catch(JSONException e) { e.printStackTrace(); }
        try {
            jState.put("currentMedia", "Piosenka");
        } catch(JSONException e) { e.printStackTrace(); }
        try {
            jState.put("playing", exoPlayer.getPlayWhenReady());
        } catch(JSONException e) { e.printStackTrace(); }
        try {
            jState.put("currentVolume", exoPlayer.getVolume());
        } catch(JSONException e) { e.printStackTrace(); }
        try {
            jState.put("duration", exoPlayer.getDuration());
        } catch(JSONException e) { e.printStackTrace(); }
        try {
            jState.put("currentPosition", exoPlayer.getCurrentPosition());
        } catch(JSONException e) { e.printStackTrace(); }

        return jState;
    }


    // AUDIO end

    private void onTone(int tone){
        Log.i(TAG, "play tone " + tone);
        toneGenerator.startTone(tone,200);
    }
}
