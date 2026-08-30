Display a system notification. Content text is specified using
-c/--content or read from stdin.

## Usage

termux-notification \[options\]

### Options

` --action action          action to execute when pressing the notification`
` --alert-once             do not alert when the notification is edited`
` --button1 text           text to show on the first notification button`
` --button1-action action  action to execute on the first notification button`
` --button2 text           text to show on the second notification button`
` --button2-action action  action to execute on the second notification button`
` --button3 text           text to show on the third notification button`
` --button3-action action  action to execute on the third notification button`
` -c/--content content     content to show in the notification. Will take precedence over stdin.`
` --group group            notification group (notifications with the same group are shown together)`
` -h/--help                show this help`
` --help-actions           show the help for actions`
` -i/--id id               notification id (will overwrite any previous notification with the same id)`
` --image-path path        absolute path to an image which will be shown in the notification`
` --led-color rrggbb       color of the blinking led as RRGGBB (default: none)`
` --led-off milliseconds   number of milliseconds for the LED to be off while it's flashing (default: 800)`
` --led-on milliseconds    number of milliseconds for the LED to be on while it's flashing (default: 800)`
` --on-delete action       action to execute when the the notification is cleared`
` --ongoing                pin the notification`
` --priority prio          notification priority (high/low/max/min/default)`
` --sound                  play a sound with the notification`
` -t/--title title         notification title to show`
` --vibrate pattern        vibrate pattern, comma separated as in 500,1000,200`
` --type type              notification style to use (default/media)`

Media actions (available with --type "media"):

` --media-next             action to execute on the media-next button`
` --media-pause            action to execute on the media-pause button`
` --media-play             action to execute on the media-play button`
` --media-previous         action to execute on the media-previous button`

### Action arguments

Commands like --action, --on-delete, --button-1-action and --media-next
take an action string as their argument, which is fed to \`dash -c\`. A
few important things must be kept in mind when using actions:

You should use actions that do things outside of the terminal, like
--action "termux-toast hello". Anything that outputs to the terminal is
useless, so the output should either be redirected (--action "ls \>
\~/ls.txt") or shown to the user in a different way (--action
"ls\|termux-toast").

Running more than one command in a single action is as easy as --action
"command1; command2; command3" or --action "if \[ -e file \]; then
termux-toast yes; else termux-toast no; fi".

For anything more complex, you should put your script in a file, make it
executable, and use that as the action: --action \~/bin/script

The action is run in a different environment (not a subshell). Thus your
environment is lost (most notably \$PATH), and \~/.profile is not
sourced either. So if you need your \$PATH you should either:

- if the action is a script, set it explicitly in the script (e.g.
  export PATH="\$HOME/bin:\$PATH")
- or use something like --action "bash -l -c 'command1; command2'").