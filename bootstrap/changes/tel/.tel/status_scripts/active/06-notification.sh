#!/data/data/com.termux/files/usr/bin/bash
# Script to display latest notification in statusbar 
# For TEL 
# made by sealyj
width=$(tput cols)
newest=$(tail -n1 ~/.tel/usr/.notifs)
if [ "$newest" == '' ] ; then
	echo -e " No notifications availible - check termux-api permissions" 
else
	echo -e " ${newest:0:$width-4}"
fi

