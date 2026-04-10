![](images/Start_failsafe_session.mp4)

![](images/Termux-failsafe-recover.mp4)

If Termux exits immediately after launch or cannot properly start shell,
it is likely that your environment is broken. The cause of which is most
likely a a fatal error in a dotfile read by your shell, which causes it
to immediately exit. To recover from such errors Termux provides a
Failsafe Session.

## Launching a Failsafe Session

1\. Close all sessions of Termux application.

2\. Either long-press the termux launcher icon and then press "failsafe"
(only works on recent android versions) or long-press the "new session"
button in the left drawer inside the termux app.

You are now in a Failsafe Session. It launches the default Shell of
Android `/system/bin/sh` with access to the Termux directory. Despite
the fact that Android shell is very limited, you can still save your
files to /sdcard or perform some recovery steps.

## Fixing the Problem

Now that you have basic access to Termux you can fix the problems. If
you don't know which dotfile is broken read the section about about
possible broken dotfiles below. It's most likely the one you tinkered
with the last. Once you have identified the broken dotfile, move it to a
new name. Example:

`mv .profile .profile.bak`

Now you can try to launch a normal session again. If it still doesn't
work, you'll have to check the other dotfiles again. Otherwise, since
you now have access to your Termux programs again, you can edit the file
with your text editor and fix the problem. After that you can move the
file back to it's original location.

## Possible broken dotfiles

If you use bash (default) these dotfiles get loaded:

- `.profile`
- `.bash_profile`
- `.bashrc`

If you use zsh:

- `.profile`
- `.zshrc`

### If you used chsh and set a broken shell

If you used `chsh` to set your default shell to something that doesn't
work as a shell your environment will also be broken. In this case use
the Failsafe Session to remove `~/.termux/shell` and launch a new normal
session.

`rm -rf /data/data/com.termux/files/home/.termux/shell`

## If the problem is not with the dotfiles

If the problem isn't with your dotfiles, you may have a broken
`$PREFIX`. This could have happened in various ways. Maybe there was a
broken update, something crashed at the wrong time or you messed around
int `$PREFIX` yourself. In this case, the best option is to use the
nuclear option. Remove `$PREFIX` entirely.

`rm -rf /data/data/com.termux/files/usr`

After that you can restart your Termux App completely, which will
trigger a reinstallation of the default `$PREFIX` contents. You'll have
to reinstall all your programs, but your `$HOME` folder remains
untouched, so all your dotfiles are still there.