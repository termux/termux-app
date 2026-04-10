Share a specified file or text from standard input.

## Usage

termux-share \[options\] \[file\]

If file to share is not specified, program will read standard input.

### Options

`-a action        which action to performed on the shared content:`
`                 edit/send/view (default:view)`
`-c content-type  content-type to use (default: guessed from file extension,`
`                 text/plain for stdin)`
`-d               share to the default receiver if one is selected`
`                 instead of showing a chooser`
`-t title         title to use for shared content (default: shared file name)`

## See Also

[Termux:API](Termux:API) - Termux addon that exposes device
functionality as API to command line programs.