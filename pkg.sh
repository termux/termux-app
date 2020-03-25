#!/data/data/com.termux/files/usr/bin/bash
set -e -u

show_help() {
	echo 'Usage: pkg command [arguments]'
	echo ''
	echo 'A tool for managing packages. Commands:'
	echo ''
	echo '  install <packages>'
	exit 1
}

if [ $# = 0 ]; then
	show_help
fi

CMD="$1"
shift 1

install_packages() {
	ALL_PACKAGES="$@"
	am startservice \
		--user 0 \
		--esa packages "${ALL_PACKAGES// /,}" \
		-a com.termux.install_packages \
		com.termux/com.termux.app.TermuxService \
		> /dev/null
}

case "$CMD" in
	h*) show_help;;
	add|i*) install_packages "$@";;
	*) echo "Unknown command: '$CMD' (run 'pkg help' for usage information)"; exit 1;;
esac
