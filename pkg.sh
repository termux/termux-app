#!/data/data/com.termux/files/usr/bin/bash
set -e -u

# The services can be tested without installing pkg by setting the export flag of Termux Service in the manifest and using adb am
# Example commands -
# adb shell am startservice --user 0 --esa packages openssh,vim -a com.termux.install_packages com.termux/com.termux.app.TermuxService
# adb shell am startservice --user 0 --esa packages openssh,vim -a com.termux.uninstall_packages com.termux/com.termux.app.TermuxService
# adb shell am startservice --user 0 -a com.termux.list_packages com.termux/com.termux.app.TermuxService

show_help() {
	echo 'Usage: pkg command [arguments]'
	echo
	echo 'A tool for managing packages. Commands:'
	echo
	echo '  install    <packages>  - Install specified packages.'
	echo '  uninstall  <packages>  - Uninstall specified packages.'
	echo '  list                   - List installed packages.'
	echo
}

if [ $# = 0 ]; then
	show_help
	exit 1
fi

CMD="$1"
shift 1

install_packages() {
	local all_packages="$*"

	am startservice \
		--user 0 \
		--es source play-store \
		--esa packages "${all_packages// /,}" \
		-a com.termux.install_packages \
		com.termux/com.termux.app.TermuxService \
		>/dev/null
}

uninstall_packages() {
	local all_packages="$*"

	am startservice \
		--user 0 \
		--esa packages "${all_packages// /,}" \
		-a com.termux.uninstall_packages \
		com.termux/com.termux.app.TermuxService \
		>/dev/null
}

list_packages() {
	am startservice \
		--user 0 \
		-a com.termux.list_packages \
		com.termux/com.termux.app.TermuxService \
		>/dev/null
}

case "$CMD" in
	help) show_help ;;
	install) install_packages "$@" ;;
	uninstall) uninstall_packages "$@" ;;
	list-installed) list_packages ;;
	*)
		echo "Unknown command: '$CMD' (run 'pkg help' for usage information)"
		exit 1
		;;
esac
