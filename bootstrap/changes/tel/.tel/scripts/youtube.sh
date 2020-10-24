#!/usr/bin/env bash
#script for starting / searching youtube with one alias
if [ "$#" == 0 ] ; then
	tel-app youtube || tel-app newpipe
else
	search_term=$@
	tel-search -yt "$search_term"
fi
