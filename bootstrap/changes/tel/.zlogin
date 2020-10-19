if [ -z $TMUX ] ; then # Tmux not running?
	~/.tel/scripts/hints.sh
	tput init # init the terminfo file using $TERM
	tmux new -d -n "$NAME" -s "TEL"
	tmux send-keys '~/.tel/scripts/startup.sh' 'C-m'
 	sleep 0.7 # wait for tmux startup
	tmux a
fi
