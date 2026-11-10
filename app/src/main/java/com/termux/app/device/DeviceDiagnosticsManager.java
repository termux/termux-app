package com.termux.app.device;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.pm.PackageManager;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkInfo;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.BatteryManager;
import android.os.Build;
import android.os.Environment;
import android.os.StatFs;
import android.os.SystemClock;
import android.telephony.TelephonyManager;
import android.util.Log;

import com.termux.shared.logger.Logger;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.lang.reflect.Method;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Locale;

/**
 * Manages device diagnostics - CPU, memory, storage, battery, thermal, network info.
 * All read-only, no privileged permissions required.
 */
public class DeviceDiagnosticsManager {

    public static final String LOG_TAG = "DeviceDiagnosticsManager";
    private final Context mContext;

    public DeviceDiagnosticsManager(Context context) {
        mContext = context.getApplicationContext();
    }

    public DeviceInfo getDeviceInfo() {
        DeviceInfo info = new DeviceInfo();
        info.cpu = getCpuInfo();
        info.memory = getMemoryInfo();
        info.storage = getStorageInfo();
        info.battery = getBatteryInfo();
        info.thermal = getThermalInfo();
        info.network = getNetworkInfo();
        info.system = getSystemInfo();
        return info;
    }

    private CpuInfo getCpuInfo() {
        CpuInfo cpu = new CpuInfo();
        cpu.architecture = Build.CPU_ABI;
        cpu.cores = Runtime.getRuntime().availableProcessors();

        // Read CPU frequencies from /sys/devices/system/cpu/
        List<Long> frequencies = new ArrayList<>();
        for (int i = 0; i < cpu.cores; i++) {
            String path = "/sys/devices/system/cpu/cpu" + i + "/cpufreq/scaling_cur_freq";
            try {
                String freqStr = readFile(path);
                if (freqStr != null) {
                    frequencies.add(Long.parseLong(freqStr.trim()) * 1000L); // Convert kHz to Hz
                }
            } catch (Exception e) {
                // Ignore
            }
        }
        cpu.currentFrequencies = frequencies;

        // Max frequencies
        List<Long> maxFrequencies = new ArrayList<>();
        for (int i = 0; i < cpu.cores; i++) {
            String path = "/sys/devices/system/cpu/cpu" + i + "/cpufreq/cpuinfo_max_freq";
            try {
                String freqStr = readFile(path);
                if (freqStr != null) {
                    maxFrequencies.add(Long.parseLong(freqStr.trim()) * 1000L);
                }
            } catch (Exception e) {
                // Ignore
            }
        }
        cpu.maxFrequencies = maxFrequencies;

        // CPU model name from /proc/cpuinfo
        cpu.model = readCpuModel();

        return cpu;
    }

    private String readCpuModel() {
        try (BufferedReader reader = new BufferedReader(new FileReader("/proc/cpuinfo"))) {
            String line;
            while ((line = reader.readLine()) != null) {
                if (line.startsWith("Hardware") || line.startsWith("model name")) {
                    String[] parts = line.split(":");
                    if (parts.length > 1) {
                        return parts[1].trim();
                    }
                }
            }
        } catch (IOException e) {
            // Ignore
        }
        return "Unknown";
    }

    private MemoryInfo getMemoryInfo() {
        MemoryInfo mem = new MemoryInfo();
        android.app.ActivityManager.MemoryInfo mi = new android.app.ActivityManager.MemoryInfo();
        android.app.ActivityManager am = (android.app.ActivityManager) mContext.getSystemService(Context.ACTIVITY_SERVICE);
        if (am != null) {
            am.getMemoryInfo(mi);
            mem.totalMemory = mi.totalMem;
            mem.availableMemory = mi.availMem;
            mem.lowMemory = mi.lowMemory;
            mem.threshold = mi.threshold;
        }
        return mem;
    }

    private StorageInfo getStorageInfo() {
        StorageInfo storage = new StorageInfo();
        File dataDir = Environment.getDataDirectory();
        if (dataDir != null) {
            StatFs stat = new StatFs(dataDir.getPath());
            long blockSize = stat.getBlockSizeLong();
            storage.totalStorage = stat.getBlockCountLong() * blockSize;
            storage.freeStorage = stat.getAvailableBlocksLong() * blockSize;
            storage.usableStorage = stat.getFreeBlocksLong() * blockSize;
        }

        // External storage
        File externalDir = Environment.getExternalStorageDirectory();
        if (externalDir != null) {
            StatFs stat = new StatFs(externalDir.getPath());
            long blockSize = stat.getBlockSizeLong();
            storage.externalTotalStorage = stat.getBlockCountLong() * blockSize;
            storage.externalFreeStorage = stat.getAvailableBlocksLong() * blockSize;
        }
        return storage;
    }

