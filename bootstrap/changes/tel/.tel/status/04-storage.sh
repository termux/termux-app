#!/data/data/com.termux/files/usr/bin/bash
# Basic Storage script for TEL
# SealyJ
intfree=$(df -h | grep data/media | rev | cut -d " " -f4 | rev)
drive=$(df -h | grep data/media | rev | cut -d " " -f1 | rev)
percused=$(df -h | grep data/media | rev | cut -d " " -f2 | rev)
#external=
echo " $intfree free on $drive"
