#!/usr/bin/env bash
running=$(pgrep -f status_manager.py)
if [ -z "$running" ] ; then
	# todo: link pane to display across all windows
	tmux new-window -n 'StatusManager' 'python ~/.tel/scripts/status_manager/status_manager.py'
	tmux join-pane -d -b -s 'StatusManager' -t 1.top #for each pane trigger joining of status pane
	tmux rename-window -t 1 $NAME 			# rename after spawn for uniqueness
	tmux select-pane -t 1.top -T 'Status' -d	#disable input to status manager window
	tmux set-window-option -t 1 aggressive-resize off  # attempt to lock pane height
else
	kill "$(pgrep -f 'status_manager.py')"
	pkill -f 'termux-api' 
fi