    private BatteryInfo getBatteryInfo() {
        BatteryInfo battery = new BatteryInfo();
        BatteryManager bm = (BatteryManager) mContext.getSystemService(Context.BATTERY_SERVICE);
        if (bm != null) {
            battery.level = bm.getIntProperty(4);
            battery.status = getBatteryStatusString(bm.getIntProperty(5));
            battery.health = getBatteryHealthString(bm.getIntProperty(6));
            battery.technology = String.valueOf(bm.getIntProperty(7));
            battery.temperature = bm.getIntProperty(8) / 10.0; // Convert to Celsius
            battery.voltage = bm.getIntProperty(9) / 1000.0; // Convert to Volts
            battery.chargeCounter = bm.getIntProperty(10);
            battery.currentNow = bm.getIntProperty(11) / 1000000.0; // Convert to Amps
        }
        return battery;
    }

    private String getBatteryStatusString(int status) {
        switch (status) {
            case BatteryManager.BATTERY_STATUS_CHARGING: return "Charging";
            case BatteryManager.BATTERY_STATUS_DISCHARGING: return "Discharging";
            case BatteryManager.BATTERY_STATUS_FULL: return "Full";
            case BatteryManager.BATTERY_STATUS_NOT_CHARGING: return "Not charging";
            case BatteryManager.BATTERY_STATUS_UNKNOWN: return "Unknown";
            default: return "Unknown";
        }
    }

    private String getBatteryHealthString(int health) {
        switch (health) {
            case BatteryManager.BATTERY_HEALTH_GOOD: return "Good";
            case BatteryManager.BATTERY_HEALTH_OVERHEAT: return "Overheat limit";
            case BatteryManager.BATTERY_HEALTH_DEAD: return "Dead";
            case BatteryManager.BATTERY_HEALTH_OVER_VOLTAGE: return "Over voltage";
            case BatteryManager.BATTERY_HEALTH_UNSPECIFIED_FAILURE: return "Unspecified failure";
            case BatteryManager.BATTERY_HEALTH_COLD: return "Cold";
            default: return "Unknown";
        }
    }

