#!/data/data/com.termux/files/usr/bin/bash
# Script to display todo list in statusbar 
# For TEL 
# made by sealyj
width=$(tput cols)
COL='\033[0;35m'
NC='\033[0m' # No Col

line1=$(head -n1 ~/.tel/data/todo)
#line2=$(head -n2 $HOME/todo.txt | tail -n1)
#line3=$(head -n3 $HOME/todo.txt | tail -n1)
if [ "$line1" == '' ] ; then
	echo -e " To-do list is empty" 
else
	echo -e " ${line1:0:$width}"
fi

