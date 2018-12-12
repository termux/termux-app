package com.termux.dom.api;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.NetworkInfo;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.AsyncTask;
import android.util.Log;

import com.termux.dom.AisCoreUtils;
import com.termux.dom.AisPanelService;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.UnsupportedEncodingException;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.URLEncoder;
import java.util.List;
import java.util.concurrent.TimeUnit;

import static com.termux.dom.api.WifiAPI.onReceiveWifiConnectionInfo;

/**
 * Receives wifi changes and creates a notification when wifi connects to a network,
 * displaying the SSID and MAC address.
 */
public class WifiReceiver extends BroadcastReceiver {

    private final static String TAG = WifiReceiver.class.getSimpleName();
    private static String message = "";
    public static String IotDeviceSsidToConnect = "";
    public static String IotSSIdToSettings = "";
    public static String IotPassToSettings = "";
    public static String IotNameToSettings = "";
    public static Context appContext;
    public static int NetworkIdToReconnectAfterAddIot;
    public static int IotNetworkIdToDeleteAfterAddIot;
    public static int IotConnectionNumOfTry = 0;

    interface IotInterface {
        String cmnd(String url, String attrToCheck);
    }

    public static class SetIotSettingsTask extends AsyncTask<String, Void, String> {

        @Override
        protected String doInBackground(String... params) {
            IotInterface iot = new IotInterface() {
                public String cmnd(String url, String attrToCheck) {

                    // check if we have correct connection if not then exit
                    WifiManager manager = (WifiManager) appContext.getApplicationContext().getSystemService(Context.WIFI_SERVICE);
                    WifiInfo info = manager.getConnectionInfo();
                    if (info.getNetworkId() != IotNetworkIdToDeleteAfterAddIot){
                        Log.d(TAG, "wrong connection, info.getNetworkId(): " + info.getNetworkId());
                        return "";
                    }
                    //
                    URL obj = null;
                    Log.d(TAG, "wsRet url: " + url);
                    try {
                        obj = new URL("http://192.168.4.1/cm?cmnd=" + url);

                        HttpURLConnection con = (HttpURLConnection) obj.openConnection();
                        con.setRequestMethod("GET");
                        con.setRequestProperty("User-Agent", "Mozilla/5.0");

                        BufferedReader in = new BufferedReader(
                                new InputStreamReader(con.getInputStream()));
                        String inputLine;
                        StringBuffer response = new StringBuffer();
                        while ((inputLine = in.readLine()) != null) {
                            response.append(inputLine);
                        }
                        in.close();
                        if (con.getResponseCode() == HttpURLConnection.HTTP_OK){
                            return "ok";
                        }

                        return "nok";


                    } catch (Exception e) {
                        Log.e(TAG, e.toString());
                        return "";
                    }

                }
            };
            // step 0
            IotConnectionNumOfTry = IotConnectionNumOfTry -1;


            // step 1 call iot Using backlog
            String wsRet = "";

            try {
            wsRet = iot.cmnd(URLEncoder.encode(
                    "Backlog " + "FriendlyName1 " + IotNameToSettings + "; SSId1 " + IotSSIdToSettings + "; Password1 " + IotPassToSettings, "UTF-8"), "");
            } catch (UnsupportedEncodingException e) {
                return "Nazwa urządzenia, problem z kodowaniem.";
            }
            // validation
            if (wsRet.equals("ok")){
                return "ok";
            }
            Log.d(TAG, "wsRet LOOP:" + IotConnectionNumOfTry);
            return "Nie udało się przesłać ustawień do urządzenia, spróbuj ponownie.";
        }

