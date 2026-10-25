#!/data/data/com.termux/files/usr/bin/bash
# fido: one command to run fido-bridge without babysitting it.
#
#   fido            start the bridge if needed (same as "fido up")
#   fido up [-v]    find the key, start the bridge in the background
#   fido down       stop the bridge
#   fido status     is it running?
#   fido log [-f]   show (or follow) the bridge's log
#   fido ping       blink the key
#   fido info       show the key's FIDO2 details
#   fido update     pull the latest fido-bridge source and reinstall it
set -u

PREFIX=${PREFIX:-/data/data/com.termux/files/usr}
SOCK=${FIDO_BRIDGE_SOCKET:-$PREFIX/tmp/fido.sock}
LOG=$PREFIX/var/log/fido-bridge.log
SRCDIR="@SRCDIR@"
UPDATE_URL=${FIDO_UPDATE_URL:-https://github.com/WoodenWillis/termux-app}
UPDATE_BRANCH=${FIDO_UPDATE_BRANCH:-claude/fido2-usb-termux-bridge-sidqoc}
WAIT_SECS=60   # time allowed for tapping the Android permission prompt

die() { echo "fido: $*" >&2; exit 1; }
bridge_pid() {   # first live fido-bridge process (skips zombies)
	local p state
	for p in $(pgrep -x fido-bridge); do
		read -r _ _ state _ <"/proc/$p/stat" 2>/dev/null || continue
		[ "$state" != Z ] && { echo "$p"; return; }
	done
}
socket_alive() { [ -S "$SOCK" ] && [ -n "$(bridge_pid)" ]; }

cmd_up() {
	local verbose=0
	[ "${1:-}" = "-v" ] && verbose=1

	if socket_alive; then
		echo "fido: bridge already running (pid $(bridge_pid))"
		return 0
	fi
	command -v termux-usb >/dev/null ||
		die "termux-usb not found: pkg install termux-api (and install the Termux:API app)"
	command -v fido-bridge >/dev/null || die "fido-bridge not installed: run make install"

	local devs
	devs=$(termux-usb -l 2>/dev/null | grep -o '/dev/bus/usb/[0-9]*/[0-9]*')
	[ -n "$devs" ] || die "no USB device found. Is the key plugged in?"

	rm -f "$SOCK"   # leftover from a bridge that was killed hard
	mkdir -p "${LOG%/*}"
	# Keep the log from growing forever.
	[ "$(stat -c %s "$LOG" 2>/dev/null || echo 0)" -gt 200000 ] && : >"$LOG"

	local dev pid i
	for dev in $devs; do
		echo "fido: starting bridge on $dev. Tap OK if Android asks."
		echo "--- $(date '+%F %T') $dev" >>"$LOG"
		# setsid: survive closing this Termux session.
		FIDO_BRIDGE_VERBOSE=$verbose FIDO_BRIDGE_SOCKET=$SOCK \
			setsid termux-usb -r -e fido-bridge "$dev" </dev/null >>"$LOG" 2>&1 &
		pid=$!
		for ((i = 0; i < WAIT_SECS * 4; i++)); do
			if socket_alive; then
				echo "fido: ready ($SOCK)"
				return 0
			fi
			kill -0 "$pid" 2>/dev/null || break   # bridge gave up on this device
			sleep 0.25
		done
		kill "$pid" 2>/dev/null
	done
	echo "fido: bridge did not start. Last log lines:" >&2
	tail -n 5 "$LOG" >&2
	return 1
}

cmd_down() {
	local pid
	pid=$(bridge_pid)
	if [ -z "$pid" ]; then
		echo "fido: bridge not running"
		return 0
	fi
	pkill -x fido-bridge
	for _ in 1 2 3 4 5 6 7 8 9 10; do
		[ -z "$(bridge_pid)" ] && { echo "fido: stopped"; return 0; }
		sleep 0.2
	done
	pkill -9 -x fido-bridge
	rm -f "$SOCK"
	echo "fido: stopped (forced)"
}

cmd_status() {
	if socket_alive; then
		echo "fido: running (pid $(bridge_pid)) on $SOCK"
	else
		echo "fido: stopped"
		return 1
	fi
}

cmd_update() {
	local repo=${SRCDIR%/*}
	[ -d "$repo/.git" ] || die "source checkout not found at $repo"
	echo "fido: fetching $UPDATE_BRANCH"
	git -C "$repo" fetch -q "$UPDATE_URL" "$UPDATE_BRANCH" || die "fetch failed"
	git -C "$repo" checkout -q FETCH_HEAD ||
		die "checkout failed (local edits in $repo? see git -C $repo status)"
	make -s -C "$SRCDIR" install || die "build failed"
	echo "fido: updated to $(git -C "$repo" log --oneline -1)"
	if socket_alive; then
		echo "fido: restart the bridge to use the new version: fido down && fido up"
	fi
}

case "${1:-up}" in
up) shift || true; cmd_up "$@" ;;
down | stop) cmd_down ;;
status) cmd_status ;;
log) [ "${2:-}" = "-f" ] && exec tail -f "$LOG" || tail -n 40 "$LOG" ;;
ping) { socket_alive || cmd_up; } && exec fido-ping "$SOCK" ;;
info) { socket_alive || cmd_up; } && exec fido-info "$SOCK" ;;
update) cmd_update ;;
-h | --help | help) sed -n '2,12p' "$0" | sed 's/^# \{0,1\}//' ;;
*) die "unknown command '$1' (try: fido help)" ;;
esac
