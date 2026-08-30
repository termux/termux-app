At launch Termux shows a terminal interface, whose text size can be
adjusted by pinch zooming or double tapping and pulling the content
towards or from you.

Besides the terminal (with keyboard shortcuts explained below) there are
three additional interface elements available: A context menu,
navigation drawer and notification.

The context menu can be shown by long pressing anywhere on the terminal.
It provides menu entries for:

- Selecting and pasting text.
- Sharing text from the terminal to other apps (e.g. email or SMS)
- Resetting the terminal if it gets stuck.
- Hangup (exiting the current terminal session).
- Styling the terminal by selecting a font and a color scheme.
- Showing this help page.

The navigation drawer is revealed by swiping inwards from the left part
of the screen (if you have gesture navigation enabled in Android, you
will need to hold briefly on the edge of the screen before swiping to
bring up the navigation drawer instead of going back in Android). It has
three elements:

- A list of sessions. Clicking on a session shows it in the terminal
  while long pressing allows you to specify a session title.
- A button to toggle visibility of the touch keyboard.
- A button to create new terminal sessions (long press for creating a
  named session or a [fail-safe](fail-safe) one).

The notification, available when a terminal session is running, is
available by pulling down the notification menu. Pressing the
notification leads to the most current terminal session. The
notification may also be expanded (by pinch-zooming or performing a
single-finger glide) to expose three actions:

- Exiting all running terminal sessions.
- Use a wake lock to avoid entering sleep mode.
- Use a high performance wifi lock to maximize wifi performance.

With a wake lock held the notification and Termux background processes
will be available even if no terminal session is running, which allows
server and other background processes to run more reliably.

### Graphical interface (via external vncviewer)

Termux also has fairly basic graphical user interface support based on
X11. To enable Graphical Environment support see [Graphical
Environment](Graphical_Environment)

# See Also

- [Touch Keyboard](Touch_Keyboard)