        @Override
        protected void onPostExecute(String result) {
            Log.d(TAG, IotNameToSettings + " END !!!");
            Log.d(TAG, IotNameToSettings + " result: " + result);
            Log.d(TAG, "IotConnectionNumOfTry: " + IotConnectionNumOfTry);
            if (result.equals("ok") || IotConnectionNumOfTry < 1) {
                IotDeviceSsidToConnect = "";
                IotSSIdToSettings = "";
                IotPassToSettings = "";
                IotNameToSettings = "";
                WifiManager wifiManager = (WifiManager) appContext.getSystemService(Context.WIFI_SERVICE);
                List<WifiConfiguration> list = wifiManager.getConfiguredNetworks();
                for (WifiConfiguration i : list) {
                    Log.i(TAG, "SSID iotSsid: " + i.SSID);
                    Log.i(TAG, "SSID i: " + i.SSID);
                    Log.i(TAG, "SSID removing?: " + (i.networkId == NetworkIdToReconnectAfterAddIot));
                    if (i.networkId == IotNetworkIdToDeleteAfterAddIot) {
                        // disable and delete this iot connection
                        wifiManager.disableNetwork(i.networkId);
                        wifiManager.removeNetwork(i.networkId);
                        Log.i(TAG, "disable and delete this iot connection: " + i.networkId);
                    } else {
                        // reenable all other wifi network
                        if (i.networkId == NetworkIdToReconnectAfterAddIot) {
                            wifiManager.disconnect();
                            wifiManager.enableNetwork(i.networkId, true);
                            wifiManager.reconnect();
                            Log.d(TAG, "SSID reconnect: " + i.networkId);
                        } else {
                            wifiManager.enableNetwork(i.networkId, false);
                            Log.d(TAG, "SSID enableNetwork: " + i.networkId);
                        }
                    }
                }
                wifiManager.saveConfiguration();
                // publish the info to user
                if (result.equals("ok")) {
                    AisPanelService.publishSpeechText("Ustawienia pomyślnie przesłane do urządzenia, za chwilę Twoje nowe urządzenie będzie dostępne w systemie.");
                } else{
                    AisPanelService.publishSpeechText(result);
                }
                // just to be sure that wifi status massage will not cover this message
                try {
                    Log.d(TAG, "TimeUnit.SECONDS.sleep 6");
                    TimeUnit.SECONDS.sleep(4);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            } else {
                try {
                    WifiAPI.onReceiveWifiConnectTheDevice(
                            appContext,
                            IotDeviceSsidToConnect,
                            IotSSIdToSettings,
                            IotPassToSettings,
                            IotNameToSettings);
                } catch (JSONException e) {
                    e.printStackTrace();
                }
            }
        }
    }


    @Override
    public void onReceive(final Context context, final Intent intent) {

        // publish only if we are on box
        if (!AisCoreUtils.onBox()){
            return;
        }


        if (intent.getAction().equals("android.net.wifi.STATE_CHANGE")){

            // wifi state change
            NetworkInfo netInfo = (NetworkInfo) intent.getExtras().get("networkInfo");

            String ssid = netInfo.getExtraInfo();
            String type = netInfo.getTypeName();
            String state = String.valueOf(netInfo.getState());
            Log.d(TAG, "STATE_CHANGE ssid " + ssid);
            Log.d(TAG, "STATE_CHANGE type " + type);
            Log.d(TAG, "STATE_CHANGE state " + state);
            if (state.equals("CONNECTED")) {
                state = "połączono";
            } else if (state.equals("DISCONNECTED")){
                state = "rozłączono";
            } else if (state.equals("CONNECTING")){
                //state = "łączenie";
                return;
            } else if (state.equals("DISCONNECTING")){
                //state = "rozłączanie";
                return;
            } else if (state.equals("SUSPENDED")){
                //state = "łączenie";
                return;
            } else if (state.equals("UNKNOWN")){
                //state = "łączenie";
                return;
            }

            if (type.equals("WIFI") && !ssid.equals("<unknown ssid>")){
                // do not repeat the message to user
                if (!message.equals(ssid + " " + netInfo.isConnected())){
                    message = ssid + " " + netInfo.isConnected();
                    JSONObject jRet = new JSONObject();
                    try {
                        jRet.put("ssid", ssid);
                        jRet.put("state", state);
                    } catch (JSONException e) {
                        e.printStackTrace();
                    }
                    AisPanelService.publishMessage(jRet.toString(), "wifi_state_change_info");

                    // special case connect the iot device
                    if (("\"" + IotDeviceSsidToConnect + "\"").equals(ssid)  && netInfo.isConnected()){
                        // step 1 wait to be sure the device is connected
                        // problem with java.net.SocketException: Network is unreachable
                        Log.d(TAG, "TimeUnit.SECONDS.sleep 6");
                        try {
                            TimeUnit.SECONDS.sleep(6);
                        } catch (InterruptedException e) {
                            e.printStackTrace();
                        }

                        // 1. add the FriendlyName <name> - Set friendly name (32 chars max)
                        SetIotSettingsTask task = new SetIotSettingsTask();
                        task.execute(new String[] {});
                    }

                    try {
                        onReceiveWifiConnectionInfo(context);
                    } catch (JSONException e) {
                        e.printStackTrace();
                    }
                }
            }
        }

    }

}
