#!/usr/bin/env bash
# called to start / stop status manager window
running=$(pgrep -f status_manager.py)
if [ -z "$running" ] ; then
	# todo: link pane to display across all windows
	tmux new-window -n 'StatusManager' 'python ~/.tel/scripts/status_manager/status_manager.py'
	tmux join-pane -d -b -s 'StatusManager' -t 1.top #for each pane trigger joining of status pane
	tmux rename-window -t 1 $NAME 			# rename after spawn for uniqueness
	tmux select-pane -t $NAME.top -d	#disable input to status manager window
else
	kill "$(pgrep -f 'status_manager.py')"
	pkill -f 'termux-api' 
fi
