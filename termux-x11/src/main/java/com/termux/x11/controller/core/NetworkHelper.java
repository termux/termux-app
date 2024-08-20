package com.termux.x11.controller.core;

import android.content.Context;
import android.net.DhcpInfo;
import android.net.wifi.WifiManager;

import java.net.InetAddress;
import java.net.InterfaceAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.net.UnknownHostException;

public class NetworkHelper {
    private final WifiManager wifiManager;

    public NetworkHelper(Context context) {
        wifiManager = (WifiManager)context.getSystemService(Context.WIFI_SERVICE);
    }

    public int getIpAddress() {
        return wifiManager != null ? wifiManager.getConnectionInfo().getIpAddress() : 0;
    }

    public int getNetmask() {
        if (wifiManager == null) return 0;

        DhcpInfo dhcpInfo = wifiManager.getDhcpInfo();
        if (dhcpInfo == null) return 0;

        int netmask = Integer.bitCount(dhcpInfo.netmask);
        if (dhcpInfo.netmask < 8 || dhcpInfo.netmask > 32) {
            try {
                InetAddress inetAddress = InetAddress.getByName(formatIpAddress(getIpAddress()));
                NetworkInterface networkInterface = NetworkInterface.getByInetAddress(inetAddress);
                if (networkInterface != null) {
                    for (InterfaceAddress address : networkInterface.getInterfaceAddresses()) {
                        if (inetAddress != null && inetAddress.equals(address.getAddress())) {
                            netmask = address.getNetworkPrefixLength();
                            break;
                        }
                    }
                }
            }
            catch (SocketException | UnknownHostException ignored) {}
        }

        return netmask;
    }

    public int getGateway() {
        if (wifiManager == null) return 0;
        DhcpInfo dhcpInfo = wifiManager.getDhcpInfo();
        return dhcpInfo != null ? dhcpInfo.gateway : 0;
    }

    public static String formatIpAddress(int ipAddress) {
        return (ipAddress & 255)+"."+((ipAddress >> 8) & 255)+"."+((ipAddress >> 16) & 255)+"."+((ipAddress >> 24) & 255);
    }

    public static String formatNetmask(int netmask) {
        return netmask == 24 ? "255.255.255.0" : (netmask == 16 ? "255.255.0.0" : (netmask == 8 ? "255.0.0.0" : "0.0.0.0"));
    }
}
