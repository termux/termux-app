#!/usr/bin/env bash

if [ $POWER_SAVER_ACTIVE == false ] ; then
	exit 0
fi

running=$(pgrep -f status_manager.py)
notifs_running=$(pgrep -f get_notifications.py)
powerline_running=$(pgrep -f powerline-daemon)

if [ ! -z "$notifs_running" ] ; then
	notifswasrunning='true'
	pkill -f 'python get_notifications.py'
else
	notifswasrunning='false'

fi

if [ ! -z "$powerline_running" ] ; then
	powerlinewasrunning='true'
	pkill -f 'powerline-daemon'
else
	powerlinewasrunning='false'

fi
if [ ! -z "$running" ] ; then
	wasrunning='true'
	~/.tel/scripts/status_manager/toggle_ui.sh
	pkill -f 'status_manager.py'
	apirunning='true'
	while [ "$apirunning" = 'true' ] ;
	do
		isapirunning=$(pgrep termux-api)
		if [ ! -z "$isapirunning" ]; then
			apirunning='true'
			pkill -f 'termux-api'
		else
			apirunning='false'
		fi
	done
else
	wasrunning='false'

fi


if [ $notifswasrunning == "true" ] || [ $wasrunning == "true" ] ; then
	#figlet -f shadow 'TEL' #| lolcat
	echo "
█████  ██████  ██
 ██    ████    ██
 ██    ██████  ██████" | lolcat -a --speed=150 -p 0.5
	echo -e "\nis in low power mode...
	"
	$POWER_SAVER_DISPLAY_COMMAND
	#use a tmux wait / set-hook onresize here
	tput cup $(tput lines) 0
	read -n 1 -s -r -p " [ Swipe up or Press any key to exit ]"
	tmux display-message ' [ Resuming TEL...]'
fi

if [ "$notifswasrunning" == "true" ] ; then
	nohup ~/.tel/scripts/get_notifications.py > /dev/null 2>&1 & 
fi

if [ "$powerlinewasrunning" == "true" ] ; then
	powerline-daemon -q
fi

if [ "$wasrunning" == "true" ] ; then
	nohup ~/.tel/scripts/status_manager/toggle_ui.sh > /dev/null 2>&1 &
fi

exit 0
