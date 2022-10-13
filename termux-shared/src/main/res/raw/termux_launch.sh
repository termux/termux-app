#!/data/data/com.termux/files/usr/bin/bash

#  copyright thewisenerd <thewisenerd@protonmail.com>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#  MA 02110-1301, USA.
#

APPS_FILE="$HOME/.apps"

GRAY='\033[1;30m'
BLUE='\033[0;34m'
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

usage() {
	echo 'usage: zx [-hl] app'
	echo '  use termux-setup-apps to refresh app cache'
	echo ''
	echo '  -h: show this message'
	echo '  -l: list all apps'
  echo '  -t: test if the app can be launched'
}

launch_app() {
  if command -v am > /dev/null; then
    line="$1"
    name=$( echo $line | cut -d'|' -f1 )
    activity=$( echo $line | cut -d'|' -f2 )
    package=$( echo $line | cut -d'|' -f3 )
    system=$( echo $line | cut -d'|' -f4 )

    printf "${NC}${BLUE}LAUNCH: ${NC}${GREEN}$name${NC} ${GRAY}$package${NC}\n"
    am start -n "$activity" --user 0
  fi
}

show_app() {
	line="$1"
	name=$( echo $line | cut -d'|' -f1 )
	activity=$( echo $line | cut -d'|' -f2 )
	package=$( echo $line | cut -d'|' -f3 )
	system=$( echo $line | cut -d'|' -f4 )

	printf "${NC}${GREEN}$name${NC} ${GRAY}$package${NC}\n"
}

find_matches() {
	cat "$APPS_FILE" | sed 's/\s/-/g' | awk 'BEGIN { FS = "|" } ; tolower($1) ~ /'${1}'/ { print }' | awk 'NF'
}

list() {
	echo "hey"
}

if [ "$#" == "0" ]; then
	usage
	exit
fi

if [[ "$1" == "-h" ]]; then
	usage
	exit
fi

if [[ "$1" == "-l" ]]; then
	while read -r line; do
		show_app "$line"
	done < "$APPS_FILE"
	exit
fi

if [[ "$1" == "-t" ]]; then
  shift;
  dry_run=1
fi

matches=$( find_matches "$1" )
count=$( echo "$matches" | wc -l )
if [[ "$matches" == "" ]]; then
	count="0"
fi

if [[ "$count" == "1" ]]; then
  if [[ -z "$dry_run" ]]; then
    launch_app "$matches"
  fi
elif [[ "$count" == "0" ]]; then
	[ -z "$dry_run" ] && >&2 printf "${NC}${RED}no match${NC}\n"
  exit 1
else
	while read -r line; do
		show_app "$line"
	done <<< "$matches"
  launch_app "$matches"
fi
