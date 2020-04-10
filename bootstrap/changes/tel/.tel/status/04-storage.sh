#!/data/data/com.termux/files/usr/bin/bash
# Basic Storage script for TEL

errormsg="~/storage not setup, run 'termux-setup-storage'"
if [ -e ~/storage ]; then
	cmd=$(df -h ~/storage)
	#drive=$(echo $cmd | grep G | rev | cut -d " " -f1 | rev)
	#percused=$(echo cmd | grep data/media | rev | cut -d " " -f2 | rev)
	intfree=$(echo $cmd | grep G | rev | cut -d " " -f4 | rev)
	echo " $intfree free on ~/storage"
else
       	echo "$errormsg"
fi
