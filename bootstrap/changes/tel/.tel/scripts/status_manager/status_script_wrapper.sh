#!/usr/bin/env bash
# used to wrap each status script
# $1 - script to wrap, $2 optional timeout
[[ -z $1 ]] && echo "no file passed format is: script pathtoscript timeout termwidth" && exit 1
[[ -z $2 ]] && echo "no terminal width passed" && exit 1
while [ 1 ]
do
	output=$(timeout $STATUS_TIMEOUT $1 $2 || echo "$1 timed out")
	echo $output
	sleep $STATUS_SCRIPTS_SLEEP
done
