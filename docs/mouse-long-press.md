# Long press while an application tracks the mouse

Finger long press keeps Termux's native text selection by default, even when a
terminal application has enabled mouse tracking.

To use right-click instead, add this to `~/.termux/termux.properties`:

```properties
mouse-right-click-on-long-press = true
```

Run `termux-reload-settings` to reload the existing properties cache. Remove the
line or set it to `false` to restore default selection.

When enabled, a finger long press in a tracked terminal sends a RIGHT press and
release at that terminal cell through the existing mouse encoder. SGR and legacy
mouse encodings remain supported. Releasing the long press does not add a LEFT
click: the existing gesture recognizer suppresses that release tap.

The option does not change tracking-off long press or physical mouse handling.
It does not implement held-finger dragging of a remote scrollbar or splitter.
