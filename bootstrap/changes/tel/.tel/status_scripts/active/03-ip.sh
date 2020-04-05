#!/data/data/com.termux/files/usr/bin/bash
#tel network status script
#v0.2 02/04/20
# updated by sealyj
#look into battery comment

wifiicon="" #text before the ip
dataicon=""  #" #
posttext="" #text after the ip fa

using_wlan=$(ip addr | grep 'state UP' -A2 | tail -n1 | awk '{print $2}' | cut -f1  -d'/')

if [ -z "$using_wlan" ]; then
	cell_info=$(termux-telephony-cellinfo)
        if [ -z "$cell_info" ] ; then
		echo "${dataicon} failed to get cell info"
	else
		nettype=$(echo "$cell_info" | jq '.[0] | .type')
		strength=$(echo "$cell_info" | jq '.[0] | .dbm')
		temp="${nettype%\"}" #remove quotes
		nettype="${temp#\"}"
       		echo -e "${dataicon}" "${nettype}" "${strength}dBm" "${posttext}"
	fi
else
	wifi_info=$(termux-wifi-connectioninfo)
        if [ -z "$wifi_info" ] ; then
		echo "${wifiicon} failed to get wifi info"
	else
		ip=$(echo "$wifi_info" | jq -r .ip)
		strength=$(echo "$wifi_info" | jq -r .rssi)
		speed=$(echo "$wifi_info" | jq -r .link_speed_mbps)
		ssid=$(echo "$wifi_info" | jq -r .ssid)
        	echo -e "${wifiicon}"  "${ip}" "- ${ssid} @" "${speed}mbps" "${strength}dBm" "${posttext}"
	
	fi

fi
