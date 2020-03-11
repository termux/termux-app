#!/data/data/com.termux/files/usr/bin/bash
pretext=" CPU: " #text before the cpu string
posttext="" #text after the cpu string
cpu=$(grep 'cpu ' /proc/stat | awk '{usage=($2+$4)*100/($2+$4+$5)} END {print usage "%"}')

printf "%s %s %s" "${pretext}" "${cpu}" "${posttext}"
