#!/data/data/com.termux/files/usr/bin/bash

#TEL status script, gets executed by byobu in the top panel
#prints the output of all scripts stored in ~/.tel/status_scripts/active, sorted by their filenames
#todo: add color options
. ~/.tel/config.sh #load config 
. ~/.tel/helpers.sh #load helper functions
SCRIPT_DIR=~/.tel/status_scripts/active/* #define script dir
clear #clear the screen before reloading, do first to avoid latency on loading scripts
while [ true ] #run forever
do
	
	for script in $SCRIPT_DIR #iterate over all scripts in script dir
	do
		output=""
		if [ $STATUS_CENTER = true ]; 
		then
			output=$(center_text "$(. $script)" "${STATUS_SPACING_CHAR}") #print the text centered if enabled in the config, use the given spacing char
		else
			output="$(. $script)" #print the text on the left
		fi
		if [ $STATUS_COLOR = true ];
		then
			echo "${output}" | lolcat -p "${STATUS_COLOR_SPREAD}"
		else
			echo "${output}"
		fi
		#todo: add setting for printing on the right
	done
sleep $STATUS_RELOAD #sleep for the configured time
done
