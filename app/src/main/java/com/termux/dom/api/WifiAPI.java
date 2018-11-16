package com.termux.dom.api;

import android.app.IntentService;
import android.content.Context;
import android.location.LocationManager;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.text.format.Formatter;
import android.util.Log;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.Arrays;
import java.util.List;
import java.util.concurrent.TimeUnit;

import com.termux.dom.AisPanelService;

public class WifiAPI {

    private final static String TAG = WifiAPI.class.getSimpleName();

    public static void onReceiveWifiConnectionInfo(final Context context) throws JSONException {
        final Runnable runnable = new Runnable() {
            @Override
            public void run() {
                try {
                    JSONObject jState = new JSONObject();
                    WifiManager manager = (WifiManager) context.getApplicationContext().getSystemService(Context.WIFI_SERVICE);
                    WifiInfo info = manager.getConnectionInfo();
                    if (info == null) {
                        jState.put("API_ERROR", "No current connection");
                    } else {
                        jState.put("bssid", info.getBSSID());
                        jState.put("frequency_mhz", info.getFrequency());
                        jState.put("ip", Formatter.formatIpAddress(info.getIpAddress()));
                        jState.put("link_speed_mbps", info.getLinkSpeed());
                        jState.put("mac_address", info.getMacAddress());
                        jState.put("network_id", info.getNetworkId());
                        jState.put("rssi", info.getRssi());
                        jState.put("ssid", info.getSSID().replaceAll("\\\"", ""));
                        jState.put("ssid_hidden", info.getHiddenSSID());
                        jState.put("supplicant_state", info.getSupplicantState().toString());
                    }
                    // publish the info
                    AisPanelService.publishMessage(jState.toString(), "wifi_connection_info");
                } catch (Exception e) {
                    Log.e(TAG, "onReceiveWifiConnectionInfo " + e);
                }
            }
        };

        if (context instanceof IntentService) {
            runnable.run();
        } else {
            new Thread(runnable).start();
        }
    }

    static boolean isLocationEnabled(Context context) {
        LocationManager lm = (LocationManager) context.getSystemService(Context.LOCATION_SERVICE);
        return lm.isProviderEnabled(LocationManager.GPS_PROVIDER);
    }

    public static void onReceiveWifiScanInfo(final Context context, final String back_topic) throws JSONException {

        final Runnable runnable = new Runnable() {
            @Override
            public void run() {
                try {
                    onReceiveWifiEnable(context, true);
                    JSONObject jRet = new JSONObject();
                    JSONArray jsonArray = new JSONArray();
                    WifiManager manager = (WifiManager) context.getApplicationContext().getSystemService(Context.WIFI_SERVICE);
                    manager.startScan();
                    TimeUnit.SECONDS.sleep(3);
                    List<ScanResult> scans = manager.getScanResults();
                    if (scans == null) {
                        jRet.put("API_ERROR", "Failed getting scan results");
                    } else if (scans.isEmpty() && !isLocationEnabled(context)) {
                        // https://issuetracker.google.com/issues/37060483:
                        // "WifiManager#getScanResults() returns an empty array list if GPS is turned off"
                        jRet.put("API_ERROR", "Location needs to be enabled on the device");
                    } else {
                        for (ScanResult scan : scans) {
                            JSONObject jState = new JSONObject();
                            jState.put("bssid", scan.BSSID);
                            jState.put("frequency_mhz", scan.frequency);
                            jState.put("rssi", scan.level);
                            jState.put("ssid", scan.SSID);
                            jState.put("timestamp", scan.timestamp);
                            if (scan.capabilities.contains("WPA2")) {
                                jState.put("capabilities", "WPA2");
                            } else if (scan.capabilities.contains("WPA")) {
                                jState.put("capabilities", "WPA");
                            } else if (scan.capabilities.contains("WEP")) {
                                jState.put("capabilities", "WEP");
                            } else if (scan.capabilities.contains("Open")) {
                                jState.put("capabilities", "Open");
                            } else  {
                                jState.put("capabilities", scan.capabilities);
                            }

                            //  2.4
                            // if (Integer.toString(scan.frequency).startsWith("24")) {
                            jsonArray.put(jState);
                    }
                }

                jRet.put("ScanResult", jsonArray);
                // publish the info
                AisPanelService.publishMessage(jRet.toString(), back_topic);
                // publish the current connection info
                onReceiveWifiConnectionInfo(context);
                } catch (Exception e) {
                    Log.e(TAG, "onReceiveWifiScanInfo " + e);
                }
            }
        };

        if (context instanceof IntentService) {
            runnable.run();
        } else {
            new Thread(runnable).start();
        }
    }

