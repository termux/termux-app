#!/usr/bin/env bash
# TEL Startup file
# these commands are ran only once when a session starts
# this file will be replaced with each update so modifications are not recommended here
clear
export NOTIFICATION_SCROLL=0
CHECK_MARK="\033[0;32m\xE2\x9C\x94\033[0m"
echo -n "\e[4mLoading Things\e[0m" $'\r'

pipkgs="$(python -m site --user-site)"
echo "$pipkgs" > ~/.tel/data/.py_site_pkgs || echo 'Error getting python install path' #set python packages path


sleep 0.1
echo -n "reading user configs..                            " $'\r'
source ~/.tel/scripts/readconfigs.sh
echo -ne "all configs sourced ${CHECK_MARK}                " $'\r'
sleep 0.1
if [ $SSH_SERVER == "true" ] ; then
	echo -n "launching ssh server - SECURITY WARNING!  " $'\r'
	sleep 0.2
	sshd
	echo -ne "launched ssh server ${CHECK_MARK}         " $'\r'
	sleep 2.2
fi

if [ "$NOTIFICATIONS_ENABLED" == "true" ] ; then
	echo -n "launching notification daemon              " $'\r'
	nohup ~/.tel/scripts/status_manager/get_notifications.py > /dev/null 2>&1 &
	echo -ne "launched notification daemon ${CHECK_MARK}" $'\r'
	sleep 0.2
fi

if [ "$STARTUP_ANIMATION_ENABLED" == "true" ] ; then
	echo -n 'launching animation                         ' $'\r'
	sleep 0.1
	tmux new-window -n 'Animation' 'python ~/.tel/scripts/animation.py'
	echo -ne "launched python animation ${CHECK_MARK}    " $'\r'
fi

if [ "$STATUS_WINDOW_ENABLED" == "true" ] ; then
	echo -n 'launching status manager                    ' $'\r'
	tmux splitw -t 1.1 -d -b '~/.tel/scripts/status_manager/toggle_ui.sh'
	echo -ne "launched status manager ${CHECK_MARK}      " $'\r'
	sleep 0.2
fi

if [ $REBUILD_CACHES_STARTUP == "true" ]; then
	echo -n "populating app cache..                       " $'\r'
	timeout 15 ~/.tel/bin/tel-app -u || echo "Failed to build app cache"
	echo -ne "apps generated ${CHECK_MARK}                "  $'\r'
	sleep 0.2

	echo -n "populating contacts cache..                  " $'\r'
	timeout 15 ~/.tel/bin/tel-dialer -u || echo "Failed to build contacts cache"
	echo -ne "contacts generated ${CHECK_MARK}            " $'\r'
	sleep 0.2
fi
# start user scripts
sh /storage/emulated/0/tel/tel_startup.sh

echo -ne "Ready!      ${CHECK_MARK}                " $'\r'
sleep 0.2
exit 0
