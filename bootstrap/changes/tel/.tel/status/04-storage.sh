#!/data/data/com.termux/files/usr/bin/bash
# Basic Storage script for TEL
# SealyJ
cmd=$(df -h ~/storage)
intfree=$(echo cmd | grep data/media | rev | cut -d " " -f4 | rev)
drive=$(echo cmd | grep data/media | rev | cut -d " " -f1 | rev)
#percused=$(echo cmd | grep data/media | rev | cut -d " " -f2 | rev)
#external=
echo " $intfree free on $drive"
