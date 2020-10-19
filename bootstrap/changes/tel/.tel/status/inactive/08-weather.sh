#!/data/data/com.termux/files/usr/bin/bash
#should request json format output and parse to approp icons
icon="" #text before the string
weather_file=~/.tel/data/weather
file_time=$(stat --format='%Y' "$weather_file")
current_time=$(date +%s)
if [ ! -e "$weather_file" ] ; then
	weather=$(timeout 10 curl -s wttr.in/$LOCATION?\format="+%C+%t+%h+%r")
	echo $weather > $weather_file
	printf "%s" "${icon} ${weather} Humidity in: $LOCATION"
	exit 0
fi
if (( file_time < ( current_time - ( 60 * $WEATHER_CHECK ) ) )); then
	weather=$(timeout 10 curl -s wttr.in/$LOCATION?\format="+%C+%t+%h+%r")
	echo "$weather" > ~/.tel/data/weather
else
	weather=$(cat $weather_file)
fi
if [ "$weather" != '' ] ; then 
	printf "%s" "${icon} ${weather} Humidity in: $LOCATION"
else
	printf "%s" "${icon} Couldn't get weather for $LOCATION"
fi
