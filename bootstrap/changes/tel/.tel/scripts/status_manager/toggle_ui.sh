#!/usr/bin/env bash
running=$(pgrep -f status_manager.py)
if [ -z "$running" ] ; then
	tmux new-window -n 'StatusManager' 'python ~/.tel/scripts/status_manager/status_manager.py'
	tmux join-pane -d -b -s 'StatusManager' -t 1.top
else
	kill "$(pgrep -f 'status_manager.py')"
	pkill -f 'termux-api' 
fi
