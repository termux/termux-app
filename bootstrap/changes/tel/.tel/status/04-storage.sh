#!/data/data/com.termux/files/usr/bin/bash
# Basic Storage script for TEL

errormsg="Error getting storage info, try running termux-setup-storage."
cmd=$(df -h ~/storage || echo $errormsg)
[[ $cmd == $errormsg ]] && echo $errormsg ; exit
intfree=$(echo $cmd | grep G | rev | cut -d " " -f4 | rev)
drive=$(echo $cmd | grep G | rev | cut -d " " -f1 | rev)
#percused=$(echo cmd | grep data/media | rev | cut -d " " -f2 | rev)
#external=
echo " $intfree free on $drive"
