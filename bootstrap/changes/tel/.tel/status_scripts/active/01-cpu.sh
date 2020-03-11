#!/data/data/com.termux/files/usr/bin/sh
pretext=" CPU: " #text before the cpu string
posttext="" #text after the cpu string
cpu=$(mpstat | awk '$12 ~ /[0-9.]+/ { print 100 - $12"%%" }')

printf "%s %s %s" "${pretext}" "${cpu}" "${posttext}"
