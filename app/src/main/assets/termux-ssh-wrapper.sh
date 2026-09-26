#!/bin/bash
# Termux SSH ControlMaster Wrapper Script
# This script automatically enables SSH connection multiplexing via ControlMaster.
# 
# The TERMUX_SSH_CONTROL_MASTER environment variable points to the control directory.
# Socket files are named using the pattern: %r@%h:%p (user@host:port)
#
# Usage: This script can be used as an alias or wrapper for ssh commands.
# Example alias in .bashrc:
#   alias ssh='termux-ssh-wrapper.sh'
# 
# Note: This script calls 'ssh-real' (the original SSH binary) internally,
# so it won't cause infinite recursion even if aliased as 'ssh'.
#
# Verification:
#   ssh -O check <host>  # Check if control socket is active
#   ssh -O exit <host>   # Close control socket

# Ensure control directory exists
CONTROL_DIR="${TERMUX_SSH_CONTROL_MASTER:-$HOME/.ssh/control}"
if [[ ! -d "$CONTROL_DIR" ]]; then
    mkdir -p "$CONTROL_DIR"
    chmod 700 "$CONTROL_DIR"
fi

# ControlMaster configuration
# ControlMaster=auto: Create a new control socket if none exists, or use existing one
# ControlPath: Location of the control socket
# ControlPersist=600: Keep control socket open for 10 minutes after last connection closes
SSH_OPTIONS=(
    -o "ControlMaster=auto"
    -o "ControlPath=${CONTROL_DIR}/%r@%h:%p"
    -o "ControlPersist=600"
)

# Execute ssh with ControlMaster options
# Call ssh-real (the original SSH binary) directly to avoid recursion
exec "${TERMUX_SSH_BINARY:-ssh-real}" "${SSH_OPTIONS[@]}" "$@"