#!/data/data/com.termux/files/usr/bin/bash
# Handles TEL's status window
# v0.11 03/04/2020
# made by @SealyJ


# import tel stuff
source ~/.tel/configs/status.sh
source ~/.tel/scripts/helpers.sh #need wraptext function
source ~/.tel/scripts/colors.sh #currently unused
 
# run the welcome / startup animation script
~/.tel/scripts/status_manager/welcome.sh

# does user want status pane?
[[ "$STATUS_ENABLED" == "false" ]] && exit 0

getnotifpath=~/.tel/scripts/status_manager/get_notifications.py
getstatusinfopath=~/.tel/scripts/status_manager/get_status_info.sh

notif_path=~/.tel/data/notifications

printf "\033c" #clear screen

if [ $NOTIFICATIONS == true ] ; then
	oldnotifs=""
	python $getnotifpath & # start notification listener as child
fi

while [ 1 ]
do
	status_rows=$(("$(ls ~/.tel/status | wc -w)" + 1))
	tmux resizep -t 0 -y $status_rows
	if [ "$NOTIFICATIONS" == true ] ; then
		notifs=$(cat $notif_path)
       		if [ "$notifs" != "$oldnotifs" ] ; then
			rows="$(tput lines)"
			#notif lines text shpuld be wrapped first for proper resize and delay
			notif_lines="$(wc -l $notif_path | cut -d' ' -f1)"
			status_rows=$(("$(ls  ~/.tel/status | wc -w)" + 1))
			if [ "$notif_lines" -gt "$rows" ]; then
				tmux resizep -t 0 -y $notif_lines
			fi
			printf "\033c" #clear screen
			center_text "Notifications:"
			echo "${notifs}"
	        	oldnotifs=$notifs
			sleep $notif_lines
			sleep 1
		else
                	status=$(. $getstatusinfopath)
	                printf "\033c" #clear screen
			echo "${status}"
			tmux clearhist -t 0
			sleep $STATUS_RELOAD
		fi
	else
                status=$(. $getstatusinfopath)
		printf "\033c" #clear screen
		echo "${status}"
		tmux clearhist -t 0
		sleep $STATUS_RELOAD
	fi
	done
