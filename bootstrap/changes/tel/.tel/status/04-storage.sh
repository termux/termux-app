#!/data/data/com.termux/files/usr/bin/bash
# Basic Storage script for TEL

errormsg="~/storage not found, please run termux-setup-storage."
cmd=$(df -h ~/storage || echo $errormsg)
if [ $cmd == $errormsg ]; then
       	echo $errormsg
else
intfree=$(echo $cmd | grep G | rev | cut -d " " -f4 | rev)
drive=$(echo $cmd | grep G | rev | cut -d " " -f1 | rev)
#percused=$(echo cmd | grep data/media | rev | cut -d " " -f2 | rev)
#external=
echo " $intfree free on $drive"
fi
