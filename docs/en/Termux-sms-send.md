Send a SMS message to the specified recipient number(s).

## Usage

` termux-sms-send -n number[,number2,number3,...] [-s slot] [text]`

The text to send is either supplied as arguments or read from stdin if
no arguments are given.

### Options

`-n number(s)  recipient number(s) - separate multiple numbers by commas`
`-s slot sim slot to use - silently fails if slot number is invalid or if missing READ_PHONE_STATE permission`

## See Also

[Termux:API](Termux:API) - Termux addon that exposes device
functionality as API to command line programs.