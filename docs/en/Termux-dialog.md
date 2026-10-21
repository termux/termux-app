Show dialog widget for user input.

## Usage

termux-dialog widget \[options\]

Output is returned in json format.

### Options

`-l, list   List all widgets and their options`
`-t, title  Set title of input dialog (optional)`

### Widget-specific options

    confirm - Show confirmation dialog
        [-i hint] text hint (optional)
        [-t title] set title of dialog (optional)

    checkbox - Select multiple values using checkboxes
        [-v ",,,"] comma delim values to use (required)
        [-t title] set title of dialog (optional)

    counter - Pick a number in specified range
        [-r min,max,start] comma delim of (3) numbers to use (optional)
        [-t title] set title of dialog (optional)

    date - Pick a date
        [-t title] set title of dialog (optional)
        [-d "dd-MM-yyyy k:m:s"] SimpleDateFormat Pattern for date widget output (optional)

    radio - Pick a single value from radio buttons
        [-v ",,,"] comma delim values to use (required)
        [-t title] set title of dialog (optional)

    sheet - Pick a value from sliding bottom sheet
        [-v ",,,"] comma delim values to use (required)
        [-t title] set title of dialog (optional)

    spinner - Pick a single value from a dropdown spinner
        [-v ",,,"] comma delim values to use (required)
        [-t title] set title of dialog (optional)

    speech - Obtain speech using device microphone
        [-i hint] text hint (optional)
        [-t title] set title of dialog (optional)

    text - Input text (default if no widget specified)
        [-i hint] text hint (optional)
        [-m] multiple lines instead of single (optional)*
        [-n] enter input as numbers (optional)*
        [-p] enter input as password (optional)
        [-t title] set title of dialog (optional)
           * cannot use [-m] with [-n]

    time - Pick a time value
        [-t title] set title of dialog (optional)

## See Also

[Termux:API](Termux:API) - Termux addon that exposes device
functionality as API to command line programs.