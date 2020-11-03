#!/usr/bin/env bash
# TEL Startup file
# these commands are ran only once when a session starts
# this file will be replaced with each update so modifications are not recommended here
clear
CHECK_MARK="\033[0;32m\xE2\x9C\x94\033[0m"
echo -n "\e[4mLoading Things\e[0m" $'\r'
sleep 0.1 # sleeps so line updates are actually visible to user
echo -n "reading user configs..                            " $'\r'
source ~/.tel/scripts/readconfigs.sh && echo -ne "all configs sourced ${CHECK_MARK}                " $'\r'
sleep 0.1

if  [ $MOTD_HINTS == "true" ] ; then # backup user motd and restore if hints disabled
	[[ ! -f ~/../usr/etc/.motd.bak ]] && cp -f ~/../usr/etc/motd ~/../usr/etc/.motd.bak && echo -ne "backed up and enabled motd hints system ${CHECK_MARK}         " $'\r' && sleep 1
	cp -f ~/../usr/etc/motd_hints ~/../usr/etc/motd && ~/.tel/scripts/hints.sh
else
	[[ -f ~/../usr/etc/.motd.bak ]] && mv -f ~/../usr/etc/.motd.bak ~/../usr/etc/motd && echo -ne "restored user motd ${CHECK_MARK}         " $'\r' && sleep 1
fi

if [ $SSH_SERVER == "true" ] ; then
	echo -n "launching ssh server - SECURITY WARNING!  " $'\r'
	sleep 0.2
	sshd
	echo -ne "launched ssh server ${CHECK_MARK}         " $'\r'
	sleep 2.2
fi

if [ "$NOTIFICATIONS_ENABLED" == "true" ] ; then
	echo -n "launching notification daemon              " $'\r'
	nohup ~/.tel/scripts/get_notifications.py > /dev/null 2>&1 &
	echo -ne "launched notification daemon ${CHECK_MARK}" $'\r'
	sleep 0.2
fi

if [ "$STARTUP_ANIMATION_ENABLED" == "true" ] ; then
	echo -n 'launching animation                         ' $'\r'
	sleep 0.1
	#tmux new-window -n '$ANIMATION_WINDOW_NAME' 'python ~/.tel/scripts/animation.py'
	python ~/.tel/scripts/animation.py
	echo -ne "launched python animation ${CHECK_MARK}    " $'\r'
fi

if [ "$STATUS_WINDOW_ENABLED" == "true" ] ; then
	echo -n 'launching status manager                    ' $'\r'
	#tmux splitw -t 1.1 -d -b '~/.tel/scripts/status_manager/toggle_ui.sh'
	nohup ~/.tel/scripts/status_manager/toggle_ui.sh > /dev/null 2>&1 &
	echo -ne "launched status manager ${CHECK_MARK}      " $'\r'
	sleep 0.2
fi

echo -ne "Ready!      ${CHECK_MARK}                " $'\r'
sleep 0.2
exit 0
