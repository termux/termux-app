#!/data/data/com.termux/files/usr/bin/bash
pretext="" #text before the ip
posttext="" #text after the ip
ip=$(ip addr | grep 'state UP' -A2 | tail -n1 | awk '{print $2}' | cut -f1  -d'/')

printf "%s %s %s" "${pretext}" "${ip}" "${posttext}"