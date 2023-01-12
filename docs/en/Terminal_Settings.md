The Termux terminal can be configured by editing the
`~/.termux/termux.properties` file. This file uses a simple `key=value`
property syntax.

Lines beginning with `#` are comments and do not get executed. Most of
the properties are included as comments in the file. Uncomment an
existing line or add a new line to configure a property.

For changes to take effect after editing the properties file, either
execute `termux-reload-settings` or restart Termux by closing all
sessions and starting the application again.

# Dark theme

Force the application to use dark theme for drawer and dialogs with

``` ini
use-black-ui=true
```

*Note: This is enabled automatically on Android 9+ if the system UI uses
a dark theme.*

# Fullscreen mode

Enable fullscreen mode with

``` ini
fullscreen=true
```

It is likely that with fullscreen enabled, the Extra Keys view may no
longer be visible. To fix, add

``` ini
use-fullscreen-workaround=true
```

*Note: Fullscreen may not work well for all devices so consider it
unstable for now.*

# Handling terminal sessions

Use shortcut key combinations to handle common session actions. Add
shortcuts to open a new session, switch between sessions, and rename
sessions with

``` ini
# Open a new terminal with ctrl + t (volume down + t)
shortcut.create-session=ctrl + t

# Go one session down with (for example) ctrl + 2
shortcut.next-session=ctrl + 2

# Go one session up with (for example) ctrl + 1
shortcut.previous-session=ctrl + 1

# Rename a session with (for example) ctrl + n
shortcut.rename-session=ctrl + n
```

# Handling the bell character

Adjust the behavior of how the [bell
character](https://en.wikipedia.org/wiki/Bell_character) is received
with the `bell-character` property

``` ini
# Vibrate device (default).
bell-character=vibrate

# Beep with a sound.
bell-character=beep

# Ignore bell character.
bell-character=ignore
```

# Handling the back key

Adjust the behavior of how the back key is handled with the `back-key`
property

``` ini
# Send the Escape key.
back-key=escape

# Hide keyboard or leave app (default).
back-key=back
```

# Handling the extra keys

See [Touch Keyboard](Touch_Keyboard)

# Workaround issues with some keyboards

Some keyboards, e.g. a default one on Samsung devices, have issues with
text input: text appearing only after pressing "enter", wrong keyboard
layout being opened, etc. They are not compatible with the Termux input
method because enforcing a word-based input. As workaround, use

``` ini
enforce-char-based-input = true
```

This will tell Termux to use
`InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS`
instead of `InputType.TYPE_NULL`.

On some devices the combination of `ctrl + space` on hardware keyboards
does not work (for example in emacs) which makes it difficult to mark
text. As a workaround, use

``` ini
ctrl-space-workaround = true
```

*Note: setting this on devices where `ctrl + space` already works will
break the functionality.*

# Adjust Terminal Margin/Padding

Terminal horizontal margin (left and right) can be adjuted with

``` ini
terminal-margin-horizontal=val
```

And vertical margin (top and bottom) with

``` ini
terminal-margin-vertical=val
```

Where `val` is an integer (representing dp units) between `0` and `100`.

The default value is `3` for horizontal margin and `0` for vertical
margin.

This can be helpful for some setups, such as text on screen edges not
being fully visible due to curvature or a screen protector/case. A
larger horizontal margin may also be helpful for those having gesture
navigation enabled on Android 10+ due to not being able to touch at
exactly the edge of the screen.