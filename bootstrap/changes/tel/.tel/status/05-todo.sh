#!/usr/bin/env bash
# Script to display todo list in statusbar 
line1=$(head -n1 ~/.tel/data/todo)
#line2=$(head -n2 $HOME/todo.txt | tail -n1)
if [ "$line1" == '' ] ; then
	echo " To-do list is empty" 
else
	echo " ${line1}"
fi
exit 0
