Get the device location.

## Usage

termux-location \[-p provider\] \[-r request\]

Output displayed in json format.

### Options

`-p provider  location provider [gps/network/passive] (default: gps)`
`-r request   kind of request to make [once/last/updates] (default: once)`

## Note

- GPS likely will not work in buildings as device must be exposed to
  satellite signal.

<!-- -->

- GPS also requires that your device clock is set correctly. For the
  answer to question why, learn how GPS works.

<!-- -->

- Do not expect immediate location result. Even network location request
  may take some time.

## See Also

[Termux:API](Termux:API) - Termux addon that exposes device
functionality as API to command line programs.