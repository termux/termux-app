Get information about types of sensors as well as live data.

## Usage

termux-sensor \[options\]

Output displayed in json format.

### Options

`-h, help           Show this help`
`-a, all            Listen to all sensors (WARNING! may have battery impact)`
`-c, cleanup        Perform cleanup (release sensor resources)`
`-l, list           Show list of available sensors`
`-s, sensors [,,,]  Sensors to listen to (can contain just partial name)`
`-d, delay [ms]     Delay time in milliseconds before receiving new sensor update`
`-n, limit [num]    Number of times to read sensor(s) (default: continuous) (min: 1)`

## Note

Different devices have different sensors. Check the available sensors
for your device with command `termux-sensor -l`.

## See Also

[Termux:API](Termux:API) - Termux addon that exposes device
functionality as API to command line programs.