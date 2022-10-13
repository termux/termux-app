#! /bin/bash

old_cnf_handler=$(declare -f command_not_found_handle |
  tail -n +3 | head -n -1)

echo Old command not found handler:
echo $old_cnf_handler

command_not_found_handle ()
{
    name="$1"

    if termux-launch -t "${name}"; then
        termux-launch "${name}" 1>&2 2>/dev/null
        return $?
    fi

    if [ ! -z "$old_cnf_handler" ]; then
      eval "$old_cnf_handler"
    else
      echo "Command not found" >&2
      return 1
    fi
}

export -f command_not_found_handle
echo "android-launcher initialized"
