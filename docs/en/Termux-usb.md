List or access USB devices.

## Usage

termux-usb \[-l \| \[-r\] \[-e command\] device\]

### Options

`-l               list available devices`
`-r               show permission request dialog if not already granted`
`-e command       execute the specified command with a file descriptor`
`                 referring to the device as its argument`

### Details

Android doesn't allow direct access to usb devices, you need to request
a file descriptor for the device from the Java API instead. This means
that Linux usb software will need to be modified to work within Termux.

Here is a sample project to get started:

Make sure you have the Termux:API application installed. Set up the
necessary packages within Termux.

`pkg install termux-api libusb clang`

Enable OTG (host) mode and insert a usb device. Wait for it to be
recognised and verify it using the API:

`termux-usb -l`

Let's assume the device is /dev/bus/usb/001/002. Ask for permission to
access it:

`termux-usb -r /dev/bus/usb/001/002`

Try using it from libusb. Save this sample code as usbtest.c:
[(download)](https://gist.githubusercontent.com/bndeff/8c391bc3fd8d9f1dbd133ac6ead7f45e/raw/6d7174a129301eeb670fe808cd9d25ec261f7f9e/usbtest.c)

`#include <stdio.h>`
`#include <assert.h>`
`#include <libusb-1.0/libusb.h>`

`int main(int argc, char **argv) {`
`    libusb_context *context;`
`    libusb_device_handle *handle;`
`    libusb_device *device;`
`    struct libusb_device_descriptor desc;`
`    unsigned char buffer[256];`
`    int fd;`
`    assert((argc > 1) && (sscanf(argv[1], "%d", &fd) == 1));`
`    libusb_set_option(NULL, LIBUSB_OPTION_NO_DEVICE_DISCOVERY);`
`    assert(!libusb_init(&context));`
`    assert(!libusb_wrap_sys_device(context, (intptr_t) fd, &handle));`
`    device = libusb_get_device(handle);`
`    assert(!libusb_get_device_descriptor(device, &desc));`
`    printf("Vendor ID: %04x\n", desc.idVendor);`
`    printf("Product ID: %04x\n", desc.idProduct);`
`    assert(libusb_get_string_descriptor_ascii(handle, desc.iManufacturer, buffer, 256) >= 0);`
`    printf("Manufacturer: %s\n", buffer);`
`    assert(libusb_get_string_descriptor_ascii(handle, desc.iProduct, buffer, 256) >= 0);`
`    printf("Product: %s\n", buffer);`
`    if (libusb_get_string_descriptor_ascii(handle, desc.iSerialNumber, buffer, 256) >= 0)`
`        printf("Serial No: %s\n", buffer);`
`    libusb_exit(context);`
`}`

This utility shows some basic information about a usb device. It takes
the device file descriptor as its only command-line argument. Let's
compile it:

`gcc usbtest.c -lusb-1.0 -o usbtest`

Use the -e option of termux-usb to run ./usbtest with the correct file
descriptor:

`termux-usb -e ./usbtest /dev/bus/usb/001/002`

## See Also

[Termux:API](Termux:API) - Termux addon that exposes device
functionality as API to command line programs.