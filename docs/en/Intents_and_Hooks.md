Termux is able to catch several intents and execute shell scripts to act
upon them:

- Sharing single files with Termux.
  Following handle will be executed:`~/bin/termux-file-editor`
  - Opening in your popular editor by using a symbolic link:
    `ln -s $PREFIX/bin/nvim ~/bin/termux-file-editor`
  - Creating a handle by creating a file and changing the access rights:
    1.  `nano ~/bin/termux-file-editor`)
    2.  `chmod +x ~/bin/termux-file-editor`)
- URL sharing available in common apps (e.g. Youtube).
  Following handle will be executed: `~/bin/termux-url-opener`
  - Sharing a URL to be downloaded.
    You can handle incoming URL by editing the content.
    `yoursth-dl -f 'bestvideo[ext=mp4][height<=720]+bestaudio' --restrict-filenames -o '~/storage/downloads/%(title)s-%(id)s.%(ext)s' $1`