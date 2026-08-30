This addon exposes device functionality as API to command line programs
in [Termux](https://github.com/termux/).

## Installation

Download the Termux:API add-on from
[F-Droid](https://f-droid.org/packages/com.termux.api/) or the Google
Play Store. It is required for the API implementations to function.

**Important: Do not mix installations of Termux and Addons between
Google Play and F-Droid.** They are presented at these portals for your
convenience. There are compatibility issues when mixing installations
from these Internet portals. This is because each download website uses
a specific key for keysigning Termux and [Addons](Addons).

### Installing termux-api package

To use Termux:API you also need to install the
[termux-api](https://github.com/termux/termux-api-package) package.

`pkg install termux-api`

### Settings

On Android 7 you may have to "protect" Termux:API by going into the
settings / protected apps menu otherwise calls to the API like
termux-battery-status will hang forever. See [issue
334](https://github.com/termux/termux-packages/issues/334#issuecomment-340581650).

## Current API implementations

[termux-battery-status](termux-battery-status):Get the status of the device battery.
[termux-brightness](termux-brightness):Set the screen brightness between 0 and 255.
[termux-call-log](termux-call-log):List call log history.
[termux-camera-info](termux-camera-info):Get information about device camera(s).
[termux-camera-photo](termux-camera-photo):Take a photo and save it to a file in JPEG format.
[termux-clipboard-get](termux-clipboard-get):Get the system clipboard text.
[termux-clipboard-set](termux-clipboard-set):Set the system clipboard text.
[termux-contact-list](termux-contact-list):List all contacts.
[termux-dialog](termux-dialog):Show a text entry dialog.
[termux-download](termux-download):Download a resource using the system download manager.
[termux-fingerprint](termux-fingerprint):Use fingerprint sensor on device to check for authentication.
[termux-infrared-frequencies](termux-infrared-frequencies):Query the infrared transmitter's supported carrier frequencies.
[termux-infrared-transmit](termux-infrared-transmit):Transmit an infrared pattern.
[termux-job-scheduler](termux-job-scheduler):Schedule a Termux script to run later, or periodically.
[termux-location](termux-location):Get the device location.
[termux-media-player](termux-media-player):Play media files.
[termux-media-scan](termux-media-scan):MediaScanner interface, make file changes visible to Android Gallery
[termux-microphone-record](termux-microphone-record):Recording using microphone on your device.
[termux-notification](termux-notification):Display a system notification.
[termux-notification-remove](termux-notification-remove):Remove a notification previously shown with termux-notification --id.
[termux-sensor](termux-sensor):Get information about types of sensors as well as live data.
[termux-share](termux-share):Share a file specified as argument or the text received on stdin.
[termux-sms-list](termux-sms-list):List SMS messages.
[termux-sms-send](termux-sms-send):Send a SMS message to the specified recipient number(s).
[termux-storage-get](termux-storage-get):Request a file from the system and output it to the specified file.
[termux-telephony-call](termux-telephony-call):Call a telephony number.
[termux-telephony-cellinfo](termux-telephony-cellinfo):Get information about all observed cell information from all radios on the device including the primary and neighboring cells.
[termux-telephony-deviceinfo](termux-telephony-deviceinfo):Get information about the telephony device.
[termux-toast](termux-toast):Show a transient popup notification.
[termux-torch](termux-torch):Toggle LED Torch on device.
[termux-tts-engines](termux-tts-engines):Get information about the available text-to-speech engines.
[termux-tts-speak](termux-tts-speak):Speak text with a system text-to-speech engine.
[termux-usb](termux-usb):List or access USB devices.
[termux-vibrate](termux-vibrate):Vibrate the device.
[termux-volume](termux-volume):Change volume of audio stream.
[termux-wallpaper](termux-wallpaper):Change wallpaper on your device.
[termux-wifi-connectioninfo](termux-wifi-connectioninfo):Get information about the current wifi connection.
[termux-wifi-enable](termux-wifi-enable):Toggle Wi-Fi On/Off.
[termux-wifi-scaninfo](termux-wifi-scaninfo):Get information about the last wifi scan.