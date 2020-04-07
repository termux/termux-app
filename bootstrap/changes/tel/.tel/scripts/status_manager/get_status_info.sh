#!/data/data/com.termux/files/usr/bin/bash

#TEL status script, gets executed by status_manager.sh to display in the top pane
#prints the output of all scripts stored in ~/.tel/status/, sorted by their filenames
#todo: add color options
. ~/.tel/configs/status.sh #load status config 
. ~/.tel/scripts/helpers.sh #load helper function

# this script is called by status_manager

SCRIPT_DIR=~/.tel/status/*.* #define script dir

#loop is handled by status_manager.sh

for script in $SCRIPT_DIR #iterate over all scripts in script dir
do
		output=""
		if [ $STATUS_CENTER = true ]; 
		then
			output=$(center_text "$(timeout $STATUS_TIMEOUT $script)" "${STATUS_SPACING_CHAR}") #print the text centered if enabled in the config, use the given spacing char
		else
			output="$(timeout $STATUS_TIMEOUT $script)" #print the text on the left
		fi
		if [ $STATUS_COLOR = true ];
		then
			echo "${output}" | lolcat -p "${STATUS_COLOR_SPREAD}"
			oldoutput="$output"
		else
			echo "${output}"
		fi
		#todo: add setting for printing on the right
done
