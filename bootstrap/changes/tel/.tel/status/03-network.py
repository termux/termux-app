#!/usr/bin/env python
#tel network status script
#v0.4 24/04/20
import os
import json
import blessed
import subprocess
from blessed import Terminal
term = Terminal(force_styling=True) #force required if output not a tty
dataicon = ""  #" #
wifiicon = ""
nodataicon = ""
try:
    os.system("pkill -f 'WifiConnectionInfo'")
#    killprev = json.loads(subprocess.check_output(["killall", "termux-wifi-connectioninfo"], universal_newlines=True, timeout=5))
    wlan = json.loads(subprocess.check_output(["termux-wifi-connectioninfo"], universal_newlines=True, timeout=5))
    if wlan["supplicant_state"] == "COMPLETED":
        if wlan["rssi"] <= 0 and wlan["rssi"] >= -45:
            strengthcol = term.green
        elif wlan["rssi"] < -45 and wlan["rssi"] >= -55:
            strengthcol = term.greenyellow
        elif wlan["rssi"] < -55 and wlan["rssi"] >= -65:
           strengthcol = term.yellow
        elif wlan["rssi"] < -65 and wlan["rssi"] >= -75:
            strengthcol = term.orange
        elif wlan["rssi"] < -75 and wlan["rssi"] >= -85:
            strengthcol = term.orangered
        elif wlan["rssi"] < -85:
            strengthcol = term.red
        wifi_str = strengthcol + wifiicon + term.normal + " " + wlan["ip"] + " @ " + wlan["ssid"] +" " + str(wlan["link_speed_mbps"]) + "mbps " + str(wlan["rssi"]) + 'dBm' 
        print(wifi_str)
    else: 
        # if wlan fails then check data
  #      killprev = json.loads(subprocess.check_output(["killall", "termux-telephony-deviceinfo"], universal_newlines=True, timeout=5))
        data = json.loads(subprocess.check_output(["termux-telephony-deviceinfo"], universal_newlines=True, timeout=5))
        if data["data_state"] == "connected":
         #   print(data)
            if data["data_activity"] == "none":
                activity_icon = ""
            elif data["data_activity"] == "dormant":
                activity_icon = "鈴"
            elif data["data_activity"] == "in":
                activity_icon = ""
            elif data["data_activity"] == "out":
                activity_icon = ""
            elif data["data_activity"] == "inout":
                activity_icon = ""

            print(dataicon + ' ' + data["network_type"] + ' ' + data["network_operator_name"] + ' ' + activity_icon)
        else:
            print(nodataicon + ' Disconnected')
except:
    print(nodataicon + ' getting data...')
    #exit() #show last data instead of useless error msg
exit()
