# Server program

This is the server program for using an MSX computer with Rookie Drive as an USB keyboard, it's written in C# targetting .NET Core 3.1. This program will:

1. Connect to [the virtual HID driver](https://tetherscript.com/hid-driver-kit-download/).
2. Check if a Rookie Drive in device mode is connected (by VID and PID).
3. Read and parse the key mappings file.
4. Enter in a loop in which any MSX keyboard status change data received from the interrupt endpoint of the Rookie Drive is translated into HID key codes and sent to the HID driver.

See [the guide to use Rookie Drive as a keyboard](https://github.com/Konamiman/NestorDevice/blob/main/docs/Keyboard.md) for more details.

## Command line arguments reference

* `-i <vid>:<pid>`: Use this pair of vendor and product ids (hexadecimal numbers) to identify the device, default is 1209:0007.
* `-k <file>`: Use this file for the key mappings, default is `KeyMappings.txt` in same directory of the application executable.
* `-d`: Debug mode, print out all the data received from the device.
* `?`: Show help (the arguments list above).

All are optional.


## Why is the source of WinUSBNet itself included?

The server program uses [WinUSBNet](https://github.com/madwizard-thomas/winusbnet) to communicate with the Rookie Drive using WinUSB. This library is available as a NuGet package, but at the time of writing this it has a bug that makes it unusable for this project: it fails to initialize an USB device that doesn't provide a language IDs descriptor (which is the case of the CH376 configured as a device in "internal firmware mode"). So I have just included the sources with the bug fixed in my project.

Oh yes, [I have reported the bug in the repository of the library](https://github.com/madwizard-thomas/winusbnet/issues/38), I'm not a monster.