Transmit an infrared pattern.

## Usage

termux-infrared-transmit -f frequency pattern

The pattern is specified in comma-separated on/off intervals, such as
'20,50,20,30'. Only patterns shorter than 2 seconds will be transmitted.

### Options

`-f frequency  IR carrier frequency in Hertz. Mandatory.`

## Note

This API can be used only on devices that have infrared transmitter.

## See Also

[Termux:API](Termux:API) - Termux addon that exposes device
functionality as API to command line programs.