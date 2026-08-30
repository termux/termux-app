Show text in a Toast (a transient popup).

## Usage

termux-toast \[options\] \[text\]

The text to show is either supplied as arguments or read from stdin if
no arguments are given.

### Options

`-h  show this help`
`-b  set background color (default: gray)`
`-c  set text color (default: white)`
`-g  set position of toast: [top, middle, or bottom] (default: middle)`
`-s  only show the toast for a short while`

## Note

Color can be a standard name (i.e. red) or 6 / 8 digit hex value (i.e.
"#FF0000" or "#FFFF0000") where order is (AA)RRGGBB. Invalid color will
revert to default value.

## See Also

[Termux:API](Termux:API) - Termux addon that exposes device
functionality as API to command line programs.