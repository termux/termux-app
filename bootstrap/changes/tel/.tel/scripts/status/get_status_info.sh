#!/data/data/com.termux/files/usr/bin/bash

#TEL status script, gets executed by byobu in the top panel
#prints the output of all scripts stored in ~/.tel/scripts/status/scripts, sorted by their filenames
#todo: add color options
. ~/.tel/configs/status.sh #load status config 
. ~/.tel/scripts/helpers.sh #load helper function

# this script is called by status_window

SCRIPT_DIR=~/.tel/scripts/status/scripts/* #define script dir

#loop now handled by ~/.tel/scripts/status/status_manager.sh
	
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
			#[[ "$oldoutput" != "$output" ]] && 
			echo "${output}" | lolcat -p "${STATUS_COLOR_SPREAD}"
			oldoutput="$output"
		else
		#	[[ "$oldoutput" != "$output" ]] && 
			echo "${output}"
			#oldoutput="$output > status.txt"
		#11	echo "$output >> $HOME/status.txt"
		fi
		#todo: add setting for printing on the right
done
