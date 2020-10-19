#!/data/data/com.termux/files/usr/bin/bash
# used to wrap each status script
# $1 - script to wrap, $2 optional timeout
#source ~/.tel/configs/status.sh
[[ -z $1 ]] && echo "no file passed format is: script pathtoscript timeout termwidth" && exit 1
[[ -z $2 ]] && echo "no terminal width passed" && exit 1
while [ 1 ]
do
	output=$(timeout $STATUS_TIMEOUT $1 $2 || echo "$1 timed out")
	#output=$($1 || echo "$1 timed out")
#	[[ -z "${output}" ]] || echo ${output}
#	echo ${output}
	echo $output
	#echo -e "\033[2K"
	sleep $STATUS_SCRIPTS_SLEEP
done
