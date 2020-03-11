#!/data/data/com.termux/files/usr/bin/sh
pretext="" #text before the time string
posttext="" #text after the time string
time=$(date '+%H:%M')

printf "%s %s %s" "${pretext}" "${time}" "${posttext}"