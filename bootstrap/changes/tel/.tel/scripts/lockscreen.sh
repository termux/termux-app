#!/usr/bin/env bash
###!/data/data/com.termux/files/usr/bin/bash	
if [ $POWER_SAVER_ACTIVE == false ] ; then
	exit 0
fi
running=$(pgrep -f status_manager.py)
notifs_running=$(pgrep -f get_notifications.py)

if [ ! -z "$notifs_running" ] ; then
	notifswasrunning='true'
	pkill -f 'python get_notifications.py'
else
	notifswasrunning='false'

fi

if [ ! -z "$running" ] ; then
	wasrunning='true'
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
#	tmux set-hook client-resized 'run-shell "~/.tel/scripts/lockscreen2.sh"'
	#register a hook here
	#tmux resize-pane -Z -t 1.1
	#tmux clock-mode -t 1.1
	tput cup $(tput lines) 0
	read -n 1 -s -r -p " [ Swipe up or Press any key to exit ]"
	#remove hook here
	tmux display-message ' [ Resuming TEL...]'
#	tmux set-hook -u client-resized
fi

if [ "$notifswasrunning" == "true" ] ; then
	nohup ~/.tel/scripts/status_manager/get_notifications.py > /dev/null 2>&1 & 
fi


if [ "$wasrunning" == "true" ] ; then
	tmux splitw -d -b -t 1.1 ~/.tel/scripts/status_manager/toggle_ui.sh
fi

exit 0