    @SuppressLint("WrongConstant")
    private ThermalInfo getThermalInfo() {
        ThermalInfo thermal = new ThermalInfo();
        // Read thermal zones
        List<ThermalZone> zones = new ArrayList<>();
        File thermalDir = new File("/sys/class/thermal");
        if (thermalDir.exists() && thermalDir.isDirectory()) {
            File[] zoneDirs = thermalDir.listFiles((dir, name) -> name.startsWith("thermal_zone"));
            if (zoneDirs != null) {
                for (File zoneDir : zoneDirs) {
                    ThermalZone zone = new ThermalZone();
                    zone.name = readFile(new File(zoneDir, "type"));
                    String tempStr = readFile(new File(zoneDir, "temp"));
                    if (tempStr != null) {
                        try {
                            zone.temperature = Long.parseLong(tempStr.trim()) / 1000.0; // Convert millidegrees to Celsius
                        } catch (NumberFormatException e) {
                            // Ignore
                        }
                    }
                    zone.mode = readFile(new File(zoneDir, "mode"));
                    zones.add(zone);
                }
            }
        }
        thermal.zones = zones;

        // Overall thermal status via PowerManager (API 29+)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            try {
                android.os.PowerManager pm = (android.os.PowerManager) mContext.getSystemService(Context.POWER_SERVICE);
                if (pm != null) {
                    Class<?> thermalManagerClass = Class.forName("android.os.ThermalManager");
                    Method getThermalStatusMethod = thermalManagerClass.getMethod("getCurrentThermalStatus");
                    Object thermalManager = mContext.getSystemService("thermal");
                    if (thermalManager != null) {
                        int status = (int) getThermalStatusMethod.invoke(thermalManager);
                        thermal.status = getThermalStatusString(status);
                    }
                }
            } catch (Exception e) {
                // Ignore - thermal manager not available
            }
        }
        return thermal;
    }

    private String getThermalStatusString(int status) {
        switch (status) {
            case 0: return "Normal";
            case 1: return "Light";
            case 2: return "Moderate";
            case 3: return "Severe";
            case 4: return "Critical";
            case 5: return "Emergency";
            case 6: return "Shutdown";
            default: return "Unknown";
        }
    }

    @SuppressLint("MissingPermission")
    private NetworkInfo getNetworkInfo() {
        NetworkInfo net = new NetworkInfo();
        ConnectivityManager cm = (ConnectivityManager) mContext.getSystemService(Context.CONNECTIVITY_SERVICE);
        if (cm == null) return net;

        // Active network
        Network activeNetwork = null;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            activeNetwork = cm.getActiveNetwork();
        }
        if (activeNetwork != null) {
            NetworkCapabilities nc = cm.getNetworkCapabilities(activeNetwork);
            if (nc != null) {
                net.type = getNetworkTypeString(nc);
                net.isMetered = nc.hasCapability(NetworkCapabilities.NET_CAPABILITY_NOT_METERED) ? false : true;
                net.isVPN = nc.hasTransport(NetworkCapabilities.TRANSPORT_VPN);
            }
        }

        // Wi-Fi info
        WifiManager wifiManager = (WifiManager) mContext.getApplicationContext().getSystemService(Context.WIFI_SERVICE);
        if (wifiManager != null) {
            WifiInfo wifiInfo = wifiManager.getConnectionInfo();
            if (wifiInfo != null) {
                net.wifiSsid = wifiInfo.getSSID().replace("\"", "");
                net.wifiBssid = wifiInfo.getBSSID();
                net.wifiSignalStrength = WifiManager.calculateSignalLevel(wifiInfo.getRssi(), 5);
                net.wifiRssi = wifiInfo.getRssi();
                net.wifiLinkSpeed = wifiInfo.getLinkSpeed();
                net.wifiFrequency = wifiInfo.getFrequency();
            }
        }

        // Mobile network info
        TelephonyManager tm = (TelephonyManager) mContext.getSystemService(Context.TELEPHONY_SERVICE);
        if (tm != null) {
            net.mobileNetworkOperator = tm.getNetworkOperatorName();
            net.mobileNetworkType = getNetworkTypeName(tm.getNetworkType());
            net.mobileSignalStrength = getSignalStrength(tm);
        }

        // IP addresses
        net.ipAddresses = getIpAddresses();

        return net;
    }

    private String getNetworkTypeString(NetworkCapabilities nc) {
        if (nc.hasTransport(NetworkCapabilities.TRANSPORT_WIFI)) return "Wi-Fi";
        if (nc.hasTransport(NetworkCapabilities.TRANSPORT_CELLULAR)) return "Mobile";
        if (nc.hasTransport(NetworkCapabilities.TRANSPORT_ETHERNET)) return "Ethernet";
        if (nc.hasTransport(NetworkCapabilities.TRANSPORT_VPN)) return "VPN";
        if (nc.hasTransport(NetworkCapabilities.TRANSPORT_BLUETOOTH)) return "Bluetooth";
        return "Unknown";
    }

    private String getNetworkTypeName(int type) {
        switch (type) {
            case TelephonyManager.NETWORK_TYPE_GPRS: return "GPRS";
            case TelephonyManager.NETWORK_TYPE_EDGE: return "EDGE";
            case TelephonyManager.NETWORK_TYPE_UMTS: return "UMTS";
            case TelephonyManager.NETWORK_TYPE_HSDPA: return "HSDPA";
            case TelephonyManager.NETWORK_TYPE_HSUPA: return "HSUPA";
            case TelephonyManager.NETWORK_TYPE_HSPA: return "HSPA";
            case TelephonyManager.NETWORK_TYPE_HSPAP: return "HSPAP / LTE-CA";
            case TelephonyManager.NETWORK_TYPE_LTE: return "LTE";
            case TelephonyManager.NETWORK_TYPE_NR: return "5G NR";
            default: return "Unknown";
        }
    }

    private int getSignalStrength(TelephonyManager tm) {
        try {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                android.telephony.SignalStrength ss = tm.getSignalStrength();
                if (ss != null) {
                    return ss.getLevel(); // 0-4
                }
            }
        } catch (Exception e) {
            // Ignore
        }
        return -1;
    }

    private List<String> getIpAddresses() {
        List<String> ips = new ArrayList<>();
        try {
            java.util.Enumeration<java.net.NetworkInterface> interfaces = java.net.NetworkInterface.getNetworkInterfaces();
            while (interfaces.hasMoreElements()) {
                java.net.NetworkInterface ni = interfaces.nextElement();
                java.util.Enumeration<java.net.InetAddress> addresses = ni.getInetAddresses();
                while (addresses.hasMoreElements()) {
                    java.net.InetAddress addr = addresses.nextElement();
                    if (!addr.isLoopbackAddress() && addr instanceof java.net.Inet4Address) {
                        ips.add(addr.getHostAddress() + " (" + ni.getName() + ")");
                    }
                }
            }
        } catch (Exception e) {
            // Ignore
        }
        return ips;
    }

    private SystemInfo getSystemInfo() {
        SystemInfo sys = new SystemInfo();
        sys.kernelVersion = readFile("/proc/version");
        sys.androidVersion = Build.VERSION.RELEASE;
        sys.apiLevel = Build.VERSION.SDK_INT;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            sys.securityPatch = Build.VERSION.SECURITY_PATCH;
        } else {
            sys.securityPatch = "Unknown";
        }
        sys.buildNumber = Build.DISPLAY;
        sys.manufacturer = Build.MANUFACTURER;
        sys.model = Build.MODEL;
        sys.fingerprint = Build.FINGERPRINT;
        sys.hardware = Build.HARDWARE;
        sys.device = Build.DEVICE;
        sys.product = Build.PRODUCT;
        sys.brand = Build.BRAND;
        sys.uptime = formatUptime(SystemClock.elapsedRealtime());
        return sys;
    }

    private String formatUptime(long millis) {
        long seconds = millis / 1000;
        long days = seconds / 86400;
        long hours = (seconds % 86400) / 3600;
        long minutes = (seconds % 3600) / 60;
        return String.format(Locale.getDefault(), "%dd %dh %dm", days, hours, minutes);
    }

    private String readFile(String path) {
        return readFile(new File(path));
    }

    private String readFile(File file) {
        if (!file.exists() || !file.canRead()) return null;
        try (BufferedReader reader = new BufferedReader(new FileReader(file))) {
            return reader.readLine();
        } catch (IOException e) {
            return null;
        }
    }

    // Data classes
    public static class DeviceInfo {
        public CpuInfo cpu;
        public MemoryInfo memory;
        public StorageInfo storage;
        public BatteryInfo battery;
        public ThermalInfo thermal;
        public NetworkInfo network;
        public SystemInfo system;
    }

    public static class CpuInfo {
        public String architecture;
        public String model;
        public int cores;
        public List<Long> currentFrequencies;
        public List<Long> maxFrequencies;
    }

    public static class MemoryInfo {
        public long totalMemory;
        public long availableMemory;
        public boolean lowMemory;
        public long threshold;
    }

    public static class StorageInfo {
        public long totalStorage;
        public long freeStorage;
        public long usableStorage;
        public long externalTotalStorage;
        public long externalFreeStorage;
    }

    public static class BatteryInfo {
        public int level;
        public String status;
        public String health;
        public String technology;
        public double temperature;
        public double voltage;
        public int chargeCounter;
        public double currentNow;
    }

    public static class ThermalInfo {
        public String status;
        public List<ThermalZone> zones;
    }

    public static class ThermalZone {
        public String name;
        public double temperature;
        public String mode;
    }

    public static class NetworkInfo {
        public String type;
        public boolean isMetered;
        public boolean isVPN;
        public String wifiSsid;
        public String wifiBssid;
        public int wifiSignalStrength;
        public int wifiRssi;
        public int wifiLinkSpeed;
        public int wifiFrequency;
        public String mobileNetworkOperator;
        public String mobileNetworkType;
        public int mobileSignalStrength;
        public List<String> ipAddresses;
    }

    public static class SystemInfo {
        public String kernelVersion;
        public String androidVersion;
        public int apiLevel;
        public String securityPatch;
        public String buildNumber;
        public String manufacturer;
        public String model;
        public String fingerprint;
        public String hardware;
        public String device;
        public String product;
        public String brand;
        public String uptime;
    }
}
