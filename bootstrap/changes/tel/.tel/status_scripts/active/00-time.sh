#!/data/data/com.termux/files/usr/bin/bash
pretext="" #text before the time string
posttext="" #text after the time string
<<<<<<< HEAD
time=$(date '+%H:%M - %A %d %B' )

printf "%s %s %s" "${pretext}" "${time}" "${posttext}"
=======
time=$(date '+%H:%M')

printf "%s %s %s" "${pretext}" "${time}" "${posttext}"
>>>>>>> 39bf067dea98098c25b87fb7aa669e15ff0a9549
