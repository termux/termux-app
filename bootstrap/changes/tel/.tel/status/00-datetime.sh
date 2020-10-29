#!/usr/bin/env bash
pretext="" #text before the time string
posttext="" #text after the time string
#time=$(date '+%H:%M:%S - %A %d %B' )
time=$(date '+%H:%M - %A %d %B' )
echo "${pretext} ${time} ${posttext}"
