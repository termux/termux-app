#!/bin/bash

subscribed_repositories() {
	local main_sources
	main_sources=$(grep -P '^\s*deb\s' "@TERMUX_PREFIX@/etc/apt/sources.list")

	if [ -n "$main_sources" ]; then
		echo "#### sources.list"
		echo "\`$main_sources\`"
	fi

	local filename repo_package supl_sources
	while read -r filename; do
		repo_package=$(dpkg -S "$filename" 2>/dev/null | cut -d : -f 1)
		supl_sources=$(grep -P '^\s*deb\s' "$filename")

		if [ -n "$supl_sources" ]; then
			if [ -n "$repo_package" ]; then
				echo "#### $repo_package (sources.list.d/$(basename "$filename"))"
			else
				echo "#### sources.list.d/$(basename "$filename")"
			fi
			echo "\`$supl_sources\`  "
		fi
	done < <(find "@TERMUX_PREFIX@/etc/apt/sources.list.d" -maxdepth 1 ! -type d)
}

updatable_packages() {
	local updatable

	if [ "$(id -u)" = "0" ]; then
		echo "Running as root. Cannot check updatable packages."
	else
		apt update >/dev/null 2>&1
		updatable=$(apt list --upgradable 2>/dev/null | tail -n +2)

		if [ -z "$updatable" ];then
			echo "All packages up to date"
		else
			echo $'```\n'"$updatable"$'\n```\n'
		fi
	fi
}

output="
### Subscribed Repositories

$(subscribed_repositories)
##


### Updatable Packages

$(updatable_packages)
##

"

echo "$output"