    public static void onReceiveWifiEnable(final Context context, final Boolean enabled) {
        final Runnable runnable = new Runnable() {
            @Override
            public void run() {
                try {
                    WifiManager manager = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
                    manager.setWifiEnabled(enabled);
            } catch (Exception e) {
                    Log.e(TAG, "onReceiveWifiEnable " + e);
            }
          }
        };
        if (context instanceof IntentService) {
        runnable.run();
        } else {
            new Thread(runnable).start();
        }
    }

    // connect to given sid
    public static void onReceiveWifiConnectToSid(final Context context, final String wifiSsid, final String WifiPass, final String WifiType)  throws JSONException {
        final Runnable runnable = new Runnable() {
            @Override
            public void run() {
                try {
                    WifiManager wifiManager = (WifiManager)context.getSystemService(Context.WIFI_SERVICE);
                    if (!WifiPass.equals("") || WifiType.trim().equals("[ESS]") || WifiType.trim().equals("Open")) {
                        // create new connection
                        WifiConfiguration conf = new WifiConfiguration();
                        conf.SSID = "\"" + wifiSsid + "\"";   // Please note the quotes. String should contain ssid in quotes
                        String testWifiType = WifiType.trim();
                        if (testWifiType.equals("WEP")) {
                            // Then, for WEP network you need to do this:
                            conf.wepKeys[0] = "\"" + WifiPass + "\"";
                            conf.wepTxKeyIndex = 0;
                            conf.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.NONE);
                            conf.allowedGroupCiphers.set(WifiConfiguration.GroupCipher.WEP40);
                        } else if (testWifiType.equals("WPA")) {
                            // For WPA network you need to add passphrase like this:
                            conf.preSharedKey = "\"" + WifiPass + "\"";
                        } else if (testWifiType.equals("WPA2")) {
                            // For WPA2 network you need to add passphrase like this:
                            conf.preSharedKey = "\"" + WifiPass + "\"";
                            conf.status = WifiConfiguration.Status.ENABLED;
                            conf.allowedGroupCiphers.set(WifiConfiguration.GroupCipher.TKIP);
                            conf.allowedGroupCiphers.set(WifiConfiguration.GroupCipher.CCMP);
                            conf.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.WPA_PSK);
                            conf.allowedPairwiseCiphers.set(WifiConfiguration.PairwiseCipher.TKIP);
                            conf.allowedPairwiseCiphers.set(WifiConfiguration.PairwiseCipher.CCMP);
                            conf.allowedProtocols.set(WifiConfiguration.Protocol.RSN);
                        } else {
                            // For Open network you need to do this:
                            conf.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.NONE);
                        }
                        // Then, you need to add it to Android wifi manager settings:
                        int netId = wifiManager.addNetwork(conf);
                        wifiManager.disconnect();
                        wifiManager.enableNetwork(netId, true);
                        wifiManager.reconnect();
                    } else {
                        // connect to  network
                        List<WifiConfiguration> list = wifiManager.getConfiguredNetworks();
                        for (WifiConfiguration i : list) {
                            if (i.SSID != null && i.SSID.equals("\"" + wifiSsid + "\"")) {
                                wifiManager.disconnect();
                                wifiManager.enableNetwork(i.networkId, true);
                                wifiManager.reconnect();
                                break;
                            }
                        }
                    }

                    // publish the info back
                    onReceiveWifiScanInfo(context, "wifi_status_info");
                    // publish the current connection info
                    onReceiveWifiConnectionInfo(context);
                } catch (Exception e) {
                    Log.e(TAG, "onReceiveWifiConnectToSid " + e);
                }
            }
        };
        if (context instanceof IntentService) {
            runnable.run();
        } else {
            new Thread(runnable).start();
        }
    }

    public static String getModelFromHostname(final String module){
        List<String> suportedModulesList = Arrays.asList(
                "SONOFF_BASIC",
                "SONOFF_RF",
                "SONOFF_SV",
                "SONOFF_TH",
                "SONOFF_DUAL",
                "SONOFF_POW",
                "SONOFF_4CH",
                "S20",
                "SLAMPHER",
                "SONOFF_TOUCH",
                "SONOFF_LED",
                "CH1",
                "CH4",
                "MOTOR",
                "ELECTRODRAGON",
                "EXS_RELAY",
                "WION",
                "WEMOS",
                "SONOFF_DEV",
                "H801",
                "SONOFF_SC",
                "SONOFF_BN",
                "SONOFF_4CHPRO",
                "HUAFAN_SS",
                "SONOFF_BRIDGE",
                "SONOFF_B1",
                "AILIGHT",
                "SONOFF_T11",
                "SONOFF_T12",
                "SONOFF_T13",
                "SUPLA1",
                "WITTY",
                "YUNSHAN",
                "MAGICHOME",
                "LUANIHVIO",
                "KMC_70011",
                "ARILUX_LC01",
                "ARILUX_LC11",
                "SONOFF_DUAL_R2",
                "ARILUX_LC06",
                "SONOFF_S31",
                "ZENGGE_ZF_WF017",
                "SONOFF_POW_R2",
                "MAXMODULE",
                "SONOFF_IFAN");

        // taking the iot model from WIFI_HOSTNAME
        // dom_<model>_<Chip Id - last 6 characters of MAC address>-<last 4 decimal chars of MAC address>
        // dom_S20_24B87B-6267

        if (!module.startsWith("dom_")){
            return null;
        }

        int start_ = module.indexOf("_"); // 3
        int end_ = module.lastIndexOf("_");
        String model = module.substring(start_+1, end_).toUpperCase();
        if (!suportedModulesList.contains( model )){
            return null;
        }

        //
        return model.toLowerCase();
    }

    // onReceiveIotScanInfo
    public static void onReceiveIotScanInfo(final Context context) throws JSONException {
        final Runnable runnable = new Runnable() {
            @Override
            public void run() {
                try {
                    onReceiveWifiEnable(context, true);
                    JSONObject jRetWifi = new JSONObject();
                    JSONObject jRetIot = new JSONObject();
                    JSONArray jsonArrayWifi = new JSONArray();
                    JSONArray jsonArrayIot = new JSONArray();
                    WifiManager manager = (WifiManager) context.getApplicationContext().getSystemService(Context.WIFI_SERVICE);
                    manager.startScan();
                    // TODO ugly... i know...
                    TimeUnit.SECONDS.sleep(3);
                    List<ScanResult> scans = manager.getScanResults();
                    if (scans == null) {
                        jRetWifi.put("API_ERROR", "Failed getting scan results");
                    } else if (scans.isEmpty() && !isLocationEnabled(context)) {
                        // https://issuetracker.google.com/issues/37060483:
                        // "WifiManager#getScanResults() returns an empty array list if GPS is turned off"
                        jRetWifi.put("API_ERROR", "Location needs to be enabled on the device");
                    } else {
                        for (ScanResult scan : scans) {
                            JSONObject jState = new JSONObject();
                            jState.put("bssid", scan.BSSID);
                            jState.put("frequency_mhz", scan.frequency);
                            jState.put("rssi", scan.level);
                            jState.put("ssid", scan.SSID);
                            jState.put("timestamp", scan.timestamp);
                            if (scan.capabilities.contains("WPA2")) {
                                jState.put("capabilities", "WPA2");
                            } else if (scan.capabilities.contains("WPA")) {
                                jState.put("capabilities", "WPA");
                            } else if (scan.capabilities.contains("WEP")) {
                                jState.put("capabilities", "WEP");
                            } else if (scan.capabilities.contains("Open")) {
                                jState.put("capabilities", "Open");
                            } else  {
                                jState.put("capabilities", scan.capabilities);
                            }

                            // put only 2.4
                            if (Integer.toString(scan.frequency).startsWith("24")) {
                                // iot or wifi
                                String model = getModelFromHostname(scan.SSID);
                                if (model != null) {
                                    jState.put("model", model);
                                    jsonArrayIot.put(jState);
                                } else {
                                    jsonArrayWifi.put(jState);
                                }
                            }
                        }
                    }

                    // if iot is empty, try to scan again
                    if (jsonArrayIot.length() == 0) {
                        TimeUnit.SECONDS.sleep(4);
                        AisPanelService.publishSpeechText("skanuje ponownie...");
                        manager.startScan();
                        // TODO ugly... i know...
                        TimeUnit.SECONDS.sleep(3);
                        scans = manager.getScanResults();
                        if (scans == null) {
                            jRetWifi.put("API_ERROR", "Failed getting scan results");
                        } else if (scans.isEmpty() && !isLocationEnabled(context)) {
                            // https://issuetracker.google.com/issues/37060483:
                            // "WifiManager#getScanResults() returns an empty array list if GPS is turned off"
                            jRetWifi.put("API_ERROR", "Location needs to be enabled on the device");
                        } else {
                            for (ScanResult scan : scans) {
                                JSONObject jState = new JSONObject();
                                jState.put("bssid", scan.BSSID);
                                jState.put("frequency_mhz", scan.frequency);
                                jState.put("rssi", scan.level);
                                jState.put("ssid", scan.SSID);
                                jState.put("timestamp", scan.timestamp);
                                if (scan.capabilities.contains("WPA2")) {
                                    jState.put("capabilities", "WPA2");
                                } else if (scan.capabilities.contains("WPA")) {
                                    jState.put("capabilities", "WPA");
                                } else if (scan.capabilities.contains("WEP")) {
                                    jState.put("capabilities", "WEP");
                                } else if (scan.capabilities.contains("Open")) {
                                    jState.put("capabilities", "Open");
                                } else  {
                                    jState.put("capabilities", scan.capabilities);
                                }

                                // put only 2.4
                                if (Integer.toString(scan.frequency).startsWith("24")) {
                                    // iot or wifi
                                    String model = getModelFromHostname(scan.SSID);
                                    if (model != null) {
                                        jState.put("model", model);
                                        jsonArrayIot.put(jState);
                                    } else {
                                        jsonArrayWifi.put(jState);
                                    }
                                }
                            }
                        }
                    }
                    // TODO improve this ugly part

                    jRetWifi.put("ScanResult", jsonArrayWifi);
                    jRetIot.put("ScanResult", jsonArrayIot);
                    // publish the info about wifis
                    AisPanelService.publishMessage(jRetWifi.toString(), "wifi_status_info");
                    // publish the info about devices
                    AisPanelService.publishMessage(jRetIot.toString(), "iot_scan_info");
                    // publish the current connection info
                    onReceiveWifiConnectionInfo(context);
                } catch (Exception e) {
                    Log.e(TAG, "onReceiveWifiScanInfo " + e);
                }
            }
        };

        if (context instanceof IntentService) {
            runnable.run();
        } else {
            new Thread(runnable).start();
        }
    }


    // connect to given device
    public static void onReceiveWifiConnectTheDevice(final Context context, final String iotSsid, final String wifiSsid, final String WifiPass, final String IotName)  throws JSONException {
        final Runnable runnable = new Runnable() {
            @Override
            public void run() {
                try {
                    WifiManager wifiManager = (WifiManager)context.getSystemService(Context.WIFI_SERVICE);

                    // save the current connection - to reconnect after the device will be added
                    WifiInfo info = wifiManager.getConnectionInfo();
                    WifiReceiver.NetworkIdToReconnectAfterAddIot = info.getNetworkId();

                    // check if the connection exists
                    // if (("\"" + IotDeviceSsidToConn/ect + "\"").equals(ssid)
                    int netId = -1;
                    List<WifiConfiguration> list = wifiManager.getConfiguredNetworks();
                    for (WifiConfiguration i : list) {
                        Log.i(TAG, "SSID iotSsid: " + i.SSID);
                        Log.i(TAG, "SSID i: " + i.SSID);
                        Log.i(TAG, "SSID removing?: " + i.SSID.equals("\"" + iotSsid + "\""));
                        if (i.SSID != null && i.SSID.equals("\"" + iotSsid + "\"")) {
                            Log.d(TAG, "the connection exists, disable and delete this iot connection: " + i.networkId);
                            wifiManager.disableNetwork(i.networkId);
                            wifiManager.removeNetwork(i.networkId);
                            wifiManager.saveConfiguration();
                        }
                    }

                    // TODO check if we were able to delete the connection
                    //  Android M apps are not allowed to modify networks that they did not create



                    // disable all other wifi network for the time of connection
                    for (WifiConfiguration i : list) {
                        Log.d(TAG, "disable all other wifi network for the time of connection " + i.networkId + " ssid: " + i.SSID);
                        // disable all other wifi network for the time of connection
                        wifiManager.disableNetwork(i.networkId);
                    }

                    // create new connection
                    WifiConfiguration conf = new WifiConfiguration();
                    conf.SSID = "\"" + iotSsid + "\"";   // Please note the quotes. String should contain ssid in quotes
                    // For Open network you need to do this:
                    conf.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.NONE);
                    // Then, you need to add it to Android wifi manager settings:
                    netId = wifiManager.addNetwork(conf);
                    Log.d(TAG, "create new connection " + netId + " ssid: " + conf.SSID);

                    wifiManager.saveConfiguration();
                    wifiManager.disconnect();
                    Log.d(TAG, "enableNetwork " + netId);
                    wifiManager.enableNetwork(netId, true);
                    wifiManager.reconnect();

                    // this wifi iot connection will be deleted after the correct configuration
                    WifiReceiver.IotNetworkIdToDeleteAfterAddIot = netId;

                    // publish the current connection info
                    onReceiveWifiConnectionInfo(context);
                } catch (Exception e) {
                    Log.e(TAG, "onReceiveWifiConnectToSid " + e);
                }
            }
        };
        if (context instanceof IntentService) {
            runnable.run();
        } else {
            new Thread(runnable).start();
        }
    }

}
