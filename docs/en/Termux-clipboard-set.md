Set the system clipboard text. The text to set is either supplied as
arguments or read from stdin if no arguments are given.

## Usage

termux-clipboard-set \[text\]

Text is read either from standard input or from command line arguments.

### Examples

`termux-clipboard-set "hello world"`

`cat file.txt | termux-clipboard-set`

## See Also

[Termux:API](Termux:API) - Termux addon that exposes device
functionality as API to command line programs.

[Termux-clipboard-get](Termux-clipboard-get) - Utility to
retrieve text from the clipboard.