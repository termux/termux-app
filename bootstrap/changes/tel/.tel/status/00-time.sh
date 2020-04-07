#!/data/data/com.termux/files/usr/bin/bash
pretext="" #text before the time string
posttext="" #text after the time string
time=$(date '+%H:%M - %A %d %B' )

printf "%s %s %s" "${pretext}" "${time}" "${posttext}"